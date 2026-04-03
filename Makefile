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
LIBS = -lm
DEBUGFLAGS = -g -O0 -fsanitize=address,undefined
RELEASEFLAGS = -O3 -flto -march=native

ifeq ($(UNAME),Darwin)
CFLAGS += -D_DARWIN_C_SOURCE
endif

TARGET = tux
SRC = src/app.c src/cache.c src/config.c src/exec.c src/file.c \
      src/fuzzy.c src/history.c src/input.c src/main.c src/query.c \
      src/term.c src/ui.c src/util.c
MODULES_SRC = src/modules/registry.c src/modules/calc.c \
              src/modules/apps.c src/modules/web.c

debug: $(SRC) $(MODULES_SRC)
	$(CC) $(SRC) $(MODULES_SRC) -o $(TARGET)-debug $(CFLAGS) $(DEBUGFLAGS) $(LIBS)

release: $(SRC) $(MODULES_SRC)
	$(CC) $(SRC) $(MODULES_SRC) -o $(TARGET) $(CFLAGS) $(RELEASEFLAGS) $(LIBS)

clean:
	rm -f $(TARGET) $(TARGET)-debug

install: $(TARGET)
	install -Dm755 $(TARGET) $(PREFIX)/bin/$(TARGET)

uninstall:
	rm -f $(PREFIX)/bin/$(TARGET)
