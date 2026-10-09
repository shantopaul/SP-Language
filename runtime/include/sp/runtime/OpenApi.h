#pragma once

#include <string>

namespace sp::rt::net {

std::string renderSwaggerUiHtml(const std::string& title, const std::string& specUrl);
std::string renderRedocHtml(const std::string& title, const std::string& specUrl);

} // namespace sp::rt::net
