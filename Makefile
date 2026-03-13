CC = gcc

CFLAGS = -Wall -Wextra -Wpedantic -Wshadow -Wconversion
DEBUGFLAGS = -g -O0 -fsanitize=address,undefined
RELEASEFLAGS = -O2

TARGET = tui-launcher
SRC = main.c

debug: $(SRC)
	$(CC) $(SRC) -o $(TARGET)-debug $(CFLAGS) $(DEBUGFLAGS)

release: $(SRC)
	$(CC) $(SRC) -o $(TARGET) $(CFLAGS) $(RELEASEFLAGS)

clean:
	rm -f $(TARGET) $(TARGET)-debug

install: $(TARGET)
	install -Dm755 $(TARGET) $(PREFIX)/bin/$(TARGET)

uninstall:
	rm -f $(PREFIX)/bin/$(TARGET)
