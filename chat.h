//made by root
#ifndef __CHAT__
	#define __CHAT__

	typedef struct {
		char* msg;
		unsigned char flags;
		//^:
		//bit 0=player message or server/event message (EX:xXcritterslayah69Xx has changed name to thelegend27)
		//bit 1=/me 
	} chatmsg_t;

#define CHATMSGSIZE 64

	typedef struct {
		unsigned short timer; //>=1:display chat 0=don't
		unsigned char msgamount; //amount of messages on screen
		unsigned char typing; //whaddya think
		char msg[CHATMSGSIZE]; //the message you're typing
	} chatstate_t;

	extern chatmsg_t chatmsgs[5];

	void Chat_SetMessage(char* str, unsigned char flags);

		void Chat_Step();

	void Chat_Display();
#endif