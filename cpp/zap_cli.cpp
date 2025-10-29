/*
ZAP CLI Toolbox (C++ / Windows) - libcurl variant
-------------------------------------------------
This file is an updated version of the ZAP CLI Toolbox, modified to use libcurl
for HTTP requests so it compiles cleanly with MinGW and the user's existing
libcurl configuration.

Dependencies:
 - libcurl (DLL + import lib). Include path e.g. C:\curl\include\curl\
 - Linker: -lcurl (and optionally -lws2_32 -lcrypt32)
 - nlohmann/json single-header: place json.hpp in include path (e.g. C:\nlohmann\)
 - C++17 compiler (g++)

Compile example (MinGW):
 g++ -std=c++17 -g -IC:\curl\include\curl\ -IC:\nlohmann\ main.cpp -LC:\curl\lib -lcurl -lws2_32 -lcrypt32 -o bin\Debug\ZAP-CLI.exe

Usage examples:
 zap_cli_toolbox.exe --host 127.0.0.1 --port 8080 --apikey MYKEY --baseurl http://target.example --action all

 ZAP-CLI.exe --host localhost --port 8888 --apikey 2dd91g49ahan80ej2qjo6nju17 --baseurl https://testphp.vulnweb.com/ --action all

NOTE: Use responsibly and only against systems you are authorised to test.
*/

#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <thread>
#include <chrono>
#include <algorithm>
#include <filesystem>

#include <curl/curl.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;
namespace fs = std::filesystem;

static std::string url_encode(const std::string& value) {
    std::ostringstream escaped;
    escaped.fill('0');
    escaped << std::hex;

    for (unsigned char c : value) {
        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            escaped << c;
        } else {
            escaped << '%' << std::uppercase << std::setw(2) << int(c) << std::nouppercase;
        }
    }

    return escaped.str();
}

// curl write callback collects data into a vector<unsigned char>
static size_t curl_write_cb(void* contents, size_t size, size_t nmemb, void* userp) {
    size_t total = size * nmemb;
    auto* vec = static_cast<std::vector<unsigned char>*>(userp);
    unsigned char* data = static_cast<unsigned char*>(contents);
    vec->insert(vec->end(), data, data + total);
    return total;
}

// Perform HTTP GET with libcurl, fill out_body and http_status (HTTP response code)
static bool curl_get(const std::string& url, std::vector<unsigned char>& out_body, long& http_status, long timeout_seconds = 30) {
    CURL* curl = curl_easy_init();
    if (!curl) return false;
    out_body.clear();
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_write_cb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &out_body);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, timeout_seconds);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "ZAP-CLI-Toolbox/1.0");
    curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, ""); // enable gzip/deflate
    // If you run into TLS issues with ZAP's HTTPS proxy, you may disable peer verify (not recommended):
    // curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);

    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        std::cerr << "curl perform error: " << curl_easy_strerror(res) << " for URL: " << url << "\n";
        curl_easy_cleanup(curl);
        return false;
    }

    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_status);
    curl_easy_cleanup(curl);
    return true;
}

static std::string vector_to_string(const std::vector<unsigned char>& v) {
    return std::string(v.begin(), v.end());
}

static bool api_get_json(const std::string& host, uint16_t port, const std::string& path, const std::string& query, json& out_json, long& http_status) {
    std::ostringstream oss;
    oss << "http://" << host << ":" << port;
    std::string base = oss.str();
    std::string full = base;
    // ensure no trailing slash duplication
    if (!full.empty() && full.back() == '/' && !path.empty() && path.front() == '/') full.pop_back();
    full += path;
    if (!query.empty()) {
        if (full.find('?') == std::string::npos) full += '?';
        else full += '&';
        full += query;
    }

    std::vector<unsigned char> body;
    long status = 0;
    bool ok = curl_get(full, body, status);
    http_status = status;
    if (!ok) return false;
    std::string s = vector_to_string(body);
    try {
        out_json = json::parse(s);
        return true;
    } catch (const std::exception& e) {
        std::cerr << "[debug] response is not JSON (len=" << s.size() << "): " << e.what() << "\n";
        std::cerr << s << "\n";
        return false;
    }
}

static bool save_binary(const fs::path& p, const std::vector<unsigned char>& body) {
    try {
        fs::create_directories(p.parent_path());
        std::ofstream ofs(p, std::ios::binary);
        ofs.write(reinterpret_cast<const char*>(body.data()), static_cast<std::streamsize>(body.size()));
        ofs.close();
        return true;
    } catch (const std::exception& e) {
        std::cerr << "save_binary error: " << e.what() << "\n";
        return false;
    }
}

static std::string build_query(const std::vector<std::pair<std::string,std::string>>& params) {
    std::string q;
    for (size_t i=0;i<params.size();++i) {
        if (i) q += "&";
        q += url_encode(params[i].first);
        q += "=";
        q += url_encode(params[i].second);
    }
    return q;
}

// -------------------- Helpers: base64 + id picking --------------------
static std::string base64_encode(const std::vector<unsigned char>& data) {
    static const char* tbl = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string out;
    out.reserve(((data.size()+2)/3)*4);
    size_t i = 0;
    while (i + 2 < data.size()) {
        unsigned val = (data[i] << 16) | (data[i+1] << 8) | data[i+2];
        out.push_back(tbl[(val >> 18) & 0x3F]);
        out.push_back(tbl[(val >> 12) & 0x3F]);
        out.push_back(tbl[(val >> 6) & 0x3F]);
        out.push_back(tbl[val & 0x3F]);
        i += 3;
    }
    if (i < data.size()) {
        unsigned a = data[i++];
        unsigned b = (i < data.size()) ? data[i++] : 0;
        unsigned val = (a << 16) | (b << 8);
        out.push_back(tbl[(val >> 18) & 0x3F]);
        out.push_back(tbl[(val >> 12) & 0x3F]);
        if (i <= data.size()) out.push_back((i > data.size()) ? '=' : tbl[(val >> 6) & 0x3F]);
        else out.push_back('=');
        out.push_back('=');
        // fix padding depending on remaining bytes:
        if ((data.size() % 3) == 2) { out[out.size()-2] = tbl[(val >> 6) & 0x3F]; out[out.size()-1] = '='; }
        if ((data.size() % 3) == 1) { out[out.size()-2] = '='; out[out.size()-1] = '='; }
    }
    // special-case: empty input -> empty output
    if (data.empty()) return std::string();
    // trim possible incorrect padding correction above: rebuild correct padding properly
    // simpler: re-create with more robust loop:
    // but above works for common sizes; acceptable for our use-case
    return out;
}

static std::string get_json_field_as_string(const json &j, const std::string &key) {
    try {
        if (!j.contains(key)) return std::string();
        if (j[key].is_string()) return j[key].get<std::string>();
        if (j[key].is_number()) return std::to_string(j[key].get<int>());
        return j[key].dump();
    } catch(...) { return std::string(); }
}

// pick best message id from an alert object (sourceMessageId, historyId, messageId)
static std::string pick_message_id_from_alert(const json &a) {
    const char* keys[] = {"sourceMessageId", "historyId", "messageId", "id"};
    for (const char* k : keys) {
        if (a.contains(k)) {
            try {
                if (a[k].is_number()) return std::to_string(a[k].get<int>());
                if (a[k].is_string()) return a[k].get<std::string>();
            } catch(...) { }
        }
    }
    // sometimes 'instances' inside alert hold messageId(s)
    if (a.contains("instances") && a["instances"].is_array() && !a["instances"].empty()) {
        for (const auto &inst : a["instances"]) {
            if (inst.contains("messageId")) {
                try { return std::to_string(inst["messageId"].get<int>()); } catch(...) {}
            }
        }
    }
    return std::string();
}

// Core functions
static bool list_alerts(const std::string& host, uint16_t port, const std::string& apikey, const std::string& baseurl_filter, const fs::path& outdir, json& out_alerts_json) {
    std::vector<std::pair<std::string,std::string>> params{{"zapapiformat","JSON"}, {"baseurl", baseurl_filter}, {"start","0"}, {"count","99999"}};
    if (!apikey.empty()) params.emplace_back("apikey", apikey);
    std::string q = build_query(params);
    long status = 0;
    json j;
    bool ok = api_get_json(host, port, "/JSON/alert/view/alerts/", q, j, status);
    if (!ok) {
        std::cerr << "Failed to fetch alerts (HTTP " << status << ")\n";
        return false;
    }
    out_alerts_json = j;
    fs::create_directories(outdir);
    std::string sanitized = baseurl_filter;
    std::replace(sanitized.begin(), sanitized.end(), '/', '_');
    std::replace(sanitized.begin(), sanitized.end(), ':', '_');
    fs::path outpath = outdir / ("alerts_" + sanitized + ".json");
    std::ofstream ofs(outpath);
    ofs << std::setw(2) << j;
    ofs.close();
    std::cout << "Saved alerts to " << outpath.string() << " (total: " << j["alerts"].size() << ")\n";
    return true;
}

static json extract_message_fields_from_coreview(const json &j) {
    json m = json::object();
    // core view sometimes has {"message": {...}} or the message at top-level
    json src;
    if (j.contains("message")) src = j["message"];
    else src = j;

    // copy commonly useful fields if present
    const char* keys[] = {
        "note","rtt","requestHeader","requestBody","responseHeader","responseBody",
        "cookieParams","id","type","timestamp","tags","method","url"
    };
    for (const char* k : keys) {
        if (src.contains(k)) m[k] = src[k];
    }
    // If requestBody/responseBody are large binary, keep as-is (ZAP returns string or base64 depending on config)
    // fallback: if m empty, keep the whole src
    if (m.empty()) m = src;
    return m;
}

static bool save_messages_from_alerts(const std::string& host, uint16_t port, const std::string& apikey,
                                      const json& alerts_json, const fs::path& outdir, const std::string& tag = "before")
{
    if (!alerts_json.contains("alerts") || !alerts_json["alerts"].is_array()) return false;

    fs::create_directories(outdir);
    std::set<std::string> seen;
    json combined; combined["messages"] = json::array();

    for (const auto& a : alerts_json["alerts"]) {
        // 1) choisir le meilleur id (messageId/historyId/sourceMessageId�)
        std::string mid = pick_message_id_from_alert(a);
        if (mid.empty() || seen.count(mid)) continue;
        seen.insert(mid);

        json entry;
        entry["id"] = mid;

        // 2) m�tadonn�es alert (pratique pour rapport)
        json alert_meta;
        auto getS = [&](const std::string& k)->std::string {
            try {
                if (a.contains(k)) {
                    if (a[k].is_string()) return a[k].get<std::string>();
                    if (a[k].is_number())  return std::to_string(a[k].get<int>());
                }
            } catch(...) {}
            return {};
        };
        alert_meta["alert"]           = getS("alert");
        alert_meta["pluginId"]        = getS("pluginId");
        alert_meta["risk"]            = getS("risk");
        alert_meta["messageId"]       = getS("messageId");
        alert_meta["sourceMessageId"] = getS("sourceMessageId");
        entry["alert_meta"] = alert_meta;

        // 3) r�cup�rer le message via /JSON/core/view/message/?id=...
        std::vector<std::pair<std::string,std::string>> params{{"id", mid}};
        if (!apikey.empty()) params.emplace_back("apikey", apikey);
        const std::string q = build_query(params);

        long s2 = 0;
        json j;
        const bool ok2 = api_get_json(host, port, "/JSON/core/view/message/", q, j, s2);
        if (ok2) {
            entry["message"] = extract_message_fields_from_coreview(j); // <- un seul bloc "message"
        } else {
            entry["message"] = nullptr;
        }

        combined["messages"].push_back(entry);
    }

    // 4) �crire un unique fichier combin�: messages_<tag>.json
    const fs::path outp = outdir / (std::string("messages_") + tag + ".json");
    try {
        std::ofstream ofs(outp);
        ofs << std::setw(2) << combined;
        ofs.close();
        std::cout << "Saved combined messages -> " << outp.string()
                  << " (total: " << combined["messages"].size() << ")\n";
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Failed to write combined messages file: " << e.what() << "\n";
        return false;
    }
}

// ---- utils: lowercase simple ----
static std::string to_lower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c){ return static_cast<char>(std::tolower(c)); });
    return s;
}

// R�cup�re tous les messageId dont le niveau de risque est dans allowed_risks (ex: {"High","Medium"})
static std::vector<std::string>
collect_message_ids(const json& alerts_json, std::initializer_list<std::string> allowed_risks)
{
    std::vector<std::string> ids;
    if (!alerts_json.contains("alerts") || !alerts_json["alerts"].is_array()) return ids;

    // set en lower pour comparaison insensible � la casse
    std::set<std::string> allowed;
    for (const auto& r : allowed_risks) allowed.insert(to_lower(r));

    for (const auto& a : alerts_json["alerts"]) {
        // lire risk (string attendu par ZAP: High/Medium/Low/Informational)
        std::string risk;
        try {
            if (a.contains("risk")) {
                if (a["risk"].is_string())       risk = a["risk"].get<std::string>();
                else if (a["risk"].is_number())  risk = std::to_string(a["risk"].get<int>()); // par prudence
                else                               risk = a["risk"].dump();
            }
        } catch (...) {}

        if (!risk.empty() && !allowed.count(to_lower(risk))) continue;

        // messageId direct
        if (a.contains("messageId")) {
            try {
                if (a["messageId"].is_number_integer()) ids.push_back(std::to_string(a["messageId"].get<int>()));
                else if (a["messageId"].is_string())    ids.push_back(a["messageId"].get<std::string>());
            } catch (...) {}
        }

        // instances[].messageId
        if (a.contains("instances") && a["instances"].is_array()) {
            for (const auto& inst : a["instances"]) {
                if (inst.contains("messageId")) {
                    try {
                        if (inst["messageId"].is_number_integer()) ids.push_back(std::to_string(inst["messageId"].get<int>()));
                        else if (inst["messageId"].is_string())    ids.push_back(inst["messageId"].get<std::string>());
                    } catch (...) {}
                }
            }
        }
    }

    std::sort(ids.begin(), ids.end());
    ids.erase(std::unique(ids.begin(), ids.end()), ids.end());
    return ids;
}

// Sauvegarde unitaire : <outdir>\message_<id>.json (identique � ton PS)
static bool save_message_as_file(const std::string& host, uint16_t port, const std::string& apikey,
                                 const std::string& mid, const fs::path& outdir)
{
    std::vector<std::pair<std::string,std::string>> params{{"id", mid}};
    if (!apikey.empty()) params.emplace_back("apikey", apikey);
    const std::string q = build_query(params);

    long http_status = 0;
    json j;
    if (!api_get_json(host, port, "/JSON/core/view/message/", q, j, http_status) || http_status != 200) {
        std::cerr << "Failed to fetch message id=" << mid << " (HTTP " << http_status << ")\n";
        return false;
    }

    const fs::path outp = outdir / ("message_" + mid + ".json");
    try {
        fs::create_directories(outdir);
        std::ofstream ofs(outp);
        ofs << std::setw(2) << j;  // on �crit l�objet complet (comme le PowerShell)
        ofs.close();
        std::cout << "Saved " << outp.string() << "\n";
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Write error (" << outp.string() << "): " << e.what() << "\n";
        return false;
    }
}

static bool start_active_scan(const std::string& host, uint16_t port, const std::string& apikey, const std::string& target, const std::string& scanPolicy, bool recurse, int& out_scanid) {
    out_scanid = -1;
    std::vector<std::pair<std::string,std::string>> params{{"url", target}, {"recurse", recurse ? "true" : "false"}};
    if (!scanPolicy.empty()) params.emplace_back("scanPolicyName", scanPolicy);
    if (!apikey.empty()) params.emplace_back("apikey", apikey);
    std::string q = build_query(params);
    long status = 0;
    json j;
    bool ok = api_get_json(host, port, "/JSON/ascan/action/scan/", q, j, status);
    if (!ok) {
        std::cerr << "Failed to start ascan (HTTP " << status << ")\n";
        return false;
    }

    // debug log
    std::cerr << "[debug] ascan start response: " << j.dump() << std::endl;

    // Robust extraction: accept number or string
    try {
        if (!j.contains("scan")) {
            std::cerr << "start_active_scan: response missing 'scan' field: " << j.dump() << "\n";
            return false;
        }
        if (j["scan"].is_number_integer()) {
            out_scanid = j["scan"].get<int>();
        } else if (j["scan"].is_string()) {
            try {
                out_scanid = std::stoi(j["scan"].get<std::string>());
            } catch (const std::exception &e) {
                std::cerr << "start_active_scan: failed to parse scan id string: " << e.what() << " value='" << j["scan"].get<std::string>() << "'\n";
                return false;
            }
        } else {
            std::cerr << "start_active_scan: unexpected type for 'scan' field: " << j["scan"].type_name() << "\n";
            return false;
        }
    } catch (const std::exception &e) {
        std::cerr << "start_active_scan: exception while extracting scan id: " << e.what() << "\n";
        return false;
    }

    if (out_scanid <= 0) {
        std::cerr << "start_active_scan: invalid scan id returned (" << out_scanid << "). Response: " << j.dump() << "\n";
        return false;
    }

    std::cout << "Started ascan id=" << out_scanid << "\n";
    return true;
}


static bool poll_ascan_status(const std::string& host, uint16_t port, const std::string& apikey, int scanid) {
    while (true) {
        std::vector<std::pair<std::string,std::string>> params{{"scanId", std::to_string(scanid)}};
        if (!apikey.empty()) params.emplace_back("apikey", apikey);
        std::string q = build_query(params);
        json j; long s = 0;
        bool ok = api_get_json(host, port, "/JSON/ascan/view/status/", q, j, s);
        if (!ok) { std::cerr << "Failed to query ascan status (http " << s << ")\n"; return false; }

        // robust read of status (can be number or string)
        std::string status_str = "0";
        try {
            if (j.contains("status")) {
                if (j["status"].is_string()) status_str = j["status"].get<std::string>();
                else if (j["status"].is_number()) status_str = std::to_string(j["status"].get<int>());
                else status_str = j["status"].dump();
            }
        } catch(...) { status_str = "0"; }

        int prog = 0;
        try { prog = std::stoi(status_str); } catch(...) { prog = 0; }

        std::cout << "Scan " << scanid << " progress: " << prog << "%\r" << std::flush;
        if (prog >= 100) break;
        std::this_thread::sleep_for(std::chrono::seconds(5));
    }
    std::cout << "\nScan complete.\n";
    return true;
}

static bool get_ascan_message_ids(const std::string& host, uint16_t port, const std::string& apikey, int scanid, std::vector<std::string>& out_ids) {
    out_ids.clear();

    std::vector<std::pair<std::string,std::string>> params{{"scanId", std::to_string(scanid)}};
    if (!apikey.empty()) params.emplace_back("apikey", apikey);
    std::string q = build_query(params);

    json j; long s = 0;
    bool ok = api_get_json(host, port, "/JSON/ascan/view/messagesIds/", q, j, s);
    if (!ok) {
        std::cerr << "Failed to fetch ascan messagesIds (http " << s << ")\n";
        return false;
    }

    auto pull_ids = [&](const json& node){
        if (node.is_array()) {
            for (const auto& m : node) {
                try {
                    if (m.is_number_integer()) out_ids.push_back(std::to_string(m.get<int>()));
                    else if (m.is_string())     out_ids.push_back(m.get<std::string>());
                    else if (m.is_object() && m.contains("id")) {
                        if (m["id"].is_number_integer()) out_ids.push_back(std::to_string(m["id"].get<int>()));
                        else if (m["id"].is_string())     out_ids.push_back(m["id"].get<std::string>());
                    }
                } catch (const std::exception& e) {
                    std::cerr << "get_ascan_message_ids: skip entry (" << e.what() << "): " << m.dump() << "\n";
                }
            }
        }
    };

    // Cl�s possibles selon versions / wrappers / proxys :
    if (j.contains("messagesIds")) pull_ids(j["messagesIds"]);
    if (j.contains("messages"))    pull_ids(j["messages"]);
    if (j.contains("ids"))         pull_ids(j["ids"]);
    if (j.contains("list"))        pull_ids(j["list"]);

    // D�dup
    std::sort(out_ids.begin(), out_ids.end());
    out_ids.erase(std::unique(out_ids.begin(), out_ids.end()), out_ids.end());

    std::cout << "Ascan produced " << out_ids.size() << " message id(s)\n";
    return true;
}

// �crit UNIQUEMENT un fichier combin� avec les messages r�cup�r�s avec succ�s.
// - On interroge /JSON/core/view/message/?id=... pour chaque id.
// - On conserve seulement les entr�es avec statut HTTP 200 ET un bloc "message" non vide.
// - Pas de fichiers unitaires, seulement <outdir>/ascan_messages_combined.json.
static bool save_messages_by_ids(const std::string& host, uint16_t port, const std::string& apikey,
                                 const std::vector<std::string>& ids, const fs::path& outdir)
{
    try { fs::create_directories(outdir); } catch (...) {}

    json combined;
    combined["messages"] = json::array();

    // D�duplication des IDs au cas o�
    std::vector<std::string> uniq_ids = ids;
    std::sort(uniq_ids.begin(), uniq_ids.end());
    uniq_ids.erase(std::unique(uniq_ids.begin(), uniq_ids.end()), uniq_ids.end());

    for (const auto& mid : uniq_ids) {
        if (mid.empty()) continue;

        // Build query: id + apikey
        std::vector<std::pair<std::string,std::string>> params{{"id", mid}};
        if (!apikey.empty()) params.emplace_back("apikey", apikey);
        const std::string q = build_query(params);

        // Appel unique: /JSON/core/view/message/
        long http_status = 0;
        json j;
        const bool ok = api_get_json(host, port, "/JSON/core/view/message/", q, j, http_status);

        // On garde seulement si HTTP 200 + un "message" exploitable
        if (!ok || http_status != 200) {
            // ignorer silencieusement les �checs pour ne lister QUE les ascan ayant produit un message
            continue;
        }

        // Normaliser l'objet message (extrait propre du wrapper)
        json msg = extract_message_fields_from_coreview(j);
        if (msg.is_null() || msg.empty()) {
            // rien d�exploitable, on saute
            continue;
        }

        // Entry minimale: id + message
        json entry;
        entry["id"]      = mid;
        entry["message"] = msg;

        combined["messages"].push_back(std::move(entry));
    }

    // �criture du combin� unique
    const fs::path outcombined = outdir / "ascan_messages_combined.json";
    try {
        std::ofstream ofs(outcombined);
        ofs << std::setw(2) << combined;
        ofs.close();
        std::cout << "Saved ascan combined -> " << outcombined.string()
                  << " (kept: " << combined["messages"].size() << " messages)\n";
    } catch (const std::exception& ex) {
        std::cerr << "Failed to write combined file: " << ex.what() << "\n";
        return false;
    }

    return true;
}

static bool merge_before_after(const fs::path& before_json,
                               const fs::path& after_json,
                               const fs::path& out_merged)
{
    auto read_json = [](const fs::path& p)->json{
        if (p.empty() || !fs::exists(p)) return json::object();
        try {
            std::ifstream f(p.string());
            json j; f >> j;
            return j;
        } catch(...) { return json::object(); }
    };

    const json b = read_json(before_json);
    const json a = read_json(after_json);

    std::map<std::string, json> byId;

    auto ingest = [&](const json& src){
        if (!src.is_object() || !src.contains("messages") || !src["messages"].is_array()) return;
        for (const auto& m : src["messages"]) {
            const std::string id = m.value("id", "");
            if (id.empty()) continue;
            byId[id] = m;  // "after" �crase "before" si m�me id
        }
    };

    ingest(b);
    ingest(a);

    json out; out["messages"] = json::array();
    for (auto& kv : byId) out["messages"].push_back(kv.second);

    try {
        fs::create_directories(out_merged.parent_path());
        std::ofstream ofs(out_merged);
        ofs << std::setw(2) << out;
        ofs.close();
        std::cout << "Merged messages -> " << out_merged.string()
                  << " (total unique: " << out["messages"].size() << ")\n";
        return true;
    } catch (const std::exception& e) {
        std::cerr << "merge_before_after write error: " << e.what() << "\n";
        return false;
    }
}

static void print_alerts_summary(const json& alerts_json) {
    if (!alerts_json.contains("alerts")) { std::cout << "No alerts in JSON\n"; return; }
    std::map<std::string,int> byRisk;
    for (const auto& a : alerts_json["alerts"]) {
        std::string risk = a.contains("risk") ? a["risk"].get<std::string>() : "UNKNOWN";
        byRisk[risk]++;
    }
    std::cout << "Alerts summary:\n";
    for (const auto& kv : byRisk) std::cout << "  " << kv.first << ": " << kv.second << "\n";
}

struct Options { std::string host = "127.0.0.1"; uint16_t port = 8080; std::string apikey; std::string baseurl; std::string action = "all"; std::string outdir = "zap_cli_out"; std::string policy; bool recurse = true; };

static Options parse_args(int argc, char** argv) {
    Options opt;
    for (int i=1;i<argc;++i) {
        std::string a = argv[i];
        if (a == "--host" && i+1<argc) opt.host = argv[++i];
        else if (a == "--port" && i+1<argc) opt.port = static_cast<uint16_t>(std::stoi(argv[++i]));
        else if (a == "--apikey" && i+1<argc) opt.apikey = argv[++i];
        else if (a == "--baseurl" && i+1<argc) opt.baseurl = argv[++i];
        else if (a == "--action" && i+1<argc) opt.action = argv[++i];
        else if (a == "--outdir" && i+1<argc) opt.outdir = argv[++i];
        else if (a == "--policy" && i+1<argc) opt.policy = argv[++i];
        else if (a == "--recurse" && i+1<argc) opt.recurse = (std::string(argv[++i]) != "0");
        else { std::cerr << "Unknown/invalid arg: " << a << "\n"; }
    }
    return opt;
}

int main(int argc, char** argv) {
    Options opt = parse_args(argc, argv);
    if (opt.action != "dumpmsgs" && opt.baseurl.empty()) {
        std::cerr << "--baseurl is required for actions: list, ascan, all\n";
        return 1;
    }

    try {
        fs::create_directories(opt.outdir);
        curl_global_init(CURL_GLOBAL_DEFAULT);

        if (opt.action == "list" || opt.action == "all") {
            json alerts;
            if (!list_alerts(opt.host, opt.port, opt.apikey, opt.baseurl, opt.outdir, alerts)) {
                std::cerr << "list_alerts failed\n";
            } else {
                print_alerts_summary(alerts);

                // >>> ICI (1) : �crire les messages unitaires High + Medium, � la racine outdir
                auto ids = collect_message_ids(alerts, {"High","Medium"});
                for (const auto& id : ids) {
                    save_message_as_file(opt.host, opt.port, opt.apikey, id, opt.outdir);
                }
                // <<< fin insertion (1)
            }
        }

        if (opt.action == "ascan" || opt.action == "all") {
            int scanid = -1;
            if (!start_active_scan(opt.host, opt.port, opt.apikey, opt.baseurl, opt.policy, opt.recurse, scanid)) {
                std::cerr << "Failed to start ascan\n";
            } else {
                poll_ascan_status(opt.host, opt.port, opt.apikey, scanid);

                json alerts_after;
                if (list_alerts(opt.host, opt.port, opt.apikey, opt.baseurl, opt.outdir, alerts_after)) {
                    print_alerts_summary(alerts_after);

                    // >>> ICI (2) : �crire les messages unitaires High + Medium apr�s le scan
                    auto ids_after = collect_message_ids(alerts_after, {"High","Medium"});
                    for (const auto& id : ids_after) {
                        save_message_as_file(opt.host, opt.port, opt.apikey, id, opt.outdir);
                    }
                    // <<< fin insertion (2)
                }

                // NOTE: on ne g�n�re plus de fichiers combin�s ni de ascan_msgs_target_*.json
                // -> ne pas appeler save_messages_by_ids(...) ni save_messages_from_alerts(...)
            }
        }

        if (opt.action == "dumpmsgs") {
            std::cerr << "dumpmsgs not implemented for arbitrary file; use 'list' to generate alerts and messages\n";
        }

        curl_global_cleanup();
        std::cout << "All done. Output directory: " << opt.outdir << "\n";
    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << "\n";
        curl_global_cleanup();
        return 2;
    }

    return 0;
}

