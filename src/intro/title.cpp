#include "../nx.h"
#include "title.h"
#include "../profile.h"
#include "../map.h"
#include "../statusbar.h"
#include "../input.h"
#include "../niku.h"
#include "../graphics/font.h"
#include "../sound/sound.h"
#include "../common/stat.h"
#include "../TextBox/TextBox.h"

#include "../settings.h"
#include "../graphics/graphics.h"
#include "../graphics/sprites.h"
#include "../autogen/sprites.h"
#include "Networking.h"
#include "NetPlayer.h"
#include "chat.h"
using namespace Graphics;
using namespace Sprites;


// music and character selections for the different Counter times
static struct
{
	uint32_t timetobeat;
	int sprite;
	int songtrack;
} titlescreens[] =
{
	{(3*3000),	SPR_CS_SUE,    2},		// 3 mins	- Sue & Safety
	{(4*3000),	SPR_CS_KING,   41},		// 4 mins	- King & White
	{(5*3000),	SPR_CS_TOROKO, 40},		// 5 mins	- Toroko & Toroko's Theme
	{(6*3000),	SPR_CS_CURLY,  36},		// 6 mins	- Curly & Running Hell
	{0xFFFFFFFF, SPR_CS_MYCHAR, 24}		// default
};

// artifical fake "loading" delay between selecting an option and it being executed,
// because it actually doesn't look good if we respond instantly.
#define SELECT_DELAY			30
#define SELECT_LOAD_DELAY		20		// delay when leaving the multisave Load dialog
#define SELECT_MENU_DELAY		8		// delay from Load to load menu

// We're going to group in multiplayer menu with the normal title screen, since it's going to re-use most of its code anyways
char Multiplayer = 0;
char IPAddress[128] = "127.0.0.1";

static struct
{
	int sprite;
	int cursel;
	int selframe, seltimer;
	int selchoice, seldelay;
	int kc_pos;
	bool in_multiload;
	
	uint32_t besttime;		// Nikumaru display
} title;


static void draw_title()
{
	// background is dk grey, not pure black
	ClearScreen(0x20, 0x20, 0x20);
	map_draw_backdrop();
	DrawFastLeftLayered();
	
	// top logo
	int tx = (SCREEN_WIDTH / 2) - (sprites[SPR_TITLE].w / 2) - 2;
	draw_sprite(tx, 40, SPR_TITLE);
	
	// draw menu

	int cx = (SCREEN_WIDTH / 2) - 32;
	int cy = (SCREEN_HEIGHT / 2) + 8;

	// TODO: change to switch statement
	if (Multiplayer == 0) {
		//show if its hosting
		const char* multitx;
		if (Host != -1) {
			multitx = "----";
		}
		else {
			multitx = "Multiplayer";
		}
		const char* mymenus[] = { "New game","Load game", "Options", multitx, "Quit" };

		if (Host == 1) {
			font_draw(cx + 10, cy - 32, "(Hosting!)");
		}

		TextBox::DrawFrame(cx - 32, cy - 16, 128, 80);

		for (int i = 0; i <= 4; i++)
		{
			font_draw(cx + 10, cy - 8, _(mymenus[i]));
			if (i == title.cursel)
			{
				draw_sprite(cx - 16, cy - 9, title.sprite, title.selframe);
			}

			cy += 12;
		}
	}
	if (Multiplayer == 1) {
		const char* mymenus[] = { "Host", "Connect", "IP", "Skin", "Name" };

		TextBox::DrawFrame(cx - 32, cy - 16, 128, 80);

		for (int i = 0; i <= 4; i++)
		{
			font_draw(cx + 10, cy - 8, (mymenus[i]));
			if (i == title.cursel)
			{
				draw_sprite(cx - 16, cy - 9, title.sprite, title.selframe);
			}

			cy += 12;
		}
	}
	if (Multiplayer == 2) {
		const char *mymenu = "Please input IP:";

		font_draw(cx + 10, cy - 16, mymenu);

		font_draw(cx + 10, cy, IPAddress);
	}
	if (Multiplayer == 3) { //skin menu
		//TextBox::DrawFrame(cx - 64, cy - 16, 160, 80);
		TextBox::DrawFrame(cx - 32, cy - 16, 128, 80);
		font_draw(cx, cy - 8, "Select a skin");
		font_draw(cx-2, cy+16, "<");
		font_draw(cx + 62, cy+16, ">");
		if (player->skin == 0) { //replace this later i guess
			//draw_sprite(cx + 24, cy + 16, title.sprite, title.selframe);
			draw_sprite(cx + 24, cy + 16, SPR_MYCHAR, title.selframe&1);
		}
		else {
			//draw_sprite(cx + 24, cy + 16, ((SPR_MYCHAR)) + player->skin, title.selframe);
			draw_sprite(cx + 24, cy + 16, (SPR_CURLYCHAR - 1) + player->skin, title.selframe&1);
		}
		const char* skinnames[] = { "Quote","Curly","Sue","King","Jack","Colon","Booster","Demon Crown","Root The Cat","Suguri","Sora" };
		font_draw(cx + 27 - (strlen(skinnames[player->skin])*2), cy + 46, skinnames[player->skin]);
		//draw_sprite(cx+24,cy+16, title.sprite+title.cursel, title.selframe);
	}
	if (Multiplayer == 4) {
		const char *mymenu = "Please input name:";

		font_draw(cx - 10, cy - 16, mymenu);
		//strcpy((char*)&names[0], "CritrSlayer69");
		font_draw(cx + 10, cy, name);
	}
	if (Multiplayer == 6) {
		Sock = ClientCreate(IPAddress, 5029);
		if (Sock != NULL) {
			_beginthread(Receive_Data, 1024, NULL);
			while (ClientNode == -1) {
				Sleep(16);
			}
			Host = 0;
			Net_FirePlayerEvent(PlayerSkinUpdateEvent);

			//set name to "Player" if empty
			if (name[0] == 0) {
				strcpy(name, "Player");
			}
			Net_FirePlayerEvent(nameevent);
			Multiplayer = 8;
		}
		else {
			Multiplayer = 7;
		}
	}
	if (Multiplayer == 5) {
		font_draw(160 - 32, 120, "Please wait..");
		Multiplayer = 6;
	}
	if (Multiplayer == 7) {
		font_draw(160 - 32, 120, "Failed to connect!");
	}

	// animate character
	if (++title.seltimer > 8)
	{
		title.seltimer = 0;
		if (++title.selframe >= sprites[title.sprite].nframes)
			title.selframe = 0;
	}
	
	// accreditation
	cx = (SCREEN_WIDTH / 2) - (sprites[SPR_PIXEL_FOREVER].w / 2);
	int acc_y = SCREEN_HEIGHT - 48;
	draw_sprite(cx, acc_y, SPR_PIXEL_FOREVER);
	
	// version
	int wd = GetFontWidth(NXVERSION);
	cx = (SCREEN_WIDTH / 2) - (wd / 2);
	font_draw(cx, acc_y + sprites[SPR_PIXEL_FOREVER].h + 4, NXVERSION, 0xf3e298);
	
	// draw Nikumaru display
	if (title.besttime != 0xffffffff)
		niku_draw(title.besttime, true);
}



static int kc_table[] = { UPKEY, UPKEY, DOWNKEY, DOWNKEY,
						  LEFTKEY, RIGHTKEY, LEFTKEY, RIGHTKEY, -1 };

void run_konami_code()
{
	if (justpushed(UPKEY) || justpushed(DOWNKEY) || \
		justpushed(LEFTKEY) || justpushed(RIGHTKEY))
	{
		if (justpushed(kc_table[title.kc_pos]))
		{
			title.kc_pos++;
			if (kc_table[title.kc_pos] == -1)
			{
				sound(SND_MENU_SELECT);
				title.kc_pos = 0;
			}
		}
		else
		{
			title.kc_pos = 0;
		}
	}
}

static void handle_input()
{

	// Input IP Address in multiplayer

	if (justpushed(DOWNKEY))
	{
		sound(SND_MENU_MOVE);
		if (Multiplayer == 0 && ++title.cursel >= 5)
			title.cursel = 0;

		if (Multiplayer == 1 && ++title.cursel >= 5)
			title.cursel = 0;
	}
	else if (justpushed(UPKEY))
	{
		sound(SND_MENU_MOVE);
		if (Multiplayer == 0 && --title.cursel < 0)
			title.cursel = 4;

		if (Multiplayer == 1 && --title.cursel < 0)
			title.cursel = 4;
	}
	else if (justpushed(LEFTKEY)) { //skin select
		/*if (Multiplayer == 3 && player->skin == 0) {
			sound(SND_MENU_MOVE);
			player->skin = 1;
		}*/
		if (Multiplayer == 3) {
			sound(SND_MENU_MOVE);
			player->skin -= 1;
			if (player->skin < 0) {
				player->skin = numskins;
			}
		}

	}
	else if (justpushed(RIGHTKEY)) { //skin select
		/*if (Multiplayer == 3 && player->skin == 1) {
			sound(SND_MENU_MOVE);
			player->skin = 0;
		}*/
		if (Multiplayer == 3) {
			sound(SND_MENU_MOVE);
			player->skin += 1;
			if (player->skin > numskins) {
				player->skin = 0;
			}
		}
	}
	
	if (buttonjustpushed() || justpushed(ENTERKEY))
	{
		sound(SND_MENU_SELECT);
		int choice = title.cursel;

		if (Multiplayer == 1) {
			// Go back if it was the shoot key
			if (justpushed(FIREKEY)) {
				Multiplayer = 0;
				choice = 40;
				title.cursel = 3;
			}
			choice += 20;
		}

		if (Multiplayer == 2) {
			// If we're in the IP Menu, then return
			Multiplayer = 1;
			choice = 40;
		}

		if (Multiplayer == 3) {
			Multiplayer = 1;
			title.cursel = 3;
			choice = 50;
		}

		if (Multiplayer == 4) {
			if (justpushed(ENTERKEY)) {
				Multiplayer = 1;
				title.cursel = 4;
			}
			choice = 40;
		}

		if (Multiplayer == 7) {
			Multiplayer = 1;
			choice = 40;
		}
		
		// handle case where user selects Load but there is no savefile,
		// or the last_save_file is deleted.
		if (title.cursel == 1 && Multiplayer == false)
		{
			if (!ProfileExists(settings->last_save_slot))
			{
				bool foundslot = false;
				for(int i=0;i<MAX_SAVE_SLOTS;i++)
				{
					if (ProfileExists(i))
					{
						stat("Last save file %d missing. Defaulting to %d instead.", settings->last_save_slot, i);
						settings->last_save_slot = i;
						foundslot = true;
					}
				}
				
				// there are no save files. Start a new game instead.
				if (!foundslot)
				{
					stat("No save files found. Starting new game instead.");
					choice = 0;
				}
			}
		}
		
		if (choice == 1 && settings->multisave)
		{
			title.selchoice = 10;
			title.seldelay = SELECT_MENU_DELAY;
		}
		else
		{
			title.selchoice = choice;
			if (choice==0) title.seldelay = SELECT_DELAY;
			else title.seldelay = 1;
//			music(0);
		}
	}
	
	run_konami_code();
}

static void selectoption(int index)
{
	switch(index)
	{
		case 0:		// New
		{
			music(0);
			
			game.switchstage.mapno = NEW_GAME_FROM_MENU;
			game.setmode(GM_NORMAL);
			pskin = player->skin;
		}
		break;
		
		case 1:		// Load
		{
			music(0);
			
			game.switchstage.mapno = LOAD_GAME_FROM_MENU;
			game.setmode(GM_NORMAL);
		}
		break;

		case 2:		// Options
		{
//			music(0);
			game.pause(GP_OPTIONS);
		}
		break;
		case 3:		// Multiplayer
		{
			if (Host == -1) {
				Multiplayer = 1;
				title.cursel = 0;
			}
		}
			break;
		case 4:		// Quit
		{
			music(0);
			game.running = false;
		}
		break;

		case 10:		// Load Menu (multisave)
		{
			textbox.SetVisible(true);
			textbox.SaveSelect.SetVisible(true, SS_LOADING);
			title.in_multiload = true;
		}
		break;
		case 20: {		// Start hosting
			Sock = Server_Create(5029);
			_beginthread(Receive_Data, 1024, NULL);
			Host = 1;
			Multiplayer = 0;

			//set name to "~Host" if empty
			if (name[0] == 0) {
				strcpy(name, "Host");
			}
		}
		break;
		case 21: {		// Connect to server
			Multiplayer = 5;
		}
		break;
		case 22: {		// Bring us to IP input screen
			Multiplayer = 2;
		}
		break;
		case 23: {	//skins
			Multiplayer = 3; //rock my forum
		}
		break;
		case 24: {	//name
			Multiplayer = 4;
			//font_draw(0, 0, "xXCritterSlayah69Xx");
		}
				 break;
	}
}



bool title_init(int param)
{
	memset(&title, 0, sizeof(title));
	game.switchstage.mapno = 0;
	game.switchstage.eventonentry = 0;
	game.showmapnametime = 0;
	textbox.SetVisible(false);
	
	if (niku_load(&title.besttime))
		title.besttime = 0xffffffff;
	
	// select a title screen based on Nikumaru time
	int t;
	for(t=0;;t++)
	{
		if (title.besttime < titlescreens[t].timetobeat || \
			titlescreens[t].timetobeat == 0xffffffff)
		{
			break;
		}
	}
	
	title.sprite = titlescreens[t].sprite;
	music(titlescreens[t].songtrack);
	
	if (AnyProfileExists())
		title.cursel = 1;	// Load Game
	else
		title.cursel = 0;	// New Game
	
	return 0;
}

void title_tick()
{
	if (!title.in_multiload)
	{
		if (title.seldelay > 0)
		{
			ClearScreen(BLACK);
			
			title.seldelay--;
			if (!title.seldelay)
				selectoption(title.selchoice);
			
			return;
		}
		
		handle_input();
		draw_title();
	}
	else
	{
		ClearScreen(BLACK);
		
		if (!textbox.SaveSelect.IsVisible())
		{	// selection was made, and settings.last_save_slot is now set appropriately
			
			sound(SND_MENU_SELECT);
			
			textbox.SetVisible(false);
			title.selchoice = 1;
			title.seldelay = SELECT_LOAD_DELAY;
			title.in_multiload = false;
		}
		else
		{
			textbox.Draw();
		}
	}
}


/*
void c------------------------------() {}
*/



