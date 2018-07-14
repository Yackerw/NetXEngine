#ifndef _NETWORKING
#define _NETWORKING
#define WIN32_LEAN_AND_MEAN
#define defaultport "5029"

#include <stdio.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <malloc.h>
#include <process.h>
#include <stdint.h>
#include <stdlib.h>

typedef struct {
	SOCKET sock;
	char *buffer;
	int size;
	int socknum;
	int buffsize;
} sockrecthread;

typedef struct {
	sockaddr_in data;
	SOCKET sock;
	char used;
	char *buffer;
	uintptr_t buffthread;
	int socknum;
	sockrecthread *recthread;
} netdata;

extern netdata *sockets;

extern int currentsock;

extern int numsocks;

extern HANDLE Proc;

extern SOCKET server;
extern SOCKET client;

extern char host;

#define MAXCLIENTS 32

extern char **databuffs;
extern int *databuffsizes;
extern HANDLE *buffmutexes;

extern int CliNum;

extern char *outbuff;

extern int outbuffsize;

extern void (**recvfuncs)(unsigned char*, int, int);
extern void (**sendfuncs)(unsigned char*, int);
extern int numrecv;
extern int numsend;

extern char*(**PlayerEventSendFuncs)();
extern int PlayerEventSendNum;
extern int *PlayerEventSendSizes;
extern void(**PlayerEventRecvFuncs)(unsigned char*, int);
extern int PlayerEventRecvNum;
extern int *PlayerEventRecvSizes;

extern char*(*PlayerJoinEventSvSend)();
extern int PlayerJoinEventSvSize;
extern void(*PlayerJoinEventSvRecv)(char*);
extern char*(*PlayerJoinEventClSend)();
extern int PlayerJoinEventClSize;
extern void(*PlayerJoinEventClRecv)(char*);

extern char*(*PlayerDisconnectSend)(int);
extern void(*PlayerDisconnectRecv)(char*);
extern int PlayerDisconnectSize;

extern int serializeid;

extern char banlist[256][32];
extern int bannum;

//WSAdata variable, stores information about winsock
extern WSADATA wsadata;
char Networking_Init();

SOCKET Server_Create();

int Server_Listen(SOCKET Server);

int Packet_Receive(SOCKET client, int length, char **netbuffer);

void Server_CloseClient(SOCKET server, int client);

void Client_Disconnect(SOCKET client);

int Packet_Send(SOCKET *client, char *netbuffer, int length);

//send packets to all the clients
int Packet_Send_Host(SOCKET server, char *netbuffer, int length);

int Packet_Send_Host_Others(SOCKET server, char *netbuffer, int length, int socknum);

void packet_receiving(void *sockettt);

int Server_Connect(SOCKET server);

SOCKET Client_Connect(const char* ip);

void Serv_Connect(void *serv);

// fun
float GetFloatBuff(char *buff, int offs);

// Parse our data
void Net_ParseBuffs();

// Send out data at end of frame
void Net_SendData();

// Add to out buff
void Net_AddToOut(char *buff, int buffsize);

// for player functions
int Net_RegisterPlayerEventSend(char *(*func)(), int buffsize);

// for player functions
int Net_RegisterPlayerEventRecv(void(*func)(unsigned char*, int), int buffsize);

void Net_FirePlayerEvent(int ev);


// Register functions to fire when joining
void Net_RegisterConnectEventSvSend(char *(*func)(), int buffsize);

// Register functions to fire when joining
void Net_RegisterConnectEventSvRecv(void(*func)(char*));

// Register functions to fire when joining
void Net_RegisterConnectEventClSend(char *(*func)(), int buffsize);

// Register functions to fire when joining
void Net_RegisterConnectEventClRecv(void(*func)(char*));

void Net_RegisterConnectEventOthersSend(char *(*func)(int), int buffsize);

void Net_RegisterConnectEventOthersRecv(void(*func)(char*));

void Net_RegisterDisconnectRecv(void(*func)(char*));

void Net_RegisterDisconnectSend(char *(*func)(int), int buffsize);

// Call this function to parse all networking related activity
void Net_Step();
#endif