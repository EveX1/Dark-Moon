# web_api_scanner.sh — Darkmoon Web One-Shot (Endpoint)

Un scanner “one-shot” orienté **endpoint HTTP** qui enchaîne :
- **ZAP** : seed via proxy + **Active Scan** + récupération d’alertes (filtrées Medium/High)
- **Arjun** : découverte de paramètres **GET** (avec **auto-skip** si la cible est GraphQL)
- **SQLmap** : prévu en mode “safe guarded” avec **timeout hard** (présent dans les prereqs/flags, logique d’auto-skip GraphQL incluse)

> **Important :** pas de brute-force / login.  
> Si l’endpoint nécessite une auth, fournis un **JWT** via `/tmp/darkmoon_jwt_token.txt` ou `GLOBAL_JWT`.

---

## 1) Pré-requis

### Binaires requis
Le script vérifie et exige :
- `curl`
- `jq`
- `nuclei`
- `sqlmap`

Optionnel :
- `arjun` (si absent, Arjun est skip)

Si un binaire requis manque → **exit 1**.

### ZAP
- Un proxy ZAP accessible (défaut `http://zap:8888`)
- Une clé API ZAP lisible (défaut `/zap/wrk/ZAP-API-TOKEN`)
- Le script utilise l’API ZAP pour activer scanners, lancer un scan, récupérer alertes.

---

## 2) Usage

### Syntaxe
```bash
./web_api_scanner.sh -u <URL> [options]
````

### Exemple HTTP “classique”

```bash
./web_api_scanner.sh -u "http://juice-shop:3000/rest/products/search"
```

### Exemple GraphQL (auto-skip Arjun/SQLmap en mode auto)

```bash
./web_api_scanner.sh -u "http://dvga:5013/graphql"
```

### Aide

```bash
./web_api_scanner.sh -h
```

---

## 3) Options CLI

| Option | Description                  | Défaut                   |
| ------ | ---------------------------- | ------------------------ |
| `-u`   | **Target URL** (obligatoire) | —                        |
| `-z`   | URL proxy ZAP                | `http://zap:8888`        |
| `-k`   | Fichier clé API ZAP          | `/zap/wrk/ZAP-API-TOKEN` |
| `-A`   | Arjun `true\|false\|auto`    | `auto`                   |
| `-S`   | SQLmap `true\|false\|auto`   | `auto`                   |
| `-t`   | Timeout SQLmap (secondes)    | `180`                    |
| `-h`   | Help                         | —                        |

> Les valeurs par défaut peuvent aussi être influencées par variables d’environnement :
> `ZAP`, `APIKEY_FILE`, `ARJUN_ENABLED`, `SQLMAP_ENABLED`, `SQLMAP_TIMEOUT`.

---

## 4) Auth JWT (feature)

### Fournir un JWT

Le script cherche un token dans cet ordre :

1. variable d’environnement `GLOBAL_JWT`
2. fichier `/tmp/darkmoon_jwt_token.txt` (si non vide)

Si présent, il est utilisé pour les requêtes **seed ZAP** :

```http
Authorization: Bearer <JWT>
```

### Décodage local (sanity check)

Si `GLOBAL_JWT` est présent, le script effectue un **decode local** du payload JWT :

* extraction du segment payload (2e bloc)
* normalisation base64url (`_-` → `/+`) + padding
* `base64 -d`, puis affichage via `jq` si possible

> Ça ne valide pas la signature, c’est uniquement un check local pratique.

---

## 5) Détection GraphQL (auto-skip intelligent)

Le script détecte si `-u` cible du GraphQL via :

1. heuristiques URL :

   * match `/graphql` ou présence de `graphql`
2. probe best-effort :

   * `curl` rapide (4s) + si `Content-Type: application/json`
   * si le body JSON contient `.errors` → considéré GraphQL

Conséquence :

* si GraphQL détecté et mode `auto` :

  * **Arjun** désactivé
  * **SQLmap** désactivé

---

## 6) Pipeline d’exécution

### [0] Contexte

Affiche :

* endpoint ciblé
* proxy ZAP
* présence (ou non) d’un JWT

### [0-bis] Vérification des binaires

* stoppe si manque : `curl jq nuclei sqlmap`
* indique si `arjun` est présent

### [1] Vérification JWT (si fourni)

* affiche le payload décodé

### [2] Détection type cible

* “GraphQL” ou “HTTP classique”
* impact sur Arjun/SQLmap si `auto`

### [3] Arjun — découverte paramètres GET (si activé)

Si Arjun ON :

* exécution :

  ```bash
  arjun -u "$TARGET_BASE" -m GET -oJ /tmp/darkmoon_arjun_tmp.json
  ```
* non bloquant : si Arjun échoue, le script **continue**
* extraction des params :

  ```bash
  jq -r '.[].params[]? // empty' | sort -u
  ```

Résultat :

* `PARAMS` = liste de paramètres GET trouvés (unique)
* fichier partagé : `/tmp/darkmoon_arjun_tmp.json`

### [4] Seeding ZAP (toujours)

But : faire passer des URLs via proxy ZAP pour alimenter l’historique / contexte.

* Si `PARAMS` est non vide :

  * génère des URLs enrichies :

    ```
    <TARGET_BASE>?<param>=darkmoon_test
    ```
  * les appelle via `curl -x "$ZAP"` (avec JWT si dispo)
  * conserve la première URL enrichie en `FIRST_ENRICHED`
* Sinon :

  * `FIRST_ENRICHED="$TARGET_BASE"`
  * seed direct via proxy ZAP

`FIRST_ENRICHED` devient la cible du scan actif ZAP.

### [5] Activation scanners ZAP

Active :

* tous les scanners actifs :

  ```
  /JSON/ascan/action/enableAllScanners/
  ```
* le passive scan :

  ```
  /JSON/pscan/action/setEnabled/?enabled=true
  ```

### [6] Lancement Active Scan ZAP

* démarre un scan **non récursif** (`recurse=false`) sur `FIRST_ENRICHED`
* récupère `SCAN_ID`
* boucle de progression :

  ```
  /JSON/ascan/view/status/?scanId=<id>
  ```

jusqu’à 100%

### [7] Alertes ZAP (filtrées)

Récupère toutes les alertes puis filtre :

* `url == FIRST_ENRICHED`
* `risk == Medium` ou `High`

Sortie (JSON par alerte) :

```json
{ "risk": "...", "alert": "...", "url": "...", "param": "...", "evidence": "..." }
```

---

## 7) Modes Arjun / SQLmap

### Arjun (`-A` / `ARJUN_ENABLED`)

* `true` : force Arjun
* `false` : désactive Arjun
* `auto` : Arjun ON **uniquement si** la cible n’est pas GraphQL

### SQLmap (`-S` / `SQLMAP_ENABLED`)

* `true` : force SQLmap
* `false` : désactive SQLmap
* `auto` : SQLmap ON **uniquement si** la cible n’est pas GraphQL

### Timeout SQLmap (`-t` / `SQLMAP_TIMEOUT`)

* paramètre prévu pour imposer un **hard timeout** (défaut `180s`)

> Note technique : dans l’extrait fourni, la phase SQLmap n’apparaît pas encore (on voit la config, la détection GraphQL et les hooks).
> La doc ci-dessus décrit le **contrôle/feature** (flags + auto-skip + timeout), et doit être complétée si tu ajoutes le bloc d’exécution SQLmap.

---

## 8) Fichiers & artefacts

* `ARJUN_TMP` : `/tmp/darkmoon_arjun_tmp.json`
* `JWT_FILE` : `/tmp/darkmoon_jwt_token.txt`
* fichier temporaire probe GraphQL :

  * `/tmp/_dm_probe_body`

---

## 9) Exemples pratiques

### HTTP endpoint + Arjun auto + ZAP default

```bash
./web_api_scanner.sh -u "http://app:8080/api/search"
```

### Forcer Arjun même si GraphQL détecté

```bash
./web_api_scanner.sh -u "http://dvga:5013/graphql" -A true
```

### Désactiver Arjun + garder ZAP

```bash
./web_api_scanner.sh -u "http://app:8080/api" -A false
```

### Scanner un endpoint protégé avec JWT

```bash
export GLOBAL_JWT="eyJhbGciOi..."
./web_api_scanner.sh -u "http://app:8080/api/me"
```

---

## 10) Ce que ce script ne fait pas (volontairement)

* pas de brute-force login
* pas de crawl récursif ZAP (scan endpoint “one-shot”)
* la logique SQLmap est “préparée” (flags/auto/timeout) mais dépend du bloc d’exécution que tu ajoutes ensuite

---
