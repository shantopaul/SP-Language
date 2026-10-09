#pragma once

#include "sp/runtime/Http.h"
#include <string>
#include <memory>
#include <map>

namespace sp::rt::net {

struct TestResponseHandle {
    int statusCode{200};
    std::string statusText{"OK"};
    std::string body;
    std::map<std::string, std::string> headers;
};

class TestHttpClient {
public:
    explicit TestHttpClient(sp::rt::http::HttpServer* server);

    std::unique_ptr<TestResponseHandle> request(
        const std::string& method,
        const std::string& path,
        const std::string& body = "",
        const std::map<std::string, std::string>& headers = {}
    );

private:
    sp::rt::http::HttpServer* m_server{nullptr};
};

} // namespace sp::rt::net
