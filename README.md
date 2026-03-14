# tux - TUI-Launcher
A simple app launcher for Unix written in C

>[!CAUTION]
> WIP. the app works but will surely have some bugs.
> feel free to test the app and report any bugs you encounter

## features
- fast fuzzy search
- no dependencies
- own terminal screen handler (no ncurses)
- scans `/usr/share/applications/` for desktop entries
- launches GUI apps from `.desktop` files

## todos
- [x] select results using arrow keys
- [x] update `app.dat` only when application entries change

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
