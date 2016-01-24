// alterego
// see gameplay : https://www.youtube.com/watch?v=Tl77rKgX5Fc

/*
todo :

phantoms must walk on ladders

succeed game

Falling ground : sprite

fade out ? (if zero:ok, sinon darken bits.)
*/

// use 320x240 mode
#include "bitbox.h"
#include "blitter.h"
#include "maps.h"
#include "chiptune.h"

#include <string.h>

extern struct ChipSong alter_chipsong;

// define to immediately start at a level
// #define START_LEVEL 3

#define SCREEN_X 32
#define SCREEN_Y 30
#define MAX_MONSTER 32 // max nb sprites per screen
#define START_LIVES 5
#define NB_LEVELS 25

enum GameState {
	state_zero,
	state_credits,
	state_title,
	state_play,
	state_pause,
	state_swap,
	state_level_ended,
	state_prestart,
	state_game_over,
};

int state, level, lives, swaps, nb_monsters, gums;
int pause;
int16_t target_swap; // target X or Y to swap
int horizontal_symmetry; // if the alterego symmetric horizontally or vertically ?

uint16_t old_gamepad;
uint16_t gamepad_pressed;

uint8_t vram[SCREEN_Y][SCREEN_X];

object *bg, *lside, *rside;
object *player, *alterego; // sprites
object *monsters[MAX_MONSTER];

uint8_t monsters_state [MAX_MONSTER];

// individual frames in animations
uint8_t monsters_frame [MAX_MONSTER];
uint8_t player_frame;
uint8_t player_state;
uint8_t alterego_frame;


void enter_title();
void enter_credits();
void enter_play();

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
	if (!pause--)
		enter_credits();
}

void enter_credits()
{
	state=state_credits;
	start_fade(maps_tmap[maps_credits]);
	pause=5*60;
}

void do_credits()
{
	if (!fade()) { // fading finished ? then pause.
		if (!pause--)
			enter_title();
	}
}

void enter_title() {
	state = state_title;
	start_fade(maps_tmap[maps_title]); // start fade
	chip_play(&alter_chipsong);
}

void do_title()
{
	if (!fade()) {	// fade in finished ?
		if (gamepad_pressed & gamepad_start) {
			enter_play(0);
		}


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
	//	vram[3][10] = maps_blue_digits+gums;
}

void new_sprite(int y,int x,uint8_t tid)
{
	// instantiates a sprite from the tile i,j on the map

	// tile_id to sprite_state
	static const uint8_t monster_tile_state[] = {
		[maps_skull_right]=maps_st_skull_right,
		[maps_skull_left]=maps_st_skull_left,
		[maps_skull_up]=maps_st_skull_up,
		[maps_skull_down]=maps_st_skull_down,

		[maps_gum_start]=maps_st_gum_idle,
		[maps_alter_gum_start]=maps_st_alter_gum_idle,
		[maps_falling_start]=maps_st_falling_falling,
	};
	uint8_t sp_state= monster_tile_state[tid];

	monsters_state[nb_monsters] = sp_state;
	monsters[nb_monsters] = sprite_new(
		maps_sprites[maps_objects_types[sp_state]],
		32+x*8,
		y*8+8,
		0);

	// height known after loading
	monsters[nb_monsters]->y -= monsters[nb_monsters]->h;

	nb_monsters++;

	vram[y][x]=1; // empty
}


void clear_sprites(void)
{
	// unload preceding level
	for (int i=0;i<nb_monsters;i++)
	{
		blitter_remove(monsters[i]);
	}
	nb_monsters=0;

	// hide player / alterego
	player->y=1024;
	alterego->y=1024;

}


void start_level(int new_level)
{
	level = new_level;

	gums=0;

	chip_play(0); // stop song

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
				case maps_alter_gum_start :
					gums++;
					new_sprite(i,j,c);
					break;

				case maps_skull_left :
				case maps_skull_up :
				case maps_skull_down :
				case maps_skull_right :
					new_sprite(i,j,c);
					break;

				case maps_falling_start : // don't do anything now
				default:
					vram[i][j]=c;
					break;
			}
		}
	swaps = vram[0][0]-maps_blue_digits;
	horizontal_symmetry = (vram[0][1]==112)?0:1; // 112 = V

	vram[0][0]=1;vram[0][1]=1;

	// draw swaps / lives guys header
	vram[2][2] = maps_lives;
	vram[3][2] = maps_lives+16;
	vram[3][3] = maps_x;

	vram[2][6] = maps_swaps;
	vram[3][6] = maps_swaps+16;
	vram[3][7] = maps_x;

	player_state = maps_st_player_idle;

	display_hud();
}

void enter_play(int level)
{
	state=state_play;
	start_level(level);
}


void animate_sprites()
{
	// simple animation loops.

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

	player->fr = maps_anims[player_state][player_frame++];
	if (player->fr==255)
	{
		player_frame=0;
		player->fr = maps_anims[player_state][0];
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

void set_at(object *o, int dx, int dy, uint8_t tid)
{
	vram[(o->y+dy)/8][(o->x-32+dx)/8]=tid;
}

uint8_t test_at(object *o, int dx, int dy, uint8_t property)
// test tile properties at this position
{
	uint8_t tid = tile_at(o,dx,dy);
	uint8_t prop = maps_tset[maps_tset_attrs_offset+tid];
	return prop & property;
}

void die_level(void);


#define CAN_WALK (maps_prop_empty | maps_prop_ladder | maps_prop_gum)
void move_player()
{
	uint8_t prev_state=player_state;


	if (gamepad_pressed & gamepad_start) {
		// start a pause
		state=state_pause;
	} else if (test_at(player,0,10, maps_prop_empty) && test_at(player,7,10, maps_prop_empty)) {
		// if nothing under : fall
		player->y++;
		player_state=maps_st_player_falling;
	} else if (gamepad_pressed & gamepad_A) {
		// starts a swap
		if ( swaps && test_at(alterego,0,4,CAN_WALK ) && test_at(alterego,7,4,CAN_WALK ) ) {
			swaps--;
			target_swap = horizontal_symmetry ? alterego->x : alterego->y;
			state=state_swap;
			chip_note(3,128,1);
		} else {
			// XXX SFX fail

		}
	} else if (GAMEPAD_PRESSED(0,left)) {
		player_state=maps_st_player_left;
		if (test_at(player,-1,4, CAN_WALK )) {
			// test if we're changing tiles and we're on a falling element  : transform into falling sprite
			if (player->x%8==0 && tile_at(player,4,14)==maps_falling_start) {
				// initiate fall animation
				set_at(player,4,14,1);
			}

			player->x--;
		}
	} else if (GAMEPAD_PRESSED(0,right)) {
		player_state=maps_st_player_right;
		if (test_at(player,+8,4, CAN_WALK )) {
			// test if we're changing tiles and we're on a falling element  : transform into falling sprite
			if (player->x%8==7 && tile_at(player,0,14)==maps_falling_start) {
				// initiate fall animation
				set_at(player,0,14,1);
			}

			player->x++;
		}
	} else if (GAMEPAD_PRESSED(0,up)   \
		&& (test_at(player,0,9,maps_prop_ladder ) \
			|| test_at(player,7,9,maps_prop_ladder ))
		) {
		player->y--;
		player_state=maps_st_player_ladder;
	} else if (GAMEPAD_PRESSED(0,down) \
			&& (test_at(player,0,11,maps_prop_ladder) \
				|| test_at(player,7,11,maps_prop_ladder))
			) {
		player->y++;
		player_state=maps_st_player_ladder;
	} else {
		player_state = maps_st_player_idle;
	}


	// touching tiles
	if (test_at(player,0,14,maps_prop_water)) {
		die_level();
	}

	// reset animation if changed state
	if (prev_state != player_state)
		player_frame = 0;

	if (player->y >= 208 && player->y<1024) // 1024 is just hidden sprite
		die_level();

}

void move_alterego()
{
	if (horizontal_symmetry) {
		alterego->x = VGA_H_PIXELS - player->x - 4;
		alterego->y = player->y;
	} else {
		alterego->x = player->x;
		alterego->y = VGA_V_PIXELS - player->y - 4;
	}
}

void move_skulls()
{
	// move skulls : check if canmove (ie y-1 is ok, x next is not blocked), cant go on ? reverse for other types of skulls
	for (int m=0;m<nb_monsters;m++)
		switch (monsters_state[m])
		{

			// skulls
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
			default :
				break;
		} // switch
}

void move_swap()
{
	if (horizontal_symmetry) {
		if (player->x == target_swap)
			state=state_play;
		else
			player->x += target_swap>player->x ? 1 : -1;
	} else {
		if (player->y == target_swap)
			state=state_play;
		else
			player->y += target_swap>player->y ? 1 : -1;
	}

}

// standard AABB collision check
int collide(object *oa, object *ob)
{
    if (oa->x+oa->w < ob->x ||
        oa->y+oa->h < ob->y ||
        oa->x > ob->x+ob->w ||
        oa->y > ob->y+ob->h) {
        return 0;
    } else {
    	return 1;
    }
}


void finish_level()
{
	clear_sprites();
	// next level !
	level+=1;
	state=state_prestart;
	chip_note(3,128,5);
	pause=120;
}

void die_level(void)
{
	lives--;
	chip_note(3,128,6); // sfx die
	clear_sprites();
	// test game over ?
	if (lives) {
		pause = 120;
		state = state_prestart;
	} else {
		// enter game over
		memcpy(vram, maps_tmap[maps_gameover],sizeof(vram));
		pause=240;
		state = state_game_over;
	}
}

void do_game_over(void)
{
	if (!pause--)
		enter_title();
}

void do_collide_player()
{
	// test collisions with all monsters
	for (int i=0;i<nb_monsters;i++)
		if (collide(player, monsters[i])) {
			switch (monsters_state[i]) {
				case maps_st_skull_up :
				case maps_st_skull_down :
				case maps_st_skull_left :
				case maps_st_skull_right :
					die_level();
					break;
				case maps_st_gum_idle :
					// hide it
					monsters[i]->y=1024;
					gums--;
					chip_note(3,128,7); // gum

					break;
			}
			break;
		}
}


void do_collide_alter()
{
	// only test altergums
	for (int i=0;i<nb_monsters;i++)
		if (
			monsters_state[i] == maps_st_alter_gum_idle \
			&& collide(alterego, monsters[i])
			)
			{
				// hide it
				monsters[i]->y=1024;
				gums--;
				chip_note(3,128,7); // gum
				break;
			}
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

	lives = START_LIVES;

	#ifndef START_LEVEL
	state=state_zero;
	pause = 60*3;
	#else
	enter_play(START_LEVEL-1);
	#endif
}


void game_frame(void)
{
	kbd_emulate_gamepad();
	gamepad_pressed = gamepad_buttons[0] & ~old_gamepad;
	old_gamepad = gamepad_buttons[0];

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

		case state_pause:
			// write PAUSE or empty
			for (int i=0;i<5;i++)
				vram[3][10+i] = (vga_frame/32)%2 ? (char[]){144,129,111,109,133}[i]: 1;
			if (gamepad_pressed & gamepad_start) {
				for (int i=0;i<5;i++)
					vram[3][10+i]=1;
				state=state_play;
			}
			break;

		case state_swap:
			move_swap();
			move_swap();
			move_swap();
			move_alterego();
			break;

		case state_play :
			display_hud();
			move_player();
			move_alterego();

			switch(vga_frame%4) {
				case 0 :
				case 2 :
					move_skulls();
					break;
				case 1 :
					do_collide_alter();
					animate_sprites();
					break;
				case 3 :
					do_collide_player();
			}

			if (!gums) {
				finish_level();
			}
			break;

		case state_prestart :
			if (!pause--)
				enter_play(level); //  restart current level
			break;

		case state_game_over :
			do_game_over();
	}
}

