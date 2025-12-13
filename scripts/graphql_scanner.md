# graphql_scanner.sh — Darkmoon A1 (Ultra Pro GraphQL Offensive Engine)

Version: `A1-ULTRA-PRO-CLEAN`

Un scanner GraphQL offensif “one-shot” qui :
- fait une **introspection complète** (`__schema`)
- **génère automatiquement** un jeu d’attaques **par argument** (SQLi / NoSQLi / RCE / traversal / fuzz)
- exécute toutes les opérations générées
- applique des **heuristiques de détection** (RCE / erreurs SQL / leaks / exceptions)
- exporte un **rapport JSON** : `/output/graphql/report.json`
- peut router le trafic via **OWASP ZAP** (proxy HTTP) si activé

---

## 1) Pré-requis

### Binaires requis
- `bash`
- `curl`
- `jq`

Le script s’arrête si `curl` ou `jq` est manquant.

### Accès cible
- Un endpoint GraphQL accessible en HTTP(S), ex: `http://dvga:5013/graphql`
- **Introspection activée** (sinon le script stoppe avec “Introspection failed / disabled”).

### Sorties / fichiers
- Introspection brute : `/tmp/darkmoon_graph_attack/introspection_raw.json`
- Rapport final : `/output/graphql/report.json`

> Le script crée automatiquement `/tmp/darkmoon_graph_attack` et `/output/graphql`.

---

## 2) Usage

### Syntaxe
```bash
./graphql_scanner.sh [GRAPHQL_URL]
````

### Cible par argument

```bash
./graphql_scanner.sh http://dvga:5013/graphql
```

### Cible par variable d’environnement

```bash
TARGET=http://autre:4000/graphql ./graphql_scanner.sh
```

### Valeur par défaut

Si rien n’est fourni :

* `TARGET` défaut = `http://dvga:5013/graphql`

---

## 3) Variables d’environnement

### Cible

* `TARGET` : endpoint GraphQL (si aucun argument CLI)

### ZAP (proxy optionnel)

* `ENABLE_ZAP` (défaut `1`)

  * `1` : tente d’utiliser ZAP comme proxy HTTP si ZAP répond
  * `0` : désactive l’usage de ZAP
* `ZAP_URL` (défaut `http://zap:8888`)
* `ZAP_KEY`

  * par défaut : lu depuis `/zap/wrk/ZAP-API-TOKEN` si présent, sinon vide
  * utilisé uniquement pour récupérer les alertes via l’API ZAP en fin d’exécution

Exemples :

```bash
# Désactiver ZAP
ENABLE_ZAP=0 ./graphql_scanner.sh http://target/graphql

# Forcer ZAP + clé
ENABLE_ZAP=1 ZAP_URL=http://127.0.0.1:8888 ZAP_KEY="xxxxx" ./graphql_scanner.sh http://target/graphql
```

---

## 4) Fonctionnement global

### Étape A — Introspection

Le script envoie une requête `__schema` qui récupère :

* `queryType.fields` (queries + args + types)
* `mutationType.fields` (mutations + args + types)
* `types` (liste des types et champs)

Si `errors` est présent dans la réponse d’introspection → arrêt immédiat.

### Étape B — Génération d’opérations offensives

Pour chaque **query** et **mutation** :

* extraction de la liste des `args`
* résolution du **type de retour** (unwrapped : `NON_NULL` / `LIST` → type de base)
* si le type de retour est un `OBJECT`, génération automatique d’un **fragment de sélection** :

  * prend jusqu’à **6 champs scalaires** du type (pour éviter des réponses énormes)
  * exemple: ` { id name email createdAt role }`

#### Stratégie “fuzz un argument à la fois”

Pour éviter l’explosion combinatoire :

* le moteur ne combine pas plusieurs arguments simultanément
* il génère des opérations où **un seul argument est fuzzé** à la fois

### Étape C — Exécution

Toutes les opérations générées sont envoyées séquentiellement.

* si ZAP est actif et joignable : `curl` passe par `HTTP_PROXY=$ZAP_URL`
* sinon : envoi direct

### Étape D — Détection (heuristiques)

Chaque réponse est analysée via des règles simples (grep/regex).
Si un “hit” est détecté :

* l’opération est affichée
* la réponse est affichée (tentative `jq .`, sinon brut)

### Étape E — Rapport JSON + alertes ZAP

Le script :

* affiche un résumé console (compteurs + listes)
* exporte `/output/graphql/report.json`
* récupère (optionnel) les alertes ZAP via :

  * `GET $ZAP_URL/JSON/core/view/alerts/?apikey=$ZAP_KEY`

---

## 5) Payload Engine (features offensives)

### Payloads intégrés

**SQLi**

* `' OR 1=1--`
* `' OR 'a'='a`
* `1 OR 1=1`
* `' UNION SELECT 1,2,3--`

**NoSQLi**

* `{"$ne": null}`
* `{"$gt": ""}`

**RCE**

* `;id`
* `| id`
* `&& id`

**Path traversal**

* `../../../../etc/passwd`
* `/etc/passwd`

**Fuzz “string”**

* `AAAAAAAAAAAAAAAAAAAA`
* `'"\\}{@£$%()*!^`
* `{ } [ ] ( )` (forme compacte)
* `{"test":1}`

### Sélection automatique des payloads par nom d’argument

Le choix est piloté par :

* **type GraphQL** (String / Int / Boolean / ID)
* **nom d’argument** (heuristique)

Règles principales (type `String`) :

* arg ∈ `{password, pass, pwd, token, secret, auth, authorization, jwt}` → SQLi payloads
* arg ∈ `{cmd, command, exec}` → RCE payloads
* arg ∈ `{file, path, dir, location}` → traversal payloads
* arg ∈ `{filter, search, query, q}` → SQLi + NoSQLi
* sinon → SQLi + fuzz strings

Types non-String :

* `Int` → `1, 0, -1, 999999`
* `Boolean` → `true, false`
* `ID` → SQLi payloads
* autres types “exotiques” → ignorés

### Échappement GraphQL

Les payloads `String/ID` sont injectés avec guillemets GraphQL et échappement de :

* `\` → `\\`
* `"` → `\"`

---

## 6) Heuristiques de détection (ce que le script “flag”)

### RCE (indicateurs)

Match si la réponse contient (case-insensitive) :

* `uid=...` / `gid=...`
* `root:x:0:0`

### SQL Injection (erreurs DB)

Match si la réponse contient :

* `SQLSTATE`
* `sqlite`, `postgres`, `mysql`
* `syntax error`

### Leaks / données sensibles

Match si la réponse contient :

* `password`, `token`, `secret`, `api_key`, `accessToken`, `jwt`

### Erreurs internes

Match si la réponse contient :

* `Exception`, `Traceback`, `panic`, `NullReference`, `stacktrace`

### Anomalies génériques

Match si la réponse contient :

* `root:`
* `invalid json`
* `bad request`
* `forbidden`
* `unauthorized`

Quand un match arrive :

* l’opération est ajoutée dans la catégorie correspondante
* la réponse complète est affichée

---

## 7) Outputs & rapport

### Console

* résumé introspection (queries/mutations/types count)
* debug (nombre d’opérations + preview des 5 premières)
* affichage **uniquement** des opérations qui matchent une heuristique
* récap final avec compteurs par catégorie

### JSON exporté

Chemin :

```text
/output/graphql/report.json
```

Structure :

```json
{
  "target": "http://example/graphql",
  "stats": { "attacks": "N" },
  "vulnerabilities": {
    "RCE": ["{ op(...) { ... } }", "..."],
    "SQLI": ["..."],
    "LEAK": ["..."],
    "INTERNAL": ["..."],
    "MISC": ["..."]
  }
}
```

> Note : `stats.attacks` est exporté en string (car injecté via `--arg`). C’est normal vu le code actuel.

---

## 8) Limitations connues

* **Introspection obligatoire** : si désactivée, le scanner s’arrête.
* **Heuristiques “string match”** : pas de preuve d’exploitation, uniquement des indicateurs rapides.
* **Un seul argument fuzzé à la fois** : coverage volontairement limitée pour éviter l’explosion combinatoire.
* **Sélection de champs limitée** : seulement champs scalaires, max ~6, uniquement si retour `OBJECT`.
* **Timeout proxy ZAP** : test rapide de disponibilité (`--max-time 2` sur `$ZAP_URL`).

---

## 9) Exemples rapides

### Scan DVGA (défaut)

```bash
./graphql_scanner.sh
```

### Scan cible explicite + ZAP off

```bash
ENABLE_ZAP=0 ./graphql_scanner.sh https://target.tld/graphql
```

### Scan avec ZAP (proxy + alertes)

```bash
ENABLE_ZAP=1 ZAP_URL=http://zap:8888 ZAP_KEY="$(cat /zap/wrk/ZAP-API-TOKEN)" \
  ./graphql_scanner.sh http://target/graphql
```

---

## 10) Interprétation des résultats

* **RCE détectée** : très fort signal (patterns `uid=` / `root:x:0:0`) → à confirmer par reproduction ciblée.
* **SQLi détectée** : souvent une erreur DB/ORM remontée → vérifier l’injection et l’impact.
* **LEAK** : présence de mots sensibles → vérifier si ce sont des champs légitimes ou un dump accidentel.
* **INTERNAL** : stacktraces/Exceptions → utile pour fingerprint et exploitation logique.
* **MISC** : réponses anormales (401/403/bad request/invalid json) → utile pour cartographier contrôles et erreurs.

---

```
