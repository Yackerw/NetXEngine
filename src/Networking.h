#ifndef _NETWORKING
#define _NETWORKING
#define WIN32_LEAN_AND_MEAN
#define defaultport 5029

#include <stdio.h>
#include <WinSock2.h>
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <stdlib.h>
#include <stdbool.h>
#include <WS2tcpip.h>
#include <process.h>
#include <time.h>

#define SendStackSize 2048
#define FullStackSize 4097
#define MAXCLIENTS 32

#define CONNECTAUTH "Yacker(tm) brand netplay v0.1, NetXEngine v0.3.=b"

#define CONN_TIMEOUT 8000 // connection timeout in ms

extern HANDLE recthread;

typedef struct {
	SOCKET sock;
	int port;
	sockaddr_in info;
} SocketInfo;

typedef struct {
	char *buff;
	int size;
	bool valid;
	long long timeout;
} SendStack_t;

typedef struct {
	char *Stack;
	int StackSize;
	char used;
} StackData_t;

typedef struct {
	sockaddr_in info;
	int id;
	SendStack_t SendStack[FullStackSize];
	int SendStackPos;
	char used;
	StackData_t ReceiveStack[10000];
	volatile int ReceiveStackPos;
	HANDLE ReceiveStackMutex;
	StackData_t ImportantStack[FullStackSize];
	int ImportantStackPos;
	time_t timeout;
} ClientInfo_t;

typedef struct {
	int id;
	short node;
	char important;
	short packetid;
	int type; // Expand this if necessary
} PacketData_t;

typedef struct {
	int parent;
	int child;
} LinkData_t;

extern LinkData_t linkedset[1024];
extern int linkednum;

extern ClientInfo_t clients[MAXCLIENTS];

extern int ClientID;
extern int ClientNode;

extern SocketInfo *Sock;

extern char Host;

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

SocketInfo *Server_Create(int port);

void CloseConn(ClientInfo_t *info);

void Packet_Send(char *buff, int node, int datasize, short type, char important = 0);

//send packets to all the clients
int Packet_Send_Host(char *netbuffer, int length, short type, char important = 0);

void Receive_Data(void *fricc);

SocketInfo *ClientCreate(char *ip, int port);

// fun
float GetFloatBuff(char *buff, int offs);

// Parse our data
void Net_ParseBuffs();

// Close down netplay
void Net_Close();

void Net_TrueClose();

// for player functions
int Net_RegisterPlayerEventSend(char *(*func)(), int buffsize, bool important = 1);

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