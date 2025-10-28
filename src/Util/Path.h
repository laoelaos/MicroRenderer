#pragma once
#include <string>
#include <filesystem>

namespace PathUtil {
    // 当前工作目录
    std::filesystem::path Cwd();
    // 当前可执行文件的目录
    std::filesystem::path ExeDir();
    // 通过探测常见的基础路径（cwd、exe目录、exe目录的父目录及其祖先）来解析可能是相对的资源路径
    // 如果找到则返回绝对路径，否则返回原始输入路径。
    std::filesystem::path Resolve(const std::filesystem::path& path);
    // 记录简短的环境摘要（cwd/exe目录）
    void LogEnvSummary();
}
