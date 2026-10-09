#pragma once

#include <string>
#include <vector>
#include <memory>
#include <mutex>
#include <deque>
#include <utility>

namespace sp::rt::obs {

enum class LogLevel : int {
    Trace = 0,
    Debug = 1,
    Info = 2,
    Warn = 3,
    Error = 4,
    Fatal = 5
};

const char* logLevelToString(LogLevel level);
LogLevel logLevelFromString(const std::string& str);
std::string currentIsoTimestamp();

struct LogRecord {
    std::string timestamp;
    LogLevel level{LogLevel::Info};
    std::string loggerName;
    std::string message;
    std::vector<std::pair<std::string, std::string>> fields;

    [[nodiscard]] std::string toJson() const;
};

class LogAppender {
public:
    virtual ~LogAppender() = default;
    virtual void append(const std::string& formattedJson, const LogRecord& record) = 0;
};

class ConsoleAppender : public LogAppender {
public:
    ConsoleAppender() = default;
    void append(const std::string& formattedJson, const LogRecord& record) override;

private:
    std::mutex m_mutex;
};

class RotatingFileAppender : public LogAppender {
public:
    explicit RotatingFileAppender(std::string filePath, int64_t maxBytes = 10 * 1024 * 1024, int64_t maxBackups = 5);
    ~RotatingFileAppender() override;

    void append(const std::string& formattedJson, const LogRecord& record) override;

private:
    void rotateIfNeeded(size_t incomingBytes);

    std::string m_filePath;
    int64_t m_maxBytes;
    int64_t m_maxBackups;
    int64_t m_currentSize{0};
    std::mutex m_mutex;
};

class MemoryAppender : public LogAppender {
public:
    explicit MemoryAppender(size_t maxCapacity = 1000);

    void append(const std::string& formattedJson, const LogRecord& record) override;

    [[nodiscard]] size_t size();
    [[nodiscard]] std::string get(size_t index);
    [[nodiscard]] std::vector<std::string> getMessages();
    void clear();

private:
    size_t m_maxCapacity;
    std::deque<std::string> m_logs;
    std::mutex m_mutex;
};

class Logger : public std::enable_shared_from_this<Logger> {
public:
    explicit Logger(std::string name = "app", LogLevel level = LogLevel::Info);

    void setLevel(LogLevel level) { m_level = level; }
    [[nodiscard]] LogLevel getLevel() const { return m_level; }
    [[nodiscard]] const std::string& getName() const { return m_name; }

    void addAppender(std::shared_ptr<LogAppender> appender);

    void log(LogLevel level, const std::string& message, const std::vector<std::pair<std::string, std::string>>& extraFields = {});

    void trace(const std::string& message) { log(LogLevel::Trace, message); }
    void debug(const std::string& message) { log(LogLevel::Debug, message); }
    void info(const std::string& message)  { log(LogLevel::Info, message); }
    void warn(const std::string& message)  { log(LogLevel::Warn, message); }
    void error(const std::string& message) { log(LogLevel::Error, message); }
    void fatal(const std::string& message) { log(LogLevel::Fatal, message); }

    [[nodiscard]] std::shared_ptr<Logger> with(const std::string& key, const std::string& value);

    static std::shared_ptr<Logger> getDefault();
    static void setDefault(std::shared_ptr<Logger> logger);

private:
    std::string m_name;
    LogLevel m_level;
    std::vector<std::shared_ptr<LogAppender>> m_appenders;
    std::vector<std::pair<std::string, std::string>> m_contextFields;
    std::mutex m_mutex;
};

} // namespace sp::rt::obs
