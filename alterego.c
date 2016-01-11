// alterego
// see gameplay : https://www.youtube.com/watch?v=Tl77rKgX5Fc


// use 320x240 mode
#include "bitbox.h"
#include "blitter.h"
#include "maps.h"

#include <string.h>

#define SCREEN_X 32
#define SCREEN_Y 30
#define MAX_MONSTER 8 // max nb per screen
#define START_LIVES 5

enum GameState { state_credits, state_title, state_prestart, state_play, state_pause, state_level_ended };
enum TileType { tile_fall=0, tile_block, tile_die, tile_stair};

enum frames_player { frame_player_idle, frame_player_left1, frame_player_left2, frame_player_jump }; // auto from sprite ?

int state, level, lives, swaps, nb_monsters;

uint8_t vram[SCREEN_Y][SCREEN_X];

object *bg, *lside, *rside, *player, *alterego;
object *monsters[MAX_MONSTER];


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

	// -- game logic
	// move skulls : check if canmove (ie y-1 is ok, x next is not blocked), cant go on ? reverse for other types of skulls
	/*
	uint8_t *next_tile;
	uint16_t *coord; //  movement coordinate
	int16_t speed; // speed step

	for (int m=0;m<nb_monsters;m++) {

		speed = frame&4?1:-1; // 0,8 : speed : +1 - 4,C : speed = -1

		// axis depends on frame & 8
		if (frame & 8 == 0) {  // 0,4 : coord = x ; 8,C coord = y
			coord = &player->x;
			next_tile = &vram[player->y/8][(player->x+speed)/8]; // NON coords
		} else {
			coord = &player->y;
			next_tile = &vram[(player->y+speed)/8][player->x/8]; // NON coords
		}

		if (tile_type[*next_tile]==tile_fall && tile_type[*(next_tile+SCREEN_X)]==tile_block) {
			*coord += speed; // bouge vraiment
		} else {
			// invert move  : 0->4 4->0 8->C C->8  : invert bit 2 of frame_id
			player->fr ^= 4;
		}
	}

*/



	// check falling, inswap,
	// collision badguy ?


	// -- display ? update vram

}


void enter_title();
void enter_credits();
void enter_play();


void game_init(void)
{
	bg = tilemap_new (maps_tset,0,0,maps_header, vram);

	// overscan bars
	lside = rect_new(0,0,32,VGA_V_PIXELS,100,RGB(0,0,0)); // left side overscan
	rside = rect_new(VGA_H_PIXELS-32,0,32,VGA_V_PIXELS,100,RGB(0,0,0)); // right side overscan

	// player=sprite_new();

	bg->x=32; // center since tilemap is 256 pixels wide

	enter_credits();
}


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



void enter_credits()
{
	state=state_credits;
	start_fade(maps_tmap[maps_credits]);
}

void do_credits()
{
	static int pause=2*60; // frames_left

	if (!fade()) { // fading finished ?
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
			enter_play();
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

const uint8_t level_nb_alter[] = {5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5};
void start_level(int new_level)
{
	message("starting level %d\n", level);
	level = new_level;
	swaps = level_nb_alter[level];


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
					/*
					player->x = j*8+32;
					player->y = i*8;
					player->fr = frame_player_idle;
					*/
					break;
				case maps_gum_start :
					vram[i][j]=maps_gum; // why ;
					break;

/*
				case maps_skull_right :
				case maps_skull_left :
				case maps_skull_up :
				case maps_skull_down :
					monster[nb_monsters]->x = i*8+4;
					monster[nb_monsters]->y = j*8+4
					monster[nb_monsters]->fr = 4*vram[j][i]-maps_skull_right; // 0,4,8, or c
					nb_monsters++;

					vram[i][j] =1;
					break;
*/
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

	display_hud();
}



void enter_play()
{
	state=state_play;
	lives = 5;
	start_level(0);
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

void game_frame(void)
{
	kbd_emulate_gamepad();

	switch (state) {
		case state_credits:
			do_credits();
			break;

		case state_title:
			do_title();
			break;

		case state_play :
			if (vga_frame % 8 == 0)
				animate_tiles();
			break;
	}
}

