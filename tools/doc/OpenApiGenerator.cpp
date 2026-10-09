#include "OpenApiGenerator.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <algorithm>
#include <regex>

namespace fs = std::filesystem;

namespace sp::doc {

static std::string trim(const std::string& str) {
    size_t start = str.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    size_t end = str.find_last_not_of(" \t\r\n");
    return str.substr(start, end - start + 1);
}

static std::string toLower(const std::string& str) {
    std::string s = str;
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return std::tolower(c); });
    return s;
}

static std::string toUpper(const std::string& str) {
    std::string s = str;
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return std::toupper(c); });
    return s;
}

static std::string escapeJson(const std::string& str) {
    std::string res;
    res.reserve(str.size() + 16);
    for (char c : str) {
        if (c == '"') res += "\\\"";
        else if (c == '\\') res += "\\\\";
        else if (c == '\b') res += "\\b";
        else if (c == '\f') res += "\\f";
        else if (c == '\n') res += "\\n";
        else if (c == '\r') res += "\\r";
        else if (c == '\t') res += "\\t";
        else res += c;
    }
    return res;
}

OpenApiGenerator::OpenApiGenerator(OpenApiOptions options)
    : m_options(std::move(options)) {}

void OpenApiGenerator::addRoute(const OpenApiRoute& route) {
    m_routes.push_back(route);
}

std::string OpenApiGenerator::normalizePath(const std::string& rawPath, std::vector<OpenApiParameter>& autoParams) {
    std::string norm = rawPath;
    if (norm.empty()) return "/";
    if (norm.front() != '/') norm = "/" + norm;

    // Convert :param to {param}
    std::regex paramRegex(":([a-zA-Z_][a-zA-Z0-9_]*)");
    std::smatch m;
    std::string temp = norm;
    std::string result;

    while (std::regex_search(temp, m, paramRegex)) {
        result += m.prefix().str();
        std::string pName = m[1].str();
        result += "{" + pName + "}";

        // Add auto parameter if not already present
        bool found = false;
        for (const auto& p : autoParams) {
            if (p.name == pName && p.in == "path") {
                found = true;
                break;
            }
        }
        if (!found) {
            OpenApiParameter param;
            param.name = pName;
            param.in = "path";
            param.description = "Path parameter '" + pName + "'";
            param.required = true;
            param.type = "string";
            autoParams.push_back(param);
        }

        temp = m.suffix().str();
    }
    result += temp;

    // Also scan existing {param} in path
    std::regex braceRegex("\\{([a-zA-Z_][a-zA-Z0-9_]*)\\}");
    temp = result;
    while (std::regex_search(temp, m, braceRegex)) {
        std::string pName = m[1].str();
        bool found = false;
        for (const auto& p : autoParams) {
            if (p.name == pName && p.in == "path") {
                found = true;
                break;
            }
        }
        if (!found) {
            OpenApiParameter param;
            param.name = pName;
            param.in = "path";
            param.description = "Path parameter '" + pName + "'";
            param.required = true;
            param.type = "string";
            autoParams.push_back(param);
        }
        temp = m.suffix().str();
    }

    return result;
}

void OpenApiGenerator::parseDocComment(const std::string& comment, OpenApiRoute& route) {
    std::istringstream stream(comment);
    std::string line;

    while (std::getline(stream, line)) {
        line = trim(line);
        while (line.starts_with("/") || line.starts_with("*")) {
            line = trim(line.substr(1));
        }

        if (line.starts_with("@route ") || line.starts_with("@endpoint ")) {
            size_t firstSpace = line.find(' ');
            std::string rest = trim(line.substr(firstSpace + 1));
            size_t methodSpace = rest.find(' ');
            if (methodSpace != std::string::npos) {
                route.method = toUpper(trim(rest.substr(0, methodSpace)));
                route.path = trim(rest.substr(methodSpace + 1));
            }
        } else if (line.starts_with("@summary ")) {
            route.summary = trim(line.substr(9));
        } else if (line.starts_with("@desc ") || line.starts_with("@description ")) {
            size_t space = line.find(' ');
            route.description = trim(line.substr(space + 1));
        } else if (line.starts_with("@tag ") || line.starts_with("@tags ")) {
            size_t space = line.find(' ');
            std::string tagsStr = trim(line.substr(space + 1));
            std::stringstream tagStream(tagsStr);
            std::string tag;
            while (std::getline(tagStream, tag, ',')) {
                tag = trim(tag);
                if (!tag.empty()) route.tags.push_back(tag);
            }
        } else if (line.starts_with("@operationId ")) {
            route.operationId = trim(line.substr(13));
        } else if (line.starts_with("@param ")) {
            std::string pStr = trim(line.substr(7));
            std::istringstream pStream(pStr);
            std::string pName, pIn, pType;
            pStream >> pName >> pIn >> pType;
            std::string pDesc;
            std::getline(pStream, pDesc);
            pDesc = trim(pDesc);

            OpenApiParameter param;
            param.name = pName;
            pIn = toLower(pIn);
            if (pIn == "path" || pIn == "query" || pIn == "header" || pIn == "cookie") {
                param.in = pIn;
            } else {
                param.in = "query";
            }
            param.type = pType.empty() ? "string" : toLower(pType);
            param.required = (param.in == "path");
            param.description = pDesc;

            // Update or add
            bool updated = false;
            for (auto& existing : route.parameters) {
                if (existing.name == param.name && existing.in == param.in) {
                    existing = param;
                    updated = true;
                    break;
                }
            }
            if (!updated) {
                route.parameters.push_back(param);
            }
        } else if (line.starts_with("@body ")) {
            std::string bStr = trim(line.substr(6));
            std::istringstream bStream(bStr);
            bStream >> route.requestBodyType;
            std::string bDesc;
            std::getline(bStream, bDesc);
            route.requestBodyDesc = trim(bDesc);
            route.requestBodyRequired = true;
        } else if (line.starts_with("@response ") || line.starts_with("@returns ")) {
            size_t space = line.find(' ');
            std::string rStr = trim(line.substr(space + 1));
            std::istringstream rStream(rStr);
            int code = 200;
            if (std::isdigit(static_cast<unsigned char>(rStr[0]))) {
                rStream >> code;
            }
            std::string rDesc;
            std::getline(rStream, rDesc);
            rDesc = trim(rDesc);
            if (rDesc.empty()) rDesc = (code == 200 ? "Success" : "Response " + std::to_string(code));

            OpenApiResponse resp;
            resp.statusCode = code;
            resp.description = rDesc;
            route.responses.push_back(resp);
        } else if (!line.empty() && route.summary.empty()) {
            // First descriptive line defaults to summary
            route.summary = line;
        }
    }
}

void OpenApiGenerator::scanSource(const std::string& source, const std::string& /*filePath*/) {
    std::istringstream stream(source);
    std::string line;
    std::string docAccumulator;
    std::map<std::string, std::string> groupPrefixes; // varName -> prefix

    // Regex patterns for route handlers:
    // app.get("/path", ...), app.post("/path", ...), router.delete("/path", ...)
    std::regex routeCallRegex("([a-zA-Z_][a-zA-Z0-9_]*)\\.(get|post|put|patch|delete|options|head|deleteRoute|route)\\s*\\(\\s*\"([^\"]+)\"");
    std::regex customRouteCallRegex("([a-zA-Z_][a-zA-Z0-9_]*)\\.route\\s*\\(\\s*\"([A-Z]+)\"\\s*,\\s*\"([^\"]+)\"");
    std::regex groupRegex("(?:let|var)\\s+([a-zA-Z_][a-zA-Z0-9_]*)\\s*=\\s*([a-zA-Z_][a-zA-Z0-9_]*)\\.group\\s*\\(\\s*\"([^\"]+)\"\\)");

    while (std::getline(stream, line)) {
        std::string trimmed = trim(line);

        if (trimmed.starts_with("///") || trimmed.starts_with("/**") || trimmed.starts_with("*")) {
            docAccumulator += trimmed + "\n";
            continue;
        }

        // Check for group definitions: let api = app.group("/api/v1");
        std::smatch gMatch;
        if (std::regex_search(trimmed, gMatch, groupRegex)) {
            std::string varName = gMatch[1].str();
            std::string parentVar = gMatch[2].str();
            std::string prefix = gMatch[3].str();
            if (groupPrefixes.count(parentVar)) {
                prefix = groupPrefixes[parentVar] + prefix;
            }
            groupPrefixes[varName] = prefix;
        }

        // Check for custom route: app.route("POST", "/path", ...)
        std::smatch cMatch;
        if (std::regex_search(trimmed, cMatch, customRouteCallRegex)) {
            std::string varName = cMatch[1].str();
            std::string method = toUpper(cMatch[2].str());
            std::string rawPath = cMatch[3].str();

            if (groupPrefixes.count(varName)) {
                rawPath = groupPrefixes[varName] + rawPath;
            }

            OpenApiRoute route;
            route.method = method;
            route.path = normalizePath(rawPath, route.parameters);

            if (!docAccumulator.empty()) {
                parseDocComment(docAccumulator, route);
            }
            if (route.summary.empty()) {
                route.summary = route.method + " " + route.path;
            }
            if (route.responses.empty()) {
                OpenApiResponse defaultResp;
                defaultResp.statusCode = 200;
                defaultResp.description = "Success";
                route.responses.push_back(defaultResp);
            }

            // Derive tag from first path segment
            if (route.tags.empty() && route.path.size() > 1) {
                size_t secondSlash = route.path.find('/', 1);
                std::string tagPart = route.path.substr(1, secondSlash == std::string::npos ? std::string::npos : secondSlash - 1);
                if (!tagPart.empty() && !tagPart.starts_with("{")) {
                    tagPart[0] = std::toupper(static_cast<unsigned char>(tagPart[0]));
                    route.tags.push_back(tagPart);
                }
            }

            m_routes.push_back(route);
            docAccumulator.clear();
            continue;
        }

        // Check for standard route: app.get("/path", ...)
        std::smatch rMatch;
        if (std::regex_search(trimmed, rMatch, routeCallRegex)) {
            std::string varName = rMatch[1].str();
            std::string method = toUpper(rMatch[2].str());
            if (method == "DELETEROUTE") method = "DELETE";
            std::string rawPath = rMatch[3].str();

            if (groupPrefixes.count(varName)) {
                rawPath = groupPrefixes[varName] + rawPath;
            }

            OpenApiRoute route;
            route.method = method;
            route.path = normalizePath(rawPath, route.parameters);

            if (!docAccumulator.empty()) {
                parseDocComment(docAccumulator, route);
            }
            if (route.summary.empty()) {
                route.summary = route.method + " " + route.path;
            }
            if (route.responses.empty()) {
                OpenApiResponse defaultResp;
                defaultResp.statusCode = (route.method == "POST" ? 201 : 200);
                defaultResp.description = (route.method == "POST" ? "Created" : "Success");
                route.responses.push_back(defaultResp);
            }

            // Derive tag from first path segment
            if (route.tags.empty() && route.path.size() > 1) {
                size_t secondSlash = route.path.find('/', 1);
                std::string tagPart = route.path.substr(1, secondSlash == std::string::npos ? std::string::npos : secondSlash - 1);
                if (!tagPart.empty() && !tagPart.starts_with("{")) {
                    tagPart[0] = std::toupper(static_cast<unsigned char>(tagPart[0]));
                    route.tags.push_back(tagPart);
                }
            }

            m_routes.push_back(route);
            docAccumulator.clear();
            continue;
        }

        // If doc comment had an explicit @route directive but no app.method matched
        if (!docAccumulator.empty() && (docAccumulator.find("@route") != std::string::npos || docAccumulator.find("@endpoint") != std::string::npos)) {
            OpenApiRoute explicitRoute;
            parseDocComment(docAccumulator, explicitRoute);
            if (!explicitRoute.method.empty() && !explicitRoute.path.empty()) {
                explicitRoute.path = normalizePath(explicitRoute.path, explicitRoute.parameters);
                if (explicitRoute.responses.empty()) {
                    OpenApiResponse defaultResp;
                    defaultResp.statusCode = 200;
                    defaultResp.description = "Success";
                    explicitRoute.responses.push_back(defaultResp);
                }
                m_routes.push_back(explicitRoute);
            }
            docAccumulator.clear();
        }

        if (!trimmed.empty() && !trimmed.starts_with("//")) {
            docAccumulator.clear();
        }
    }
}

void OpenApiGenerator::scanFile(const std::string& filePath) {
    std::ifstream file(filePath);
    if (!file.is_open()) return;
    std::stringstream buffer;
    buffer << file.rdbuf();
    scanSource(buffer.str(), filePath);
}

void OpenApiGenerator::scanDirectory(const std::string& dirPath) {
    if (!fs::exists(dirPath)) return;
    for (const auto& entry : fs::recursive_directory_iterator(dirPath)) {
        if (entry.is_regular_file() && entry.path().extension() == ".sp") {
            scanFile(entry.path().string());
        }
    }
}

std::string OpenApiGenerator::generateJson(bool pretty) const {
    std::ostringstream ss;
    std::string ind1 = pretty ? "  " : "";
    std::string ind2 = pretty ? "    " : "";
    std::string ind3 = pretty ? "      " : "";
    std::string ind4 = pretty ? "        " : "";
    std::string ind5 = pretty ? "          " : "";
    std::string ind6 = pretty ? "            " : "";
    std::string nl = pretty ? "\n" : "";

    ss << "{" << nl;
    ss << ind1 << "\"openapi\": \"3.0.3\"," << nl;
    ss << ind1 << "\"info\": {" << nl;
    ss << ind2 << "\"title\": \"" << escapeJson(m_options.title) << "\"," << nl;
    ss << ind2 << "\"version\": \"" << escapeJson(m_options.version) << "\"," << nl;
    ss << ind2 << "\"description\": \"" << escapeJson(m_options.description) << "\"" << nl;
    ss << ind1 << "}," << nl;

    ss << ind1 << "\"servers\": [" << nl;
    ss << ind2 << "{" << nl;
    ss << ind3 << "\"url\": \"" << escapeJson(m_options.serverUrl) << "\"," << nl;
    ss << ind3 << "\"description\": \"Primary Server\"" << nl;
    ss << ind2 << "}" << nl;
    ss << ind1 << "]," << nl;

    // Group routes by path
    std::map<std::string, std::vector<OpenApiRoute>> pathsMap;
    for (const auto& r : m_routes) {
        pathsMap[r.path].push_back(r);
    }

    ss << ind1 << "\"paths\": {" << nl;
    size_t pathIndex = 0;
    for (const auto& [path, routes] : pathsMap) {
        ss << ind2 << "\"" << escapeJson(path) << "\": {" << nl;
        for (size_t rIdx = 0; rIdx < routes.size(); ++rIdx) {
            const auto& route = routes[rIdx];
            std::string methodLower = toLower(route.method);
            ss << ind3 << "\"" << methodLower << "\": {" << nl;
            ss << ind4 << "\"summary\": \"" << escapeJson(route.summary) << "\"," << nl;
            if (!route.description.empty()) {
                ss << ind4 << "\"description\": \"" << escapeJson(route.description) << "\"," << nl;
            }
            if (!route.operationId.empty()) {
                ss << ind4 << "\"operationId\": \"" << escapeJson(route.operationId) << "\"," << nl;
            }

            // Tags
            if (!route.tags.empty()) {
                ss << ind4 << "\"tags\": [";
                for (size_t t = 0; t < route.tags.size(); ++t) {
                    ss << "\"" << escapeJson(route.tags[t]) << "\"" << (t + 1 < route.tags.size() ? ", " : "");
                }
                ss << "]," << nl;
            }

            // Parameters
            if (!route.parameters.empty()) {
                ss << ind4 << "\"parameters\": [" << nl;
                for (size_t p = 0; p < route.parameters.size(); ++p) {
                    const auto& param = route.parameters[p];
                    ss << ind5 << "{" << nl;
                    ss << ind6 << "\"name\": \"" << escapeJson(param.name) << "\"," << nl;
                    ss << ind6 << "\"in\": \"" << escapeJson(param.in) << "\"," << nl;
                    ss << ind6 << "\"required\": " << (param.required ? "true" : "false") << "," << nl;
                    if (!param.description.empty()) {
                        ss << ind6 << "\"description\": \"" << escapeJson(param.description) << "\"," << nl;
                    }
                    ss << ind6 << "\"schema\": { \"type\": \"" << escapeJson(param.type) << "\" }" << nl;
                    ss << ind5 << "}" << (p + 1 < route.parameters.size() ? "," : "") << nl;
                }
                ss << ind4 << "]," << nl;
            }

            // Request Body
            if (route.requestBodyRequired || route.method == "POST" || route.method == "PUT" || route.method == "PATCH") {
                ss << ind4 << "\"requestBody\": {" << nl;
                ss << ind5 << "\"required\": " << (route.requestBodyRequired ? "true" : "false") << "," << nl;
                if (!route.requestBodyDesc.empty()) {
                    ss << ind5 << "\"description\": \"" << escapeJson(route.requestBodyDesc) << "\"," << nl;
                }
                ss << ind5 << "\"content\": {" << nl;
                ss << ind6 << "\"application/json\": {" << nl;
                ss << ind6 << "  \"schema\": { \"type\": \"object\" }" << nl;
                ss << ind6 << "}" << nl;
                ss << ind5 << "}" << nl;
                ss << ind4 << "}," << nl;
            }

            // Responses
            ss << ind4 << "\"responses\": {" << nl;
            for (size_t respIdx = 0; respIdx < route.responses.size(); ++respIdx) {
                const auto& resp = route.responses[respIdx];
                ss << ind5 << "\"" << resp.statusCode << "\": {" << nl;
                ss << ind6 << "\"description\": \"" << escapeJson(resp.description) << "\"," << nl;
                ss << ind6 << "\"content\": {" << nl;
                ss << ind6 << "  \"" << escapeJson(resp.contentType) << "\": {" << nl;
                ss << ind6 << "    \"schema\": { \"type\": \"" << escapeJson(resp.schemaType) << "\" }" << nl;
                ss << ind6 << "  }" << nl;
                ss << ind6 << "}" << nl;
                ss << ind5 << "}" << (respIdx + 1 < route.responses.size() ? "," : "") << nl;
            }
            ss << ind4 << "}" << nl; // end responses

            ss << ind3 << "}" << (rIdx + 1 < routes.size() ? "," : "") << nl; // end method
        }
        pathIndex++;
        ss << ind2 << "}" << (pathIndex < pathsMap.size() ? "," : "") << nl; // end path
    }
    ss << ind1 << "}" << nl; // end paths
    ss << "}" << nl;

    return ss.str();
}

std::string OpenApiGenerator::generateYaml() const {
    std::ostringstream ss;
    ss << "openapi: 3.0.3\n";
    ss << "info:\n";
    ss << "  title: \"" << m_options.title << "\"\n";
    ss << "  version: \"" << m_options.version << "\"\n";
    ss << "  description: \"" << m_options.description << "\"\n";
    ss << "servers:\n";
    ss << "  - url: \"" << m_options.serverUrl << "\"\n";
    ss << "    description: Primary Server\n";
    ss << "paths:\n";

    std::map<std::string, std::vector<OpenApiRoute>> pathsMap;
    for (const auto& r : m_routes) {
        pathsMap[r.path].push_back(r);
    }

    for (const auto& [path, routes] : pathsMap) {
        ss << "  \"" << path << "\":\n";
        for (const auto& route : routes) {
            std::string methodLower = toLower(route.method);
            ss << "    " << methodLower << ":\n";
            ss << "      summary: \"" << route.summary << "\"\n";
            if (!route.description.empty()) {
                ss << "      description: \"" << route.description << "\"\n";
            }
            if (!route.tags.empty()) {
                ss << "      tags:\n";
                for (const auto& tag : route.tags) {
                    ss << "        - \"" << tag << "\"\n";
                }
            }
            if (!route.parameters.empty()) {
                ss << "      parameters:\n";
                for (const auto& p : route.parameters) {
                    ss << "        - name: \"" << p.name << "\"\n";
                    ss << "          in: " << p.in << "\n";
                    ss << "          required: " << (p.required ? "true" : "false") << "\n";
                    ss << "          schema:\n";
                    ss << "            type: " << p.type << "\n";
                }
            }
            ss << "      responses:\n";
            for (const auto& resp : route.responses) {
                ss << "        \"" << resp.statusCode << "\":\n";
                ss << "          description: \"" << resp.description << "\"\n";
            }
        }
    }

    return ss.str();
}

std::string OpenApiGenerator::generateSwaggerHtml(const std::string& openApiJsonUrl) const {
    std::string html = R"HTML(<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <title>)HTML" + m_options.title + R"HTML( - Swagger UI</title>
    <link rel="stylesheet" type="text/css" href="https://cdn.jsdelivr.net/npm/swagger-ui-dist@5/swagger-ui.css" />
    <style>
        html { box-sizing: border-box; overflow: -moz-scrollbars-vertical; overflow-y: scroll; }
        *, *:before, *:after { box-sizing: inherit; }
        body { margin: 0; background: #fafafa; }
        .topbar { display: none; }
    </style>
</head>
<body>
    <div id="swagger-ui"></div>
    <script src="https://cdn.jsdelivr.net/npm/swagger-ui-dist@5/swagger-ui-bundle.js" charset="UTF-8"></script>
    <script src="https://cdn.jsdelivr.net/npm/swagger-ui-dist@5/swagger-ui-standalone-preset.js" charset="UTF-8"></script>
    <script>
    window.onload = function() {
        window.ui = SwaggerUIBundle({
            url: ")HTML" + openApiJsonUrl + R"HTML(",
            dom_id: '#swagger-ui',
            deepLinking: true,
            presets: [
                SwaggerUIBundle.presets.apis,
                SwaggerUIStandalonePreset
            ],
            layout: "BaseLayout"
        });
    };
    </script>
</body>
</html>)HTML";
    return html;
}

std::string OpenApiGenerator::generateRedocHtml(const std::string& openApiJsonUrl) const {
    std::string html = R"HTML(<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <title>)HTML" + m_options.title + R"HTML( - Redoc</title>
    <link href="https://fonts.googleapis.com/css?family=Montserrat:300,400,700|Roboto:300,400,700" rel="stylesheet">
    <style>body { margin: 0; padding: 0; }</style>
</head>
<body>
    <redoc spec-url=")HTML" + openApiJsonUrl + R"HTML("></redoc>
    <script src="https://cdn.jsdelivr.net/npm/redoc@latest/bundles/redoc.standalone.js"></script>
</body>
</html>)HTML";
    return html;
}

bool OpenApiGenerator::saveJson(const std::string& outputPath) const {
    fs::path p(outputPath);
    if (p.has_parent_path()) {
        fs::create_directories(p.parent_path());
    }
    std::ofstream out(outputPath);
    if (!out.is_open()) return false;
    out << generateJson(true);
    return true;
}

bool OpenApiGenerator::saveSwaggerHtml(const std::string& outputPath, const std::string& openApiJsonUrl) const {
    fs::path p(outputPath);
    if (p.has_parent_path()) {
        fs::create_directories(p.parent_path());
    }
    std::ofstream out(outputPath);
    if (!out.is_open()) return false;
    out << generateSwaggerHtml(openApiJsonUrl);
    return true;
}

bool OpenApiGenerator::saveRedocHtml(const std::string& outputPath, const std::string& openApiJsonUrl) const {
    fs::path p(outputPath);
    if (p.has_parent_path()) {
        fs::create_directories(p.parent_path());
    }
    std::ofstream out(outputPath);
    if (!out.is_open()) return false;
    out << generateRedocHtml(openApiJsonUrl);
    return true;
}

} // namespace sp::doc
