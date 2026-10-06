#include "LspServer.h"
#include "sp/Version.h"
#include <iostream>
#include <string_view>

#if defined(_WIN32)
#include <fcntl.h>
#include <io.h>
#endif

int main(int argc, char* argv[]) {
    if (argc > 1) {
        std::string_view arg = argv[1];
        if (arg == "--version" || arg == "-v") {
            std::cout << "spls (SP Language Server) version " << sp::SP_VERSION_STRING << "\n"
                      << "Copyright (c) 2026 Shanto Paul\n";
            return 0;
        }
        if (arg == "--help" || arg == "-h") {
            std::cout << "Usage: spls [options]\n\n"
                      << "Options:\n"
                      << "  -v, --version  Display language server version\n"
                      << "  -h, --help     Display this help message\n\n"
                      << "spls operates as a Language Server Protocol (LSP) daemon communicating over stdio.\n";
            return 0;
        }
    }

#if defined(_WIN32)
    // Put stdin and stdout into binary mode so CRLF line endings are preserved exactly
    _setmode(_fileno(stdin), _O_BINARY);
    _setmode(_fileno(stdout), _O_BINARY);
#endif

    std::ios_base::sync_with_stdio(false);
    sp::lsp::LspServer server(std::cin, std::cout);
    server.run();

    return 0;
}
