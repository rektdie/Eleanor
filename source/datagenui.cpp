#include <algorithm>
#include <cctype>
#include <clocale>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <string>
#include <vector>

#include "datagenui.h"
#include "datagen.h"
#include "stopwatch.h"

// OS-dependent terminal detection includes
#ifdef _WIN32
    #include <windows.h>
    #include <io.h>
#else
    #include <unistd.h>
    #include <langinfo.h>
    #include <sys/ioctl.h>
#endif

namespace DATAGEN {

namespace {

constexpr int kMinWidth = 64;
constexpr int kMaxWidth = 96;
constexpr std::size_t kMaxSamples = 720;

bool Interactive() {
    static const bool value = [] {
#ifdef _WIN32
        return _isatty(_fileno(stdout)) != 0;
#else
        return isatty(STDOUT_FILENO) != 0;
#endif
    }();
    return value;
}

bool UseColor() {
    static const bool value = [] {
#ifdef _WIN32
        HANDLE out = GetStdHandle(STD_OUTPUT_HANDLE);
        DWORD mode = 0;
        if (out != INVALID_HANDLE_VALUE && GetConsoleMode(out, &mode))
            SetConsoleMode(out, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
        return Interactive() && std::getenv("NO_COLOR") == nullptr;
#else
        return Interactive() && std::getenv("NO_COLOR") == nullptr;
#endif
    }();
    return value;
}

bool UseUnicode() {
    static const bool value = [] {
        std::setlocale(LC_ALL, "");
#ifdef _WIN32
        SetConsoleOutputCP(CP_UTF8);
        return true;
#else
        const char* codeset = nl_langinfo(CODESET);
        return codeset != nullptr
            && (std::strstr(codeset, "UTF-8") != nullptr || std::strstr(codeset, "UTF8") != nullptr);
#endif
    }();
    return value;
}

int DetectWidth() {
#ifdef _WIN32
    CONSOLE_SCREEN_BUFFER_INFO info;
    if (GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &info))
        return info.srWindow.Right - info.srWindow.Left + 1;
#else
    struct winsize ws;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0 && ws.ws_col > 0)
        return ws.ws_col;
#endif
    return 80;
}

std::string Sgr(const char* code) {
    return UseColor() ? "\033[" + std::string(code) + "m" : std::string();
}

std::string Reset() { return Sgr("0"); }

std::string Fg(int r, int g, int b) {
    if (!UseColor()) return "";
    return "\033[38;2;" + std::to_string(r) + ";" + std::to_string(g) + ";" + std::to_string(b) + "m";
}

// Cyan -> spring green across the progress bar.
std::string GradientAt(double t) {
    t = std::clamp(t, 0.0, 1.0);
    return Fg(static_cast<int>(std::lround(0 + t * 80)),
              static_cast<int>(std::lround(180 + t * 75)),
              static_cast<int>(std::lround(255 - t * 115)));
}

struct Glyphs {
    const char* tl;
    const char* tr;
    const char* bl;
    const char* br;
    const char* h;
    const char* v;
    const char* full;
    const char* empty;
    const char* dot;
};

Glyphs PickGlyphs() {
    if (UseUnicode()) {
        return {"\u256D", "\u256E", "\u2570", "\u256F", "\u2500",
                "\u2502", "\u2588", "\u2591", "\u00B7"};
    }
    return {"+", "+", "+", "+", "-", "|", "#", ".", "."};
}

std::vector<std::string> SparkLevels() {
    if (UseUnicode()) {
        return {"\u2581", "\u2582", "\u2583", "\u2584", "\u2585", "\u2586", "\u2587", "\u2588"};
    }
    return {"_", ".", "-", "=", "+", "*", "#", "%", "@"};
}

// Width in terminal cells, ignoring ANSI escapes and counting UTF-8 sequences once.
int VisibleWidth(const std::string& s) {
    int len = 0;
    for (std::size_t i = 0; i < s.size(); ++i) {
        if (s[i] == '\033') {
            std::size_t j = i + 1;
            if (j < s.size() && s[j] == '[') {
                ++j;
                while (j < s.size()
                       && (std::isdigit(static_cast<unsigned char>(s[j])) || s[j] == ';')) {
                    ++j;
                }
                if (j < s.size()) ++j;
                i = j - 1;
                continue;
            }
        }
        if ((static_cast<unsigned char>(s[i]) & 0xC0) != 0x80) ++len;
    }
    return len;
}

std::string PadRight(const std::string& s, int width) {
    const int pad = width - VisibleWidth(s);
    return pad > 0 ? s + std::string(pad, ' ') : s;
}

// Hard guarantee that nothing ever escapes the frame, whatever the contents.
std::string Truncate(const std::string& s, int maxWidth) {
    if (VisibleWidth(s) <= maxWidth) return s;

    std::string out;
    int len = 0;
    bool colored = false;

    for (std::size_t i = 0; i < s.size(); ++i) {
        if (s[i] == '\033') {
            std::size_t j = i + 1;
            if (j < s.size() && s[j] == '[') {
                ++j;
                while (j < s.size()
                       && (std::isdigit(static_cast<unsigned char>(s[j])) || s[j] == ';')) {
                    ++j;
                }
                if (j < s.size()) ++j;
                out.append(s, i, j - i);
                colored = true;
                i = j - 1;
                continue;
            }
        }

        if ((static_cast<unsigned char>(s[i]) & 0xC0) == 0x80) {
            out += s[i];
            continue;
        }

        if (len >= maxWidth) break;
        out += s[i];
        ++len;
    }

    if (colored) out += Reset();
    return out;
}

std::string Rule(const char* left, const char* right, const Glyphs& g, int inner,
                 const std::string& color) {
    std::string s = color + left;
    for (int i = 0; i < inner; ++i) s += g.h;
    s += right;
    s += Reset();
    return s;
}

std::string Row(const std::string& content, const Glyphs& g, int inner, const std::string& border) {
    return border + g.v + Reset() + PadRight(Truncate(content, inner), inner) + border + g.v + Reset();
}

std::string SplitRow(const std::string& left, const std::string& right, const Glyphs& g, int inner,
                     const std::string& border) {
    const int gap = inner - VisibleWidth(left) - VisibleWidth(right);
    return Row(left + std::string(gap > 0 ? gap : 1, ' ') + right, g, inner, border);
}

std::string DotLeaders(const std::string& key, const std::string& value, int width,
                       const std::string& keyColor, const std::string& valueColor,
                       const Glyphs& g) {
    const int dots = width - VisibleWidth(key) - VisibleWidth(value) - 2;
    std::string s = keyColor + key + Reset();

    if (dots > 0) {
        s += ' ';
        for (int i = 0; i < dots; ++i) s += g.dot;
    }

    return s + ' ' + valueColor + value + Reset();
}

std::string FormatCount(long long value) {
    char buf[64];
    const double d = static_cast<double>(value);
    const double a = std::abs(d);

    if (a >= 1e9) std::snprintf(buf, sizeof(buf), "%.2fB", d / 1e9);
    else if (a >= 1e6) std::snprintf(buf, sizeof(buf), "%.2fM", d / 1e6);
    else if (a >= 1e3) std::snprintf(buf, sizeof(buf), "%.1fK", d / 1e3);
    else std::snprintf(buf, sizeof(buf), "%lld", value);

    return buf;
}

std::string FormatThousands(long long value) {
    std::string digits = std::to_string(value);
    for (int i = static_cast<int>(digits.size()) - 3; i > 0; i -= 3)
        digits.insert(i, ",");
    return digits;
}

std::string FormatDuration(double seconds) {
    if (!std::isfinite(seconds) || seconds < 0) return "--";

    const long long total = static_cast<long long>(seconds);
    const long long h = total / 3600;
    const long long m = (total % 3600) / 60;
    const long long s = total % 60;

    char buf[64];
    if (h > 0) std::snprintf(buf, sizeof(buf), "%lldh %02lldm", h, m);
    else if (m > 0) std::snprintf(buf, sizeof(buf), "%lldm %02llds", m, s);
    else std::snprintf(buf, sizeof(buf), "%llds", s);

    return buf;
}

std::string BuildSparkline(const std::deque<double>& samples, int width,
                           const std::vector<std::string>& levels, const std::string& color) {
    if (width <= 0 || samples.empty()) return "";

    const int count = static_cast<int>(samples.size());
    const int take = std::min(count, width);
    const int start = count - take;

    double lo = samples[start];
    double hi = samples[start];
    for (int i = start; i < count; ++i) {
        lo = std::min(lo, samples[i]);
        hi = std::max(hi, samples[i]);
    }

    const double range = hi - lo;
    const int last = static_cast<int>(levels.size()) - 1;

    std::string bar;
    // Pad the left side with blanks so "no data yet" is not confused with a low sample.
    for (int i = 0; i < width - take; ++i) bar += ' ';

    for (int i = start; i < count; ++i) {
        const int idx = range > 1e-9
            ? static_cast<int>(std::lround((samples[i] - lo) / range * last))
            : last;
        bar += levels[std::clamp(idx, 0, last)];
    }

    return color + bar + Reset();
}

struct RateTracker {
    std::deque<double> samples;
    double lastTime = 0.0;
    double lastCount = 0.0;
    double current = 0.0;
    double smoothed = 0.0;
    double peak = 0.0;

    void sample(double count, double elapsed) {
        if (elapsed - lastTime < 1.0) return;

        const double window = elapsed - lastTime;
        const double instant = window > 0 ? (count - lastCount) / window : 0.0;

        lastTime = elapsed;
        lastCount = count;

        samples.push_back(instant);
        if (samples.size() > kMaxSamples) samples.pop_front();

        current = instant;
        smoothed = smoothed == 0.0 ? instant : smoothed * 0.65 + instant * 0.35;
        peak = std::max(peak, instant);
    }

    void reset() {
        samples.clear();
        lastTime = lastCount = current = smoothed = peak = 0.0;
    }
};

RateTracker& PositionsRate() {
    static RateTracker tracker;
    return tracker;
}

RateTracker& NodesRate() {
    static RateTracker tracker;
    return tracker;
}

// Shared header row: "ELEANOR - DATAGEN" on the left, a status pill on the right.
std::string HeaderRow(const Glyphs& g, int inner, const std::string& border,
                      const std::string& status, const std::string& statusColor) {
    const std::string bullet = UseUnicode() ? "\u25CF" : "*";
    const std::string left = Sgr("1") + Fg(190, 220, 255) + "ELEANOR"
        + Fg(90, 100, 120) + " " + g.dot + " " + Sgr("0") + Fg(140, 150, 170) + "DATAGEN";
    const std::string pill = statusColor + bullet + " " + Sgr("1") + status + Reset();
    return SplitRow(" " + left, pill + " ", g, inner, border);
}

struct Snapshot {
    long long positions = 0;
    long long games = 0;
    long long files = 0;
    long long nodes = 0;
    long long rejected = 0;
    double elapsed = 0;
    double posSpeed = 0;
    double posPeak = 0;
    double nodeSpeed = 0;
};

Snapshot TakeSnapshot(const DatagenStats& stats, Stopwatch& stopwatch) {
    Snapshot snap;
    snap.positions = stats.positions.load(std::memory_order_relaxed);
    snap.games = stats.games.load(std::memory_order_relaxed);
    snap.files = stats.files.load(std::memory_order_relaxed);
    snap.nodes = stats.nodes.load(std::memory_order_relaxed);
    snap.rejected = stats.rejected.load(std::memory_order_relaxed);
    snap.elapsed = stopwatch.GetElapsedSec();

    PositionsRate().sample(static_cast<double>(snap.positions), snap.elapsed);
    NodesRate().sample(static_cast<double>(snap.nodes), snap.elapsed);

    snap.posSpeed = PositionsRate().smoothed;
    snap.posPeak = PositionsRate().peak;
    snap.nodeSpeed = NodesRate().smoothed;
    return snap;
}

std::string FormatSpeed(double perSecond) {
    char buf[64];
    if (perSecond >= 1e9) std::snprintf(buf, sizeof(buf), "%.2fB/s", perSecond / 1e9);
    else if (perSecond >= 1e6) std::snprintf(buf, sizeof(buf), "%.2fM/s", perSecond / 1e6);
    else if (perSecond >= 1e3) std::snprintf(buf, sizeof(buf), "%.1fK/s", perSecond / 1e3);
    else std::snprintf(buf, sizeof(buf), "%.0f/s", perSecond);
    return buf;
}

void PrintPlainLine(const Snapshot& snap, int targetPositions, int threads) {
    const double progress = targetPositions > 0
        ? 100.0 * snap.positions / targetPositions : 0.0;
    const double remaining = snap.posSpeed > 0
        ? (targetPositions - snap.positions) / snap.posSpeed : 0.0;

    std::printf("[datagen] %lld/%d (%.1f%%)  %s  nodes %s  elapsed %s  eta %s  games %lld  files %lld  threads %d\n",
                snap.positions, targetPositions, progress,
                FormatSpeed(snap.posSpeed).c_str(), FormatCount(snap.nodes).c_str(),
                FormatDuration(snap.elapsed).c_str(), FormatDuration(remaining).c_str(),
                snap.games, snap.files, threads);
    std::fflush(stdout);
}

std::vector<std::string> BuildFrame(const Snapshot& snap, int targetPositions, int threads,
                                    bool finished) {
    const Glyphs g = PickGlyphs();
    const auto levels = SparkLevels();

    const int width = std::clamp(DetectWidth(), kMinWidth, kMaxWidth);
    const int inner = width - 2;

    const std::string accent = Fg(120, 190, 255);
    const std::string border = Fg(70, 80, 100);
    const std::string label = Fg(130, 140, 160);
    const std::string value = Fg(235, 240, 250);
    const std::string heading = Sgr("1") + Fg(150, 200, 255);
    const std::string good = Fg(120, 235, 160);
    const std::string warn = Fg(245, 200, 110);

    const double progress = targetPositions > 0
        ? std::clamp(static_cast<double>(snap.positions) / targetPositions, 0.0, 1.0)
        : 0.0;

    const double remaining = snap.posSpeed > 0
        ? std::max(0.0, (targetPositions - snap.positions) / snap.posSpeed) : 0.0;

    const double avgGame = snap.games > 0
        ? static_cast<double>(snap.positions) / snap.games : 0.0;

    const long long attempted = snap.games + snap.rejected;
    const double acceptRate = attempted > 0
        ? 100.0 * snap.games / attempted : 0.0;

    std::vector<std::string> out;
    out.push_back(Rule(g.tl, g.tr, g, inner, border));

    out.push_back(HeaderRow(g, inner, border, finished ? "STOPPED" : "GENERATING",
                            finished ? warn : good));

    out.push_back(Rule(g.v, g.v, g, inner, border));

    // Headline counters.
    {
        char pct[32];
        std::snprintf(pct, sizeof(pct), "%.1f%%", progress * 100.0);

        const std::string left = "  " + Sgr("1") + Fg(225, 235, 250) + FormatCount(snap.positions)
            + Fg(110, 120, 140) + " / " + FormatCount(targetPositions)
            + Fg(110, 120, 140) + "  positions";

        const std::string right = Sgr("1") + GradientAt(progress) + pct + Reset();

        out.push_back(SplitRow(left, right + " ", g, inner, border));

        const int barWidth = inner - 4;
        const int filled = static_cast<int>(std::lround(progress * barWidth));

        std::string bar;
        for (int i = 0; i < barWidth; ++i) {
            if (i < filled) bar += GradientAt(barWidth > 1 ? static_cast<double>(i) / (barWidth - 1) : 0.0) + g.full;
            else bar += Fg(45, 50, 62) + g.empty;
        }
        bar += Reset();

        out.push_back(Row("  " + bar, g, inner, border));
    }

    out.push_back(Row("", g, inner, border));
    out.push_back(Rule(g.v, g.v, g, inner, border));

    // Two-column statistics.
    const int half = (inner - 3) / 2;
    const int rightStart = half + 3;
    const int rightWidth = inner - rightStart - 2;

    auto statRow = [&](const char* lk, const std::string& lv, const char* rk, const std::string& rv) {
        const std::string l = DotLeaders(lk, lv, half, label, value, g);
        const std::string r = DotLeaders(rk, rv, rightWidth, label, value, g);
        out.push_back(Row("  " + l + "   " + r, g, inner, border));
    };

    out.push_back(Row("  " + heading + "THROUGHPUT" + Reset() + "      "
                      + heading + "SESSION" + Reset(), g, inner, border));
    out.push_back(Row("", g, inner, border));

    statRow("speed", FormatSpeed(snap.posSpeed), "elapsed", FormatDuration(snap.elapsed));
    statRow("peak", FormatSpeed(snap.posPeak), "remaining", FormatDuration(remaining));
    statRow("nodes", FormatCount(snap.nodes), "threads", std::to_string(threads));
    statRow("node speed", FormatSpeed(snap.nodeSpeed), "progress",
            [&] { char b[32]; std::snprintf(b, sizeof(b), "%.1f%%", progress * 100.0); return std::string(b); }());

    out.push_back(Row("", g, inner, border));
    statRow("games", FormatThousands(snap.games), "files", FormatThousands(snap.files));
    statRow("rejected", FormatThousands(snap.rejected), "avg game",
            [&] { char b[32]; std::snprintf(b, sizeof(b), "%.1f pos", avgGame); return std::string(b); }());
    statRow("accept", [&] { char b[32]; std::snprintf(b, sizeof(b), "%.1f%%", acceptRate); return std::string(b); }(),
            "nodes/pos",
            [&] { char b[32]; std::snprintf(b, sizeof(b), "%.0f",
                snap.positions > 0 ? static_cast<double>(snap.nodes) / snap.positions : 0.0); return std::string(b); }());

    // Throughput history.
    const int sparkWidth = inner - 4;
    out.push_back(Row("", g, inner, border));
    out.push_back(Row("  " + label + "throughput history" + Reset(), g, inner, border));
    out.push_back(Row("  " + BuildSparkline(PositionsRate().samples, sparkWidth, levels, accent),
                      g, inner, border));

    out.push_back(Rule(g.v, g.v, g, inner, border));

    out.push_back(Row("  " + label
                      + (finished ? "run finished " + std::string(g.dot) + " data written to data/*.binpack"
                                  : "Ctrl+C to stop safely " + std::string(g.dot) + " data is flushed on exit")
                      + Reset(),
                      g, inner, border));

    out.push_back(Rule(g.bl, g.br, g, inner, border));
    return out;
}

void DrawFrame(const std::vector<std::string>& frame) {
    std::string buffer;
    buffer.reserve(frame.size() * 96);
    buffer += "\033[H";

    for (const std::string& line : frame) {
        buffer += line;
        buffer += '\n';
    }

    buffer += "\033[J";
    std::fwrite(buffer.data(), 1, buffer.size(), stdout);
    std::fflush(stdout);
}

void ClearFrame() {
    if (!Interactive()) return;
    std::fputs("\033[H\033[J", stdout);
    std::fflush(stdout);
}

} // namespace

void PrintProgress(const DatagenStats& stats, int targetPositions, Stopwatch &stopwatch, int threads) {
    const Snapshot snap = TakeSnapshot(stats, stopwatch);

    if (!Interactive()) {
        PrintPlainLine(snap, targetPositions, threads);
        return;
    }

    DrawFrame(BuildFrame(snap, targetPositions, threads, false));
}

void PrintSummary(const DatagenStats& stats, int targetPositions, Stopwatch &stopwatch, int threads) {
    const Snapshot snap = TakeSnapshot(stats, stopwatch);

    if (!Interactive()) {
        PrintPlainLine(snap, targetPositions, threads);
        return;
    }

    ClearFrame();
    DrawFrame(BuildFrame(snap, targetPositions, threads, true));
}

void PrintUsage() {
    const Glyphs g = PickGlyphs();

    // Help text is denser than the dashboard, so allow a slightly wider frame.
    const int width = std::clamp(DetectWidth(), kMinWidth, 100);
    const int inner = width - 2;

    const std::string accent = Fg(120, 190, 255);
    const std::string border = Fg(70, 80, 100);
    const std::string label = Fg(130, 140, 160);
    const std::string value = Fg(235, 240, 250);
    const std::string heading = Sgr("1") + Fg(150, 200, 255);

    std::vector<std::string> out;
    out.push_back(Rule(g.tl, g.tr, g, inner, border));
    out.push_back(HeaderRow(g, inner, border, "HELP", accent));
    out.push_back(Rule(g.v, g.v, g, inner, border));

    auto blank = [&]() { out.push_back(Row("", g, inner, border)); };
    auto section = [&](const char* name) {
        blank();
        out.push_back(Row("  " + heading + name + Reset(), g, inner, border));
    };
    auto line = [&](const std::string& content) {
        out.push_back(Row("    " + content, g, inner, border));
    };
    // A two-column row: fixed-width term on the left, description on the right.
    auto term = [&](const std::string& name, const std::string& desc, int nameWidth = 11) {
        out.push_back(Row("    " + Sgr("1") + accent + PadRight(name + Reset(), nameWidth) + desc,
                          g, inner, border));
    };
    auto note = [&](const std::string& text) {
        line(label + g.dot + " " + text + Reset());
    };
    auto usage = [&](const std::string& args, const std::string& comment) {
        const std::string padded = PadRight(value + args + Reset(), 28);
        line(comment.empty() ? padded : padded + label + comment + Reset());
    };

    section("SYNTAX");
    usage("Eleanor datagen [positions] [threads] [username]", "");
    usage("Eleanor datagen --help", "");

    section("ARGUMENTS");
    term("positions", "Target size in thousands, multiplied by 1000.", 12);
    term("", label + "Default 1 = 1,000 positions.");
    term("threads", "Number of worker threads.", 12);
    term("", label + "No default, minimum 2.");
    term("username", "Enables online mode: registers a session", 12);
    term("", "on the coordination server and uploads the data.");

    section("EXAMPLES");
    usage("Eleanor datagen 1000 22", "1,000,000 positions on 22 threads");
    usage("Eleanor datagen 10 8", "10,000 positions on 8 threads");
    usage("Eleanor datagen 1000 22 me", "1,000,000 positions, online mode");

    section("NOTES");
    note("Data is written to data/datagen_<timestamp>.binpack");
    note("Press Ctrl+C to stop safely; partial data is flushed and merged.");
    note("Each game starts from the initial position after 8-9 random plies.");
    note("Openings scored beyond +/-200cp are discarded for evenness.");
    note("A threads value of N starts N-1 generators, minimum 2.");

    blank();
    out.push_back(Rule(g.bl, g.br, g, inner, border));

    for (const std::string& l : out) {
        std::fputs((l + "\n").c_str(), stdout);
    }
    std::fflush(stdout);
}

} // namespace DATAGEN
