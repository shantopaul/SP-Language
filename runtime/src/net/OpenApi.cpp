#include "sp/runtime/OpenApi.h"
#include <cstring>
#include <cstdlib>

namespace sp::rt::net {

namespace {

char* alloc_c_str(const std::string& str) {
    char* copy = static_cast<char*>(malloc(str.size() + 1));
    if (copy) {
        std::memcpy(copy, str.c_str(), str.size() + 1);
    }
    return copy;
}

} // namespace

std::string renderSwaggerUiHtml(const std::string& title, const std::string& specUrl) {
    std::string safeTitle = title.empty() ? "API Documentation (Swagger UI)" : title;
    std::string safeUrl = specUrl.empty() ? "/openapi.json" : specUrl;

    return R"(<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="utf-8" />
  <meta name="viewport" content="width=device-width, initial-scale=1" />
  <title>)" + safeTitle + R"(</title>
  <link rel="stylesheet" href="https://unpkg.com/swagger-ui-dist@5.11.0/swagger-ui.css" />
  <style>
    body { margin: 0; background: #fafafa; }
    .topbar { display: none; }
  </style>
</head>
<body>
  <div id="swagger-ui"></div>
  <script src="https://unpkg.com/swagger-ui-dist@5.11.0/swagger-ui-bundle.js"></script>
  <script src="https://unpkg.com/swagger-ui-dist@5.11.0/swagger-ui-standalone-preset.js"></script>
  <script>
    window.onload = function() {
      window.ui = SwaggerUIBundle({
        url: ")" + safeUrl + R"(",
        dom_id: '#swagger-ui',
        deepLinking: true,
        presets: [
          SwaggerUIBundle.presets.apis,
          SwaggerUIStandalonePreset
        ],
        plugins: [
          SwaggerUIBundle.plugins.DownloadUrl
        ],
        layout: "StandaloneLayout"
      });
    };
  </script>
</body>
</html>)";
}

std::string renderRedocHtml(const std::string& title, const std::string& specUrl) {
    std::string safeTitle = title.empty() ? "API Documentation (Redoc)" : title;
    std::string safeUrl = specUrl.empty() ? "/openapi.json" : specUrl;

    return R"(<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="utf-8" />
  <meta name="viewport" content="width=device-width, initial-scale=1" />
  <title>)" + safeTitle + R"(</title>
  <link href="https://fonts.googleapis.com/css?family=Montserrat:300,400,700|Roboto:300,400,700" rel="stylesheet">
  <style>
    body { margin: 0; padding: 0; }
  </style>
</head>
<body>
  <redoc spec-url=")" + safeUrl + R"("></redoc>
  <script src="https://cdn.redoc.ly/redoc/latest/bundles/redoc.standalone.js"></script>
</body>
</html>)";
}

} // namespace sp::rt::net

extern "C" {

const char* sp_rt_openapi_swagger_html(const char* title, const char* spec_url) {
    std::string t = title ? title : "";
    std::string u = spec_url ? spec_url : "";
    return sp::rt::net::alloc_c_str(sp::rt::net::renderSwaggerUiHtml(t, u));
}

const char* sp_rt_openapi_redoc_html(const char* title, const char* spec_url) {
    std::string t = title ? title : "";
    std::string u = spec_url ? spec_url : "";
    return sp::rt::net::alloc_c_str(sp::rt::net::renderRedocHtml(t, u));
}

} // extern "C"
