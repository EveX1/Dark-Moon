#!/usr/bin/env bash
set -euo pipefail

########################################
# PARAMÈTRES
########################################
BASE_URL=""
THREADS=50

while getopts "u:t:" opt; do
  case "$opt" in
    u) BASE_URL="${OPTARG%/}" ;;
    t) THREADS="$OPTARG" ;;
    *)
      echo "Usage: $0 -u http://target [-t threads]"
      exit 1
      ;;
  esac
done

[[ -z "$BASE_URL" ]] && {
  echo "Missing -u"
  exit 1
}

########################################
# WORDLIST EMBARQUÉE
########################################
WORDLIST="$(mktemp)"
cat > "$WORDLIST" << 'EOF'
admin
api
rest
v1
v2
users
user
login
logout
register
products
product
search
basket
cart
checkout
orders
order
profile
status
health
metrics
config
debug
test
EOF

########################################
# DOSSIERS TEMP
########################################
WORKDIR="$(mktemp -d)"
FIRST_PASS_JSON="$WORKDIR/ffuf_root.json"
SECOND_PASS_DIR="$WORKDIR/second_pass"

mkdir -p "$SECOND_PASS_DIR"

########################################
# 1ʳᵉ ITÉRATION FFUF (ROOT)
########################################
echo "[*] First pass on $BASE_URL"

ffuf \
  -u "$BASE_URL/FUZZ" \
  -w "$WORDLIST" \
  -mc 200,204,301,302,307,401,403 \
  -t "$THREADS" \
  -o "$FIRST_PASS_JSON" \
  -of json \
  -s

########################################
# EXTRACTION DES URLS DÉCOUVERTES
########################################
mapfile -t DISCOVERED_URLS < <(
  jq -r '.results[].url' "$FIRST_PASS_JSON" | sort -u
)

echo "[*] Discovered ${#DISCOVERED_URLS[@]} endpoints"

########################################
# 2ᵉ ITÉRATION FFUF (ASYNC)
########################################
echo "[*] Launching second pass (async)"

for URL in "${DISCOVERED_URLS[@]}"; do
  (
    SAFE_NAME="$(echo "$URL" | sed 's|https\?://||; s|/|_|g')"
    OUT_JSON="$SECOND_PASS_DIR/$SAFE_NAME.json"

    echo "  [+] Fuzzing $URL"

    ffuf \
      -u "$URL/FUZZ" \
      -w "$WORDLIST" \
      -mc 200,204,301,302,307,401,403 \
      -t "$THREADS" \
      -o "$OUT_JSON" \
      -of json \
      -s
  ) &
done

########################################
# ATTENTE DES JOBS
########################################
wait
echo "[✓] Second pass completed"

########################################
# RÉSUMÉ FINAL
########################################
echo "[*] Results stored in: $WORKDIR"
########################################
# SYNTHÈSE FINALE DES URLS
########################################
echo
echo "=============================="
echo " FINAL DISCOVERED ENDPOINTS"
echo "=============================="
echo

{
  # Root pass
  jq -r '.results[].url' "$FIRST_PASS_JSON"

  # Second pass
  for f in "$SECOND_PASS_DIR"/*.json; do
    [[ -f "$f" ]] || continue
    jq -r '.results[].url' "$f"
  done
} \
| sed "s|^$BASE_URL||" \
| sed 's|//|/|g' \
| sort -u
