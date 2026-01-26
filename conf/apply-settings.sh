#!/usr/bin/env bash
set -euo pipefail

#######################################
# Paths
#######################################
ENV_FILE="/root/.env"

OPENCODE_CONFIG_DIR="/root/.config/opencode"
OPENCODE_CONFIG_FILE="$OPENCODE_CONFIG_DIR/opencode.json"

OPENCODE_AUTH_DIR="/root/.local/share/opencode"
OPENCODE_AUTH_FILE="$OPENCODE_AUTH_DIR/auth.json"

fail() { echo "❌ $*" >&2; exit 1; }
log()  { echo "[INIT] $*" >&2; }

#######################################
# Load .env safely (CRLF safe)
#######################################
if [ -f "$ENV_FILE" ]; then
  while IFS= read -r line || [ -n "$line" ]; do
    line="${line//$'\r'/}"
    [[ -z "$line" || "$line" =~ ^[[:space:]]*# ]] && continue
    [[ "$line" =~ ^[A-Za-z_][A-Za-z0-9_]*=.*$ ]] || continue
    export "$line"
  done < "$ENV_FILE"
else
  log ".env not found → using fallback model"
fi

#######################################
# Decide model strategy (OpenRouter vs fallback)
#######################################
USE_OPENROUTER=true

if [ -z "${OPENROUTER_PROVIDER:-}" ] || \
   [ -z "${OPENROUTER_API_KEY:-}" ] || \
   [ -z "${OPENCODE_MODEL:-}" ]; then
  USE_OPENROUTER=false
fi

if [ "$USE_OPENROUTER" = true ]; then
  FINAL_MODEL="$OPENROUTER_PROVIDER/$OPENCODE_MODEL"
  log "Using OpenRouter model: $FINAL_MODEL"
else
  FINAL_MODEL="opencode/big-pickle"
  log "OpenRouter vars missing → fallback to $FINAL_MODEL"
fi

#######################################
# Create directories
#######################################
mkdir -p "$OPENCODE_CONFIG_DIR" "$OPENCODE_AUTH_DIR"

#######################################
# Write opencode.json (ALWAYS)
#######################################
cat > "$OPENCODE_CONFIG_FILE" <<EOF
{
  "\$schema": "https://opencode.ai/config.json",

  "mcp": {
    "darkmoon": {
      "type": "local",
      "command": ["/usr/local/bin/darkmoon-mcp"],
      "timeout": 36000000,
      "enabled": true
    }
  },

  "permission": { "*": "allow" },

  "agent": {
    "fastcmp-pentest": {
      "mode": "primary",
      "model": "$FINAL_MODEL",
      "mcp": ["darkmoon"],
      "prompt_file": "/root/.opencode/agents/fastcmp-pentest.md"
    }
  }
}
EOF

echo "✅ OpenCode config written to $OPENCODE_CONFIG_FILE"

#######################################
# Write auth.json ONLY if OpenRouter is used
#######################################
if [ "$USE_OPENROUTER" = true ]; then
  cat > "$OPENCODE_AUTH_FILE" <<EOF
{
  "$OPENROUTER_PROVIDER": {
    "type": "api",
    "key": "$OPENROUTER_API_KEY"
  }
}
EOF
  echo "✅ OpenCode auth written to $OPENCODE_AUTH_FILE"
else
  rm -f "$OPENCODE_AUTH_FILE"
  log "No auth.json written (fallback model does not require API key)"
fi

#######################################
# Optional warmup (SAFE, NON BLOCKING)
#######################################

echo "[INIT] optional opencode warmup…" >&2

# IMPORTANT:
# - call the binary directly (avoid bashrc wrapper requiring /dev/tty)
# - create a minimal env so it doesn't try to go TUI
# - hard-timeout + hard-kill fallback
if command -v /usr/local/bin/opencode >/dev/null 2>&1; then
  (
    export TERM=dumb
    export CI=1
    export OPENCODE_TUI=0
    export NO_COLOR=1

    # Run in background and kill hard if needed
    /usr/bin/timeout -k 1s 2s /usr/local/bin/opencode --model "$FINAL_MODEL" </dev/null >/dev/null 2>&1 || true
  ) || true
fi

log "Warmup finished"
echo "[INIT] warmup done" >&2
exit 0