# Pins (monte x/crypto/x/net assez haut pour couvrir 2024/2025)
PIN_X_CRYPTO="v0.37.0"   # (>= 0.35.0 et 0.31.0)
PIN_X_NET="v0.41.0"      # (>= 0.38.0)
PIN_X_TEXT="v0.15.0"     # OK
PIN_OAUTH2="v0.27.0"     # fixe 2025-22868
PIN_YAML_V3="v3.0.1"     # fixe 2022-28948
PIN_PROTOBUF="v1.34.2"   # fixe 2024-24786

msg "Installation des outils dans $KUBE_DIR (FORCAGE build pin + CGO si besoin)"

# 0) waybackurls (no CVE connu, mais pin indirects quand même)
msg "waybackurls …"
go_build_with_pins "https://github.com/tomnomnom/waybackurls" "." "waybackurls" || warn "waybackurls KO"

# 1) kubectl-who-can
msg "kubectl-who-can …"
go_build_with_pins "https://github.com/aquasecurity/kubectl-who-can" "cmd/kubectl-who-can" "kubectl-who-can" || warn "kubectl-who-can KO"

# 2) kubeletctl (deps spécifiques)
msg "kubeletctl …"
go_build_with_pins "https://github.com/cyberark/kubeletctl" "cmd/kubeletctl" "kubeletctl" \
  "go get github.com/mitchellh/go-ps@v1.0.0" || warn "kubeletctl KO"

# 3) rbac-police
msg "rbac-police …"
go_build_with_pins "https://github.com/PaloAltoNetworks/rbac-police" "." "rbac-police" || warn "rbac-police KO"

# 4) kubescape (sous-dossier cmd/cli si présent)
msg "kubescape …"
tmp="$(mktemp -d)"
if git clone --depth=1 https://github.com/kubescape/kubescape "$tmp/src"; then
  if [ -d "$tmp/src/cmd/cli" ]; then
    (
      cd "$tmp/src/cmd/cli"
      go get "golang.org/x/crypto@${PIN_X_CRYPTO}"
      go get "golang.org/x/net@${PIN_X_NET}"
      go get "golang.org/x/text@${PIN_X_TEXT}"
      go get "golang.org/x/oauth2@${PIN_OAUTH2}"
      go get "google.golang.org/protobuf@${PIN_PROTOBUF}"
      go get "gopkg.in/yaml.v3@${PIN_YAML_V3}"
      go mod tidy
      CGO_ENABLED=0 go build -trimpath -ldflags="-s -w" -o "$KUBE_DIR/kubescape" .
    )
  else
    (
      cd "$tmp/src"
      go get "golang.org/x/crypto@${PIN_X_CRYPTO}"
      go get "golang.org/x/net@${PIN_X_NET}"
      go get "golang.org/x/text@${PIN_X_TEXT}"
      go get "golang.org/x/oauth2@${PIN_OAUTH2}"
      go get "google.golang.org/protobuf@${PIN_PROTOBUF}"
      go get "gopkg.in/yaml.v3@${PIN_YAML_V3}"
      go mod tidy
      CGO_ENABLED=0 go build -trimpath -ldflags="-s -w" -o "$KUBE_DIR/kubescape" .
    )
  fi
fi
rm -rf "$tmp" || true
[ -x "$KUBE_DIR/kubescape" ] && ok "kubescape" || warn "kubescape KO"

# 5) katana — CGO=1
msg "katana …"
tmp="$(mktemp -d)"
if git clone --depth=1 https://github.com/projectdiscovery/katana "$tmp/src"; then
  if [ -d "$tmp/src/cmd/katana" ]; then
    (
      cd "$tmp/src/cmd/katana"
      go get "golang.org/x/crypto@${PIN_X_CRYPTO}"
      go get "golang.org/x/net@${PIN_X_NET}"
      go get "golang.org/x/text@${PIN_X_TEXT}"
      go get "google.golang.org/protobuf@${PIN_PROTOBUF}"
      go get "gopkg.in/yaml.v3@${PIN_YAML_V3}"
      go mod tidy
      CGO_ENABLED=1 go build -trimpath -ldflags="-s -w" -o "$KUBE_DIR/katana" .
    )
  else
    (
      cd "$tmp/src"
      go get "golang.org/x/crypto@${PIN_X_CRYPTO}"
      go get "golang.org/x/net@${PIN_X_NET}"
      go get "golang.org/x/text@${PIN_X_TEXT}"
      go get "google.golang.org/protobuf@${PIN_PROTOBUF}"
      go get "gopkg.in/yaml.v3@${PIN_YAML_V3}"
      go mod tidy
      CGO_ENABLED=1 go build -trimpath -ldflags="-s - w" -o "$KUBE_DIR/katana" ./cmd/katana
    )
  fi
fi
rm -rf "$tmp" || true
[ -x "$KUBE_DIR/katana" ] && ok "katana" || warn "katana KO"

# Récapitulatif
msg "Binaires installés dans $KUBE_DIR :"
ls -lh "$KUBE_DIR" || true