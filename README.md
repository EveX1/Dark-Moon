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

il est possible d'exécuter une campagne de pentest avec un prompt personalité qui est par defaut dans le répertoire /prompt/, il doit être placé à cet endroit et etre appelé via l'argument prompt-file

Commande type (réseau interne Compose : hôte `zap`, port `8888`, token lu dans le volume partagé `/zap/wrk`) :

```bash
docker compose exec darkmoon bash -lc './agentfactory \
  --web \
  --prompt-file dvga_extreme_autopwn.txt \
  --baseurl "http://dvga:5013/" \
  --http-env dvga.env'
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
docker compose exec darkmoon bash -lc './agentfactory \
  --k8s \
  --prompt-file /opt/darkmoon/prompt/dvga_extreme_autopwn.txt \
  --baseurl "http://localhost:1230"'   
'
```

## Commande type (Option B, kubeconfig monté en /root/.kube)

```bash
docker compose exec darkmoon bash -lc '
  cd "$DM_HOME" \
  && ./agentfactory \
       --k8s \
       --kubeconfig "/root/.kube/config" \
       --context "kind-hacklab" \
       --outdir "./out" \
       --baseurl "http://localhost:1230"
'
```
