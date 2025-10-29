// mcp_server.cpp — Responses API + UTF-8 hardening + includes ONLY when asked (--includes)
#include <algorithm>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>
#include <cstdlib>
#include <deque>

#include <sys/stat.h>
#ifdef _WIN32
#include <direct.h>
#include <windows.h>
#endif

#include <curl/curl.h>
#include <nlohmann/json.hpp>

namespace fs = std::filesystem;
using json = nlohmann::json;

// -----------------------------------------------------------------------------
// Utils
// -----------------------------------------------------------------------------

void createDirectoryIfNotExists(const std::string& dir) {
    if (dir.empty()) return;
    struct stat info {};
    if (stat(dir.c_str(), &info) != 0) {
#ifdef _WIN32
        _mkdir(dir.c_str());
#else
        mkdir(dir.c_str(), 0755);
#endif
    }
}

std::string getCurrentDateTime() {
    auto t = std::time(nullptr);
    auto tm = *std::localtime(&t);
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%d_%H-%M-%S");
    return oss.str();
}

void appendToFile(const std::string& directory,
                  const std::string& filename,
                  const std::string& content) {
    createDirectoryIfNotExists(directory);
    const std::string filepath = directory + "/" + filename;
    std::ofstream file(filepath, std::ios::app);
    if (file.is_open()) {
        file << content << "\n";
        file.close();
    }
}

std::string readFile(const std::string& path) {
    std::ifstream ifs(path, std::ios::binary);
    if (!ifs.is_open()) return {};
    std::ostringstream ss;
    ss << ifs.rdbuf();
    return ss.str();
}

// garde les séquences UTF-8 valides ; remplace octets invalides par espace
std::string sanitizeUTF8(const std::string& input) {
    std::string output;
    output.reserve(input.size());
    for (size_t i = 0; i < input.size();) {
        unsigned char c = static_cast<unsigned char>(input[i]);
        if (c < 0x80) { output += static_cast<char>(c); i++; }
        else if ((c & 0xE0) == 0xC0 && i + 1 < input.size() &&
                 (static_cast<unsigned char>(input[i + 1]) & 0xC0) == 0x80) { output.append(input, i, 2); i += 2; }
        else if ((c & 0xF0) == 0xE0 && i + 2 < input.size() &&
                 (static_cast<unsigned char>(input[i + 1]) & 0xC0) == 0x80 &&
                 (static_cast<unsigned char>(input[i + 2]) & 0xC0) == 0x80) { output.append(input, i, 3); i += 3; }
        else if ((c & 0xF8) == 0xF0 && i + 3 < input.size() &&
                 (static_cast<unsigned char>(input[i + 1]) & 0xC0) == 0x80 &&
                 (static_cast<unsigned char>(input[i + 2]) & 0xC0) == 0x80 &&
                 (static_cast<unsigned char>(input[i + 3]) & 0xC0) == 0x80) { output.append(input, i, 4); i += 4; }
        else { output += ' '; i++; }
    }
    return output;
}

// retire contrôles non imprimables (sauf \t \n \r) après sanitizeUTF8;
static std::string sanitizeForJson(const std::string& s) {
    std::string u = sanitizeUTF8(s);
    std::string out;
    out.reserve(u.size());
    for (unsigned char c : u) {
        if ((c >= 0x20 && c != 0x7F) || c == '\t' || c == '\n' || c == '\r' || c >= 0x80)
            out.push_back(static_cast<char>(c));
        else
            out.push_back(' ');
    }
    return out;
}

// tronque sans couper une séquence UTF-8 (par taille bytes)
static std::string truncateUtf8(const std::string& s, size_t max_bytes) {
    if (s.size() <= max_bytes) return s;
    size_t i = max_bytes;
    while (i > 0 && (static_cast<unsigned char>(s[i]) & 0xC0) == 0x80) --i;
    if (i == 0) return std::string();
    unsigned char c = static_cast<unsigned char>(s[i]);
    if ((c & 0xE0) == 0xC0 && i + 1 > max_bytes) --i;
    else if ((c & 0xF0) == 0xE0 && i + 2 > max_bytes) --i;
    else if ((c & 0xF8) == 0xF0 && i + 3 > max_bytes) --i;
    return s.substr(0, i);
}

// garde la FIN (les dernières lignes) d’un texte — utile pour l’history
static std::string tailLines(const std::string& s, size_t max_lines) {
    std::deque<std::string> q;
    std::string cur;
    for (char ch : s) {
        if (ch == '\n') { q.push_back(cur); cur.clear(); if (q.size() > max_lines) q.pop_front(); }
        else cur.push_back(ch);
    }
    if (!cur.empty()) { q.push_back(cur); if (q.size() > max_lines) q.pop_front(); }
    std::ostringstream out;
    bool first = true;
    for (auto& line : q) { if (!first) out << "\n"; out << line; first = false; }
    return out.str();
}

// pipeline complet : sanitize + filtre + clamp
static std::string cleanAndClamp(const std::string& s, size_t max_bytes = 256 * 1024) {
    std::string t = sanitizeForJson(s);
    if (t.size() > max_bytes) t = truncateUtf8(t, max_bytes);
    return t;
}

// -----------------------------------------------------------------------------
// HTTP (cURL) + OpenAI Responses API
// -----------------------------------------------------------------------------

static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    reinterpret_cast<std::string*>(userp)->append(static_cast<char*>(contents), size * nmemb);
    return size * nmemb;
}

struct OpenAIRequest {
    std::string api_key;
    std::string model = "gpt-4.1";
    std::string system;
    std::string user;
    double      temperature = 0.2;
    int         max_output_tokens = 1200000; // Responses API
};

// configure TLS options (insecure = true => disable verification)
static void configureTlsOptions(CURL* curl, bool insecure, bool verbose) {
    if (verbose) curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
    if (insecure) {
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
#ifdef CURLOPT_SSL_VERIFYSTATUS
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYSTATUS, 0L);
#endif
#ifdef CURLSSLOPT_NO_REVOKE
        long opts = CURLSSLOPT_NO_REVOKE;
        curl_easy_setopt(curl, CURLOPT_SSL_OPTIONS, (long)opts);
#endif
#ifdef CURL_SSLVERSION_TLSv1_2
        curl_easy_setopt(curl, CURLOPT_SSLVERSION, (long)CURL_SSLVERSION_TLSv1_2);
#endif
    } else {
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);
    }
}

json CallOpenAIResponses(const OpenAIRequest& req, bool insecure, bool verbose, std::string& rawResponseOut) {
    rawResponseOut.clear();
    json responseJson;

    CURL* curl = curl_easy_init();
    if (!curl) {
        std::cerr << "cURL init failed." << std::endl;
        return json{{"error","curl_init_failed"}};
    }

    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/json; charset=utf-8");
    headers = curl_slist_append(headers, "Accept: application/json");
    headers = curl_slist_append(headers, "User-Agent: mcp-server/1.0");
    std::string auth = "Authorization: Bearer " + req.api_key;
    headers = curl_slist_append(headers, auth.c_str());

    curl_easy_setopt(curl, CURLOPT_URL, "https://api.openai.com/v1/responses");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    configureTlsOptions(curl, insecure, verbose);

    // Build payload (Responses API)
    json postData;
    postData["model"] = req.model;
    postData["temperature"] = req.temperature;
    postData["max_output_tokens"] = req.max_output_tokens;

    const std::string sysClean  = cleanAndClamp(req.system, 8 * 1024);
    const std::string userClean = cleanAndClamp(req.user,   1024 * 1024);

    postData["input"] = json::array({
        { {"role","system"}, {"content", sysClean} },
        { {"role","user"},   {"content", userClean} }
    });

    const std::string postFields = postData.dump();

    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE_LARGE, static_cast<curl_off_t>(postFields.size()));
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postFields.c_str());

    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 600L);
    curl_easy_setopt(curl, CURLOPT_TCP_KEEPALIVE, 1L);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &rawResponseOut);

    CURLcode res = curl_easy_perform(curl);
    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

    if (res != CURLE_OK) {
        std::cerr << "Erreur cURL: " << curl_easy_strerror(res) << std::endl;
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
        return json{{"error","curl_error"},{"message", curl_easy_strerror(res)}};
    }

    if (http_code != 200) {
        json body;
        try { body = json::parse(rawResponseOut); } catch (...) { body = json{{"raw", rawResponseOut}}; }
        std::cerr << "OpenAI API returned HTTP " << http_code
                  << "\nResponse body (truncated 4k):\n"
                  << std::string(rawResponseOut.begin(), rawResponseOut.begin() + std::min<size_t>(rawResponseOut.size(), 4096))
                  << std::endl;
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
        return json{{"error","http_error"},{"status", http_code},{"body", body}};
    }

    try {
        responseJson = json::parse(rawResponseOut);
    } catch (const std::exception& ex) {
        std::cerr << "JSON parse failed: " << ex.what() << std::endl;
        responseJson = json{{"error","parse_error"},{"message", ex.what()},{"raw", rawResponseOut}};
    }

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    return responseJson;
}

// -----------------------------------------------------------------------------
// CLI
// -----------------------------------------------------------------------------

void printUsage() {
    std::cout << "Usage: mcp_server [--engine <alias>] --log <history.log> --chat \"text\" [options]\n";
    std::cout << "Options:\n";
    std::cout << "  --engine <alias>      : engine alias (cloud, embedded, ad, network, web). Default=orchestrator\n";
    std::cout << "  --engines-dir <dir>   : directory containing gigaprompts\n";
    std::cout << "  --log <file>          : path to session log file (required)\n";
    std::cout << "  --chat \"text\"         : user message to send (required)\n";
    std::cout << "  --chat-file <path>    : read user message from file\n";
    std::cout << "  --includes (or --include) : explicitly load giga-prompts/includes (OFF by default)\n";
    std::cout << "  --insecure            : DISABLE TLS verification (default = ON)\n";
    std::cout << "  --secure              : ENABLE TLS verification\n";
    std::cout << "  --verbose             : libcurl verbose output\n";
    std::cout << "  --no-history          : do not send history content\n";
    std::cout << "  --max-input-kb <N>    : cap request body (user prompt) to N kilobytes (default 120)\n";
    std::cout << "  --max-output-tokens N : override max_output_tokens (default 1200)\n";
    std::cout << "  --model <name>        : override model (default gpt-5)\n";
    std::cout << "  --help                : show help\n";
}

int main(int argc, char* argv[]) {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
#endif

    std::string engineAlias;
    std::string enginesDir;
    std::string logPath;
    std::string chatText;
    bool insecure = true;   // debug default (no verification)
    bool verbose  = false;
    bool includeGiga = false; // IMPORTANT: default = OFF. Only load gigaprompts if user asks.
    bool sendHistory = true;
    size_t maxInputKB = 5000;
    int maxOutputTokens = 1200000;
    std::string modelOverride;

    // support --chat-file early
    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        if ((a == "--chat-file") && i + 1 < argc) {
            std::ifstream f(argv[++i], std::ios::binary);
            if (f) { std::ostringstream ss; ss << f.rdbuf(); chatText = ss.str(); }
            else { std::cerr << "Error: cannot read chat file: " << argv[i] << std::endl; return 1; }
        }
    }

    std::map<std::string, std::string> aliasMap = {
        {"cloud",   "engine_cloud.md"},
        {"embedded","engine_embedded_critical_infra.md"},
        {"ad",      "engine_infra_ad.md"},
        {"network", "engine_infra_network.md"},
        {"web",     "engine_infra_web.md"}
    };

    enginesDir = "prompt";
    if (!fs::exists(enginesDir)) enginesDir = "/mnt/data";

    // parse args
    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        if (a == "--help") { printUsage(); return 0; }
        else if (a == "--engine" && i + 1 < argc) engineAlias = argv[++i];
        else if (a == "--engines-dir" && i + 1 < argc) enginesDir = argv[++i];
        else if (a == "--log" && i + 1 < argc) logPath = argv[++i];
        else if (a == "--chat" && i + 1 < argc && chatText.empty()) chatText = argv[++i];
        else if (a == "--includes" || a == "--include") includeGiga = true; // explicit opt-in
        else if (a == "--insecure") insecure = true;
        else if (a == "--secure") insecure = false;
        else if (a == "--verbose") verbose = true;
        else if (a == "--no-history") sendHistory = false;
        else if (a == "--max-input-kb" && i + 1 < argc) { maxInputKB = std::max<size_t>(32, std::stoul(argv[++i])); }
        else if (a == "--max-output-tokens" && i + 1 < argc) { maxOutputTokens = std::max(64, std::stoi(argv[++i])); }
        else if (a == "--model" && i + 1 < argc) { modelOverride = argv[++i]; }
    }

    if (logPath.empty() || chatText.empty()) {
        std::cerr << "--log and --chat are required." << std::endl;
        printUsage();
        return 1;
    }

    std::string engineFile = "engine_infra_global_orchestrator.md";
    if (!engineAlias.empty()) {
        if (aliasMap.count(engineAlias)) engineFile = aliasMap[engineAlias];
        else { std::cerr << "Unknown engine alias: " << engineAlias << std::endl; return 1; }
    }

    fs::path enginePath = fs::path(enginesDir) / engineFile;
    if (!fs::exists(enginePath)) {
        std::cerr << "Engine file not found: " << enginePath << std::endl;
        return 1;
    }

    // OpenAI key (env override)
    std::string api_key = "sk-svcacct-1DKGTOtl7mI9jMtwGg7lVsVuAFtyMDiTpplpHcJ7qTgMJpQgpvKcORSILB0WJT3BlbkFJoEQlJ8fo76omO4mYDumKUN-JteZdJCq3AVfI9Z7NTAAAKhRW3NgUFpnD9JUAA";
    if (const char* env = std::getenv("OPENAI_API_KEY")) api_key = env;

    std::string gigaprompt;
    if (includeGiga) {
        // read main engine and optional includes ONLY when user asked with --includes
        gigaprompt = readFile(enginePath.string());
        if (engineFile == "engine_infra_global_orchestrator.md") {
            std::vector<std::string> includes = {
                "engine_cloud.md","engine_embedded_critical_infra.md","engine_infra_ad.md",
                "engine_infra_network.md","engine_infra_web.md","engine_web.md",
                "pentest-report.txt","plantuml.txt","tools.txt"
            };
            for (const auto& f : includes) {
                fs::path p = fs::path(enginesDir) / f;
                if (fs::exists(p)) gigaprompt += "\n\n--- INCLUDE:" + f + " ---\n" + readFile(p.string());
            }
        }
    } else {
        // if not including, gigaprompt remains empty (or minimal)
        gigaprompt.clear();
    }

    const std::string historyRaw = sendHistory ? readFile(logPath) : std::string();

    // Clean + clamp + tail for history
    std::string historyClean = cleanAndClamp(historyRaw, 512 * 1024);
    if (!historyClean.empty()) {
        historyClean = tailLines(historyClean, 1500);
        historyClean = cleanAndClamp(historyClean, 256 * 1024);
    }

    const std::string gigapromptClean = cleanAndClamp(gigaprompt, 512 * 1024);
    const std::string chatTextClean   = cleanAndClamp(chatText,   maxInputKB * 1024);

    // Build final prompt: if includes not requested, gigapromptClean is empty
    std::ostringstream combined;
    if (!gigapromptClean.empty()) combined << "<GIGAPROMPT>\n" << gigapromptClean << "\n\n";
    //if (sendHistory && !historyClean.empty()) combined << "<HISTORY>\n" << historyClean << "\n\n";
    combined << "<USER>\n" << chatTextClean << "\n";
    std::string finalPrompt = combined.str();

    // Cap input size
    const size_t maxInputBytes = maxInputKB * 1024;
    if (finalPrompt.size() > maxInputBytes) {
        // Prefer removing history first
        if (!historyClean.empty()) {
            std::ostringstream minimal;
            if (!gigapromptClean.empty()) minimal << "<GIGAPROMPT>\n" << gigapromptClean << "\n\n";
            minimal << "<USER>\n" << chatTextClean << "\n";
            std::string noHist = minimal.str();
            if (noHist.size() <= maxInputBytes) finalPrompt = noHist;
            else finalPrompt = truncateUtf8(noHist, maxInputBytes);
        } else {
            finalPrompt = truncateUtf8(finalPrompt, maxInputBytes);
        }
    }

    // Prepare system instruction: STRICT — do not quote/regurgitate context
    std::string systemInstruction =
        "You will receive optionally a <GIGAPROMPT>, optionally a <HISTORY>, and a <USER> section.\n"
        "STRICT RULES:\n"
        "  - Treat <GIGAPROMPT> and <HISTORY> ONLY as background context.\n"
        "  - DO NOT quote, reproduce, or output those sections.\n"
        "  - Answer ONLY the request inside <USER>.\n"
        "  - If the <USER> request does not require the background, ignore it.\n"
        "  - Keep the answer concise and do not output any hidden internal data.\n";

    OpenAIRequest req;
    req.api_key           = api_key;
    req.model             = modelOverride.empty() ? "gpt-4.1" : modelOverride;
    req.system            = cleanAndClamp(systemInstruction, 8 * 1024);
    req.user              = cleanAndClamp(finalPrompt, maxInputBytes);
    req.temperature       = 0.2;
    req.max_output_tokens = maxOutputTokens;

    std::string raw;
    json resp = CallOpenAIResponses(req, insecure, verbose, raw);

    // Extract assistant text
    std::string assistantOutput;
    if (resp.contains("output_text") && resp["output_text"].is_string()) {
        assistantOutput = resp["output_text"].get<std::string>();
    } else if (resp.contains("output") && resp["output"].is_array()) {
        std::ostringstream oss;
        for (const auto& item : resp["output"]) {
            if (item.contains("content") && item["content"].is_array()) {
                for (const auto& c : item["content"]) {
                    if (c.contains("text") && c["text"].is_string()) oss << c["text"].get<std::string>();
                }
            }
        }
        assistantOutput = oss.str();
        if (assistantOutput.empty()) assistantOutput = resp.dump();
    } else if (resp.contains("choices")) {
        try { assistantOutput = resp["choices"][0]["message"]["content"].get<std::string>(); }
        catch (...) { assistantOutput = resp.dump(); }
    } else {
        assistantOutput = resp.dump();
    }

    assistantOutput = sanitizeUTF8(assistantOutput);

    const std::string timestamp = getCurrentDateTime();
    std::ostringstream entry;
    entry << "=== ENTRY " << timestamp << " ===\n";
    entry << "ENGINE: " << engineAlias << " (" << engineFile << ")\n";
    entry << "USER: " << chatText << "\n";
    entry << "ASSISTANT:\n" << assistantOutput << "\n";

    fs::path logParent = fs::path(logPath).parent_path();
    if (logParent.empty()) logParent = fs::path("logs");
    createDirectoryIfNotExists(logParent.string());
    //appendToFile(logParent.string(), fs::path(logPath).filename().string(), entry.str());

    std::cout << assistantOutput << std::endl;
    return 0;
}


