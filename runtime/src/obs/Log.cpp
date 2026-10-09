#include "sp/runtime/Log.h"
#include <iostream>
#include <fstream>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <filesystem>

namespace fs = std::filesystem;

namespace sp::rt::obs {

namespace {

std::string escapeJson(const std::string& s) {
    std::ostringstream o;
    for (char c : s) {
        switch (c) {
            case '"':  o << "\\\""; break;
            case '\\': o << "\\\\"; break;
            case '\b': o << "\\b";  break;
            case '\f': o << "\\f";  break;
            case '\n': o << "\\n";  break;
            case '\r': o << "\\r";  break;
            case '\t': o << "\\t";  break;
            default:
                if (static_cast<unsigned char>(c) <= 0x1f) {
                    o << "\\u" << std::hex << std::setw(4) << std::setfill('0') << static_cast<int>(c);
                } else {
                    o << c;
                }
        }
    }
    return o.str();
}

} // namespace

const char* logLevelToString(LogLevel level) {
    switch (level) {
        case LogLevel::Trace: return "TRACE";
        case LogLevel::Debug: return "DEBUG";
        case LogLevel::Info:  return "INFO";
        case LogLevel::Warn:  return "WARN";
        case LogLevel::Error: return "ERROR";
        case LogLevel::Fatal: return "FATAL";
        default: return "INFO";
    }
}

LogLevel logLevelFromString(const std::string& str) {
    if (str == "TRACE" || str == "trace") return LogLevel::Trace;
    if (str == "DEBUG" || str == "debug") return LogLevel::Debug;
    if (str == "WARN"  || str == "warn")  return LogLevel::Warn;
    if (str == "ERROR" || str == "error") return LogLevel::Error;
    if (str == "FATAL" || str == "fatal") return LogLevel::Fatal;
    return LogLevel::Info;
}

std::string currentIsoTimestamp() {
    using namespace std::chrono;
    auto now = system_clock::now();
    auto ms = duration_cast<milliseconds>(now.time_since_epoch()) % 1000;
    auto timer = system_clock::to_time_t(now);

    std::tm gmt{};
#if defined(_WIN32)
    gmtime_s(&gmt, &timer);
#else
    gmtime_r(&timer, &gmt);
#endif

    std::ostringstream oss;
    oss << std::put_time(&gmt, "%Y-%m-%dT%H:%M:%S")
        << '.' << std::setfill('0') << std::setw(3) << ms.count()
        << 'Z';
    return oss.str();
}

std::string LogRecord::toJson() const {
    std::ostringstream ss;
    ss << "{\"timestamp\":\"" << escapeJson(timestamp) << "\","
       << "\"level\":\"" << logLevelToString(level) << "\","
       << "\"logger\":\"" << escapeJson(loggerName) << "\","
       << "\"message\":\"" << escapeJson(message) << "\"";

    for (const auto& [k, v] : fields) {
        ss << ",\"" << escapeJson(k) << "\":\"" << escapeJson(v) << "\"";
    }
    ss << "}";
    return ss.str();
}

// ----------------------------------------------------------------------------
// ConsoleAppender
// ----------------------------------------------------------------------------
void ConsoleAppender::append(const std::string& formattedJson, const LogRecord& record) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (record.level >= LogLevel::Warn) {
        std::cerr << formattedJson << "\n";
        std::cerr.flush();
    } else {
        std::cout << formattedJson << "\n";
        std::cout.flush();
    }
}

// ----------------------------------------------------------------------------
// RotatingFileAppender
// ----------------------------------------------------------------------------
RotatingFileAppender::RotatingFileAppender(std::string filePath, int64_t maxBytes, int64_t maxBackups)
    : m_filePath(std::move(filePath)), m_maxBytes(maxBytes), m_maxBackups(maxBackups) {
    if (fs::exists(m_filePath)) {
        std::error_code ec;
        m_currentSize = static_cast<int64_t>(fs::file_size(m_filePath, ec));
    }
}

RotatingFileAppender::~RotatingFileAppender() = default;

void RotatingFileAppender::rotateIfNeeded(size_t incomingBytes) {
    if (m_maxBytes <= 0 || m_currentSize + static_cast<int64_t>(incomingBytes) < m_maxBytes) {
        return;
    }

    std::error_code ec;
    // Rotate backups: e.g. .5 -> removed, .4 -> .5, etc.
    for (int64_t i = m_maxBackups - 1; i >= 1; --i) {
        fs::path src = m_filePath + "." + std::to_string(i);
        fs::path dst = m_filePath + "." + std::to_string(i + 1);
        if (fs::exists(src, ec)) {
            fs::rename(src, dst, ec);
        }
    }

    // Rename current file to .1
    if (fs::exists(m_filePath, ec)) {
        fs::path firstBackup = m_filePath + ".1";
        fs::rename(m_filePath, firstBackup, ec);
    }
    m_currentSize = 0;
}

void RotatingFileAppender::append(const std::string& formattedJson, const LogRecord& record) {
    (void)record;
    std::lock_guard<std::mutex> lock(m_mutex);
    std::string line = formattedJson + "\n";
    rotateIfNeeded(line.size());

    std::ofstream out(m_filePath, std::ios::app);
    if (out.is_open()) {
        out << line;
        out.flush();
        m_currentSize += static_cast<int64_t>(line.size());
    }
}

// ----------------------------------------------------------------------------
// MemoryAppender
// ----------------------------------------------------------------------------
MemoryAppender::MemoryAppender(size_t maxCapacity) : m_maxCapacity(maxCapacity) {}

void MemoryAppender::append(const std::string& formattedJson, const LogRecord& record) {
    (void)record;
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_logs.size() >= m_maxCapacity) {
        m_logs.pop_front();
    }
    m_logs.push_back(formattedJson);
}

size_t MemoryAppender::size() {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_logs.size();
}

std::string MemoryAppender::get(size_t index) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (index < m_logs.size()) {
        return m_logs[index];
    }
    return "";
}

std::vector<std::string> MemoryAppender::getMessages() {
    std::lock_guard<std::mutex> lock(m_mutex);
    return std::vector<std::string>(m_logs.begin(), m_logs.end());
}

void MemoryAppender::clear() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_logs.clear();
}

// ----------------------------------------------------------------------------
// Logger
// ----------------------------------------------------------------------------
Logger::Logger(std::string name, LogLevel level)
    : m_name(std::move(name)), m_level(level) {}

void Logger::addAppender(std::shared_ptr<LogAppender> appender) {
    if (!appender) return;
    std::lock_guard<std::mutex> lock(m_mutex);
    m_appenders.push_back(std::move(appender));
}

void Logger::log(LogLevel level, const std::string& message, const std::vector<std::pair<std::string, std::string>>& extraFields) {
    if (level < m_level) return;

    LogRecord record;
    record.timestamp = currentIsoTimestamp();
    record.level = level;
    record.loggerName = m_name;
    record.message = message;

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        record.fields = m_contextFields;
        for (const auto& f : extraFields) {
            record.fields.push_back(f);
        }

        std::string jsonStr = record.toJson();
        for (auto& appender : m_appenders) {
            appender->append(jsonStr, record);
        }
    }
}

std::shared_ptr<Logger> Logger::with(const std::string& key, const std::string& value) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto child = std::make_shared<Logger>(m_name, m_level);
    child->m_appenders = m_appenders;
    child->m_contextFields = m_contextFields;
    child->m_contextFields.emplace_back(key, value);
    return child;
}

static std::shared_ptr<Logger> s_defaultLogger = nullptr;
static std::mutex s_defaultLoggerMutex;

std::shared_ptr<Logger> Logger::getDefault() {
    std::lock_guard<std::mutex> lock(s_defaultLoggerMutex);
    if (!s_defaultLogger) {
        s_defaultLogger = std::make_shared<Logger>("app", LogLevel::Info);
        s_defaultLogger->addAppender(std::make_shared<ConsoleAppender>());
    }
    return s_defaultLogger;
}

void Logger::setDefault(std::shared_ptr<Logger> logger) {
    std::lock_guard<std::mutex> lock(s_defaultLoggerMutex);
    s_defaultLogger = std::move(logger);
}

} // namespace sp::rt::obs
