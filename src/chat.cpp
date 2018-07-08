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
#include "stdio.h"
using namespace Graphics;
using namespace Sprites;

FILE* chatlogfile;

const int ctx = 8; //chat text x
const int cty = 224;// 236;// 176;

const bool showchat = true;

int chatevent;

chatmsg_t chatmsgs[5];
chatstate_t chatstate;

char formatmsg[96];

void Chat_WriteToLog(char* str) {
	if (chatlogfile == NULL) {
		chatlogfile = fopen("chatlog.txt", "a");
	}
	if (chatlogfile != NULL && str != NULL) {
		fwrite(str, sizeof(char), strlen(str), chatlogfile);
		const unsigned char lineending[] = { 13,10 };
		fwrite(&lineending, sizeof(unsigned char), 2, chatlogfile); //yeah i know..but not everyone uses the TECHNOLOGICALLY ADVANCED windows 10 version of notepad that supports proper line endings (this means me)
		fclose(chatlogfile);
		chatlogfile = NULL;
	}
}


void Chat_Init() {
	chatevent=Net_RegisterPlayerEventSend(Chat_SendMessage, 64);
	Net_RegisterPlayerEventRecv(Chat_ReceiveMessage, 64);

	//open chatlog file
	chatlogfile = fopen("chatlog.txt", "w");
	const char logwarning[] = "(This file gets overwritten every time you start the game. Be sure to save any chatlogs you might want to keep!)";
	Chat_WriteToLog((char*)&logwarning);
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
	char* sendmsg = (char*)malloc(64);
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
		//strcpy(formatmsg, "PlayerName");
		strcpy(formatmsg, names[node]);
		memmove(msg, &msg[3], CHATMSGSIZE - 3);
		strcat(formatmsg, (char*)msg);
	}
	else { //standard message, standard formatting
		strcpy(formatmsg, "<");
		if (node == CliNum && host != 1) {
			strcat(formatmsg, "~"); //host prefix
		}
		strcat(formatmsg, names[node]);
		strcat(formatmsg, ">");
		strcat(formatmsg, (char*)msg);
	}

	Chat_SetMessage(formatmsg, tempmsgflags);

	Chat_WriteToLog(formatmsg);

	chatstate.timer = (60 * 5);
	sound(SND_COMPUTER_BEEP);
}

void Chat_EnterMessage() {
	unsigned char tempmsgflags = 0;
	//char formatmsg[96];
	formatmsg[0] = 0;

	if (memcmp("/ban ", chatstate.msg, 5) == 0 && host == 1) {
		char IP[32];
		char msg[64];
		memcpy(msg, chatstate.msg + 5, 59);
		int i = 0;
		while (i < MAXCLIENTS) {
			if (sockets[i].used) {
				InetNtopA(sockets[i].data.sin_family, &sockets[i].data.sin_addr, IP, 32);
				if (strcmp(msg, IP) == 0) {
					strcpy(banlist[bannum], IP);
					closesocket(sockets[i].sock);
					chatstate.msg[0] = 0;
					chatstate.typing = 0;
					chatstate.timer = (60 * 5);
					bannum++;
					return;
				}
			}
			i++;
		}
		chatstate.msg[0] = 0;
		chatstate.typing = 0;
		chatstate.timer = (60 * 5);
		return;
	}

	if (memcmp("/kick ", chatstate.msg, 6) == 0 && host == 1) {
		char IP[32];
		char msg[64];
		memcpy(msg, chatstate.msg + 6, 59);
		int i = 0;
		while (i < MAXCLIENTS) {
			if (sockets[i].used) {
				InetNtopA(sockets[i].data.sin_family, &sockets[i].data.sin_addr, IP, 32);
				if (strcmp(msg, IP) == 0) {
					closesocket(sockets[i].sock);
					chatstate.msg[0] = 0;
					chatstate.typing = 0;
					chatstate.timer = (60 * 5);
					return;
				}
			}
			i++;
		}
		chatstate.msg[0] = 0;
		chatstate.typing = 0;
		chatstate.timer = (60 * 5);
		return;
	}

	Net_FirePlayerEvent(chatevent);

	//if (chatstate.msg[0] == 0x2F && chatstate.msg[1] == 0x6D && chatstate.msg[2] == 0x65) { //me check
	if (memcmp("/me", chatstate.msg, 3) == 0) { //me
		tempmsgflags |= 2;
		//copy everything past /me to the start (add player name too but for right now just this)
		//memcpy(chatstate.msg, &chatstate.msg[4], 61);
		//add player name without <>
		//strcpy(formatmsg, "PlayerName");
		strcpy(formatmsg, name);
		//memcpy(chatstate.msg, &chatstate.msg[3], CHATMSGSIZE - 3);
		memmove(chatstate.msg, &chatstate.msg[3], CHATMSGSIZE - 3);
		strcat(formatmsg, chatstate.msg);
	}
	else { //standard message, standard formatting
		strcpy(formatmsg, "<");
		if (host == 1) {
			strcat(formatmsg, "~"); //host prefix
		}
		strcat(formatmsg, name);
		strcat(formatmsg, ">");
		strcat(formatmsg, chatstate.msg);
	}

	//if (chatstate.msgamount < 4) {
	Chat_SetMessage(formatmsg, tempmsgflags);

	Chat_WriteToLog(formatmsg);

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
	if (ch >= 32 && ch <= 126) {
		if (ch != 8 && msglen < 63) {
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
	//enter text
	if (msglen > 0 && ch == 8) {
		chatstate.msg[msglen - 1] = 0;
		msglen -= 1;
	}
}

void Chat_Step() {
	if (chatstate.timer != 0) {
		chatstate.timer -= 1;
	}
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
				if (chatmsgs[i].flags & 1) { //server/event/player join message
					msgcol = 0xFF00FFFF; //cyan
				}
				if (chatmsgs[i].flags & 2) { //me color
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

	/*unsigned char i2 = 0;
	while (i2 < 15) {
		font_draw(0, i2 * 12, names[i2]);
		i2++;
	}*/
}
