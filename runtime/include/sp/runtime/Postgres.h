#pragma once

#include <string>
#include <vector>
#include <memory>
#include <cstdint>
#include <map>

namespace sp::runtime {

struct PgConfig {
    std::string host{"127.0.0.1"};
    int port{5432};
    std::string user{"postgres"};
    std::string password{""};
    std::string database{"postgres"};
    std::string sslmode{"disable"};
    int connectTimeoutMs{5000};

    static PgConfig parseConnectionString(const std::string& connStr);
};

struct PgColumn {
    std::string name;
    uint32_t typeOid{0};
    int16_t format{0}; // 0 = text, 1 = binary
};

struct PgRow {
    std::vector<std::string> values;
    std::vector<bool> isNull;
};

class PgResult {
public:
    PgResult() = default;

    bool isOk() const { return m_ok; }
    void setOk(bool ok) { m_ok = ok; }

    const std::string& errorMessage() const { return m_errorMessage; }
    void setErrorMessage(const std::string& msg) { m_errorMessage = msg; }

    const std::string& commandTag() const { return m_commandTag; }
    void setCommandTag(const std::string& tag);

    int64_t affectedRows() const { return m_affectedRows; }

    int rowCount() const { return static_cast<int>(m_rows.size()); }
    int columnCount() const { return static_cast<int>(m_columns.size()); }

    const std::string& columnName(int col) const;
    uint32_t columnTypeOid(int col) const;

    const std::string& getValue(int row, int col) const;
    bool isNull(int row, int col) const;

    void addColumn(PgColumn col) { m_columns.push_back(std::move(col)); }
    void addRow(PgRow row) { m_rows.push_back(std::move(row)); }

private:
    bool m_ok{true};
    std::string m_errorMessage;
    std::string m_commandTag;
    int64_t m_affectedRows{0};
    std::vector<PgColumn> m_columns;
    std::vector<PgRow> m_rows;
};

class PgConnection {
public:
    PgConnection() = default;
    ~PgConnection();

    PgConnection(const PgConnection&) = delete;
    PgConnection& operator=(const PgConnection&) = delete;

    PgConnection(PgConnection&& other) noexcept;
    PgConnection& operator=(PgConnection&& other) noexcept;

    bool connect(const PgConfig& config);
    bool connect(const std::string& connStr);
    void close();

    bool isConnected() const;
    const std::string& lastError() const { return m_lastError; }

    std::shared_ptr<PgResult> query(const std::string& sql);
    std::shared_ptr<PgResult> queryParams(const std::string& sql, const std::vector<std::string>& params);

    bool ping();

    bool beginTransaction();
    bool commit();
    bool rollback();

private:
    bool sendStartupMessage();
    bool processAuthMessages();
    bool sendPasswordMessage(const std::string& password);
    bool sendMd5PasswordMessage(const std::vector<uint8_t>& salt);
    bool handleScramAuth(const std::vector<uint8_t>& saslList);
    bool readMessage(char& msgType, std::vector<uint8_t>& payload);
    bool sendMessage(char msgType, const std::vector<uint8_t>& payload);

    uintptr_t m_socket{0}; // Native OS socket descriptor
    PgConfig m_config;
    std::string m_lastError;
    bool m_connected{false};
    char m_transactionStatus{'I'}; // 'I' = idle, 'T' = transaction, 'E' = error
    int32_t m_backendPid{0};
    int32_t m_backendKey{0};
};

} // namespace sp::runtime
