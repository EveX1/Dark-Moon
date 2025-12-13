#!/usr/bin/env bash
set -euo pipefail

###############################################
# DARKMOON - WEB ONE-SHOT (ENDPOINT)
# - ZAP seed + active scan + alerts
# - Arjun (GET param discovery) avec auto-skip GraphQL
# - SQLmap (safe guarded) avec timeout hard
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
  -S  SQLmap: true|false|auto (default: auto; auto désactive si GraphQL)
  -t  SQLmap timeout seconds (default: 180)
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
  # heuristiques rapides
  [[ "$u" =~ /graphql(\?|$) ]] && return 0
  [[ "$u" =~ graphql ]] && return 0

  # best effort: probe rapide
  local headers
  headers="$(curl -sS -m 4 -D - -o /tmp/_dm_probe_body "$u" || true)"
  if grep -qi '^content-type:.*application/json' <<<"$headers"; then
    if jq -e '.errors? // empty' </tmp/_dm_probe_body >/dev/null 2>&1; then
      return 0
    fi
  fi
  return 1
}

should_enable_auto() {
  # $1=mode(auto|true|false), $2=is_graphql(0/1)
  local mode="$1"
  local isg="$2"
  if [ "$mode" = "true" ]; then echo "true"; return; fi
  if [ "$mode" = "false" ]; then echo "false"; return; fi
  # auto:
  if [ "$isg" = "1" ]; then echo "false"; else echo "true"; fi
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

# Arjun optionnel (puisqu’on peut le désactiver)
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

    PARAMS="$(jq -r '.[].params[]? // empty' "$ARJUN_TMP" 2>/dev/null | sort -u || true)"
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
  : > "$ARJUN_TMP"
fi
echo

############## [4] SEED ZAP ##############

echo "==========================================="
echo "[4] Seeding ZAP"
echo "==========================================="

FIRST_ENRICHED=""

if [ -n "${PARAMS:-}" ]; then
  for P in $PARAMS; do
    ENRICHED="${TARGET_BASE}?${P}=darkmoon_test"
    echo "   -> Seed via proxy ZAP : $ENRICHED"
    if [ -n "${GLOBAL_JWT:-}" ]; then
      curl -s -x "$ZAP" "$ENRICHED" -H "Authorization: Bearer $GLOBAL_JWT" >/dev/null || true
    else
      curl -s -x "$ZAP" "$ENRICHED" >/dev/null || true
    fi
    if [ -z "$FIRST_ENRICHED" ]; then
      FIRST_ENRICHED="$ENRICHED"
    fi
  done
else
  FIRST_ENRICHED="$TARGET_BASE"
  echo "   -> Seed via proxy ZAP : $FIRST_ENRICHED"
  if [ -n "${GLOBAL_JWT:-}" ]; then
    curl -s -x "$ZAP" "$FIRST_ENRICHED" -H "Authorization: Bearer $GLOBAL_JWT" >/dev/null || true
  else
    curl -s -x "$ZAP" "$FIRST_ENRICHED" >/dev/null || true
  fi
fi

if [ -z "$FIRST_ENRICHED" ]; then
  echo "[!] Aucune URL seed générée."
  exit 1
fi

echo
echo "[i] URL choisie pour l'Active Scan ZAP : $FIRST_ENRICHED"
echo

############## [5] ZAP SCAN ##############

echo "==========================================="
echo "[5] Activation des scanners ZAP"
echo "==========================================="
curl -s "$ZAP/JSON/ascan/action/enableAllScanners/?apikey=$APIKEY" >/dev/null || true
curl -s "$ZAP/JSON/pscan/action/setEnabled/?apikey=$APIKEY&enabled=true" >/dev/null || true
echo "[OK] Active + Passive scanners activés."
echo

echo "==========================================="
echo "[6] Lancement Active Scan ZAP sur :"
echo "    $FIRST_ENRICHED"
echo "==========================================="

ESCAPED_URL="$(printf '%s' "$FIRST_ENRICHED" | sed 's/&/%26/g')"

SCAN_ID="$(
  curl -s "$ZAP/JSON/ascan/action/scan/?apikey=$APIKEY&url=$ESCAPED_URL&recurse=false&inScopeOnly=false" \
  | jq -r '.scan // empty'
)"

if [ -z "$SCAN_ID" ] || [ "$SCAN_ID" = "null" ]; then
  echo "[!] Impossible de démarrer le scan actif (SCAN_ID vide ou null)."
else
  echo "[OK] Scan ID : $SCAN_ID"
  echo
  echo "[6-bis] Progression :"
  while true; do
    RAW="$(curl -s "$ZAP/JSON/ascan/view/status/?apikey=$APIKEY&scanId=$SCAN_ID")"
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

curl -s "$ZAP/JSON/core/view/alerts/?apikey=$APIKEY&start=0&count=9999" \
  | jq -c --arg URL "$FIRST_ENRICHED" '
      .alerts[]
      | select(.url == $URL and (.risk == "Medium" or .risk == "High"))
      | {risk, alert, url, param, evidence}
    ' || echo "[!] Impossible de parser les alertes ZAP."
echo