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
- Modular query system for extensibility

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

Tux supports custom configuration to add keywords for applications. Create a configuration file at `~/.config/tux/config.toml`:

```toml
[apps]
firefox.keywords = ["browser", "web"]
visual-studio-code.keywords = ["code", "editor", "ide"]
blender.keywords = ["3d", "modeling"]
"visual studio code".keywords = ["code", "editor"]
```

Both quoted and unquoted app names are supported:
```toml
[apps]
firefox.keywords = ["browser", "web"]
"firefox".keywords = ["browser", "web"]
```

**Important:** App names containing spaces MUST be quoted:
```toml
[apps]
# ✅ Valid - quoted
"visual studio code".keywords = ["code", "editor"]

# ❌ Invalid - must use quotes
visual studio code.keywords = ["code", "editor"]
```

**Note:** Keywords are case-insensitive and will be matched when searching. You can use alternative names or aliases for applications to make them easier to find.

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

### Project Structure

```
tux-launcher/
├── include/          # Header files
│   ├── query.h      # Query parsing and dispatching
│   ├── calc.h       # Calculator functionality
│   └── ...
├── src/             # Source code
│   ├── app.c        # Application lifecycle
│   ├── query.c      # Query parser and dispatcher
│   ├── calc.c       # Calculator implementation
│   ├── search.c     # Search algorithms
│   ├── fuzzy.c      # Fuzzy matching
│   ├── cache.c      # Caching system
│   └── ...
├── Makefile         # Build configuration
└── README.md
```

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
