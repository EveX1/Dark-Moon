#!/usr/bin/env bash
set -euo pipefail

# Reprend tes helpers si disponibles
command -v msg >/dev/null 2>&1 || msg(){ echo "[*] $*"; }
command -v ok  >/dev/null 2>&1 || ok(){  echo "[OK] $*"; }
command -v warn>/dev/null 2>&1 || warn(){echo "[WARN] $*" >&2; }

PY_PREFIX="/opt/darkmoon/python"
PY_BIN="$PY_PREFIX/bin/python3"
PIP_BIN="$PY_PREFIX/bin/pip3"

msg "Vérification runtime Python embarqué…"
if [ ! -x "$PY_BIN" ]; then
  warn "Python non trouvé à $PY_BIN. Construis d'abord Python dans le Dockerfile (bloc builder) puis recopie /opt/darkmoon/python dans l'image runtime."
  exit 1
fi

# Pas de cache, wheels manylinux quand dispo
"$PIP_BIN" --version

# Pins raisonnables et compatibles 2025
PIN_IMPACKET="==0.12.0"
PIN_NETEXEC="==1.10.2"     # (nom PyPI: netexec — anciennement CME)
PIN_BLOODHOUND="==1.7.2"   # bloodhound (ingestor python)

msg "Installation impacket$PIN_IMPACKET …"
"$PIP_BIN" install --no-cache-dir "impacket${PIN_IMPACKET}" && ok "impacket"

msg "Installation netexec$PIN_NETEXEC …"
"$PIP_BIN" install --no-cache-dir "netexec${PIN_NETEXEC}" && ok "netexec"

msg "Installation bloodhound$PIN_BLOODHOUND …"
"$PIP_BIN" install --no-cache-dir "bloodhound${PIN_BLOODHOUND}" && ok "bloodhound (ingestor)"

# Wrappers conviviaux dans /usr/local/bin
install -d /usr/local/bin

cat >/usr/local/bin/impacket-smbclient <<'EOF'
#!/bin/sh
exec /opt/darkmoon/python/bin/python3 -m impacket.smbclient "$@"
EOF
chmod +x /usr/local/bin/impacket-smbclient

cat >/usr/local/bin/netexec <<'EOF'
#!/bin/sh
exec /opt/darkmoon/python/bin/netexec "$@"
EOF
chmod +x /usr/local/bin/netexec

cat >/usr/local/bin/bloodhound-python <<'EOF'
#!/bin/sh
# alias explicite pour l’ingestor python (évite la confusion avec l’app desktop)
exec /opt/darkmoon/python/bin/bloodhound "$@"
EOF
chmod +x /usr/local/bin/bloodhound-python

ok "Wrappers: impacket-smbclient, netexec, bloodhound-python"