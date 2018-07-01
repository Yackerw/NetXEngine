//made by root

#include "graphics/graphics.h"
#include "graphics/sprites.h"
#include "graphics/font.h"
#include "autogen/sprites.h"
#include "input.h"
#include "Networking.h"
#include "NetPlayer.h"
#include "chat.h"
using namespace Graphics;
using namespace Sprites;

const int ctx = 8; //chat text x
const int cty = 236;// 176;

const bool showchat = true;

chatmsg_t chatmsgs[5];
chatstate_t chatstate;

void Chat_SetMessage(char* str, unsigned char flags) {
	memmove(chatmsgs + sizeof(chatmsg_t), chatmsgs, sizeof(chatmsg_t) * 4);
	//strcpy((char*)&chatmsgs[0].msg,str);
	chatmsgs[0].msg = str;
	if (chatstate.msgamount < 4) {
		chatstate.msgamount++;
	}
}

void Chat_Step() {
	if (chatstate.timer != 0) {
		chatstate.timer -= 1;
	}
	unsigned char i = 0;
	while (i < CHATMSGSIZE) {
		chatstate.msg[i] = 0;
		i++;
	}
	chatstate.msg[0] = 65 + (rand() % 27);

	Chat_SetMessage((char*)&chatstate.msg, 0);
	Sleep(16 * 60);
}

void Chat_Display() {
	chatstate.timer = 69;
	//chatstate.msgamount = 0;
	chatstate.typing = 1;
	Chat_Step();
	if (chatstate.timer >= 1) {
		/*chatmsgs[0].msg = "test";
		chatmsgs[1].msg = "test2";
		chatmsgs[2].msg = "test3";
		chatmsgs[3].msg = "test4";
		chatmsgs[4].msg = "test69xd";*/
		uint32_t msgcol = 16777215U;

		unsigned char i = 0;
		while (i < 5) {
			if (chatmsgs[i].msg != NULL) {
				if (chatmsgs[i].flags >= 0) { //if theres any more flags change this to & 1 or & 2 for server/me
					msgcol = 0xFFFFFF00; //yellow
				}
				//font_draw(ctx, cty + (12 * i), chatmsgs[i].msg);
				font_draw(ctx, (cty-14*chatstate.typing) - (12 * chatstate.msgamount) + (12 * i), chatmsgs[i].msg, msgcol,true);
			}
			i++;
		}

		if (chatstate.typing) {
			font_draw(ctx, cty, (char*)&chatstate.msg, 0xFFFFFF00, true);
		}
	}
}