
USE_ENGINE = 1

NAME=alterego

GAME_C_FILES = $(NAME).c maps_data.c song.c $(BITBOX)/lib/chiptune.c
# this will fail first time
GAME_BINARY_FILES = $(wildcard *.spr) maps.tset maps.tmap

DEFINES += VGAMODE_320 VGA_8BIT

include $(BITBOX)/lib/bitbox.mk
.DELETE_ON_ERROR:

$(NAME).c: maps.h
song.c: alter.song
	python $(BITBOX)/scripts/song2C.py $^ > $@

maps.h: maps.tmx
	@mkdir -p $(dir $@)
	python $(BITBOX)/scripts/tmx.py -maxs $^ > $*.h

clean::
	rm -rf *.spr *.tmap *.tset maps.h maps_data.c song.c