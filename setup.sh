#!/usr/bin/env bash
# --- PROLOGUE OBLIGATOIRE ---
set -euo pipefail
# Empêche le téléchargement auto d'un toolchain Go différent
export GOTOOLCHAIN=local
# Evite les warnings/erreurs "buildvcs"
export GOFLAGS="${GOFLAGS:-} -buildvcs=false"

# Où déposer les binaires buildés
export KUBE_DIR="${KUBE_DIR:-/out/bin}"
mkdir -p "$KUBE_DIR"

# Helpers utilisés plus bas
msg(){ echo "[*] $*"; }
ok(){  echo "[OK] $*"; }
warn(){ echo "[WARN] $*" >&2; }

# Installe les deps build C/libpcap si nécessaire
ensure_build_deps() {
  if command -v pkg-config >/dev/null 2>&1 && pkg-config --exists libpcap; then
    return 0
  fi
  if command -v apt-get >/dev/null 2>&1; then
    msg "Installation des deps build (libpcap-dev, gcc, pkg-config)…"
    apt-get update -y
    apt-get install -y --no-install-recommends libpcap-dev pkg-config gcc
    rm -rf /var/lib/apt/lists/*
  else
    warn "libpcap headers introuvables et pas d'apt-get. Installe libpcap-dev/gcc/pkg-config dans l'image."
  fi
}

# Builder générique avec pins (utilisé pour quelques utilitaires simples)
go_build_with_pins(){
  local repo="$1" sub="$2" out="$3"; shift 3 || true
  local tmp; tmp="$(mktemp -d)"
  git clone --depth=1 "$repo" "$tmp/src"
  (
    cd "$tmp/src/${sub}"
    # Pins communs
    go get "golang.org/x/crypto@${PIN_X_CRYPTO}"
    go get "golang.org/x/net@${PIN_X_NET}"
    go get "golang.org/x/text@${PIN_X_TEXT}"
    go get "google.golang.org/protobuf@${PIN_PROTOBUF}"
    go get "gopkg.in/yaml.v3@${PIN_YAML_V3}"
    # Commandes supplémentaires éventuelles passées en arguments
    for cmd in "$@"; do [ -n "${cmd:-}" ] && eval "$cmd"; done
    go mod tidy
    CGO_ENABLED="${CGO_ENABLED_OVERRIDE:-0}" go build -trimpath -ldflags="-s -w" -o "$KUBE_DIR/$out" .
  )
  rm -rf "$tmp"
  [ -x "$KUBE_DIR/$out" ] && ok "$out build" || warn "$out KO"
}
# --- FIN PROLOGUE ---

# Pins (conservés si besoin de build local, mais inutiles pour go install @latest)
PIN_X_CRYPTO="v0.37.0"
PIN_X_NET="v0.41.0"
PIN_X_TEXT="v0.15.0"
PIN_OAUTH2="v0.27.0"
PIN_YAML_V3="v3.0.1"
PIN_PROTOBUF="v1.34.2"

msg "Installation des outils dans $KUBE_DIR (FORCAGE build pin + CGO si besoin)"

# 0) waybackurls
msg "waybackurls …"
go_build_with_pins "https://github.com/tomnomnom/waybackurls" "." "waybackurls" || warn "waybackurls KO"

# 1) kubectl-who-can — build durci avec pins de dépendances
msg "kubectl-who-can (hardened) …"
CGO_ENABLED_OVERRIDE=0 \
go_build_with_pins \
  "https://github.com/aquasecurity/kubectl-who-can" \
  "cmd/kubectl-who-can" \
  "kubectl-who-can" \
  'go get golang.org/x/crypto@v0.35.0' \
  'go get golang.org/x/net@v0.39.0' \
  'go get golang.org/x/oauth2@v0.27.0' \
  'go get github.com/nwaples/rardecode/v2@v2.2.0' \
  'go get github.com/ulikunitz/xz@v0.5.15' \
  || warn "kubectl-who-can KO"

# 2) kubeletctl — build durci depuis les sources
msg "kubeletctl (hardened) …"

KUBELETCTL_REPO="https://github.com/cyberark/kubeletctl"
tmp="$(mktemp -d)"

git clone --depth=1 "$KUBELETCTL_REPO" "$tmp/src"

(
  set -e
  cd "$tmp/src"

  # 0) Remplacer jwt-go vulnérable par le fork maintenu
  if grep -R "github.com/dgrijalva/jwt-go" -n . >/dev/null 2>&1; then
    grep -rl "github.com/dgrijalva/jwt-go" . \
      | xargs sed -i 's#"github.com/dgrijalva/jwt-go"#"github.com/golang-jwt/jwt/v4"#g'
  fi

  # 1) Monter les libs Go critiques à des versions patchées (en dur)
  go get github.com/golang-jwt/jwt/v4@v4.5.2

  go get golang.org/x/crypto@v0.37.0
  go get golang.org/x/net@v0.41.0
  go get golang.org/x/text@v0.22.0
  go get golang.org/x/oauth2@v0.27.0

  # 2) Deps vulnérables spécifiques kubeletctl
  go get go.mongodb.org/mongo-driver@v1.15.0
  go get github.com/gogo/protobuf@v1.3.2

  # ⚠ On ne touche pas (pour l’instant) à k8s.io/* pour ne pas casser la compat
  # Si besoin on fera un deuxième round juste sur ces deps.

  # 3) Nettoyage du graph de dépendances
  go mod tidy

  # 4) Build du binaire kubeletctl (doc officielle : build à la racine)
  CGO_ENABLED=0 go build -trimpath -ldflags="-s -w" -o "$KUBE_DIR/kubeletctl" .
)

rm -rf "$tmp"

if [ -x "$KUBE_DIR/kubeletctl" ]; then
  ok "kubeletctl build (hardened)"
else
  warn "kubeletctl KO (hardened) – binaire non généré"
  exit 1
fi

# 4) kubescape — version figée (fix incompatibilité docker/buildx)
msg "kubescape (version stable v3.0.9)…"

KUBESCAPE_REPO="https://github.com/kubescape/kubescape"
tmp="$(mktemp -d)"

git clone "$KUBESCAPE_REPO" "$tmp/src"

(
  set -e
  cd "$tmp/src"

  # Figer sur une version compatible (évite les erreurs moby/moby vs docker/docker)
  git checkout v3.0.9

  # Build propre
  CGO_ENABLED=0 GO111MODULE=on GOTOOLCHAIN=local \
    go build -trimpath -ldflags="-s -w" -o "$KUBE_DIR/kubescape" .
)

rm -rf "$tmp"

if [ -x "$KUBE_DIR/kubescape" ]; then
  ok "kubescape build (v3.0.9)"
else
  warn "kubescape KO (v3.0.9)"
fi

# 5) rbac-police — build local durci (plus de binaire précompilé)
msg 'rbac-police (hardened from source) …'

RBAC_OUT="$KUBE_DIR/rbac-police"
RBAC_REPO="https://github.com/PaloAltoNetworks/rbac-police"
tmp="$(mktemp -d)"
tmp_ok=""

if git clone --depth=1 "$RBAC_REPO" "$tmp/src"; then
  (
    set -e
    cd "$tmp/src"

    #
    # 1) Monter les dépendances critiques à des versions patchées
    #

    # 🔹 OPA — CVE-2025-46569 (fix officiel en v1.4.0)
    # On essaie d'abord v1.4.0 en force (require + replace)
    if go mod edit -require=github.com/open-policy-agent/opa@v1.4.0 \
       && go mod edit -replace=github.com/open-policy-agent/opa=github.com/open-policy-agent/opa@v1.4.0; then
      go get github.com/open-policy-agent/opa@v1.4.0 || true
    else
      warn "go mod edit OPA@v1.4.0 KO, tentative fallback v0.68.0 (sans fix complet CVE-2025-46569)"
      go mod edit -require=github.com/open-policy-agent/opa@v0.68.0 || true
      go mod edit -replace=github.com/open-policy-agent/opa=github.com/open-policy-agent/opa@v0.68.0 || true
      go get github.com/open-policy-agent/opa@v0.68.0 || true
    fi

    # 🔹 x/* & compagnies
    # x/net — CVE-2025-22870/22872 : prendre une version récente (>=0.38.0)
    go mod edit -require=golang.org/x/net@v0.46.0 || true
    go mod edit -replace=golang.org/x/net=golang.org/x/net@v0.46.0 || true
    go get golang.org/x/net@v0.46.0 || true

    # crypto / oauth2 / sys / text / protobuf / yaml — mêmes versions que pour le reste de ton setup
    go mod edit -require=golang.org/x/crypto@v0.37.0 || true
    go mod edit -require=golang.org/x/oauth2@v0.27.0 || true
    go mod edit -require=golang.org/x/sys@v0.26.0 || true
    go mod edit -require=golang.org/x/text@v0.15.0 || true
    go mod edit -require=google.golang.org/protobuf@v1.34.2 || true
    go mod edit -require=gopkg.in/yaml.v3@v3.0.1 || true

    go get golang.org/x/crypto@v0.37.0 || true
    go get golang.org/x/oauth2@v0.27.0 || true
    go get golang.org/x/sys@v0.26.0 || true
    go get golang.org/x/text@v0.15.0 || true
    go get google.golang.org/protobuf@v1.34.2 || true
    go get gopkg.in/yaml.v3@v3.0.1 || true

    #
    # (optionnel mais utile pour debug)
    #
    go list -m github.com/open-policy-agent/opa golang.org/x/net || true

    #
    # 2) Nettoyage du graph de dépendances
    #
    go mod tidy

    #
    # 3) Build du binaire (doc officielle: go build à la racine)
    #
    CGO_ENABLED=0 GO111MODULE=on GOTOOLCHAIN=local \
      go build -trimpath -ldflags="-s -w" -o "$RBAC_OUT" .
  ) && tmp_ok="yes" || warn "rbac-police hardened build KO"
else
  warn "git clone rbac-police KO (réseau / GitHub ?)"
fi

rm -rf "$tmp"

if [ -n "$tmp_ok" ] && [ -x "$RBAC_OUT" ]; then
  ok "rbac-police build (hardened from source)"
else
  warn "rbac-police indisponible (build durci échoué). Création d’un stub."
  cat >"$RBAC_OUT"<<'EOF'
#!/bin/sh
echo "[WARN] rbac-police n'a pas pu être téléchargé/compilé lors du build."
echo "       Vérifie la connectivité GitHub et les dépendances Go (OPA, x/*, protobuf, yaml)."
exit 126
EOF
  chmod +x "$RBAC_OUT"
fi

# 6) katana — DOC OFFICIELLE: go install
msg "katana …"
CGO_ENABLED=1 GO111MODULE=on GOTOOLCHAIN=local \
  go install github.com/projectdiscovery/katana/cmd/katana@latest
KATANA_BIN="$(go env GOPATH)/bin/katana"
if [ -x "$KATANA_BIN" ]; then
  install -D -m0755 "$KATANA_BIN" "$KUBE_DIR/katana"
  ok "katana install (go install)"
else
  warn "katana KO (binaire introuvable)"
fi

# 6) naabu — build local avec deps patchées (au lieu de go install direct)
msg "naabu (hardened) …"

NAABU_REPO="https://github.com/projectdiscovery/naabu"
tmp="$(mktemp -d)"

# Versions patchées (à ajuster au besoin si upstream change)
PIN_RARDECODE_V2="v2.2.0"
PIN_XZ="v0.5.15"
PIN_OAUTH2="v0.27.0"

git clone --depth=1 "$NAABU_REPO" "$tmp/src"
(
  cd "$tmp/src"

  # On est à la racine du module (go.mod), pas dans /v2/
  # On force les versions sûres des dépendances vulnérables
  go get "github.com/nwaples/rardecode/v2@${PIN_RARDECODE_V2}"
  go get "github.com/ulikunitz/xz@${PIN_XZ}"
  go get "golang.org/x/oauth2@${PIN_OAUTH2}"

  go mod tidy

  # 1er essai : binaire sans CGO (pas de scan full mais moins chiant)
  if CGO_ENABLED=0 go build -trimpath -ldflags="-s -w" -o "$KUBE_DIR/naabu" ./cmd/naabu; then
    ok "naabu build (CGO=0, deps patchées)"
  else
    warn "naabu CGO=0 KO, tentative avec CGO=1 (libpcap)"
    ensure_build_deps
    if CGO_ENABLED=1 go build -trimpath -ldflags="-s -w" -o "$KUBE_DIR/naabu" ./cmd/naabu; then
      ok "naabu build (CGO=1, deps patchées)"
    else
      warn "naabu KO même avec CGO=1, aucun binaire généré"
    fi
  fi
)

rm -rf "$tmp"
[ -x "$KUBE_DIR/naabu" ] || warn "naabu non présent dans $KUBE_DIR (build échoué)"

# 7) httpx — build durci avec deps patchées
msg "httpx (hardened) …"

HTTPX_REPO="https://github.com/projectdiscovery/httpx"
tmp="$(mktemp -d)"

git clone --depth=1 "$HTTPX_REPO" "$tmp/src"

(
  set -e
  cd "$tmp/src"

  # Patch des dépendances vulnérables signalées par Trivy
  go get github.com/go-viper/mapstructure/v2@v2.4.0
  go get github.com/nwaples/rardecode/v2@v2.2.0
  go get github.com/ulikunitz/xz@v0.5.15
  go get golang.org/x/oauth2@v0.27.0

  # IMPORTANT : forcer tlsx à la version attendue par httpx
  # (logs : "downgraded ... tlsx v1.2.1 => v1.1.9", donc on le remonte)
  go get github.com/projectdiscovery/tlsx@v1.2.1

  # Nettoyage du graph de dépendances
  go mod tidy

  # Build du binaire httpx
  CGO_ENABLED=0 GO111MODULE=on GOTOOLCHAIN=local \
    go build -trimpath -ldflags="-s -w" \
      -o "$KUBE_DIR/httpx" ./cmd/httpx
)

rm -rf "$tmp"

if [ -x "$KUBE_DIR/httpx" ]; then
  ok "httpx build (hardened)"
else
  warn "httpx KO (hardened build)"
fi

# 8) nuclei — DOC OFFICIELLE: go install (module v3)
msg "nuclei …"
GO111MODULE=on GOTOOLCHAIN=local \
  go install github.com/projectdiscovery/nuclei/v3/cmd/nuclei@latest
NUCLEI_BIN="$(go env GOPATH)/bin/nuclei"
if [ -x "$NUCLEI_BIN" ]; then
  install -D -m0755 "$NUCLEI_BIN" "$KUBE_DIR/nuclei"
  ok "nuclei install (go install)"
else
  warn "nuclei KO (binaire introuvable)"
fi

# 9) zgrab2 — DOC OFFICIELLE: go install (binaire cmd/zgrab2)
msg "zgrab2 …"
GO111MODULE=on GOTOOLCHAIN=local \
  go install github.com/zmap/zgrab2/cmd/zgrab2@latest
ZGRAB2_BIN="$(go env GOPATH)/bin/zgrab2"
if [ -x "$ZGRAB2_BIN" ]; then
  install -D -m0755 "$ZGRAB2_BIN" "$KUBE_DIR/zgrab2"
  ok "zgrab2 install (go install)"
else
  warn "zgrab2 KO (binaire introuvable)"
fi

# 10) ffuf — fuzzer / directory & parameter discovery
msg "ffuf …"
go_build_with_pins "https://github.com/ffuf/ffuf" "." "ffuf" || warn "ffuf KO"

# 11) subfinder — DOC OFFICIELLE: go install (module v2)
msg "subfinder …"

GO111MODULE=on GOTOOLCHAIN=local \
  go install github.com/projectdiscovery/subfinder/v2/cmd/subfinder@latest

SUBFINDER_BIN="$(go env GOPATH)/bin/subfinder"

if [ -x "$SUBFINDER_BIN" ]; then
  install -D -m0755 "$SUBFINDER_BIN" "$KUBE_DIR/subfinder"
  ok "subfinder install (go install)"
else
  warn "subfinder KO (binaire introuvable)"
fi

# Récapitulatif
msg "Binaires installés dans $KUBE_DIR :"
ls -lh "$KUBE_DIR" || true
