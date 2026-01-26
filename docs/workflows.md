# 🔁 Darkmoon — Workflows MCP

Ce document explique **ce que sont les workflows MCP**, comment ils fonctionnent,
et comment en créer de nouveaux.

Public cible :
- développeurs
- pentesters avancés
- contributeurs

---

## 1. Qu’est-ce qu’un workflow MCP ?

Un workflow est :
- un **module Python**,
- exposé par le MCP,
- qui encapsule une **suite cohérente d’actions**,
- exécutée dans la toolbox Docker.

👉 Un workflow = une tâche métier complète.

---

## 2. Où vivent les workflows

Les workflows sont situés dans :

````

mcp/src/tools/workflows/

````

Exemples :
- `port_scan.py`
- `vulnerability_scan.py`
- `web_crawler.py`

---

## 3. Découverte dynamique

Au démarrage :
- le MCP scanne automatiquement les workflows,
- expose leurs méthodes,
- les rend accessibles à l’IA.

👉 Aucun enregistrement manuel requis.

---

## 4. Structure d’un workflow

Chaque workflow :
- hérite de `BaseWorkflow`,
- définit une ou plusieurs méthodes,
- gère ses timeouts,
- structure ses résultats.

---

## 5. Exemple : scan de vulnérabilités

Le workflow `VulnerabilityScanWorkflow` :

- crée un workspace dédié,
- lance Nuclei,
- parse les résultats JSON,
- corrèle par sévérité,
- retourne un résumé structuré.

👉 Ce n’est pas juste un appel outil.
👉 C’est une **logique complète**.

---

## 6. Appel par un agent

Un agent peut appeler :

```text
run_workflow("vulnerability_scan", "scan_vulnerabilities", {...})
````

L’agent :

* choisit le bon workflow,
* décide quand l’exécuter,
* interprète les résultats.

---

## 7. Avantages des workflows

* réutilisables,
* testables,
* auditables,
* plus sûrs que des commandes brutes.

---

## 8. Créer un nouveau workflow

1. Copier `TEMPLATE.py`
2. Implémenter la logique
3. Respecter la structure
4. Tester localement
5. Relancer le MCP

📄 Guide détaillé :
`mcp/WORKFLOW_GUIDE.md`

---

## 9. Bonnes pratiques

* Un workflow = une mission
* Ne pas mélanger trop de responsabilités
* Toujours structurer les outputs
* Gérer les timeouts proprement

---

## 10. Résumé

Les workflows :

* sont le bras armé de Darkmoon,
* encapsulent la logique offensive,
* sécurisent l’exécution des outils.

---

➡️ Pour comprendre le MCP lui-même :
voir `docs/mcp.md`