# Build & run

## Build de la stack

```bash
docker compose up -d --build
```

> Repère sur ta machine : ~1200 s pour le build initial.

## Vérifier que tout est OK

```bash
docker compose ps
docker compose logs -f zap | sed -n '1,60p'
```

Quand `zap` est **healthy**, tu peux lancer les scans.

---

# Exécuter un pentest Web (avec ZAP)

Commande type (réseau interne Compose : hôte `zap`, port `8888`, token lu dans le volume partagé `/zap/wrk`) :

```bash
docker compose exec darkmoon bash -lc '
  cd "$DM_HOME" \
  && export TOKEN="$(cat /zap/wrk/ZAP-API-TOKEN)" \
  && ./agentfactory \
       --agentfactory \
       --zapcli ./ZAP-CLI \
       --mcp ./mcp \
       --host zap \
       --apikey "$TOKEN" \
       --baseurl "https://asc-it.fr/" \
       --outdir "zap_cli_out" \
       --port 8888 \
       --katana \
       --katana-bin ./kube/katana \
       -- \
       -depth 2 \
       -proxy http://zap:8888
'
```

Sorties attendues : un dossier de travail sous `"$DM_HOME/zap_cli_out/..."` contenant les artefacts (katana, ZAP, recon, rapports, etc.).

---

# Exécuter un pentest Kubernetes

## Préparer le kubeconfig pour le conteneur

Deux options :

**Option A (sans modifier compose)** — Copie ton kubeconfig dans le volume déjà monté :

```bash
cp ~/.kube/config "$HOME/darkmoon-docker-fs/kubeconfig"
```

**Option B (si tu préfères monter ~/.kube en lecture seule)** — Ajoute ceci dans `docker-compose.yml` (service `darkmoon`, section `volumes`) :

```yaml
- ${HOME}/.kube:/root/.kube:ro
```

Puis tu pourras utiliser `--kubeconfig "/root/.kube/config"` dans la commande.

## Commande type (Option A, kubeconfig dans le volume)

```bash
docker compose exec darkmoon bash -lc '
  cd "$DM_HOME" \
  && ./agentfactory \
       --k8s \
       --mcp ./mcp \
       --kubeconfig "$DM_HOME/kubeconfig" \
       --context "kind-hacklab" \
       --outdir "./out" \
       --baseurl "http://localhost:1230"
'
```

## Commande type (Option B, kubeconfig monté en /root/.kube)

```bash
docker compose exec darkmoon bash -lc '
  cd "$DM_HOME" \
  && ./agentfactory \
       --k8s \
       --mcp ./mcp \
       --kubeconfig "/root/.kube/config" \
       --context "kind-hacklab" \
       --outdir "./out" \
       --baseurl "http://localhost:1230"
'
```
Voici **tout ton texte intégralement converti en Markdown**, sans aucune omission, sans résumé, sans modification du contenu.
J’ai seulement structuré proprement en Markdown (titres, listes, blocs de code, sections), tout en **respectant 100% du texte fourni**.

---

# TODO List

## 1 — Install supplémentaire

### Dockerfile / Setup.sh / Setup.py :

Ajouter **ffuf** et **Arjun**

---

# ✅ 1) INSTALLATION STANDARD DE **FFUF**

Méthode officielle :
`go install github.com/ffuf/ffuf/v2@latest`

### 🔧 À ajouter dans **setup.sh** (fichier de build Go)

Juste après un bloc comme katana ou nuclei :

```bash
# ffuf — DOC OFFICIELLE: go install
msg "ffuf …"
GO111MODULE=on GOTOOLCHAIN=local \
  go install github.com/ffuf/ffuf/v2@latest

FFUF_BIN="$(go env GOPATH)/bin/ffuf"
if [ -x "$FFUF_BIN" ]; then
  install -D -m0755 "$FFUF_BIN" "$KUBE_DIR/ffuf"
  ok "ffuf install (go install)"
else
  warn "ffuf KO (binaire introuvable)"
fi
```

📍 À insérer dans **/mnt/data/setup.sh** tel quel.

---

# ✅ 2) INSTALLATION STANDARD DE **ARJUN**

Méthode officielle : installation Python via pip :
`pip install arjun`

### 🔧 À ajouter dans **setup_py.sh** (puisque c’est un outil Python)

Juste après les autres outils Python comme wafw00f, sqlmap, awscli :

```bash
msg "Installation arjun …"
"$PIP_BIN" install --no-cache-dir arjun && ok "arjun"
```

### 🔧 Et ajouter également le wrapper dans `/usr/local/bin` (à la suite des autres) :

```bash
# arjun (venv Darkmoon)
cat >/usr/local/bin/arjun <<'EOF'
#!/bin/sh
exec /opt/darkmoon/python/bin/arjun "$@"
EOF
chmod +x /usr/local/bin/arjun
```

📍 À insérer dans **/mnt/data/setup_py.sh** tel quel.

---

# 2 — Gestion des prompts

Dans la fonction **run_api_web_autopwn_mcp**, on trouve les prompts suivants :

### System prompt (constante, squelette, jamais modifié)

```cpp
// system_prompt = rôle global minimal (obligatoire)
std::string system_prompt =
    "Tu es un moteur d'exploitation WEB/API AUTONOME.\n"
    "Tu ne renvoies qu'une seule commande shell POSIX par réponse.\n"
    "Jamais d'explication, jamais de commentaire, jamais de texte superflu.\n"
    "Tu restes strictement sur le host/port de la cible.\n"
    "Quand tu as totalement compromis la cible : echo \"DVGA_AUTOPWN_DONE\".\n";
```

À terme, ce dernier sera stocké dans un fichier.

---

### Base Prompt (contexte complet + recon + historique)

```cpp
// base_prompt = contexte complet + recon + historique
std::string base_prompt;
base_prompt =
    "CIBLE WEB/API PRINCIPALE : " + opt.baseurl + "\n"
    "Tu dois rester strictement sur CE host/port. Jamais localhost. Jamais un autre domaine.\n"
    "\n"
    "TON RÔLE :\n"
    "Tu es un agent offensif autonome chargé d’exploiter une application WEB/API.\n"
    "Tu dois enchaîner les actions offensives en fonction :\n"
    "- du contexte de reconnaissance fourni (<recon>),\n"
    "- des résultats ZAP/Nuclei/Katana/FFUF/Arjun/MCP,\n"
    "- de l’historique exact des commandes.\n"
    "\n"
    "SURFACE D’ATTAQUE À COUVRIR (non exhaustif, mais prioritaire) :\n"
    "1) GRAPHQL\n"
    "   - Détection d’endpoint : /graphql, /api/graphql, /v1/graphql, etc.\n"
    "   - Introspection, enumeration, abuse de droits, injections.\n"
    "   - Lectures d’objets sensibles (users, pastes, audits).\n"
    "   - Fonctions système (systemDebug, systemHealth, diagnostics).\n"
    "   - Mutations et modifications de données si présentes.\n"
    "\n"
    "2) API REST / JSON\n"
    "   - Déduction depuis FFUF, Katana, Wayback, Arjun, Nuclei, MCP.\n"
    "   - Paramètres sensibles, bruteforce de chemins, fuzzing des verbs HTTP.\n"
    "   - Détection SQLi : classique, boolean-based, time-based.\n"
    "   - Détection XSS (reflected/stored), injection JSON, erreurs backend.\n"
    "   - Détection SSRF, LFI, RFI, Path Traversal, uploads dangereux.\n"
    "\n"
    "3) AUTHENTICATION / SESSION\n"
    "   - Bruteforce contrôlé (max 3 tentatives par motif).\n"
    "   - Test de contournement d’auth, tokens faibles, JWT, cookies.\n"
    "   - Test des endpoints admin/api/v1/admin/v1/users/…\n"
    "\n"
    "4) ABUS DE LOGIQUE\n"
    "   - IDOR (modification d’ID numériques).\n"
    "   - Mass assignment, bypass de permissions.\n"
    "   - Endpoints “debug”, “health”, “diagnostics”, “status”.\n"
    "\n"
    "RÈGLES CRITIQUES :\n"
    "- Toujours une seule commande POSIX par réponse.\n"
    "- Jamais un mot, jamais une explication, jamais un commentaire.\n"
    "- Ne jamais réessayer une stratégie qui a déjà échoué 3 fois dans l’historique.\n"
    "- Reconnaître automatiquement un succès :\n"
    "    * Exfiltration de données sensibles (users, tokens, secrets…)\n"
    "    * Fonctions système accessibles (debug/health/update/diagnostics)\n"
    "    * Dump d’objets internes\n"
    "    * Flag, secret, mot de passe admin trouvé\n"
    "    * RCE, lecture de /proc, ps, env, ou sortie système\n"
    "- Dès qu’un succès notable apparaît → renvoyer :\n"
    "    echo \"DVGA_AUTOPWN_DONE\"\n"
    "\n"
    "RECON CENTRALISÉ (fourni par l’orchestrateur) :\n"
    "<recon>\n" + recon_context + "\n</recon>\n"
    "\n"
    "HISTORIQUE STRICT DES COMMANDES ET RÉSULTATS :\n"
    "Tu dois t’en servir pour changer de stratégie immédiatement si :\n"
    "- un endpoint échoue plusieurs fois,\n"
    "- un pattern de brute-force est détecté,\n"
    "- une donnée interne ou un flag apparaît.\n"
    "\n"
    "<full_history>\n";
```

---

## Optimisation souhaitée du prompt

L’objectif :

* plus économe
* plus riche
* plus profond

Ajouter davantage :

* d’instructions **d’arrêt**
* de **déclenchement**
* de **tools disponibles**
* de **scénarios de workflow**

Inspiré des **GIGA PROMPTS**, sans prendre tout, mais le plus pertinent.

But :
➡️ Avoir un modèle de prompt suffisamment efficace pour être **dupliqué** sur d’autres scopes (AD / Infra / SCADA / etc).

---

## Problème actuel

L’autopown **ne fait que du curl** avec le prompt minimaliste → on peut aller beaucoup plus loin.

---

## Dans run_agentfactory : prompts de rapport

Actuellement il existe :

* `prompt-rapport`
* `prompt-rapport-md`

Ils :

* ne sont **pas pertinents**
* analysent mal `dvga_prompt.txt`

Or `dvga_prompt.txt` contient :

* logs de zap
* logs nuclei
* logs ffuf
* logs autopown MCP

Il faut donc rédiger **un prompt explicatif du contenu des logs**, qui :

* explique ce qu’il y a dedans
* explique ce que ça fait
* traduit cela en **rapport de pentest**

---

# 3 — Build Docker & build C++

Pour éviter un rebuild complet de l’image, on peut mettre **le fichier C++ à la racine** :

### Structure attendue

```
└─$ ls
agentfactory  agentfactory.cpp  cpp  docker-compose.yml  Dockerfile  
entrypoint.sh  ffuf  json.hpp  mon_prompt.txt  prompt  
prompt-rapport-k8s-md.txt  prompt-rapport-k8s.txt  
prompt-rapport-md.txt  prompt-rapport.txt  
README.md  setup_py.sh  setup.sh  zap_cli_out
```

### Build statique depuis Alpine :

```bash
docker run --rm -v "$PWD:/src" -w /src alpine:3.20 \
sh -lc '
set -e
apk add --no-cache g++ musl-dev
g++ -std=gnu++17 -O2 -pipe -pthread -s \
   -I/src \
   -static -static-libgcc -static-libstdc++ \
   -o /src/agentfactory /src/agentfactory.cpp
file /src/agentfactory
'
```

### Installation dans le conteneur :

```bash
docker cp ./agentfactory darkmoon:/opt/darkmoon/agentfactory
docker compose exec darkmoon bash -lc '
set -e
chmod 0755 /opt/darkmoon/agentfactory
ln -sf /opt/darkmoon/agentfactory /usr/local/bin/agentfactory
/opt/darkmoon/agentfactory --help >/dev/null 2>&1 || true
echo "[OK] agentfactory statique installé."
'
```

### Build local rapide

```bash
g++ -std=gnu++17 -O2 -pipe -pthread -o agentfactory agentfactory.cpp
```

---

# 4 — Flow Web actuel (visible dans run_agentfactory)

```
katana → ffuf → arjun → nuclei → zap → curl → mcp_autopown → rapport
```

run_agentfactory = ORCHESTRATEUR GLOBAL
Il :

1. Initialise l’environnement
2. Charge la cible
3. Prépare ZAP / HTTPX / NAABU / Nuclei
4. Enchaîne les phases :

---

## Phases détaillées

### Reconnaissance

* **run_recon_web_chain**
* lance whatweb, httpx, nuclei
* produit recon_web_* + alerts_* → base de tout

---

### Validation WEB (désactivée pour l’instant)

* **build_web_confirmation_plan**
* **execute_confirmation_plan**
* **run_web_validation_phase**

---

### Validation Infra (désactivée)

* **run_infra_validation_phase**
* **run_post_naabu_enrichment**

---

### Post-exploitation (désactivé)

* **collect_web_postexploit_surfaces**
* **build_web_postexploit_plan**
* **run_postexploit_web_phase**

---

### API (désactivé)

* **harvest_api_from_urls_file**
* **api_catalog_from_enum_files**

---

### MAIL (désactivé)

* **harvest_mail_triggers**
* **append_mail_flows**
* **analyse_postexploit_results_mail**

---

### DB (désactivé)

* **harvest_db_triggers**
* **add_db_flow**
* **analyse_postexploit_results_db**

---

### DNS/SNMP/EDGE (désactivé)

* **try_axfr_and_save**
* **try_snmp_communities_and_save**
* **sample_telemetry_artifacts_and_save**
* **harvest_edge_triggers**
* **append_edge_flows_impl / append_edge_flows**
* **analyse_postexploit_results_edge_impl**

---

### NET (désactivé)

* **append_net_flows**
* **analyse_postexploit_results_net**

---

# 5 — CLI actuelle et future

### Actuelle (lourde)

```bash
docker compose exec darkmoon bash -lc '
  cd "$DM_HOME" \
  && export TOKEN="$(cat /zap/wrk/ZAP-API-TOKEN)" \
  && ./agentfactory \
       --agentfactory \
       --zapcli ./ZAP-CLI \
       --mcp ./mcp \
       --host zap \
       --apikey "$TOKEN" \
       --baseurl "http://dvga:5013/" \
       --outdir "zap_cli_out" \
       --port 8888 \
       --katana \
       --katana-bin ./kube/katana \
       -- \
       -depth 2 \
       -proxy http://zap:8888
'
```

### Futur (simplifié)

```bash
docker compose exec darkmoon bash -lc '
  cd "$DM_HOME" \
  && export TOKEN="$(cat /zap/wrk/ZAP-API-TOKEN)" \
  && ./agentfactory \
       --web \
       --baseurl "http://dvga:5013/" \
'
```

---

# 6 — Code de la CLI (pour activer/désactiver par défaut)

Il suffit de mettre les booléens de `false` à `true`.

```cpp
struct CLIOptions {
    bool agentfactory=false;
    bool k8s=false;
    bool autopwn_postexploit = false;
    bool intrusive = false;
    std::string lab_sender_domain; 
    std::string mcp;
    std::string zapcli;
    std::string host="localhost";
    uint16_t port=8888;
    std::string apikey;
    std::string baseurl;
    std::string outdir="zap_cli_out";
    bool wait_browse=false;

    // K8s
    std::string kube_dir="kube";
    std::string kubeconfig;
    std::string kubecontext;

    // Katana
    bool use_katana=false;
    std::string katana_bin;
    std::vector<std::string> katana_args;

    // ZAP avancé
    std::string zap_context_name;
    std::string zap_context_file;
    std::vector<std::string> zap_auth_users;
    std::vector<std::string> zap_focus_prefixes;
    std::string zap_scan_policy;
    bool zap_ajax_spider = false;
    std::string zap_proxy;
    int zap_throttle_ms = 0;
    std::string zap_default_user_agent;
    std::vector<std::string> zap_scripts;

    // Multi-host / scan config
    std::vector<std::string> targets;
    std::string targets_file;
    std::string default_scheme = "https";

    std::string scan_profile = "full";
    std::string custom_ports;
    bool enable_udp = false;

    bool naabu_auto_tune = false;
    int  naabu_rate = 20000;
    int  naabu_retries = 2;
    std::string naabu_top_ports;

    bool enable_rescan = true;
};
```

---

# 7 — Documentation souhaitée (TODO)

Lors des tests :

* ajouter tools si nécessaire
* documenter toutes les features ajoutées
* documenter tout changement apporté

---



> Remarques :
>
> * `--context` doit correspondre à un contexte valide dans ton kubeconfig.
> * `--baseurl` est un paramètre d’output/endpoint interne à ton flux (garde ta valeur ou adapte-la si nécessaire).
> * Les résultats seront écrits dans `"$DM_HOME/out"` (donc visibles sur l’hôte sous `${HOME}/darkmoon-docker-fs/out`).
