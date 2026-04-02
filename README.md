# Tux - TUI-Launcher

A fast, dependency-free app launcher for Unix systems written in C.

![GitHub issues](https://img.shields.io/github/issues/JulB3y/tux)
![GitHub stars](https://img.shields.io/github/stars/JulB3y/tux)
![License](https://img.shields.io/github/license/JulB3y/tux)
![Build Status](https://img.shields.io/badge/build-passing-brightgreen)

> [!CAUTION]  
> **Work in Progress** - The app works but may have bugs. Feel free to test and report any issues you encounter!

---

## Features

### Performance
- fast fuzzy search with optimized algorithms
- no external libraries
- caching for instant startup
- lazy loading of search results
- optimized memory usage with smart allocations
- efficient string matching with early returns
- qsort-based result sorting for O(n log n) complexity
- memory-mapped file I/O for large cache files

### Calculator
- Built-in calculator with automatic equation detection
- Supports basic operations: `+`, `-`, `*`, `/`, `^` (power)
- Parentheses for grouping: `(1+2)*3`
- Mathematical functions: `sqrt()`, `sin()`, `cos()`, `tan()`, `log()`, `exp()`
- Implicit multiplication: `2(1+3)` → `8`
- Real-time results displayed inline with query
- Copy results to clipboard with Enter

### Technical
- Works on Linux and macOS
- custom TUI (no ncurses)
- scans `/usr/share/applications/` for `.desktop` entries
- **Modular query system** with module registry for extensibility

---

## Installation

### From AUR (Arch Linux)

```bash
yay -S tux
```

### Build from Source

The easiest way is to compile it yourself:

```bash
# Build the release binary
make release

# Install system-wide
sudo make install

# Uninstall
sudo make uninstall
```

### Manual Installation

After building, you can also manually copy the binary:

```bash
sudo cp tux /usr/local/bin/
sudo chmod +x /usr/local/bin/tux
```

---

## Usage

### Configuration

Tux supports custom configuration via TOML. Create a configuration file at `~/.config/tux/config.toml`:

#### Custom Keywords for Apps

```toml
[apps]
firefox.keywords = ["browser", "web"]
visual-studio-code.keywords = ["code", "editor", "ide"]
blender.keywords = ["3d", "modeling"]
"visual studio code".keywords = ["code", "editor"]
```

**Note:** App names containing spaces MUST be quoted:
```toml
# Valid - quoted
"visual studio code".keywords = ["code", "editor"]

# Invalid - must use quotes
visual studio code.keywords = ["code", "editor"]
```

Keywords are case-insensitive and will be matched when searching.

#### Web Search Fallback

```toml
[web-search]
url = "https://duckduckgo.com/?q={q}"
```

The `{q}` placeholder is replaced with your query and automatically URL-encoded.

#### Complete Example

```toml
[apps]
firefox.keywords = ["browser", "web", "ff"]
"visual studio code".keywords = ["code", "editor", "vscode"]

[web-search]
url = "https://duckduckgo.com/?q={q}"
```

### Calculator

Tux automatically switches to calculator mode when you type mathematical expressions:

```bash
# Basic operations
1+1                    → 2
2*3+4                  → 10
(1+2)*3                → 9
2^10                   → 1024

# Functions
sqrt(16)               → 4
sin(3.14)              → ~0
log(10)                → 2.3026

# Implicit multiplication
2(1+3)                → 8
4(2+3)                 → 20
```

**Features:**
- Automatic detection: Calculator activates when operators, parentheses, or functions are detected
- Real-time results: See the calculation result as you type
- Copy to clipboard: Press Enter to copy the result to your clipboard
- Supports: `+`, `-`, `*`, `/`, `^`, `()`, `sqrt()`, `sin()`, `cos()`, `tan()`, `log()`, `exp()`

**Note:** Calculator mode only activates when the query contains mathematical operators, parentheses, or functions. Otherwise, it searches for applications as usual.

### Keyboard Shortcuts

| Key | Action |
|-----|--------|
| `Arrow Up` | Move selection up |
| `Arrow Down` | Move selection down |
| `Enter` | Launch selected app / Copy calc result to clipboard |
| `Escape` | Clear query / Exit |

## Roadmap

*(Development not necessarily in order)*

- [x] Calculator functionality
- [ ] User configuration
  - [x] Custom keywords for apps
  - [ ] Style options
  - [ ] Color schemes
  - [ ] Custom app paths
- [ ] File search and opening
- [ ] Web search with query
- [ ] Quick shell commands

---

## Architecture

### Modular Architecture

Tux uses a **modular architecture** with a central **Module Registry**. Each feature (calculator, app search, web search) is implemented as a self-contained module.

```
┌────────────────────────────────────────────────────────────┐
│                     Tux Application                         │
└────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌────────────────────────────────────────────────────────────┐
│                   Module Registry                            │
│  ┌──────────────────────────────────────────────────────┐  │
│  │  calc  → modules/calc.c  (math expressions)          │  │
│  │  apps  → modules/apps.c  (app launcher)              │  │
│  │  web   → modules/web.c   (web search fallback)       │  │
│  └──────────────────────────────────────────────────────┘  │
└────────────────────────────────────────────────────────────┘
```

### Project Structure

```
tux-launcher/
├── include/              # Header files
│   ├── app.h            # Application lifecycle
│   ├── cache.h          # Memory-mapped caching
│   ├── config.h         # TOML configuration
│   ├── fuzzy.h          # Fuzzy matching algorithm
│   ├── module.h         # Module system definitions
│   ├── query.h          # Query parsing and dispatching
│   ├── ui.h             # Terminal UI rendering
│   └── ...
├── src/                 # Source code implementation
│   ├── app.c
│   ├── cache.c
│   ├── config.c
│   ├── exec.c
│   ├── fuzzy.c
│   ├── query.c          # Query dispatch (uses module registry)
│   ├── ui.c
│   └── ...
├── src/modules/         # Feature modules
│   ├── registry.c       # Module registry implementation
│   ├── module.h         # Module interface definitions
│   ├── calc.c/h         # Calculator module
│   ├── apps.c/h         # Application search module
│   └── web.c/h          # Web search module
├── docs/                # Documentation
├── Makefile
└── README.md
```

### Module System

The module system allows adding new features without modifying core code:

```c
typedef struct Module {
    const char *name;                      // Module identifier
    bool (*match)(const char *query);      // Check if query belongs to this module
    void *(*execute)(Module *module, const char *query);  // Handle the query
    void (*destroy)(Module *module);       // Cleanup function
    void *context;                         // Module-specific data
} Module;
```

Modules are discovered via `registry_find_by_query()` which calls each module's `match()` function until one returns true.

---

## Credits

Special thanks to contributors who made this project possible:

- **[HenryLoM](https://github.com/HenryLoM)** - Added macOS support

---

## License

This project is open source. Check the repository for license details.

---

## Reporting Issues

Found a bug? Have a suggestion? Please open an issue on GitHub with:

- Your operating system and version
- Steps to reproduce the issue
- Expected vs actual behavior

---

## Contributing

Contributions are welcome! Feel free to:

- Report bugs
- Suggest new features
- Submit pull requests
- Improve documentation