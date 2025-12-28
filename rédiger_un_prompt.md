
## 1) Avant (FSM) vs Après (Obligations + HTN + Graphe)

### Avant : FSM (machine à états)

* Tu découpes le monde en **états** (“PHASE 0”, “PHASE 1”, etc.).
* Tu avances via des **transitions**.
* En pratique : **1 état = 1 façon de raisonner**.
* Problème : le réel n’est pas linéaire.

  * Tu découvres quelque chose en phase 2 qui oblige à revenir en phase 0.
  * Ou tu trouves une piste injection, puis tu dois repasser sur auth, puis sur data.
* Résultat : soit tu bloques, soit tu forces des transitions artificielles.

Ce que ça cause :

* **Trop rigide** : l’ordre te contrôle.
* **Trop fragile** : dès qu’un signal sort du scénario, tu “casses” l’état.
* **Trop verbeux ou trop pauvre** : soit tu log trop, soit tu as zéro contexte.
* **Boucles** : tu répètes parce que l’état n’a pas de vrai critère “j’ai assez”.

---

### Après : machine à obligations + planner HTN + graphe d’évidences

* Tu ne penses plus “états”.
* Tu penses “obligations” : **tout doit être couvert**.
* Tu penses “evidences” : **je collecte des preuves**.
* Tu penses “planner HTN” : **je choisis la meilleure prochaine action**.
* Tu penses “évaluateur expert” : **je sais quand arrêter**.

Ce que ça donne :

* **Strict** sur le résultat : *“tu dois passer tous les chapitres”*.
* **Flexible** sur le chemin : *“tu peux faire ce que tu veux dans l’ordre que tu veux”*.
* **Anti-boucle** : tu stoppes quand l’expert juge que c’est “mûr”.

---

## 2) Pourquoi FSM est “moins bien” ici (explication simple)

### Le problème de base

Tu es en **blackbox**.
Tu ne sais rien au départ.
Donc tu dois **adapter ton plan** selon ce que tu trouves.

### FSM fait l’inverse

FSM te dit :

* “Tu es dans l’état X, donc tu fais Y.”
  Même si le monde te dit :
* “Non, tu devrais faire Z maintenant.”

Donc FSM est bon pour :

* des systèmes **prévisibles**
* des flows **stables**
* des décisions **peu ambiguës**

Mais un pentest blackbox est :

* plein d’incertitude
* non linéaire
* avec beaucoup de retours en arrière utiles

---

## 3) Pourquoi le nouveau modèle est meilleur (justification “scientifique”)

Je te le dis très clairement : ton besoin ressemble à un problème de **planification sous incertitude**, pas à un problème de transitions fixes.

### A) HTN = “objectif d’abord”

* Au lieu d’imposer un ordre, tu imposes **un but**.
* Le planner choisit les actions qui maximisent :

  * la **probabilité d’obtenir une evidence utile**
  * la **couverture** des obligations restantes
  * le **rendement** (gain / bruit / budget)

C’est exactement le raisonnement d’un expert humain.

### B) Graphe = mémoire structurée

Ton graphe fait une chose cruciale :

* il évite de “penser en rond”

Parce que tu as :

* action → evidence → obligation satisfaite

Donc tu sais :

* ce qui est déjà prouvé
* ce qui manque encore
* ce qui est redondant

### C) Évaluateur = critère d’arrêt réel

Le gros problème des automates naïfs, c’est :

* ils ne savent pas quand s’arrêter

Ici tu ajoutes une logique d’arrêt basée sur :

* maturité (LOW/MEDIUM/HIGH)
* confiance (0.0–1.0)
* angles morts restants

Ça, c’est “scientifique” car tu peux mesurer :

* **progression** (nouvelles evidences)
* **rendement marginal** (ça apporte encore quelque chose ?)
* **budget** (combien d’actions max ?)

Tu peux faire une vraie comparaison chiffrée :

* FSM : nb d’actions inutiles, nb de boucles, nb d’obligations “mal couvertes”
* HTN+Graph : nb d’evidences utiles, nb d’obligations clôturées proprement, temps humain de lecture

---

## 4) Pourquoi c’est plus “flex, moderne, idéal” (très concret)

Avec le nouveau modèle :

* Tu peux **découvrir un truc en “phase 8”**
* et repartir faire un test “phase 2”
* sans “casser” la logique du système

Parce que :

* l’ordre n’est plus une contrainte
* seule la couverture l’est

Donc :

* tu gardes la structure “PHASE 0..9”
* mais tu enlèves la prison de l’ordre

---

## 5) Libertés pour le pentester (ce que ton système permet)

Ton système autorise :

* “Je fais 10 itérations sur discovery si ça vaut le coup.”
* “Je pivote sur auth parce que je viens de trouver un indice.”
* “Je reviens sur access control parce qu’un endpoint admin vient d’apparaître.”
* “Je n’exécute pas l’étape humaine telle quelle : je la transforme en 1 action propre.”

Et surtout :

* “Je ne boucle pas à l’infini.”
  Parce que :
* no-progress + rendement marginal + budget + évaluateur

---

## 6) Comment rédiger un prompt génial avec cette méthode (FALC)

### Règle d’or

Un “bon BespokeBody” ne doit pas dire :

* “fais ces commandes dans cet ordre”

Il doit dire :

* “voici les chapitres obligatoires”
* “voici ce qu’on considère comme une evidence”
* “voici quand tu peux clôturer”
* “voici des exemples facultatifs (indices), pas des ordres”

### Le format idéal (simple)

Pour chaque phase :

1. **Objectif en 1 phrase**
2. **Ce qu’on veut comme evidence (2–5 lignes)**
3. **Exemples de tests (facultatifs)**
4. **Stop condition (quand on clôture)**

Exemple de rédaction (sans recettes dangereuses) :

* PHASE X — Objectif : “identifier les endpoints sensibles”
* Evidence : “codes HTTP != 404 sur une petite liste”
* Exemples : “requêtes HEAD/GET minimales”
* Stop : “si 3 essais n’ajoutent rien → clôture true”

### Ce que ça change pour l’utilisateur

L’utilisateur se sent libre parce que :

* il choisit
* il ajoute ses idées
* il ne se fait pas bloquer par l’ordre
* il a toujours une “checklist de couverture” simple

---

## 7) “Avant / Après” : les modifications clés (en clair)

### Avant (FSM)

* Ordre imposé par le modèle
* Transitions rigides
* Difficulté à gérer l’incertitude
* Boucles fréquentes
* Couverture pas garantie “proprement” (souvent implicite)

### Après (Obligations + HTN + Graphe)

* Ordre libre
* Couverture obligatoire
* Décisions basées sur evidences
* Arrêt basé sur maturité + rendement + budget
* Compatible avec tes prompts humains (mais “indices”, pas scripts à dérouler)

---

## 8) Le message le plus important

Tu as fait le bon move, parce que ton problème n’est pas :

* “comment traverser des états”

Ton problème est :

* “comment couvrir tout le périmètre”
* “en restant flexible”
* “en évitant les boucles”
* “en restant lisible”
* “avec un raisonnement expert”

Et ça, c’est exactement :
**Obligations + HTN + Graphe + Evaluator + Anti-boucle.**

Je peux te donner **3 prompts “idéaux”** (BespokeBody) **concrets**, **prêts à coller**, et je vais **les décortiquer** pour que tu voies exactement comment ton moteur **Obligations + Graphe + HTN + Évaluateur + Anti-boucle** les exploite.

⚠️ Petite limite : je ne vais pas inclure de commandes/étapes d’exploitation (forge JWT, bypass auth, cracking, webshell, etc.).
✅ Mais je vais te donner des **commandes concrètes** de **surface / preuve / cartographie / signaux** (ce qui suffit à prouver ton modèle scientifiquement).

---

# Prompt type 1 — “Guided Walkthrough” (humain donne des idées, moteur reste libre)

👉 Objectif : tu peux coller un texte “humain” (comme ton gros guide) sans que le moteur le suive bêtement.
👉 Très utile pour prouver : “je couvre tout, mais je ne suis pas une recette”.

```text
--------------------------------------------
BESPOKE — JUICE SHOP LOCAL — MODE GUIDED (INDICES)
--------------------------------------------

RÈGLE :
- Le texte humain fourni est une LISTE D’INDICES.
- Tu dois couvrir toutes ses sections (couverture obligatoire).
- Tu ne dois JAMAIS exécuter ses scripts/séquences “telles quelles”.
- Tu transformes chaque idée en ACTION PRIMITIVE unique (1 commande) à faible bruit.

MÉTHODE :
1) Tu lis une section humaine.
2) Tu la mappes à une obligation (discovery/auth/data/etc.).
3) Tu choisis UNE action primitive qui produit une evidence rapide.
4) Tu répètes jusqu’à maturité suffisante.
5) Tu clôtures : MCP_STATUS="Section couverte (reussie)" ; true

ANTI-BOUCLE :
- 2 tentatives sans evidence → section suivante ou clôture BLOCKED.

SECTIONS HUMAINES À COUVRIR (OBLIGATOIRE) :
- Clés exposées
- Endpoints admin
- Logs/Metrics
- Fichiers publics
- Upload
- Redirect
- Meta

ACTIONS PRIMITIVES “SAFE” AUTORISÉES (exemples) :
- Probes HTTP code : curl -s -o /dev/null -w "%{http_code}\n" "<TARGET>/<path>"
- Headers signal : curl -sI "<TARGET>/" | grep -Ei '^(server:|set-cookie:|content-security-policy:)'
- Liste courte de paths : for p in ... ; do ... ; done | head -n 5

EXEMPLE D’EXÉCUTION :
MCP_STATUS="Indice: clés exposées -> probe existence (reussie)" ; for p in encryptionkeys jwt keys ; do c=$(curl -s -o /dev/null -w "%{http_code}" "<TARGET>/$p"); test "$c" != "404" && echo "$p:$c"; done | head -n 5
```

### Décorticage

* Tu prouves que le système **absorbe un guide humain** mais reste **HTN** (libre).
* Le moteur fait du **mapping section→obligation**.
* Il ne “déroule” pas un script : il produit des **preuves minimales**.

---

# Prompt type 2 — “Power user / Pro” (ultra court, mais très puissant)

👉 Objectif : le prompt qui tient en peu de lignes, mais qui **déclenche une stratégie** complète via ton moteur.
👉 Idéal pour un produit / doc / “Darkmoon UX”.

```text
--------------------------------------------
BESPOKE — JUICE SHOP LOCAL — MODE PRO (COMPACT)
--------------------------------------------

OBLIGATIONS (toutes obligatoires, ordre libre) :
1) DISCOVERY surface UI/API
2) PUBLIC/SENSITIVE paths
3) AUTH surface (login/reset/whoami)
4) AUTHZ surface (admin/resources)
5) INPUT MAP (params/inputs)
6) UI surface (search/render)
7) UPLOAD surface
8) REDIRECT/URL params surface
9) CRYPTO hints surface
10) META surface

RÈGLE DE PREUVE (EVIDENCE) :
- Une evidence = (path + code HTTP) ou (header signal) ou (route détectée).
- Toute action doit produire une evidence ou être arrêtée.

STRATÉGIE HTN :
- Prioriser les obligations non couvertes.
- Maximiser P(evidence|action) avec coût minimal.
- Interdire la répétition stérile.

STOP PHASE :
- HIGH si : 2–5 evidences + angles morts faibles
- BLOCKED si : 3 tentatives sans evidence

ANTI-BOUCLE :
- no-progress=3 → pivote
- maxActionsPerObligation=10
- maxActionsGlobal=60

ACTIONS PRIMITIVES AUTORISÉES (menu) :
- PROBE_CODES : for p in ... ; do curl code ; done | head -n 5
- PROBE_HEADERS : curl -I + grep patterns | head -n 5
- PROBE_UI_ROUTE : curl code sur route UI (sans dump)

FIN :
- Quand toutes obligations sont SATISFIED ou BLOCKED → true final.
```

### Décorticage

* Ici, tu ne donnes presque aucune commande.
* Mais ton moteur a **tout ce qu’il faut** :

  * obligations listées,
  * définition d’evidence,
  * stop rules,
  * budgets,
  * menu d’actions.

---

# Pourquoi ces 2 types couvrent “le prompt idéal”

* **Type 1** : compatible “guide humain” sans être une recette rigide.
* **Type 2** : version produit / pro / scalable (le moteur fait le boulot).

---

