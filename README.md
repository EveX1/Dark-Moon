# 🌑 Darkmoon — Agentic AI Pentest Platform

Darkmoon est une **plateforme de pentest agentique basée sur l’IA**, conçue pour exécuter **de vrais tests d’intrusion**, de bout en bout, en combinant :

- des **agents IA autonomes**,
- un **serveur MCP sécurisé**,
- une **toolbox Docker industrielle**,
- et un **orchestrateur LLM agnostique**.

> Une phrase → une campagne de pentest complète.

---

## 🚀 Pourquoi Darkmoon existe

Les outils de sécurité sont :
- fragmentés,
- manuels,
- peu corrélés,
- difficiles à industrialiser.

Darkmoon répond à ce problème en proposant :
- une **orchestration agentique**,
- une **exécution réelle des outils**,
- une **corrélation automatique des résultats**,
- une **architecture reproductible et extensible**.

Darkmoon **n’est pas** :
- un simple scanner,
- un wrapper d’outils,
- un chatbot qui lance des commandes.

Darkmoon est une **chaîne d’exécution offensive complète**, pilotée par l’IA.

---

## 🧠 Vue d’ensemble (simple)

1. L’utilisateur écrit un objectif :
   > *“Exécute un scan de vulnérabilité sur example.com”*

2. Un **agent IA** comprend la mission.
3. Il appelle le **serveur MCP Darkmoon**.
4. Le MCP exécute **de vrais outils** dans la toolbox Docker.
5. Les résultats sont analysés.
6. L’agent **enchaîne, exploite, itère**.

---

## 🧩 Composants principaux

| Composant | Rôle |
|---------|------|
| **OpenCode** | Orchestrateur IA / agents |
| **Agents Markdown** | Logique offensive autonome |
| **MCP Darkmoon (FastMCP)** | Pont IA ↔ outils |
| **Toolbox Docker** | Outils de pentest réels |
| **Docker & volumes** | Isolation et persistance |

---

## ⚡ Démarrage rapide

```bash
docker compose build --no-cache
docker compose up -d
````

Puis :

```bash
chmod +x ./darkmoon.sh
./darkmoon.sh
```

ou

```bash
sudo mv ./darkmoon.sh /usr/local/bin/darkmoon
sudo chmod 755 /usr/local/bin/darkmoon
darkmoon
```

### 🧪 Lancer un test d’intrusion complet avec un agent IA

Darkmoon utilise **OpenCode** pour orchestrer des **agents IA spécialisés**.
Les agents sont sélectionnables **directement dans l’interface** via `@`.

---

### 🔹 Principe

* Chaque agent représente une **stratégie de pentest autonome**
* Les agents sont définis en **Markdown**
* Ils utilisent **exclusivement le MCP Darkmoon** pour exécuter de vrais outils
* L’orchestration est **automatique, sans interaction humaine**

---

### 🔹 Lancer Darkmoon (mode interactif)

```bash
darkmoon
```

Cela ouvre l’interface OpenCode (TUI ou CLI selon le contexte).

---

### 🔹 Sélectionner un agent avec `@`

Dans l’interface OpenCode, tapez :

```
@
```

➡️ OpenCode affiche la **liste des agents disponibles**
(ex: `FastMCP Pentest Agent`, `pentest-web`, etc.)

Sélectionnez :

```
@FastMCP Pentest Agent
```

---

### 🔹 Exemple : lancer un pentest de bout en bout

Une fois l’agent sélectionné, saisissez simplement :

```
@FastMCP Pentest Agent lance un test d'intrusion de bout en bout sur le domaine dark-moon.org
```

---

### 🔹 Ce qu’il se passe automatiquement

Sans aucune autre action de votre part, l’agent va :

1. Construire une **cartographie de la cible**
2. Lancer des **phases de reconnaissance**
3. Exécuter des **scans de vulnérabilités**
4. Enchaîner des **workflows MCP** (nuclei, recon, crawling, etc.)
5. Exploiter si des failles sont détectées
6. Corréler les résultats
7. Itérer jusqu’à épuisement des vecteurs

👉 **Aucune confirmation demandée**
👉 **Aucune commande à écrire**
👉 **Aucune orchestration manuelle**

---

### 🔹 Exemple de prompts équivalents

```text
@FastMCP Pentest Agent exécute un scan de vulnérabilité complet sur dark-moon.org
```

```text
@FastMCP Pentest Agent réalise un pentest web approfondi sur https://dark-moon.org
```

```text
@FastMCP Pentest Agent analyse la surface d’attaque publique du domaine dark-moon.org
```

L’agent choisira **lui-même** :

* les workflows MCP à appeler,
* les outils à exécuter,
* l’ordre et la profondeur des attaques.

---

### 🔹 Vérifier l’état de la toolbox (optionnel)

Avant ou pendant une mission, vous pouvez demander :

```
exécute la fonction health_check
```

L’agent (ou OpenCode) appellera le MCP pour :

* vérifier que la toolbox est prête,
* lister les outils disponibles,
* confirmer l’état global du système.

---

### 🔹 Important à comprendre

* Le langage utilisé est **du langage naturel**
* Il n’y a **pas de syntaxe spéciale**
* L’IA comprend l’intention, pas une commande

👉 Vous **décrivez un objectif**, pas une procédure.

---

### 🧠 Résumé rapide

| Action                     | Résultat                    |
| -------------------------- | --------------------------- |
| `darkmoon`                 | Lance OpenCode              |
| `@`                        | Liste les agents            |
| `@FastMCP Pentest Agent …` | Lance un pentest complet    |
| Langage naturel            | Orchestration automatique   |
| MCP                        | Exécution réelle des outils |


---

## 📁 Documentation complète

👉 Toute la documentation détaillée est dans `/docs`.

* [`docs/setup.md`](docs/setup.md)
  Installation, configuration, lancement

* [`docs/architecture.md`](docs/architecture.md)
  Architecture détaillée + diagrammes Mermaid

* [`docs/agents.md`](docs/agents.md)
  Agents IA, règles, exemples

* [`docs/mcp.md`](docs/mcp.md)
  MCP Darkmoon, outils, workflows

* [`docs/workflows.md`](docs/workflows.md)
  Création et exécution des workflows

* [`docs/rebuild-and-troubleshooting.md`](docs/rebuild-and-troubleshooting.md)
  Rebuild propre, erreurs Docker, WSL

* [`docs/security-threat-model.md`](docs/security-threat-model.md)
  Threat model de Darkmoon lui-même

* [`docs/contributing.md`](docs/contributing.md)
  Guide contributeur

* [`docs/toolbox.md`](docs/toolbox.md)
  Documentation de la toolbox de Darkmoon

---

## 🔐 Sécurité & philosophie

* Aucun secret hardcodé
* Isolation Docker stricte
* Outils exécutés uniquement via MCP
* LLM **agnostique**
* Agents **auditables en Markdown**

---

## 🧠 À qui s’adresse Darkmoon

* Pentesters
* Red Team
* RSSI / CTO
* Chercheurs sécurité
* Équipes DevSecOps

---

## 🌓 Conclusion

Darkmoon est une **plateforme offensive agentique**, conçue pour :

* penser comme un pentester,
* agir comme une toolbox industrielle,
* évoluer comme un framework IA moderne.

Ce n’est pas une promesse marketing.
C’est une **architecture complète, observable et extensible**.