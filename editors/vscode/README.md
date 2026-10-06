# SP Language Support for Visual Studio Code

Official Visual Studio Code extension providing language intelligence, syntax highlighting, formatting, and debugging for the **SP Programming Language**.

Powered by **`spls`** (the official SP Language Server).

---

## Features

- **Full Syntax Highlighting**: Comprehensive TextMate grammar covering all SP constructs, keywords, types, string interpolations, and comments.
- **Language Server Protocol (`spls`)**:
  - **Autocomplete & IntelliSense**: Context-aware autocompletion for keywords, types, functions, classes, and members.
  - **Hover Documentation**: Detailed type signatures and doc comments (`///` and `/** ... */`) rendered directly on cursor hover.
  - **Go to Definition**: Jump directly to function, class, and variable definitions.
  - **Diagnostics & Error Squiggles**: Instant syntax and semantic type check diagnostics directly as you type.
  - **Code Formatting**: Automatic formatting powered by `spc fmt`.
  - **Rename Symbol**: Project-wide / file-wide symbol renaming with automatic reference updates.
- **Built-in Snippets**: Quick templates for functions, classes, structs, loops, test blocks, and `main()`.
- **Integrated Commands**:
  - `SP: Run Current File` (`spc run`)
  - `SP: Build Current File` (`spc build`)
  - `SP: Run Tests in Workspace` (`spc test`)
  - `SP: Format File` (`spc fmt`)
  - `SP: Restart Language Server`

---

## Requirements

1. **SP Toolchain** installed on your system.
   - Default Windows location: `C:\SP-Language\bin\spls.exe`
   - Download or build from source: [https://splang.shantopaul.com](https://splang.shantopaul.com)
2. Ensure `spls` and `spc` are accessible on your system `PATH`.

---

## Extension Settings

This extension contributes the following settings:

* `sp.languageServer.path`: Absolute path to the custom `spls` executable.
* `sp.languageServer.trace.server`: Trace communication between VS Code and `spls` (`off`, `messages`, `verbose`).
* `sp.format.onSave`: Automatically format files on save using `spc fmt` (default: `true`).

---

## Official Links

* **Official Website**: [https://splang.shantopaul.com](https://splang.shantopaul.com)
* **Official Documentation**: [https://docs.splang.shantopaul.com](https://docs.splang.shantopaul.com)
* **Package Registry**: [https://registry.splang.shantopaul.com](https://registry.splang.shantopaul.com)

---

## License

MIT License. Copyright (c) 2026 Shanto Paul.
