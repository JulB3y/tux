CC = gcc

UNAME := $(shell uname)

CFLAGS = -Wall -Wextra -Wpedantic -Wshadow -Wconversion -Iinclude \
         -Wformat=2 -Wformat-overflow -Wformat-truncation \
         -Wnull-dereference -Winit-self -Wuninitialized \
         -Wstrict-aliasing -Wfloat-equal -Wpointer-arith \
         -Wcast-align -Wstrict-prototypes -Wmissing-prototypes \
         -Wmissing-declarations -Wredundant-decls -Wnested-externs \
         -Wlogical-op -Wswitch-default -Wbad-function-cast \
         -Wnonnull -Wvla
DEBUGFLAGS = -g -O0 -fsanitize=address,undefined
RELEASEFLAGS = -O2

ifeq ($(UNAME),Darwin)
CFLAGS += -D_DARWIN_C_SOURCE
endif

TARGET = tux
SRC = $(wildcard src/*.c)

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
