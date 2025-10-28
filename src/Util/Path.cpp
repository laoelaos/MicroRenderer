#include "Path.h"
#include "Logger.h"
#include <vector>
#if defined(_WIN32)
#  include <windows.h>
#endif

namespace PathUtil {
    std::filesystem::path Cwd() {
        try { return std::filesystem::current_path(); }
        catch (...) { return {}; }
    }

    std::filesystem::path ExeDir() {
    #if defined(_WIN32)
        wchar_t buf[MAX_PATH];
        DWORD len = GetModuleFileNameW(nullptr, buf, MAX_PATH);
        if (len > 0) {
            std::filesystem::path p(buf);
            return p.parent_path();
        }
        return {};
    #else
        std::error_code ec;
        auto p = std::filesystem::read_symlink("/proc/self/exe", ec);
        if (!ec && !p.empty()) return p.parent_path();
        return {};
    #endif
    }

    static std::filesystem::path TryResolve(const std::filesystem::path& base, const std::filesystem::path& rel) {
        if (base.empty()) return {};
        auto p = base / rel;
        if (std::filesystem::exists(p)) return std::filesystem::absolute(p);
        return {};
    }

    std::filesystem::path Resolve(const std::filesystem::path& path) {
        if (path.empty()) return path;
        if (path.is_absolute() && std::filesystem::exists(path)) return path;

        // Try candidates
        std::vector<std::filesystem::path> bases;
        bases.push_back(Cwd());
        auto exe = ExeDir();
        if (!exe.empty()) {
            bases.push_back(exe);
            bases.push_back(exe.parent_path());
            bases.push_back(exe.parent_path().parent_path());
        }

        for (const auto& b : bases) {
            auto r = TryResolve(b, path);
            if (!r.empty()) return r;
        }
        return path; // fallback (will likely fail at open); caller will log both
    }

    void LogEnvSummary() {
        auto cwd = Cwd();
        auto exe = ExeDir();
        LOGI("PathUtil::LogEnvSummary: cwd='{}' exeDir='{}'", cwd.string(), exe.string());
    }
}

