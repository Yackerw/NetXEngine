//made by root

#include "graphics/graphics.h"
#include "graphics/sprites.h"
#include "graphics/font.h"
#include "autogen/sprites.h"
#include "sound/sound.h"
#include "game.h"
#include "input.h"
#include "Networking.h"
#include "NetPlayer.h"
#include "chat.h"
using namespace Graphics;
using namespace Sprites;

const int ctx = 8; //chat text x
const int cty = 228;// 236;// 176;

const bool showchat = true;

int chatevent;

chatmsg_t chatmsgs[5];
chatstate_t chatstate;

char formatmsg[96];

void Chat_Init() {
	chatevent=Net_RegisterPlayerEventSend(Chat_SendMessage, 64);
	Net_RegisterPlayerEventRecv(Chat_ReceiveMessage, 64);
}

void Chat_SetMessage(char* str, unsigned char flags) {
	//memmove(chatmsgs + sizeof(chatmsg_t), chatmsgs, sizeof(chatmsg_t) * 4);
	memmove(((char*)chatmsgs) + sizeof(chatmsg_t), chatmsgs, sizeof(chatmsg_t) * 4);
	//strcpy((char*)&chatmsgs[0].msg,str);
	//chatmsgs[0].msg = str;
	strcpy(chatmsgs[0].msg, str);
	chatmsgs[0].flags = flags;

	if (chatstate.msgamount < 4) {
		chatstate.msgamount++;
	}
}

char* Chat_SendMessage() {
	char* sendmsg = (char*)calloc(64, sizeof(char));
	strcpy(sendmsg, chatstate.msg);

	return sendmsg;
}

void Chat_ReceiveMessage(unsigned char* msg, int node) {
	unsigned char tempmsgflags = 0;
	formatmsg[0] = 0;

	msg[63] = 0;

	//font_draw(0, 0, (char*)msg);

	if (memcmp("/me", msg, 3) == 0) { //me
		tempmsgflags |= 2;
		//add player name without <>
		strcpy(formatmsg, "PlayerName");
		memmove(msg, &msg[3], CHATMSGSIZE - 3);
		strcat(formatmsg, (char*)msg);
	}
	else { //standard message, standard formatting
		strcpy(formatmsg, "<");
		strcat(formatmsg, "~"); //host prefix
		strcat(formatmsg, "PlayerName");
		strcat(formatmsg, ">");
		strcat(formatmsg, (char*)msg);
	}

	Chat_SetMessage(formatmsg, tempmsgflags);

	chatstate.timer = (60 * 5);
	sound(SND_COMPUTER_BEEP);
}

void Chat_EnterMessage() {
	unsigned char tempmsgflags = 0;
	//char formatmsg[96];
	formatmsg[0] = 0;

	Net_FirePlayerEvent(chatevent);

	//if (chatstate.msg[0] == 0x2F && chatstate.msg[1] == 0x6D && chatstate.msg[2] == 0x65) { //me check
	if (memcmp("/me", chatstate.msg, 3) == 0) { //me
		tempmsgflags |= 2;
		//copy everything past /me to the start (add player name too but for right now just this)
		//memcpy(chatstate.msg, &chatstate.msg[4], 61);
		//add player name without <>
		strcpy(formatmsg, "PlayerName");
		//memcpy(chatstate.msg, &chatstate.msg[3], CHATMSGSIZE - 3);
		memmove(chatstate.msg, &chatstate.msg[3], CHATMSGSIZE - 3);
		strcat(formatmsg, chatstate.msg);
	}
	else { //standard message, standard formatting
		strcpy(formatmsg, "<");
		strcat(formatmsg, "~"); //host prefix
		strcat(formatmsg, "PlayerName");
		strcat(formatmsg, ">");
		strcat(formatmsg, chatstate.msg);
	}

	//if (chatstate.msgamount < 4) {
	Chat_SetMessage(formatmsg, tempmsgflags);

	chatstate.msg[0] = 0;
	chatstate.typing = 0;
	chatstate.timer = (60 * 5);
	sound(SND_COMPUTER_BEEP);
	//Chat_SetMessage((char*)&formatmsg, tempmsgflags);
	//}
	//Sleep(16 * 60);
}

void Chat_AddChar(int ch) {
	unsigned char msglen = strlen(chatstate.msg);
	if (ch != SDLK_DOWN && ch != SDLK_UP && ch != SDLK_LEFT && ch != SDLK_RIGHT && ch != SDLK_RETURN) {
		//enter text
		if (msglen > 0 && ch == 8) {
			chatstate.msg[msglen - 1] = 0;
			msglen -= 1;
		}
		else if (ch != 8 && msglen < 63) {
			strcat(chatstate.msg, (char*)&ch);
		}
	}
	else if (ch == SDLK_RETURN) {
		//pressed enter
		msglen = strlen(chatstate.msg);
		if (msglen == 0) {
			chatstate.typing = 0;
		}
		else {
			Chat_EnterMessage();
		}
	}
}

void Chat_Step() {
	if (chatstate.timer != 0) {
		chatstate.timer -= 1;
	}
	/*unsigned char i = 0;
	while (i < CHATMSGSIZE) {
		chatstate.msg[i] = 0;
		i++;
	}
	chatstate.msg[0] = 65 + (rand() % 27);*/

	/*if (rand() % 10 < 5) {
		strcpy(chatstate.msg, "/me test");
	}
	else {
		strcpy(chatstate.msg, "memes");
	}*/
	//format message
}

void Chat_Display() {
	//chatstate.timer = 69;
	//chatstate.msgamount = 0;
	//chatstate.typing = 1;
	Chat_Step();

	unsigned char chatyoff = (64 * textbox.IsVisible()); //move chat up when a tsc textbox is open
	if (chatstate.timer >= 1 || chatstate.typing) {
		/*chatmsgs[0].msg = "test";
		chatmsgs[1].msg = "test2";
		chatmsgs[2].msg = "test3";
		chatmsgs[3].msg = "test4";
		chatmsgs[4].msg = "test69xd";*/
		uint32_t msgcol = 16777215U;


		unsigned char i = 0;
		while (i < 5) {
			msgcol = 16777215U;
			if (chatmsgs[i].msg != NULL) {
				if (chatmsgs[i].flags > 0) { //if theres any more flags change this to & 1 or & 2 for server/me
					msgcol = 0xFFFFFF00; //yellow
				}
				//font_draw(ctx, cty + (12 * i), chatmsgs[i].msg);
				//font_draw(ctx, (cty-14*chatstate.typing) - (12 * chatstate.msgamount) + (12 * i), chatmsgs[i].msg, msgcol,true);
				//font_draw(ctx, (cty - 14 * chatstate.typing) - (12 * i), chatmsgs[i].msg, msgcol, true);
				font_draw(ctx, (cty - (64*textbox.IsVisible()) - 14 * chatstate.typing) - (12 * i), chatmsgs[i].msg, msgcol, true);
			}
			i++;
		}
	}
	if (chatstate.typing) {
		DrawLine(ctx, cty + 12 - chatyoff, ctx + (5 * strlen(chatstate.msg))+5, cty + 12 - chatyoff, 0x00000000);//16777215U);
		font_draw(ctx, cty - chatyoff, (char*)&chatstate.msg, 0xFFFFFF00, true);
	}
}