# tux - TUI-Launcher
A simple app launcher for Unix written in C

>[!CAUTION]
> WIP. the app works but will surely have some bugs.
> feel free to test the app and report any bugs you encounter

## features
- works on Linux and MacOS
- fast fuzzy search
- no dependencies
- own terminal screen handler (no ncurses)
- scans `/usr/share/applications/` for desktop entries or MacOS paths
- launches GUI apps from `.desktop` files


## roadmap
(development not in order)
- [ ] adding calculator functionality
- [ ] providing user config
    - [ ] style options
    - [ ] color options
    - [ ] custom paths
- [ ] searching and opening files
- [ ] open web with query
- [ ] execute quick shell cmds

## installation
you can find the package in the arch user repository and can install it through yay:
```bash
yay -S tux
```

the easiest way to use the application is to compile it yourself.

Build the binary:
```bash
make release
```

Install it on your system:
```bash
sudo make install
```

Remove it from your system
```bash
sudo make uninstall
```

## Credits
Thanks for contributing to my project:
- [HenryLoM](https://github.com/HenryLoM) for adding MacOS support
