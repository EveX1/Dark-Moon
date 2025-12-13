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

# Exécuter un pentest Web & autres stacks

il est possible d'exécuter une campagne de pentest avec un prompt personalité qui est par defaut dans le répertoire `/prompt/`, il doit être placé à cet endroit et etre appelé via l'argument prompt-file

Commande type (réseau interne Compose : hôte `zap`, port `8888`, token lu dans le volume partagé `/zap/wrk`) :

```bash
docker compose exec darkmoon bash -lc './agentfactory \
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
---

# Liste de la toolbox:

Voici **tous les outils réellement installés / présents dans l’image finale** via **Dockerfile + setup.sh + setup_py.sh** (et les symlinks/wrappers), dans un tableau.

> ⚠️ Je **n’inclus pas** les *libs* (libssl, zlib, etc.) ni les *outils de build* du stage `builder` (gcc, make…), car ils ne sont pas dans l’image runtime finale.
> ⚠️ `docker-compose` installe/contient aussi **ZAP** dans un autre container (`ghcr.io/zaproxy/zaproxy:weekly`) → **pas inclus ici** car ce n’est **pas** “installé via le Dockerfile darkmoon”.

### Outils installés dans l’image runtime `darkmoon`

| Outil (commande)                 | Source / méthode d’installation              | Emplacement (binaire)                                                   | Notes                                                                          |
| -------------------------------- | -------------------------------------------- | ----------------------------------------------------------------------- | ------------------------------------------------------------------------------ |
| `bash`                           | `apt-get install`                            | `/bin/bash`                                                             | Shell runtime                                                                  |
| `ca-certificates`                | `apt-get install`                            | (système)                                                               | Certificats TLS                                                                |
| `tzdata`                         | `apt-get install`                            | (système)                                                               | Timezone                                                                       |
| `dig` / `nslookup`… (`dnsutils`) | `apt-get install dnsutils`                   | `/usr/bin/dig`                                                          | DNS tooling                                                                    |
| `curl` (Debian)                  | `apt-get install curl`                       | `/usr/bin/curl`                                                         | Curl système                                                                   |
| `curl` (custom 8.15.0)           | build + `COPY /out/curl` + `PATH`            | `/opt/darkmoon/curl/bin/curl`                                           | **Prioritaire** dans le `PATH`                                                 |
| `jq`                             | `apt-get install jq`                         | `/usr/bin/jq`                                                           | JSON CLI                                                                       |
| `hydra`                          | `apt-get install hydra`                      | `/usr/bin/hydra`                                                        | Brute force                                                                    |
| `snmp*` (`snmp`)                 | `apt-get install snmp`                       | `/usr/bin/snmpwalk` etc.                                                | Suite SNMP                                                                     |
| `ssh` (client)                   | `apt-get install openssh-client`             | `/usr/bin/ssh`                                                          | SSH client                                                                     |
| `dirb`                           | build depuis sources + copy                  | `/usr/local/bin/dirb`                                                   | Wordlists copiées aussi                                                        |
| Wordlists DIRB                   | copy                                         | `/usr/share/wordlists/dirb/`                                            | + symlink compat `/usr/share/dirb/wordlists`                                   |
| `waybackurls`                    | Go build (`setup.sh`) + copy                 | `/usr/local/bin/waybackurls`                                            | Recon URLs archive.org                                                         |
| `kubectl`                        | download binaire officiel (dl.k8s.io) + copy | `/usr/local/bin/kubectl`                                                | v1.34.2 (ARG)                                                                  |
| `kube-bench`                     | `go install` + copy                          | `/usr/local/bin/kube-bench`                                             | v0.14.0                                                                        |
| `grpcurl`                        | build depuis sources + copy                  | `/usr/local/bin/grpcurl`                                                | deps Go patchées                                                               |
| `agentfactory`                   | build C++ + symlink                          | `/opt/darkmoon/agentfactory` + `/usr/local/bin/agentfactory`            | Binaire C++ statique                                                           |
| `mcp`                            | build C++ + symlink                          | `/opt/darkmoon/mcp` + `/usr/local/bin/mcp`                              | Linké sur libcurl custom                                                       |
| `ZAP-CLI`                        | build C++ + symlink                          | `/opt/darkmoon/ZAP-CLI` + `/usr/local/bin/ZAP-CLI`                      | Client maison (pas l’image ZAP)                                                |
| `ruby`                           | build Ruby 3.3.5 + copy                      | `/opt/darkmoon/ruby/bin/ruby`                                           | Ruby embarqué                                                                  |
| `gem`                            | Ruby install                                 | `/opt/darkmoon/ruby/bin/gem`                                            | RubyGems                                                                       |
| `bundler`                        | `gem install bundler:2.7.2`                  | `/opt/darkmoon/ruby/bin/bundle`                                         | Utilisé pour WhatWeb                                                           |
| `whatweb`                        | git clone + gems bundler + wrapper           | `/usr/local/bin/whatweb`                                                | Wrapper lance `/opt/darkmoon/whatweb/whatweb` avec bundler                     |
| `python3`                        | build Python 3.12.6 + copy                   | `/opt/darkmoon/python/bin/python3`                                      | Python embarqué                                                                |
| `pip3`                           | `--with-ensurepip`                           | `/opt/darkmoon/python/bin/pip3`                                         | Pip embarqué                                                                   |
| `impacket`                       | `pip install impacket==0.12.0`               | (site-packages)                                                         | Lib + entrypoints impacket                                                     |
| `impacket-smbclient`             | wrapper custom                               | `/usr/local/bin/impacket-smbclient`                                     | `python -m impacket.smbclient`                                                 |
| `rpcdump.py`                     | wrapper custom                               | `/usr/local/bin/rpcdump.py`                                             | `python -m impacket.examples.rpcdump`                                          |
| `NetExec` (`nxc` / `netexec`)    | `pip install git+...NetExec@v1.4.0`          | `/opt/darkmoon/python/bin/nxc` (ou `netexec`)                           | Install depuis GitHub                                                          |
| `netexec`                        | wrapper custom                               | `/usr/local/bin/netexec`                                                | Appelle `nxc` ou `netexec`                                                     |
| `crackmapexec`                   | wrapper custom                               | `/usr/local/bin/crackmapexec`                                           | Alias compat → `netexec`                                                       |
| `bloodhound` (ingestor python)   | `pip install bloodhound==1.7.2`              | `/opt/darkmoon/python/bin/bloodhound`                                   | Ingestor Python                                                                |
| `bloodhound-python`              | wrapper custom                               | `/usr/local/bin/bloodhound-python`                                      | Alias explicite                                                                |
| `wafw00f`                        | `pip install wafw00f`                        | `/opt/darkmoon/python/bin/wafw00f` + wrapper                            | Wrapper `/usr/local/bin/wafw00f`                                               |
| `sqlmap`                         | `pip install sqlmap`                         | `/opt/darkmoon/python/bin/sqlmap` + wrapper                             | Wrapper `/usr/local/bin/sqlmap`                                                |
| `arjun`                          | `pip install arjun`                          | `/opt/darkmoon/python/bin/arjun` + wrapper                              | Wrapper `/usr/local/bin/arjun`                                                 |
| `aws` (AWS CLI)                  | `pip install awscli`                         | `/opt/darkmoon/python/bin/aws` + wrapper                                | Wrapper `/usr/local/bin/aws`                                                   |
| `naabu`                          | build Go (setup.sh) + copy                   | `/opt/darkmoon/kube/naabu` + `/usr/local/bin/naabu`                     | Port scanner                                                                   |
| `httpx`                          | build Go (setup.sh) + copy                   | `/opt/darkmoon/kube/httpx` + `/usr/local/bin/httpx`                     | HTTP probing                                                                   |
| `nuclei`                         | `go install` (setup.sh) + copy               | `/opt/darkmoon/kube/nuclei` + `/usr/local/bin/nuclei`                   | Scanner templates                                                              |
| `nuclei-templates`               | `nuclei -update-templates` + copy            | `/opt/darkmoon/nuclei-templates`                                        | + copie init `/root/nuclei-templates`                                          |
| `zgrab2`                         | `go install` (setup.sh) + copy               | `/opt/darkmoon/kube/zgrab2` + `/usr/local/bin/zgrab2`                   | Banner grabber                                                                 |
| `katana`                         | `go install` (setup.sh) + copy               | `/opt/darkmoon/kube/katana` + `/usr/local/bin/katana`                   | Crawler                                                                        |
| `kubescape`                      | build Go (setup.sh, v3.0.9) + copy           | `/opt/darkmoon/kube/kubescape` + `/usr/local/bin/kubescape`             | K8s security scanner                                                           |
| `kubectl-who-can`                | build Go (setup.sh) + copy                   | `/opt/darkmoon/kube/kubectl-who-can` + `/usr/local/bin/kubectl-who-can` | K8s RBAC                                                                       |
| `kubeletctl`                     | build Go (setup.sh) + copy                   | `/opt/darkmoon/kube/kubeletctl` + `/usr/local/bin/kubeletctl`           | Kubelet tooling                                                                |
| `rbac-police`                    | build Go (setup.sh) + copy                   | `/opt/darkmoon/kube/rbac-police` + `/usr/local/bin/rbac-police`         | Peut être un **stub** si build échoue                                          |
| `ffuf`                           | build Go (setup.sh)                          | `/out/bin/ffuf` (builder)                                               | /usr/local/bin/ |
| `acl` (`setfacl`)                | `apt-get install acl`                        | `/usr/bin/setfacl`                                                      | Pour permissions par défaut `/opt/darkmoon/scripts`                            |



## “outils” installés par `pip install impacket==0.12.0` (scripts fournis par Impacket)

Ces scripts sont installés comme commandes dans `/opt/darkmoon/python/bin/` (donc dans le `PATH`).

| Outil (commande)     | Source / méthode | Emplacement                                   | Notes                               |
| -------------------- | ---------------- | --------------------------------------------- | ----------------------------------- |
| `secretsdump.py`     | pip (impacket)   | `/opt/darkmoon/python/bin/secretsdump.py`     | Dump secrets AD                     |
| `wmiexec.py`         | pip (impacket)   | `/opt/darkmoon/python/bin/wmiexec.py`         | Exec WMI                            |
| `psexec.py`          | pip (impacket)   | `/opt/darkmoon/python/bin/psexec.py`          | Exec via SMB service                |
| `smbexec.py`         | pip (impacket)   | `/opt/darkmoon/python/bin/smbexec.py`         | Exec SMB                            |
| `atexec.py`          | pip (impacket)   | `/opt/darkmoon/python/bin/atexec.py`          | Exec via AT scheduler               |
| `dcomexec.py`        | pip (impacket)   | `/opt/darkmoon/python/bin/dcomexec.py`        | Exec DCOM                           |
| `mssqlclient.py`     | pip (impacket)   | `/opt/darkmoon/python/bin/mssqlclient.py`     | Client MSSQL                        |
| `smbclient.py`       | pip (impacket)   | `/opt/darkmoon/python/bin/smbclient.py`       | SMB client (en plus de ton wrapper) |
| `lookupsid.py`       | pip (impacket)   | `/opt/darkmoon/python/bin/lookupsid.py`       | RID/SID enum                        |
| `GetADUsers.py`      | pip (impacket)   | `/opt/darkmoon/python/bin/GetADUsers.py`      | Enum users AD                       |
| `GetNPUsers.py`      | pip (impacket)   | `/opt/darkmoon/python/bin/GetNPUsers.py`      | AS-REP roast                        |
| `GetUserSPNs.py`     | pip (impacket)   | `/opt/darkmoon/python/bin/GetUserSPNs.py`     | Kerberoast                          |
| `ticketer.py`        | pip (impacket)   | `/opt/darkmoon/python/bin/ticketer.py`        | Golden/Silver tickets               |
| `raiseChild.py`      | pip (impacket)   | `/opt/darkmoon/python/bin/raiseChild.py`      | Trust abuse                         |
| `addcomputer.py`     | pip (impacket)   | `/opt/darkmoon/python/bin/addcomputer.py`     | Add machine account                 |
| `changepasswd.py`    | pip (impacket)   | `/opt/darkmoon/python/bin/changepasswd.py`    | Change password                     |
| `getPac.py`          | pip (impacket)   | `/opt/darkmoon/python/bin/getPac.py`          | Kerberos PAC                        |
| `getTGT.py`          | pip (impacket)   | `/opt/darkmoon/python/bin/getTGT.py`          | Kerberos TGT                        |
| `getST.py`           | pip (impacket)   | `/opt/darkmoon/python/bin/getST.py`           | Kerberos ST                         |
| `klistattack.py`     | pip (impacket)   | `/opt/darkmoon/python/bin/klistattack.py`     | Kerberos attack helper              |
| `samrdump.py`        | pip (impacket)   | `/opt/darkmoon/python/bin/samrdump.py`        | SAMR enum                           |
| `reg.py`             | pip (impacket)   | `/opt/darkmoon/python/bin/reg.py`             | Remote registry ops                 |
| `netview.py`         | pip (impacket)   | `/opt/darkmoon/python/bin/netview.py`         | Network view                        |
| `services.py`        | pip (impacket)   | `/opt/darkmoon/python/bin/services.py`        | Service control                     |
| `eventquery.py`      | pip (impacket)   | `/opt/darkmoon/python/bin/eventquery.py`      | Event logs                          |
| `dpapi.py`           | pip (impacket)   | `/opt/darkmoon/python/bin/dpapi.py`           | DPAPI ops                           |
| `ntlmrelayx.py`      | pip (impacket)   | `/opt/darkmoon/python/bin/ntlmrelayx.py`      | NTLM relay                          |
| `smbserver.py`       | pip (impacket)   | `/opt/darkmoon/python/bin/smbserver.py`       | SMB server                          |
| `rbcd.py`            | pip (impacket)   | `/opt/darkmoon/python/bin/rbcd.py`            | RBCD abuse                          |
| `findDelegation.py`  | pip (impacket)   | `/opt/darkmoon/python/bin/findDelegation.py`  | Delegation enum                     |
| `GetLAPSPassword.py` | pip (impacket)   | `/opt/darkmoon/python/bin/GetLAPSPassword.py` | LAPS retrieval                      |
| `keylistattack.py`   | pip (impacket)   | `/opt/darkmoon/python/bin/keylistattack.py`   | Keylist attack                      |
| `ping.py`            | pip (impacket)   | `/opt/darkmoon/python/bin/ping.py`            | Ping PoC (impacket)                 |
| `sniffer.py`         | pip (impacket)   | `/opt/darkmoon/python/bin/sniffer.py`         | Sniffer helper                      |

---

# Scripts:

deux scripts disponible dans `/scripts/` sont disponible orchestrant les tools préinstallés, ces derniers snt accompagnés d'une documentation pur chacuns d'entres eux