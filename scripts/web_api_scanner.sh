#!/usr/bin/env bash
set -euo pipefail

###############################################
# DARKMOON - WEB ONE-SHOT (ENDPOINT)
# - ZAP seed + active scan + alerts
# - Arjun (GET param discovery) avec auto-skip GraphQL
# - Seed POST JSON pour endpoints type login (ex: Juice Shop)
#
# NOTE: pas de brute-force login ici.
#       -> Fournis un JWT via /tmp/darkmoon_jwt_token.txt ou env GLOBAL_JWT
###############################################

usage() {
  cat <<'USAGE'
Usage:
  ./script -u <URL> [options]

Required:
  -u  Target URL (ex: http://juice-shop:3000/rest/products/search or http://dvga:5013/graphql)

Options:
  -z  ZAP proxy URL (default: http://zap:8888)
  -k  ZAP API key file (default: /zap/wrk/ZAP-API-TOKEN)
  -A  Arjun: true|false|auto (default: auto; auto désactive si GraphQL)
  -S  SQLmap: true|false|auto (default: auto; auto désactive si GraphQL)  [NOT USED here]
  -t  SQLmap timeout seconds (default: 180)                              [NOT USED here]
  -h  Help
USAGE
}

############## DEFAULTS ##############

ZAP="${ZAP:-http://zap:8888}"
APIKEY_FILE="${APIKEY_FILE:-/zap/wrk/ZAP-API-TOKEN}"

ARJUN_ENABLED="${ARJUN_ENABLED:-auto}"   # true|false|auto
SQLMAP_ENABLED="${SQLMAP_ENABLED:-auto}" # true|false|auto
SQLMAP_TIMEOUT="${SQLMAP_TIMEOUT:-180}"

TARGET_BASE=""

# Fichiers temporaires partagés
ARJUN_TMP="/tmp/darkmoon_arjun_tmp.json"
JWT_FILE="/tmp/darkmoon_jwt_token.txt"

# Probe tmp
PROBE_BODY="/tmp/_dm_probe_body"

############## CLI PARSING ##############

while getopts ":u:z:k:A:S:t:h" opt; do
  case "$opt" in
    u) TARGET_BASE="$OPTARG" ;;
    z) ZAP="$OPTARG" ;;
    k) APIKEY_FILE="$OPTARG" ;;
    A) ARJUN_ENABLED="$OPTARG" ;;
    S) SQLMAP_ENABLED="$OPTARG" ;;
    t) SQLMAP_TIMEOUT="$OPTARG" ;;
    h) usage; exit 0 ;;
    \?) echo "[!] Option invalide: -$OPTARG"; usage; exit 1 ;;
    :)  echo "[!] Option -$OPTARG requiert une valeur"; usage; exit 1 ;;
  esac
done

if [ -z "${TARGET_BASE:-}" ]; then
  echo "[!] Tu dois fournir une URL avec -u"
  usage
  exit 1
fi

# Normalisation URL: supprime les doubles slash hors schéma (http://)
TARGET_BASE="$(printf '%s' "$TARGET_BASE" | sed 's#://#§#; s#//#/#g; s#§#://#')"

if [ ! -r "$APIKEY_FILE" ]; then
  echo "[!] Impossible de lire la clé API ZAP: $APIKEY_FILE"
  exit 1
fi
APIKEY="$(cat "$APIKEY_FILE")"

############## HELPERS ##############

jwt_decode_payload() {
  local token="$1"
  local payload
  payload="$(printf '%s' "$token" | cut -d. -f2 || true)"
  payload="$(printf '%s' "$payload" | tr '_-' '/+' )"

  local mod=$(( ${#payload} % 4 ))
  if [ $mod -eq 2 ]; then
    payload="${payload}=="
  elif [ $mod -eq 3 ]; then
    payload="${payload}="
  fi

  printf '%s' "$payload" | base64 -d 2>/dev/null || printf '{}'
}

is_graphql_target() {
  local u="$1"
  [[ "$u" =~ /graphql(\?|$) ]] && return 0
  [[ "$u" =~ graphql ]] && return 0

  local headers
  headers="$(curl -sS -m 4 -D - -o "$PROBE_BODY" "$u" || true)"
  if grep -qi '^content-type:.*application/json' <<<"$headers"; then
    if jq -e '.errors? // empty' <"$PROBE_BODY" >/dev/null 2>&1; then
      return 0
    fi
  fi
  return 1
}

should_enable_auto() {
  local mode="$1"
  local isg="$2"
  if [ "$mode" = "true" ]; then echo "true"; return; fi
  if [ "$mode" = "false" ]; then echo "false"; return; fi
  if [ "$isg" = "1" ]; then echo "false"; else echo "true"; fi
}

is_likely_json_login() {
  local u="$1"
  [[ "$u" =~ /rest/user/login$ ]] && return 0
  [[ "$u" =~ /login$ ]] && return 0
  return 1
}

seed_get_via_zap_proxy() {
  local url="$1"
  if [ -n "${GLOBAL_JWT:-}" ]; then
    curl -s -x "$ZAP" "$url" -H "Authorization: Bearer $GLOBAL_JWT" >/dev/null || true
  else
    curl -s -x "$ZAP" "$url" >/dev/null || true
  fi
}

seed_post_json_via_zap_proxy() {
  local url="$1"
  local json_body="$2"
  if [ -n "${GLOBAL_JWT:-}" ]; then
    curl -s -x "$ZAP" "$url" \
      -H "Authorization: Bearer $GLOBAL_JWT" \
      -H "Content-Type: application/json" \
      -d "$json_body" >/dev/null || true
  else
    curl -s -x "$ZAP" "$url" \
      -H "Content-Type: application/json" \
      -d "$json_body" >/dev/null || true
  fi
}

zap_access_url() {
  # Forcer l'URL dans le Sites Tree / Scan Tree de ZAP
  local url="$1"
  curl -sG "$ZAP/JSON/core/action/accessUrl/" \
    --data-urlencode "apikey=$APIKEY" \
    --data-urlencode "url=$url" \
    --data-urlencode "followRedirects=true" >/dev/null || true
}

############## [0] CONTEXTE / JWT ##############

GLOBAL_JWT="${GLOBAL_JWT:-}"

if [ -z "$GLOBAL_JWT" ] && [ -s "$JWT_FILE" ]; then
  GLOBAL_JWT="$(cat "$JWT_FILE" 2>/dev/null || true)"
fi

echo "==========================================="
echo "[0] Contexte"
echo "==========================================="
echo "[i] Endpoint ciblé : $TARGET_BASE"
echo "[i] ZAP proxy      : $ZAP"
if [ -n "${GLOBAL_JWT:-}" ]; then
  echo "[i] JWT présent    : oui (env GLOBAL_JWT ou $JWT_FILE)"
else
  echo "[i] JWT présent    : non (tu peux en fournir un via env GLOBAL_JWT ou $JWT_FILE)"
fi
echo

############## [0-bis] BINAIRES REQUIS ##############

MISSING=0
REQUIRED=(curl jq nuclei sqlmap)
for bin in "${REQUIRED[@]}"; do
  if ! command -v "$bin" >/dev/null 2>&1; then
    echo "[!] Binaire manquant : $bin"
    MISSING=1
  else
    echo "[OK] $bin trouvé : $(command -v "$bin")"
  fi
done

if command -v arjun >/dev/null 2>&1; then
  echo "[OK] arjun trouvé : $(command -v arjun)"
else
  echo "[i] arjun absent (ok si Arjun désactivé/auto-skip)"
fi

if [ "$MISSING" -eq 1 ]; then
  echo "[!] Installe les binaires manquants avant de relancer."
  exit 1
fi
echo

############## [1] JWT CHECK LOCAL (OPTIONNEL) ##############

if [ -n "${GLOBAL_JWT:-}" ]; then
  echo "==========================================="
  echo "[1] Vérification JWT (local decode)"
  echo "==========================================="
  DECODED_PAYLOAD="$(jwt_decode_payload "$GLOBAL_JWT")"
  echo "$DECODED_PAYLOAD" | jq . 2>/dev/null || echo "$DECODED_PAYLOAD"
  echo
fi

############## DETECTION GRAPHQL ##############

IS_GRAPHQL=0
if is_graphql_target "$TARGET_BASE"; then
  IS_GRAPHQL=1
fi

echo "==========================================="
echo "[2] Détection type cible"
echo "==========================================="
if [ "$IS_GRAPHQL" -eq 1 ]; then
  echo "[i] Cible détectée comme GraphQL -> Arjun/SQLmap en auto seront désactivés."
else
  echo "[i] Cible détectée comme HTTP “classique”."
fi
echo

############## [3] ARJUN (GET PARAM DISCOVERY) ##############

ARJUN_ON="$(should_enable_auto "$ARJUN_ENABLED" "$IS_GRAPHQL")"

echo "==========================================="
echo "[3] Découverte de paramètres avec Arjun"
echo "    Cible : $TARGET_BASE"
echo "==========================================="

PARAMS=""

: > "$ARJUN_TMP"

if [ "$ARJUN_ON" = "true" ] && command -v arjun >/dev/null 2>&1; then
  set +e
  arjun -u "$TARGET_BASE" -m GET -oJ "$ARJUN_TMP"
  ARJUN_RC=$?
  set -e

  if [ "$ARJUN_RC" -ne 0 ]; then
    echo "[!] Arjun a échoué (non bloquant). Skip."
    : > "$ARJUN_TMP"
  fi

  if [ -s "$ARJUN_TMP" ]; then
    echo "[i] Résultat brut Arjun :"
    cat "$ARJUN_TMP" || true
    echo

    PARAMS_EXACT="$(jq -r --arg U "$TARGET_BASE" '
      if has($U) then .[$U].params[]? else empty end
    ' "$ARJUN_TMP" 2>/dev/null | sort -u || true)"

    if [ -n "$PARAMS_EXACT" ]; then
      PARAMS="$PARAMS_EXACT"
    else
      PARAMS="$(jq -r '.[].params[]? // empty' "$ARJUN_TMP" 2>/dev/null | sort -u || true)"
    fi

    if [ -n "$PARAMS" ]; then
      echo "[OK] Paramètres trouvés : $PARAMS"
    else
      echo "[i] Aucun paramètre exploitable trouvé."
    fi
  else
    echo "[i] Fichier Arjun vide."
  fi
else
  echo "[i] Skip Arjun (ARJUN_ENABLED=$ARJUN_ENABLED, GraphQL=$IS_GRAPHQL, ou arjun absent)."
fi
echo

############## [4] SEED ZAP ##############

echo "==========================================="
echo "[4] Seeding ZAP"
echo "==========================================="

FIRST_ENRICHED=""

# Cas spécial: endpoint login / JSON POST => seed un POST JSON
if [ "$IS_GRAPHQL" -eq 0 ] && is_likely_json_login "$TARGET_BASE"; then
  echo "   -> Seed POST JSON via proxy ZAP : $TARGET_BASE"
  seed_post_json_via_zap_proxy "$TARGET_BASE" '{"email":"test@test.com","password":"test"}'
  FIRST_ENRICHED="$TARGET_BASE"
fi

if [ -z "$FIRST_ENRICHED" ]; then
  if [ -n "${PARAMS:-}" ]; then
    for P in $PARAMS; do
      ENRICHED="${TARGET_BASE}?${P}=darkmoon_test"
      echo "   -> Seed via proxy ZAP : $ENRICHED"
      seed_get_via_zap_proxy "$ENRICHED"
      if [ -z "$FIRST_ENRICHED" ]; then
        FIRST_ENRICHED="$ENRICHED"
      fi
    done
  else
    FIRST_ENRICHED="$TARGET_BASE"
    echo "   -> Seed via proxy ZAP : $FIRST_ENRICHED"
    seed_get_via_zap_proxy "$FIRST_ENRICHED"
  fi
fi

if [ -z "$FIRST_ENRICHED" ]; then
  echo "[!] Aucune URL seed générée."
  exit 1
fi

echo
echo "[i] URL choisie pour l'Active Scan ZAP : $FIRST_ENRICHED"
echo

echo "==========================================="
echo "[5] ZAP – Scan AGRESSIF (blackbox, URL only)"
echo "==========================================="

# 1️⃣ Activer TOUS les scanners (stable + alpha + beta)
curl -sG "$ZAP/JSON/ascan/action/enableAllScanners/" \
  --data-urlencode "apikey=$APIKEY" >/dev/null || true

# 2️⃣ Attack strength = HIGH, Alert threshold = LOW
for id in $(curl -sG "$ZAP/JSON/ascan/view/scanners/" \
  --data-urlencode "apikey=$APIKEY" | jq -r '.scanners[].id'); do
  curl -sG "$ZAP/JSON/ascan/action/setScannerAttackStrength/" \
    --data-urlencode "apikey=$APIKEY" \
    --data-urlencode "id=$id" \
    --data-urlencode "attackStrength=HIGH" >/dev/null || true

  curl -sG "$ZAP/JSON/ascan/action/setScannerAlertThreshold/" \
    --data-urlencode "apikey=$APIKEY" \
    --data-urlencode "id=$id" \
    --data-urlencode "alertThreshold=LOW" >/dev/null || true
done

# 3️⃣ Activer les input vectors avancés
curl -sG "$ZAP/JSON/ascan/action/setOptionAttackHeaders/" \
  --data-urlencode "apikey=$APIKEY" \
  --data-urlencode "Boolean=true" >/dev/null || true

curl -sG "$ZAP/JSON/ascan/action/setOptionAttackJSONParameters/" \
  --data-urlencode "apikey=$APIKEY" \
  --data-urlencode "Boolean=true" >/dev/null || true

curl -sG "$ZAP/JSON/ascan/action/setOptionAttackURLPath/" \
  --data-urlencode "apikey=$APIKEY" \
  --data-urlencode "Boolean=true" >/dev/null || true

# 4️⃣ Passif activé (pour DOM XSS, headers, SPA leaks)
curl -sG "$ZAP/JSON/pscan/action/setEnabled/" \
  --data-urlencode "apikey=$APIKEY" \
  --data-urlencode "enabled=true" >/dev/null || true

# 5️⃣ Forcer l’URL dans le Sites Tree
zap_access_url "$FIRST_ENRICHED"

# 6️⃣ Lancer l’Active Scan STRICTEMENT sur l’URL
RAW_SCAN_RESP="$(
  curl -sG "$ZAP/JSON/ascan/action/scan/" \
    --data-urlencode "apikey=$APIKEY" \
    --data-urlencode "url=$FIRST_ENRICHED" \
    --data-urlencode "recurse=false" \
    --data-urlencode "inScopeOnly=false"
)"

SCAN_ID="$(echo "$RAW_SCAN_RESP" | jq -r '.scan // empty')"

if [ -z "$SCAN_ID" ] || [ "$SCAN_ID" = "null" ]; then
  echo "[!] Active scan non démarré"
  echo "$RAW_SCAN_RESP"
else
  echo "[OK] Active Scan ID: $SCAN_ID"
  while true; do
    PCT="$(curl -sG "$ZAP/JSON/ascan/view/status/" \
      --data-urlencode "apikey=$APIKEY" \
      --data-urlencode "scanId=$SCAN_ID" | jq -r '.status')"
    echo -ne "   ActiveScan: $PCT%   \r"
    [ "$PCT" -ge 100 ] && break
    sleep 1
  done
  echo
fi
echo


BASE_NOQUERY="$(printf '%s' "$FIRST_ENRICHED" | sed 's/[?].*$//')"

# IMPORTANT: ZAP refuse ascan si l'URL n'est pas dans le Scan Tree
echo "[i] Forçage de l'URL dans le Sites Tree (ZAP core/accessUrl)"
zap_access_url "$BASE_NOQUERY"
zap_access_url "$FIRST_ENRICHED"

RAW_SCAN_RESP="$(
  curl -sG "$ZAP/JSON/ascan/action/scan/" \
    --data-urlencode "apikey=$APIKEY" \
    --data-urlencode "url=$BASE_NOQUERY" \
    --data-urlencode "recurse=false" \
    --data-urlencode "inScopeOnly=false"
)"

SCAN_ID="$(printf '%s' "$RAW_SCAN_RESP" | jq -r '.scan // empty' 2>/dev/null || true)"

if [ -z "${SCAN_ID:-}" ] || [ "$SCAN_ID" = "null" ]; then
  echo "[!] Impossible de démarrer le scan actif (SCAN_ID vide ou null)."
  echo "[i] Réponse brute ZAP /ascan/action/scan :"
  echo "$RAW_SCAN_RESP"

  echo
  echo "[i] Sanity checks utiles :"
  echo "   -> ZAP version:"
  curl -sG "$ZAP/JSON/core/view/version/" --data-urlencode "apikey=$APIKEY" || true
  echo
  echo "   -> Access check cible:"
  if is_likely_json_login "$BASE_NOQUERY"; then
    curl -s -o /dev/null -w "HTTP=%{http_code}\n" \
      -H "Content-Type: application/json" \
      -d '{"email":"test@test.com","password":"test"}' \
      "$BASE_NOQUERY" || true
  else
    curl -s -o /dev/null -w "HTTP=%{http_code}\n" "$BASE_NOQUERY" || true
  fi
else
  echo "[OK] Scan ID : $SCAN_ID"
  echo
  echo "[6-bis] Progression :"
  while true; do
    RAW="$(curl -sG "$ZAP/JSON/ascan/view/status/" \
      --data-urlencode "apikey=$APIKEY" \
      --data-urlencode "scanId=$SCAN_ID"
    )"
    PCT="$(echo "$RAW" | jq -r '.status // "0"')"
    if ! [[ "$PCT" =~ ^[0-9]+$ ]]; then
      PCT="0"
    fi
    echo -ne "   $PCT%   \r"
    if [ "$PCT" -ge 100 ]; then
      break
    fi
    sleep 1
  done
  echo
  echo "[OK] Active Scan terminé."
fi
echo

############## [7] ZAP ALERTS ##############

echo "==========================================="
echo "[7] Alertes ZAP Medium/High (filtrées)"
echo "==========================================="

# Filtre prefix sur l'URL de base
BASE_ALERT="$(printf '%s' "$BASE_NOQUERY" | sed 's/[?].*$//')"

curl -sG "$ZAP/JSON/core/view/alerts/" \
  --data-urlencode "apikey=$APIKEY" \
  --data-urlencode "start=0" \
  --data-urlencode "count=9999" \
  | jq -c --arg BASE "$BASE_ALERT" '
      .alerts[]
      | select((.risk == "Medium" or .risk == "High"))
      | select((.url | startswith($BASE)))
      | {risk, alert, url, param, evidence}
    ' || echo "[!] Impossible de parser les alertes ZAP."
echo

############## [8] NUCLEI ##############
echo "==========================================="
echo "[8] Nuclei (URL)"
echo "==========================================="

if [ "$NUCLEI_ENABLED" = "true" ]; then
  : > "$NUCLEI_OUT"
  # Safe default: no aggressive fuzzing; still very useful for known misconfigs
  nuclei -u "$BASE_NOQUERY" -jsonl -o "$NUCLEI_OUT" >/dev/null 2>&1 || true
  if [ -s "$NUCLEI_OUT" ]; then
    echo "[OK] Nuclei findings:"
    cat "$NUCLEI_OUT"
  else
    echo "[i] Nuclei: rien."
  fi
else
  echo "[i] Nuclei désactivé."
fi
echo