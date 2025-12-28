#!/usr/bin/env bash

set -e

# ---------
# Defaults
# ---------
SERVICE="darkmoon"

# ---------
# Help
# ---------
usage() {
  echo "Usage: darkmoon -p <prompt-file> -b <baseurl>"
  echo ""
  echo "Options:"
  echo "  -p    Prompt file (ex: dvga_extreme_autopwn.txt)"
  echo "  -b    Base URL (ex: http://dvga:5013/)"
  echo ""
  exit 1
}

# ----------------
# Argument parsing
# ----------------
while getopts "p:b:h" opt; do
  case $opt in
    p) PROMPT_FILE="$OPTARG" ;;
    b) BASEURL="$OPTARG" ;;
    h) usage ;;
    *) usage ;;
  esac
done

# ----------------
# Validation
# ----------------
if [[ -z "$PROMPT_FILE" || -z "$BASEURL" ]]; then
  usage
fi

# ----------------
# Execution
# ----------------
docker compose exec "$SERVICE" bash -lc "./agentfactory \
  --prompt-file \"$PROMPT_FILE\" \
  --baseurl \"$BASEURL\""