// alterego
// see gameplay : https://www.youtube.com/watch?v=Tl77rKgX5Fc


// use 320x240 mode
#include "bitbox.h"
#include "blitter.h"
#include "maps.h"

#include <string.h>

// #define SKIP_INTRO

#define SCREEN_X 32
#define SCREEN_Y 30
#define MAX_MONSTER 8 // max nb per screen
#define START_LIVES 5

enum GameState {
	state_zero,
	state_credits,
	state_title,
	state_prestart,
	state_play,
	state_pause,
	state_level_ended
};

enum TileType {
	tile_fall=0,
	tile_block,
	tile_die,
	tile_stair
};

enum frames_player {
	frame_player_idle,
	frame_player_left1,
	frame_player_left2,
	frame_player_jump
	}; // auto from sprite ?

int state, level, lives, swaps, nb_monsters;
int horizontal_symmetry; // if the alterego symmetric horizontally or vertically ?

uint8_t vram[SCREEN_Y][SCREEN_X];

object *bg, *lside, *rside;
object *player, *alterego; // sprites
object *monsters[MAX_MONSTER];

uint8_t monsters_state [MAX_MONSTER];

// individual frames in animations
uint8_t monsters_frame [MAX_MONSTER];
uint8_t player_frame;
uint8_t alterego_frame;


void level_frame( void )
{
	// -- input : left, right, up, down, swap (special frame ?)


	// move guy if possible :
	/*
	si etat=swappe :
	  	continue a swapper jusqu'a la fin
	si etat=tombe :
	    y++
		si dessous = bloque, arret
	sinon : // normal
		calc prochain en fn UDLR
		si dessous=vide :
			etat=tombe
		si next=vide & dessous = bloque :
			avance
	  	si sur une echelle & U/D: next=move

	sur un pont : faire tomber !

	// L/R si prochain ne bloque pas

	// UD si sur un tile echelle
	// tombe si sur un

	// move alterego

	// in_swap ?
	*/





	// check falling, inswap,
	// collision badguy ?


	// -- display ? update vram

}


void enter_title();
void enter_credits();
void enter_play();


// Fade fill vram from a given map in pointer.
// will reset if pass a new map pointer, to continue just pass zero
// will return zero when finished

#define FADE_FRAMES 180 // nb frames to fade (speed)
static int fade_pos=0;
static const uint8_t *fade_map;
void start_fade(const uint8_t *new_map)
{
	fade_pos=0;
	fade_map=new_map;
}

// returns zero if finished
int fade()
{
	uint8_t * const vram_arr = (uint8_t*) vram; // flat view
	if (!fade_map)
		return 0;

 	// continue fading
	for (int i=0;i<(1+SCREEN_X*SCREEN_Y/FADE_FRAMES);i++) { // fill speed : chars / frame to attain a full screen in N frames.
		vram_arr[fade_pos]=fade_map[fade_pos];
		fade_pos += 127; // prime with 960, ie no 2 no 3 factor
		fade_pos %= sizeof(vram);
		if (!fade_pos) {
			fade_map=0;
			break;
		}
	}
	return fade_pos;
}

void do_zero()
{
	static int pause = 60*3;
	if (!pause--)
		enter_credits();
}

void enter_credits()
{
	state=state_credits;
	start_fade(maps_tmap[maps_credits]);
}

void do_credits()
{
	static int pause=3*60; // frames_left

	if (!fade()) { // fading finished ? then pause.
		if (!pause--)
			enter_title();
	}
}

void enter_title() {
	vga_frame=0;
	state = state_title;
	start_fade(maps_tmap[maps_title]); // start fade
}

void do_title()
{
	if (!fade()) {	// fade in finished ?
		if (GAMEPAD_PRESSED(0,start))
			enter_play(0);
		// blink until press START button
		if (vga_frame%128==0) {
			// erase 2 lines
			memset(&vram[22][10],1,12);
			memset(&vram[23][10],1,12);
		} else if (vga_frame%128==64) {
			// copy lines back
			memcpy(&vram[22][10],&maps_tmap[maps_title][22*32+10],12);
			memcpy(&vram[23][10],&maps_tmap[maps_title][23*32+10],12);
		}
	}
}


void display_hud()
//  display lives and swaps left
{
	vram[3][4] = maps_blue_digits+lives;
	vram[3][8] = maps_blue_digits+swaps;
}
const uint8_t monster_tile_state[] = {
	[maps_skull_right]=maps_st_skull_right,
	[maps_skull_left]=maps_st_skull_left,
	[maps_skull_up]=maps_st_skull_up,
	[maps_skull_down]=maps_st_skull_down,
};


const uint8_t level_nb_alter[] = {8,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5};
const uint8_t level_hori[]     = {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1};

void start_level(int new_level)
{
	message("starting level %d\n", level);
	level = new_level;
	swaps = level_nb_alter[level];

	nb_monsters = 0;

	// load game screen to vram
	for (int i=0;i<30;i++)
		for (int j=0;j<32;j++)
		{
			uint8_t c=maps_tmap[maps_map1+level][i*32+j];
			switch(c)
			{
				case maps_player_start :
					// put player here
					vram[i][j]=1; // empty

					player->x = j*8+32;
					player->y = i*8+8-player->h;
					player->fr = 0;

					break;
				case maps_gum_start :
					vram[i][j]=maps_gum; // why ;
					break;

				case maps_skull_left :
				case maps_skull_up :
				case maps_skull_down :
				case maps_skull_right :
					vram[i][j]=1; // empty

					monsters[nb_monsters]->x = 32+j*8;
					monsters[nb_monsters]->y = i*8+8-monsters[nb_monsters]->h;
					monsters_state[nb_monsters] = monster_tile_state[c];
					nb_monsters++;
					break;

				default:
					vram[i][j]=c;
					break;
			}
		}

	// draw swaps / lives guys header
	vram[2][2] = maps_lives;
	vram[3][2] = maps_lives+16;
	vram[3][3] = maps_x;

	vram[2][6] = maps_swaps;
	vram[3][6] = maps_swaps+16;
	vram[3][7] = maps_x;

	horizontal_symmetry = level_hori[level];

	display_hud();
}



void enter_play(int level)
{
	state=state_play;
	lives = 5;
	start_level(level);
}

void animate_tiles()
{
	// scan tiles and animate them.
	for (int i=6;i<25;i++) // 1/4 screen ?
		for (int j=2;j<29;j++)
		{
			uint8_t c=vram[i][j];
			// gums
			if (c>=maps_gum && c<maps_gum+7) {
				vram[i][j]+=1;
			} else if (c==maps_gum+7) {
				vram[i][j] = maps_gum;
			// alter gums
			} else if (c>=maps_alter_gum && c<maps_alter_gum+7) {
				vram[i][j]+=1;
			} else if (c==maps_alter_gum+7) {
				vram[i][j]= maps_alter_gum;
			}

			// water
			// stars
		}
}

void animate_sprites()
{
	// simple animation loops

	// animate monsters
	for (int i=0;i<nb_monsters;i++){
		uint8_t c=monsters_frame[i]++;
		uint8_t s=monsters_state[i];

		monsters[i]->fr = maps_anims[s][c];
		if (monsters[i]->fr==255)
		{
			monsters_frame[i]=0;
			monsters[i]->fr = maps_anims[s][0];
		}
	}
	// player
	// state = depends on buttons pressed !

	player->fr = maps_anims[maps_st_player_idle][player_frame++];
	if (player->fr==255)
	{
		player_frame=0;
		player->fr = maps_anims[maps_st_player_idle][0];
	}

	// alter ego
	alterego->fr = maps_anims[maps_st_alter_alter][alterego_frame++];
	if (alterego->fr==255)
	{
		alterego_frame=0;
		alterego->fr = maps_anims[maps_st_alter_alter][alterego_frame];
	}

}

uint8_t tile_at(object *o, int dx, int dy)
{
	// gets the tile id at the object + dx/dy
	return vram[(o->y+dy)/8][(o->x-32+dx)/8];
}

uint8_t test_at(object *o, int dx, int dy, uint8_t property)
// test tile properties at this position
{
	uint8_t tid = tile_at(o,dx,dy);
	uint8_t prop = maps_tset[maps_tset_attrs_offset+tid];
	return prop & property;
}

void move_alterego()
{
	if (horizontal_symmetry) {
		alterego->x = VGA_H_PIXELS - player->x;
		alterego->y = player->y;
	} else {
		alterego->x = player->x;
		alterego->y = VGA_V_PIXELS - player->y;
	}
}

void move_skulls()
{
	// move skulls : check if canmove (ie y-1 is ok, x next is not blocked), cant go on ? reverse for other types of skulls
	for (int m=0;m<nb_monsters;m++)
		switch (monsters_state[m])
		{
			case maps_st_skull_right :
				monsters[m]->x++; // go right
				// test collide right / rightdown empty with tile properties
				if (
					test_at(monsters[m],8,15,maps_prop_empty) || // right down empty ?
					!test_at(monsters[m],8,0, maps_prop_empty) // collide right ?
					)
					monsters_state[m]=maps_st_skull_left;
				break;
			case maps_st_skull_left :
				monsters[m]->x--; // go left
				if (
					test_at(monsters[m],0,15,maps_prop_empty) || // left down empty ?
					!test_at(monsters[m],0,0, maps_prop_empty) // collide left ?
					)
					monsters_state[m]=maps_st_skull_right;
				break;
			case maps_st_skull_up :
				monsters[m]->y--; // go up
				if (!test_at(monsters[m],0,-1,maps_prop_empty)) // test collide up
					monsters_state[m]=maps_st_skull_down;
				break;
			case maps_st_skull_down :
				monsters[m]->y++; // go down
				if (!test_at(monsters[m],0,8,maps_prop_empty)) // test collide down
					monsters_state[m]=maps_st_skull_up;
				break;
		} // switch
}

void game_init(void)
{
	bg = tilemap_new (maps_tset,0,0,maps_header, vram);
	// initialize with empty, not transparent
	memset(vram,1,sizeof(vram));

	// overscan bars
	lside = rect_new(0,0,32,VGA_V_PIXELS,100,RGB(0,0,0)); // left side overscan
	rside = rect_new(VGA_H_PIXELS-32,0,32,VGA_V_PIXELS,100,RGB(0,0,0)); // right side overscan

	// player=sprite_new();

	bg->x=32; // center since tilemap is 256 pixels wide

	player = sprite_new(maps_sprites[maps_t_player], 0,1024,1); // 0,1024
	alterego = sprite_new(maps_sprites[maps_t_alter], 0,1024,0); // 0,1024

	for (int i=0;i<MAX_MONSTER;i++) {
		monsters[i] = sprite_new(maps_sprites[maps_t_skull], 0,1024,0); // 0,1024
	}

	#ifndef SKIP_INTRO
	state=state_zero;
	#else
	enter_play(14);
	#endif

}


void game_frame(void)
{
	kbd_emulate_gamepad();

	switch (state) {
		case state_zero :
			do_zero();
			break;

		case state_credits:
			do_credits();
			break;

		case state_title:
			do_title();
			break;

		case state_play :
			display_hud();
			// move_player
			move_alterego();
			if (vga_frame %2 == 0)
				move_skulls();

			if (vga_frame % 8 == 0)
				animate_tiles();
			else if (vga_frame % 4==1)
				animate_sprites();
			break;
	}
}

