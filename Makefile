
NAME=alterego

GAME_C_FILES = $(NAME).c maps_data.c song.c 
GAME_C_FILES += lib/blitter/blitter.c lib/blitter/blitter_tmap.c lib/blitter/blitter_sprites.c \
	lib/chiptune/chiptune.c lib/chiptune/player.c lib/chiptune/chiptune.c 
# this will fail first time
GAME_BINARY_FILES = $(wildcard *.spr) maps.tset maps.tmap

DEFINES += VGA_MODE=320 VGA_BPP=8

include $(BITBOX)/kernel/bitbox.mk
.DELETE_ON_ERROR:

$(NAME).c: maps.h
song.c: alter.song
	python $(BITBOX)/lib/chiptune/song2C.py $^ > $@

maps.h: maps.tmx
	@mkdir -p $(dir $@)
	python $(BITBOX)/lib/blitter/scripts/tmx.py -maxs $^ > $*.h

clean::
	rm -rf *.spr *.tmap *.tset maps.h maps_data.c song.c