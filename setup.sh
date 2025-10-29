#!/usr/bin/env bash
set -euo pipefail

# --- Répertoire d’installation local (à côté du script) ---
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
KUBE_DIR="$SCRIPT_DIR/kube"
mkdir -p "$KUBE_DIR"

# Forcer Go modules et éviter un workspace parasite
export GO111MODULE=on
export GOWORK=off
# Par défaut, builds "statiques" quand possible ; on surchargera pour katana.
export CGO_ENABLED=0

echo "[*] Installation des dépendances de base…"
sudo apt update
sudo apt install -y curl dnsutils nmap whatweb dirb golang git build-essential unzip ca-certificates

echo "[*] Installation waybackurls (GOPATH/bin)…"
go install github.com/tomnomnom/waybackurls@latest || true
export PATH="$PATH:$(go env GOPATH)/bin"

# Tous les binaires doivent finir dans ./kube
export GOBIN="$KUBE_DIR"

# Petite aide pour symlink propre en /usr/local/bin
symlink_bin() {
  local src="$1"
  local name="$2"
  if [ -x "$src" ]; then
    echo "[*] Création du lien /usr/local/bin/${name} → $src"
    sudo ln -sf "$src" "/usr/local/bin/${name}" || true
  fi
}

echo "[*] Installation/Build des outils dans $KUBE_DIR …"

# 1) kubectl-who-can
echo "[*] kubectl-who-can …"
if ! go install github.com/aquasecurity/kubectl-who-can/cmd/kubectl-who-can@latest 2>/dev/null; then
  echo "    go install a échoué, fallback en build local…"
  tmpdir="$(mktemp -d)"
  git clone --depth=1 https://github.com/aquasecurity/kubectl-who-can "$tmpdir/kubectl-who-can"
  if [ -d "$tmpdir/kubectl-who-can/cmd/kubectl-who-can" ]; then
    ( cd "$tmpdir/kubectl-who-can/cmd/kubectl-who-can" \
      && go mod tidy || true \
      && go build -o "$KUBE_DIR/kubectl-who-can" . )
  else
    ( cd "$tmpdir/kubectl-who-can" \
      && go mod tidy || true \
      && go build -o "$KUBE_DIR/kubectl-who-can" . )
  fi
  rm -rf "$tmpdir"
fi
chmod +x "$KUBE_DIR/kubectl-who-can"
symlink_bin "$KUBE_DIR/kubectl-who-can" "kubectl-who-can"

# 2) kubeletctl
echo "[*] kubeletctl …"
if ! go install github.com/cyberark/kubeletctl/cmd/kubeletctl@latest 2>/dev/null; then
  echo "    go install a échoué, fallback en build local…"
  tmpdir2="$(mktemp -d)"
  git clone --depth=1 https://github.com/cyberark/kubeletctl "$tmpdir2/kubeletctl"
  if [ -d "$tmpdir2/kubeletctl/cmd/kubeletctl" ]; then
    ( cd "$tmpdir2/kubeletctl/cmd/kubeletctl" \
      && go mod tidy || true \
      && go get github.com/mitchellh/go-ps@latest || true \
      && go build -o "$KUBE_DIR/kubeletctl" . )
  else
    ( cd "$tmpdir2/kubeletctl" \
      && go mod tidy || true \
      && go get github.com/mitchellh/go-ps@latest || true \
      && go build -o "$KUBE_DIR/kubeletctl" . )
  fi
  rm -rf "$tmpdir2"
fi
chmod +x "$KUBE_DIR/kubeletctl"
symlink_bin "$KUBE_DIR/kubeletctl" "kubeletctl"

# 3) rbac-police
echo "[*] rbac-police …"
if ! go install github.com/PaloAltoNetworks/rbac-police@latest 2>/dev/null; then
  echo "    go install a échoué, fallback en build local…"
  tmpdir3="$(mktemp -d)"
  git clone --depth=1 https://github.com/PaloAltoNetworks/rbac-police "$tmpdir3/rbac-police"
  ( cd "$tmpdir3/rbac-police" \
    && go mod tidy || true \
    && go build -o "$KUBE_DIR/rbac-police" . )
  rm -rf "$tmpdir3"
fi
chmod +x "$KUBE_DIR/rbac-police"
symlink_bin "$KUBE_DIR/rbac-police" "rbac-police"

# 4) kubescape
echo "[*] kubescape …"
if ! go install github.com/kubescape/kubescape@latest 2>/dev/null; then
  echo "    go install a échoué, fallback en build local…"
  tmpdir4="$(mktemp -d)"
  git clone --depth=1 https://github.com/kubescape/kubescape "$tmpdir4/kubescape"
  if [ -d "$tmpdir4/kubescape/cmd/cli" ]; then
    ( cd "$tmpdir4/kubescape/cmd/cli" \
      && go mod tidy || true \
      && go build -o "$KUBE_DIR/kubescape" . )
  else
    ( cd "$tmpdir4/kubescape" \
      && go mod tidy || true \
      && go build -o "$KUBE_DIR/kubescape" . )
  fi
  rm -rf "$tmpdir4"
fi
chmod +x "$KUBE_DIR/kubescape"
symlink_bin "$KUBE_DIR/kubescape" "kubescape"

# 5) katana (ProjectDiscovery) — nécessite CGO
echo "[*] katana …"
katana_ok=0

# Essai 1 : go install direct dans $GOBIN avec CGO
export CGO_ENABLED=1
if go install github.com/projectdiscovery/katana/cmd/katana@latest 2>/dev/null; then
  katana_ok=1
else
  echo "    go install a échoué, fallback en build local…"
  tmpdir5="$(mktemp -d)"
  if git clone --depth=1 https://github.com/projectdiscovery/katana "$tmpdir5/katana" 1>/dev/null; then
    if [ -d "$tmpdir5/katana/cmd/katana" ]; then
      ( cd "$tmpdir5/katana/cmd/katana" \
        && go mod tidy || true \
        && CGO_ENABLED=1 go build -o "$KUBE_DIR/katana" . ) && katana_ok=1 || true
    else
      ( cd "$tmpdir5/katana" \
        && go mod tidy || true \
        && CGO_ENABLED=1 go build -o "$KUBE_DIR/katana" ./cmd/katana ) && katana_ok=1 || true
    fi
  fi
  rm -rf "$tmpdir5" || true
fi

# Essai 2 : binaire précompilé si build Go KO
if [ "${katana_ok}" -ne 1 ]; then
  echo "    fallback binaire précompilé…"
  os="$(uname -s | tr '[:upper:]' '[:lower:]')"
  arch="$(uname -m)"
  case "$arch" in
    x86_64|amd64)  kat_arch="amd64" ;;
    aarch64|arm64) kat_arch="arm64" ;;
    *) kat_arch="" ;;
  esac
  if [ -n "${kat_arch}" ]; then
    tmpzip="$(mktemp)"
    url="https://github.com/projectdiscovery/katana/releases/latest/download/katana_${os}_${kat_arch}.zip"
    if curl -fsSL "$url" -o "$tmpzip"; then
      unzip -o "$tmpzip" -d "$KUBE_DIR" >/dev/null
      rm -f "$tmpzip"
      katana_ok=1
    else
      echo "    Impossible de télécharger ${url}"
    fi
  else
    echo "    Architecture non supportée pour binaire précompilé: ${arch}"
  fi
fi

# Vérif finale : le fichier existe et est exécutable ?
if [ -x "$KUBE_DIR/katana" ]; then
  chmod +x "$KUBE_DIR/katana" || true
  katana_ok=1
fi

if [ "${katana_ok}" -ne 1 ]; then
  echo "ERR: Katana n’a pas pu être installé. Vérifie le toolchain CGO (gcc) et ta connectivité."
  exit 1
fi

# Symlink global (fonctionne même si PATH n’est pas modifié)
symlink_bin "$KUBE_DIR/katana" "katana"

echo
echo "[✓] Binaires installés dans $KUBE_DIR :"
ls -lh "$KUBE_DIR" || true

# Test Katana (affiche l’aide/version)
echo
echo "[*] Vérification Katana…"
if command -v katana >/dev/null 2>&1; then
  echo "  -> katana trouvé: $(command -v katana)"
  katana -version || katana -h || true
else
  echo "  -> katana non trouvé dans le PATH."
fi

echo
echo "Ajoute ./kube à ton PATH pour cette session (optionnel) :"
echo "  export PATH=\"$KUBE_DIR:\$PATH\""