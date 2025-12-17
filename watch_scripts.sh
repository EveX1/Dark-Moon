#!/usr/bin/env bash
set -euo pipefail

SCRIPTS_DIR="${1:-/opt/darkmoon/scripts}"

# Si le FS supporte chmod, on force une première passe
chmod -R a+rx "$SCRIPTS_DIR" 2>/dev/null || true

# Trigger temps réel: create/modify/move => chmod immédiat
inotifywait -m -r \
  -e create,close_write,move,attrib \
  --format '%w%f' \
  "$SCRIPTS_DIR" \
| while read -r path; do
    # Si c'est un fichier, tente +x
    if [ -f "$path" ]; then
      chmod a+rx "$path" 2>/dev/null || true
    fi
  done