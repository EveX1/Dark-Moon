#!/usr/bin/env bash
set -euo pipefail

# Helpers (définis seulement s'ils n'existent pas déjà)
if ! command -v msg >/dev/null 2>&1; then
  msg() {
    echo "[*] $*"
  }
fi

if ! command -v ok >/dev/null 2>&1; then
  ok() {
    echo "[OK] $*"
  }
fi

if ! command -v warn >/dev/null 2>&1; then
  warn() {
    echo "[WARN] $*" >&2
  }
fi

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
PIN_BLOODHOUND="==1.7.2"   # bloodhound (ingestor python)

msg "Installation impacket$PIN_IMPACKET …"
"$PIP_BIN" install --no-cache-dir "impacket${PIN_IMPACKET}" && ok "impacket"

msg "Installation bloodhound$PIN_BLOODHOUND …"
"$PIP_BIN" install --no-cache-dir "bloodhound${PIN_BLOODHOUND}" && ok "bloodhound (ingestor)"

# Wrappers conviviaux dans /usr/local/bin
install -d /usr/local/bin

# impacket.smbclient
cat >/usr/local/bin/impacket-smbclient <<'EOF'
#!/bin/sh
exec /opt/darkmoon/python/bin/python3 -m impacket.smbclient "$@"
EOF
chmod +x /usr/local/bin/impacket-smbclient

# netexec (ex-CrackMapExec) – utilise les binaires installés par pip (nxc / netexec)
cat >/usr/local/bin/netexec <<'EOF'
#!/bin/sh
if [ -x /opt/darkmoon/python/bin/nxc ]; then
  exec /opt/darkmoon/python/bin/nxc "$@"
elif [ -x /opt/darkmoon/python/bin/netexec ]; then
  exec /opt/darkmoon/python/bin/netexec "$@"
else
  echo "[-] NetExec (nxc) n'est pas installé dans /opt/darkmoon/python/bin" >&2
  exit 127
fi
EOF
chmod +x /usr/local/bin/netexec

# Alias CrackMapExec compat
cat >/usr/local/bin/crackmapexec <<'EOF'
#!/bin/sh
exec /usr/local/bin/netexec "$@"
EOF
chmod +x /usr/local/bin/crackmapexec

# bloodhound (ingestor python)
cat >/usr/local/bin/bloodhound-python <<'EOF'
#!/bin/sh
# alias explicite pour l’ingestor python (évite la confusion avec l’app desktop)
exec /opt/darkmoon/python/bin/bloodhound "$@"
EOF
chmod +x /usr/local/bin/bloodhound-python

# rpcdump.py (impacket)
cat >/usr/local/bin/rpcdump.py <<'EOF'
#!/bin/sh
exec /opt/darkmoon/python/bin/python3 -m impacket.examples.rpcdump "$@"
EOF
chmod +x /usr/local/bin/rpcdump.py

# wafw00f (venv Darkmoon)
cat >/usr/local/bin/wafw00f <<'EOF'
#!/bin/sh
exec /opt/darkmoon/python/bin/wafw00f "$@"
EOF
chmod +x /usr/local/bin/wafw00f

# sqlmap (venv Darkmoon)
cat >/usr/local/bin/sqlmap <<'EOF'
#!/bin/sh
exec /opt/darkmoon/python/bin/sqlmap "$@"
EOF
chmod +x /usr/local/bin/sqlmap

# awscli (venv Darkmoon)
cat >/usr/local/bin/aws <<'EOF'
#!/bin/sh
exec /opt/darkmoon/python/bin/aws "$@"
EOF
chmod +x /usr/local/bin/aws

ok "Wrappers: impacket-smbclient, netexec, crackmapexec, bloodhound-python, rpcdump.py, wafw00f, sqlmap, aws"

# Outils supplémentaires

msg "Installation wafw00f …"
"$PIP_BIN" install --no-cache-dir wafw00f && ok "wafw00f"

msg "Installation sqlmap …"
"$PIP_BIN" install --no-cache-dir sqlmap && ok "sqlmap"

msg "Installation awscli …"
"$PIP_BIN" install --no-cache-dir awscli && ok "awscli"
