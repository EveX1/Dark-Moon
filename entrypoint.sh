#!/usr/bin/env bash
set -euo pipefail

DM_IMAGE_DIR="/opt/darkmoon-image"
DM_HOME="${DM_HOME:-/opt/darkmoon}"

mkdir -p "$DM_HOME"

# Si première exécution (volume vide), on déploie l'arbo demandée
if [ ! -f "$DM_HOME/.initialized" ]; then
  echo "[*] Initialisation du volume dans $DM_HOME ..."
  # Copie binaire racine
  cp -a "$DM_IMAGE_DIR/agentfactory" "$DM_HOME/" || true
  cp -a "$DM_IMAGE_DIR/mcp"          "$DM_HOME/" || true
  cp -a "$DM_IMAGE_DIR/ZAP-CLI"      "$DM_HOME/" || true
  # Dossier kube (binaires Go)
  mkdir -p "$DM_HOME/kube"
  cp -a "$DM_IMAGE_DIR/kube/." "$DM_HOME/kube/" || true
  # Dossier prompt (au parent de l'app = à la racine du volume, comme demandé)
  mkdir -p "$DM_HOME/prompt"
  cp -a "$DM_IMAGE_DIR/prompt/." "$DM_HOME/prompt/" || true
  # Fichiers prompt-rapport* à la racine
  cp -a "$DM_IMAGE_DIR"/prompt-rapport*.txt "$DM_HOME/" || true
    # Fichiers prompt-rapport* à la racine
  if [ -f "$DM_IMAGE_DIR/mon_prompt.txt" ]; then
  install -m 0644 "$DM_IMAGE_DIR/mon_prompt.txt" "$DM_HOME/mon_prompt.txt" \
    && echo "[OK] mon_prompt.txt copié"
  else
    echo "[WARN] mon_prompt.txt absent de l'image au premier run"
  fi

  # Marqueur d'init
  touch "$DM_HOME/.initialized"
  echo "[✓] Volume initialisé."
fi

# Garantir mon_prompt.txt même après init (au cas où la copie d’init a été bloquée par le FS)
if [ ! -f "$DM_HOME/mon_prompt.txt" ] && [ -f "$DM_IMAGE_DIR/mon_prompt.txt" ]; then
  install -m 0644 "$DM_IMAGE_DIR/mon_prompt.txt" "$DM_HOME/mon_prompt.txt" \
    && echo "[OK] mon_prompt.txt rattrapé"
fi

# Ajoute au PATH pour cette session
export PATH="$DM_HOME:$DM_HOME/kube:$PATH"

# watcher scripts en background (trigger)
if [ -d /opt/darkmoon/scripts ]; then
  /opt/darkmoon/bin/watch_scripts.sh /opt/darkmoon/scripts >/dev/null 2>&1 &
fi

exec "$@"