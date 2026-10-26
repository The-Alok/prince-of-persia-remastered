# Project: SDLPoP

CC = gcc
RM = rm -f

HFILES = common.h config.h data.h proto.h types.h pak.h mac_asset_import.h
OBJ = main.o data.o seg000.o seg001.o seg002.o seg003.o seg004.o seg005.o seg006.o seg007.o seg008.o seg009.o pak.o seqtbl.o replay.o options.o lighting.o screenshot.o menu.o midi.o opl3.o stb_vorbis.o
BIN = ../prince
ENGINE = ../runtime/prince-engine

OS      := $(shell uname)

CPPFLAGS += -Wall -D_GNU_SOURCE=1
CFLAGS += -std=c99 -O2

LIBS := $(shell pkg-config --libs sdl2 SDL2_image)
CFLAGS += $(shell pkg-config --cflags sdl2 SDL2_image)

ifeq ($(OS),Darwin)
CPPFLAGS += -D_THREAD_SAFE -DOSX
endif

all: $(BIN) $(ENGINE)

seg008.o: mac_palace_palette.h
seg006.o: mac_sword_table.h

clean:
	$(RM) $(OBJ) launcher.o mac_asset_import.o $(BIN) $(ENGINE)

install:
	./install.sh

uninstall:
	./uninstall.sh

$(ENGINE): $(OBJ)
	mkdir -p ../runtime
	$(CC) $(LDFLAGS) $(OBJ) -o $@ $(LIBS) -lm

$(BIN): launcher.o mac_asset_import.o
	$(CC) $(LDFLAGS) launcher.o mac_asset_import.o -o $@ $(LIBS) -lm

%.o: %.c $(HFILES)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) -c $<

.PHONY: all clean

