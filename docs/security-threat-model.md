# 🔐 Darkmoon — Threat Model & Security Design

Ce document décrit le **threat model de Darkmoon lui-même**.

Objectif :
- comprendre les surfaces d’attaque,
- justifier les choix d’architecture,
- démontrer que Darkmoon est **conçu de manière défensive**, malgré sa vocation offensive.

Public cible :
- RSSI
- auditeurs
- architectes sécurité
- clients exigeants

---

## 1. Principe fondamental

Darkmoon repose sur un principe non négociable :

> **L’IA ne doit jamais pouvoir exécuter librement du code.**

Tout est construit autour de cette contrainte.

---

## 2. Actifs à protéger

| Actif | Description |
|-----|------------|
| Host utilisateur | Système de l’opérateur |
| Clés API LLM | Accès aux modèles |
| Toolbox | Outils de pentest |
| Configuration OpenCode | Agents, prompts |
| Résultats de scan | Données sensibles |

---

## 3. Modèle de menace global

Menaces considérées :

- prompt injection
- exécution de commandes arbitraires
- fuite de secrets
- escalade de privilèges
- sortie de périmètre Docker
- abus du LLM

---

## 4. Frontières de sécurité (défense en profondeur)

### 4.1 IA ↔ Exécution

| Élément | Mesure |
|------|-------|
| Agents | Markdown auditables |
| IA | Aucune commande directe |
| MCP | Seul point d’exécution |

👉 **Barrière la plus importante**.

---

### 4.2 MCP ↔ Toolbox

| Élément | Mesure |
|------|-------|
| Exécution | Docker isolé |
| Outils | Whitelist |
| Timeouts | Contrôlés |
| Parsing | Structuré |

---

### 4.3 Toolbox ↔ Host

| Élément | Mesure |
|------|-------|
| Isolation | Docker |
| Volumes | Contrôlés |
| Réseau | Limité |
| Permissions | Root maîtrisé |

---

## 5. Gestion des secrets

- Clés API **jamais** hardcodées
- `.env` hors image
- `auth.json` généré dynamiquement
- Volumes persistés côté utilisateur

---

## 6. Prompt Injection & LLM Safety

Mesures :

- agents stricts (pas de raisonnement exposé),
- MCP obligatoire,
- pas d’auto-modification des règles,
- pas d’input utilisateur dynamique non contrôlé.

👉 Une injection ne permet **pas** d’exécuter du code.

---

## 7. Risques assumés

| Risque | Justification |
|-----|---------------|
| Outils offensifs | Cœur du produit |
| Root dans toolbox | Nécessaire |
| Docker socket | Maîtrisé |

👉 Ces risques sont **connus, contrôlés et documentés**.

---

## 8. Ce que Darkmoon ne fait PAS

- pas d’auto-propagation,
- pas de persistance hors périmètre,
- pas d’exploitation destructive,
- pas d’exécution hors scope.

---

## 9. Conclusion sécurité

Darkmoon est :
- offensif par vocation,
- défensif par conception,
- contrôlé par architecture.

👉 **La sécurité est une contrainte fondatrice, pas un ajout.**