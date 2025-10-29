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

> Remarques :
>
> * `--context` doit correspondre à un contexte valide dans ton kubeconfig.
> * `--baseurl` est un paramètre d’output/endpoint interne à ton flux (garde ta valeur ou adapte-la si nécessaire).
> * Les résultats seront écrits dans `"$DM_HOME/out"` (donc visibles sur l’hôte sous `${HOME}/darkmoon-docker-fs/out`).
