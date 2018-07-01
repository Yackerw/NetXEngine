//made by root
#ifndef __CHAT__
	#define __CHAT__

#define CHATMSGSIZE 64

	typedef struct {
		char msg[CHATMSGSIZE + 17]; //message + <14 char player name> + identifier (host/admin)
		unsigned char flags;
		//^:
		//bit 0=player message or server/event message (EX:xXcritterslayah69Xx has changed name to thelegend27)
		//bit 1=/me 
	} chatmsg_t;

	typedef struct {
		unsigned short timer; //>=1:display chat 0=don't
		unsigned char msgamount; //amount of messages on screen
		unsigned char typing; //whaddya think
		char msg[CHATMSGSIZE]; //the message you're typing
	} chatstate_t;

	extern chatmsg_t chatmsgs[5];

	extern chatstate_t chatstate;

	void Chat_Init();

	void Chat_SetMessage(char* str, unsigned char flags);

	char* Chat_SendMessage();

	void Chat_ReceiveMessage(unsigned char* msg, int node);

	void Chat_EnterMessage();

	void Chat_AddChar(int ch);

		void Chat_Step();

	void Chat_Display();
#endif
