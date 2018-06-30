#include "Networking.h"
#include "tsc.h"
#include "game.h"
#include "ObjManager.h"
#include "object.h"
#include "map.h"
#include "player.h"
#include "NetPlayer.h"

netdata *sockets;

int currentsock;

int numsocks;

HANDLE Proc;

SOCKET server;
SOCKET client;

char host;

char **databuffs;
int *databuffsizes;
HANDLE *buffmutexes;

char *outbuff;

HANDLE outbuffmutex;

int outbuffsize;

void(**recvfuncs)(unsigned char*, int, int);
void(**sendfuncs)(unsigned char*, int);
int numrecv;
int numsend;

char*(**PlayerEventSendFuncs)();
int PlayerEventSendNum;
int *PlayerEventSendSizes;
void(**PlayerEventRecvFuncs)(unsigned char*, int);
int PlayerEventRecvNum;
int *PlayerEventRecvSizes;

char*(*PlayerJoinEventSvSend)();
int PlayerJoinEventSvSize;
void(*PlayerJoinEventSvRecv)(char*);
char*(*PlayerJoinEventClSend)();
int PlayerJoinEventClSize;
void(*PlayerJoinEventClRecv)(char*);

char*(*PlayerJoinEventOthersSend)(int);
void(*PlayerJoinEventOthersRecv)(char*);
int PlayerJoinEventOthersSize;

char*(*PlayerDisconnectSend)(int);
void(*PlayerDisconnectRecv)(char*);
int PlayerDisconnectSize;

int CliNum;

// current available serialization ID
int serializeid;

//WSAdata variable, stores information about winsock
WSADATA wsadata;

char Networking_Init() {
	//initialize startup
	int startup = WSAStartup(MAKEWORD(2, 2), &wsadata);
	if (startup != 0) {
		printf("Networking init failed: %d\n", startup);
		return 0;
	}
	printf("Networking started!\n");
	sockets = (netdata*)calloc(MAXCLIENTS, sizeof(netdata)*MAXCLIENTS);
	currentsock = 0;
	numsocks = 0;
	// Set up our mutexes and databuffs
	int i = 0;
	outbuff = (char*)malloc(sizeof(char) * 524288);
	databuffs = (char**)malloc(sizeof(char*)*MAXCLIENTS);
	databuffsizes = (int*)calloc(MAXCLIENTS, sizeof(int));
	buffmutexes = (HANDLE*)malloc(sizeof(HANDLE)*MAXCLIENTS);
	PlayerEventSendSizes = (int*)malloc(sizeof(int)*64);
	PlayerEventRecvSizes = (int*)malloc(sizeof(int)*64);
	PlayerEventSendFuncs = (char*(**)())malloc(sizeof(void(*)(void))*64);
	PlayerEventRecvFuncs = (void(**) (unsigned char*, int))realloc(PlayerEventRecvFuncs, sizeof(void(*)(void))*64); // Visual studio insists I can't malloc this variable, but will let me realloc it. Send help
	//PlayerEventRecvFuncs = (void(**)(unsigned char*,int))malloc(sizeof(void(*)(void)));
	netobjs = (serobj*)calloc(sizeof(serobj) * MAX_OBJECTS, 1);
	ObjSyncTickFuncs = (char*(**)(Object*))malloc(sizeof(void(*)(void)) * OBJ_LAST);
	ObjSyncTickFuncsRecv = (void(**)(char*,int))calloc(sizeof(void(*)(void)) * OBJ_LAST,1);
	ObjSyncTickSizes = (int*)malloc(sizeof(int)*OBJ_LAST);
	outbuffsize = 0;
	PlayerEventSendNum = 0;
	PlayerEventRecvNum = 0;
	numsend = 0;
	numrecv = 0;
	TscExec = 0;
	while (i < MAXCLIENTS) {
		buffmutexes[i] = CreateMutex(NULL, FALSE, NULL);
		databuffs[i] = (char*)malloc(sizeof(char) * 524288);
		i++;
	}
	outbuffmutex = CreateMutex(NULL, FALSE, NULL);
	return 1;
}

SOCKET Server_Create() {
	//address info
	struct addrinfo *result = NULL, *ptr = NULL, hints;
	//zero it out
	ZeroMemory(&hints, sizeof(hints));
	//ipv4, socket stream, tcp, passive?
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;

	int rresult = getaddrinfo(NULL, "5029", &hints, &result);
	if (rresult != 0) {
		printf("getaddrinfo failed: %d\n", rresult);
		return INVALID_SOCKET;
	}

	//create the socket
	SOCKET Server = INVALID_SOCKET;
	Server = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	if (Server == INVALID_SOCKET) {
		printf("Error at socket(): %ld\n", WSAGetLastError());
		freeaddrinfo(result);
		return INVALID_SOCKET;
	}

	//bind socket to IP address
	rresult = bind(Server, result->ai_addr, (int)result->ai_addrlen);
	if (rresult == SOCKET_ERROR) {
		printf("bind failed with error: %d\n", WSAGetLastError());
		freeaddrinfo(result);
		closesocket(Server);
		return INVALID_SOCKET;
	}
	//set TCP_NODELAY so we can BLAST THROUGH WITH SONIC SPEED
	char nodelaything[] = { 1 };
	if (setsockopt(Server, IPPROTO_TCP, TCP_NODELAY, nodelaything, 1)) {
		printf("Setsockopt failed with error: %d\n", WSAGetLastError());
		return INVALID_SOCKET;
	}
	printf("Server succesfully created!\n");
	return Server;
}

int Server_Listen(SOCKET Server) {
	//listen
	if (listen(Server, SOMAXCONN) == SOCKET_ERROR) {
		printf("Listen failed with error: %ld\n", WSAGetLastError());
		closesocket(Server);
		return INVALID_SOCKET;
	}
	printf("Listening!\n");
}

int Packet_Receive(SOCKET client, int length, char **netbuffer) {
	//char *netbuffer;
	netbuffer[0] = (char*)malloc(sizeof(char)*length);
	int success = 1;

	success = recv(client, (char*)netbuffer[0], length, 0);
	if (success > 0) {
		int i = 0;
		//while (i < success) {
		//	printf("Packet received: %i\n", netbuffer[i]);
		//	i++;
		//}
		netbuffer[0] = (char*)realloc(netbuffer[0], (success + 1) * sizeof(char));
		printf("AA %i\n", netbuffer[0][0]);
	}
	else if (success == 0) {
		printf("No packet received\n");
		free(netbuffer[0]);
		//netbuffer = (char*)realloc(netbuffer, 1);
		netbuffer[0] = 0;
	}
	else {
		printf("ERROR on packet receive: %d\n", WSAGetLastError());
		free(netbuffer[0]);
		netbuffer[0] = 0;
		//netbuffer = (char*)realloc(netbuffer, 1);
	}
	return success;
}

// BAD FUNCTION! SO BAD! Don't use it! EVER!
void Server_CloseClient(SOCKET server, int client) {
	if (sockets[client].used == 1) {
		int success = shutdown(sockets[client].sock, SD_BOTH);
		closesocket(sockets[client].sock);
		sockets[client].used = 0;
		int i = client;
		int freesock = 0;
		while (i < MAXCLIENTS) {
			if (freesock != 0) {
				if (sockets[i].used == 1) {
					sockets[i - 1] = sockets[i];
					sockets[i - 1].used = 1;
					sockets[i - 1].socknum = sockets[i].socknum - 1;
					sockets[i - 1].recthread = sockets[i].recthread;
					sockets[i - 1].recthread->socknum = sockets[i - 1].socknum;
					sockets[i].used = 0;
					sockets[i].socknum = 0;
				}
			}
			else {
				if (sockets[i].used == 0) {
					freesock = 1;
				}
			}
			i++;
		}
		numsocks -= 1;
	}
}

void Client_Disconnect(SOCKET client) {
	int success = shutdown(client, SD_BOTH);
	if (success == SOCKET_ERROR) {
		printf("Disconnect failed: %d\n", WSAGetLastError());
	}
	closesocket(client);
}

int Packet_Send(SOCKET client, char *netbuffer, int length) {
	int success = 1;
	success = send(client, netbuffer, length, 0);
	int totalsucc = success;
	// If windows has decided that there's not enough space for us to send, we need to create a loop to continue sending until it is succesful
	while (totalsucc < length) {
		if (success == SOCKET_ERROR) {
			printf("Send failed: %d\n", WSAGetLastError());
			return -1;
		}
		success = send(client, netbuffer + totalsucc, length - totalsucc, 0);
		totalsucc += success;
	}
	// err check
	if (success == SOCKET_ERROR) {
		printf("Send failed: %d\n", WSAGetLastError());
		return -1;
	}
	//printf("Packet sent!\n");
	return 1;
}

//send packets to all the clients
int Packet_Send_Host(SOCKET server, char *netbuffer, int length) {
	int i = 0;
	while (i < MAXCLIENTS) {
		if (sockets[i].used == 1) {
			int success = 1;
			success = send(sockets[i].sock, netbuffer, length, 0);
			int totalsucc = success;
			while (totalsucc < length && success != SOCKET_ERROR) {
				success = send(sockets[i].sock, netbuffer + totalsucc, length - totalsucc, 0);
				totalsucc += success;
			}
			if (success == SOCKET_ERROR) {
				printf("Send failed: %d\n", WSAGetLastError());
				// Another thread will handle removing this!
			}
		}
		i++;
	}
	return 1;
}

int Packet_Send_Host_Others(SOCKET server, char *netbuffer, int length, int socknum) {
	int i = 0;
	while (i < MAXCLIENTS) {
		if (sockets[i].used == 1 && i != socknum) {
			char *newbuff = (char*)malloc((sizeof(char)*length) + 8);
			memcpy(newbuff + 8, netbuffer, length);
			newbuff[0] = 6;
			newbuff[1] = 0;
			newbuff[2] = 0;
			newbuff[3] = 0;
			newbuff[5] = 0;
			newbuff[6] = 0;
			newbuff[7] = 0;
			if (socknum < i) {
				newbuff[4] = socknum + 2;
			}
			else {
				newbuff[4] = socknum + 3;
			}
			int success = 1;
			success = send(sockets[i].sock, newbuff, length + 8, 0);
			if (success == SOCKET_ERROR) {
				printf("Send failed: %d\n", WSAGetLastError());
				return -1;
			}
		}
		i++;
	}
	return 1;
}

void packet_receiving(void *sockettt) {
	int buffpos;
	sockrecthread *sockett = (sockrecthread*)sockettt;
	char socknumber = 0;
	int expecteddata = 0;
	unsigned char *recvbuff = (unsigned char*)malloc(20480 * sizeof(float));
	int recvbuffsize = 0;
	unsigned char *sizebuff = (unsigned char*)malloc(4 * sizeof(char));
	int sizebuffsize = 0;
	if (host == 1) {
		socknumber = sockett->socknum;
	}
	//sockett->buffsize = 1;
	//sockett->buffer = (char*)malloc(sizeof(char));
	//wait for & receive a packet. get current size of the recieving buffer to append the newly received buffer to the end of it, after allocating enough memory.
	//THIS FUNCTION IS TO ONLY BE USED ON A SEPERATE THREAD
	while (sockets[sockett->socknum].used) {
		if (host == 1) {
			socknumber = sockett->socknum;
		}
		unsigned char **temp = (unsigned char**)malloc(sizeof(char**));
		int bytes = Packet_Receive(sockett->sock, sizeof(float) * 20480, (char**)temp);
		if (bytes == SOCKET_ERROR || bytes == 0) {
			sockets[sockett->socknum].used = false;
			// Fire the disconnect event
			char *discnnbuff = PlayerDisconnectSend(sockett->socknum);
			discnnbuff = (char*)realloc(discnnbuff, PlayerDisconnectSize + 4);
			memmove(discnnbuff + 4, discnnbuff, PlayerDisconnectSize);
			int tmp = 5;
			memmove(discnnbuff, &tmp, sizeof(int));
			Net_AddToOut(discnnbuff, PlayerDisconnectSize + 4);
			free(discnnbuff);
			free(sockett);
			free(recvbuff);
			free(sizebuff);
			return;
		}
		unsigned char *tempbuff = temp[0];
		free(temp);
		float *temptemp = (float*)(void*)(tempbuff);
		int *tempint = (int*)(void*)(tempbuff);
		// Anticipating identifier for size of buffer
		if (expecteddata == 0) {
			// If we have an incomplete buffer from earlier, F I X
			if (sizebuffsize > 0) {
				memcpy(sizebuff + sizebuffsize, tempbuff, min(bytes, 4 - sizebuffsize));
				int oldsize = sizebuffsize;
				sizebuffsize += bytes;
				// received enough to determine packet size
				if (sizebuffsize >= 4) {
					expecteddata = ((int*)sizebuff)[0];
					// Handle excessive data
					if (sizebuffsize > 4) {
						memcpy(recvbuff, tempbuff + (4 - oldsize), bytes - (4 - oldsize));
					}
				}
				sizebuffsize = 0;
			}
			// Otherwise we need to create a new buffer for receiving packet size
			else {
				// If we've received enough to fill the whole buffer, then we're already done!
				if (bytes >= 4) {
					expecteddata = tempint[0];
					// and read potential data
					if (bytes > 4) {
						memcpy(recvbuff, tempbuff + 4, bytes - 4);
						recvbuffsize += bytes - 4;
					}
				}
				// If we didn't receive the whole packet size (this should NEVER happen in normal circumstances, but gotta cover my bases)
				else {
					memcpy(sizebuff, tempbuff, bytes);
					sizebuffsize += bytes;
				}
			}
		}
		// Otherwise receive data proper
		else {
			memcpy(recvbuff + recvbuffsize, tempbuff, bytes);
			recvbuffsize += bytes;
		}

		// We need control over this!
		WaitForSingleObject(buffmutexes[socknumber], INFINITE);
		// So long as we have sufficient data, write it to be used!
		while (recvbuffsize >= expecteddata && recvbuffsize > 0) {
			// Copy data to our useful buffs
			memcpy(databuffs[socknumber] + databuffsizes[socknumber], recvbuff, expecteddata);
			databuffsizes[socknumber] += expecteddata;
			// If we've received additional data, then let's store it
			if (recvbuffsize > expecteddata) {
				// If we've received enough data for the next packet size, then let's store it
				if (recvbuffsize >= expecteddata + 4) {
					int tmp = ((int*)(recvbuff + expecteddata))[0];
					// Copy from our receiving array to our final array
					memmove(recvbuff, recvbuff + expecteddata + 4, recvbuffsize - (expecteddata + 4));
					recvbuffsize -= expecteddata;
					recvbuffsize -= 4;
					expecteddata = tmp;
				}
				else {
					// Insufficient data, set up our sizebuff
					memcpy(sizebuff, recvbuff, recvbuffsize - expecteddata);
					sizebuffsize = recvbuffsize - expecteddata;
					expecteddata = 0;
					recvbuffsize = 0;
				}
			}
			else {
				expecteddata = 0;
				recvbuffsize = 0;
			}
		}
		ReleaseMutex(buffmutexes[socknumber]);
		free(tempbuff);
	}
	//free(sockett->buffer);
	free(sockett);
	free(recvbuff);
	free(sizebuff);
	return;
}

int Server_Connect(SOCKET server) {
	SOCKET client;
	struct sockaddr_in info = { 0 };
	int size = sizeof(info);

	//accept client
	client = accept(server, (sockaddr*)&info, &size);
	if (client == INVALID_SOCKET) {
		printf("accept failed: %d\n", WSAGetLastError());
		closesocket(server);
		return 0;
	}
	char nodelaything[] = { 1 };
	if (setsockopt(client, IPPROTO_TCP, TCP_NODELAY, nodelaything, 1)) {
		printf("Setsockopt failed with error: %d\n", WSAGetLastError());
		return INVALID_SOCKET;
	}
	// Fire our connect event
	char *buff = PlayerJoinEventSvSend();
	buff = (char*)realloc(buff, PlayerJoinEventSvSize + 8);
	memmove(buff + 8, buff, PlayerJoinEventSvSize);
	int tmp = 0;
	memcpy(buff + 4, &tmp, sizeof(int));
	tmp = PlayerJoinEventSvSize + 4;
	memcpy(buff, &tmp, sizeof(int));
	Packet_Send(client, buff, tmp + 4);
	free(buff);
	// Find free socket
	int freesock = 0;
	while (sockets[freesock].used == true) {
		freesock++;
	}
	// Fire our event to tell everyone else that someone connected!
	buff = PlayerJoinEventOthersSend(freesock);
	buff = (char*)realloc(buff, PlayerJoinEventOthersSize + 8);
	memmove(buff + 8, buff, PlayerJoinEventOthersSize);
	tmp = 4;
	memcpy(buff + 4, &tmp, sizeof(int));
	tmp = PlayerJoinEventOthersSize + 4;
	memcpy(buff, &tmp, sizeof(int));
	printf("%i\n",tmp);
	Packet_Send_Host(server, buff, tmp + 4);
	free(buff);
	// Set up socket stuff
	sockets[freesock].sock = client;
	sockets[freesock].data = info;
	sockets[freesock].used = 1;
	sockets[freesock].socknum = freesock;
	sockrecthread *tempval = (sockrecthread *)malloc(sizeof(sockrecthread));
	sockets[freesock].recthread = tempval;
	tempval->sock = sockets[freesock].sock;
	tempval->size = 33 * sizeof(char);
	tempval->socknum = freesock;
	sockets[freesock].buffthread = _beginthread(packet_receiving, 128, tempval);
	numsocks++;
	char *temp;
	temp = (char*)malloc(sizeof(char) * 16);
	inet_ntop(AF_INET, &info.sin_addr, temp, sizeof(info.sin_addr));
	printf("Connection succesful from %s\n", temp);
	return 1;
}

SOCKET Client_Connect(const char* ip) {
	//address info
	struct addrinfo *result = NULL,
		*ptr = NULL,
		hints;
	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	//get info from address
	int serverinf = getaddrinfo(ip, "5029", &hints, &result);
	if (serverinf != 0) {
		printf("getaddrinfo failed: error %d\n", serverinf);
		return INVALID_SOCKET;
	}
	SOCKET clientsock = INVALID_SOCKET;
	ptr = result;
	// Attempt to connect to an address until one succeeds
	for (ptr = result; ptr != NULL; ptr = ptr->ai_next) {

		// Create a SOCKET for connecting to server
		clientsock = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
		if (clientsock == INVALID_SOCKET) {
			printf("socket failed with error: %ld\n", WSAGetLastError());
			return INVALID_SOCKET;
		}

		// Connect to server.
		serverinf = connect(clientsock, ptr->ai_addr, (int)ptr->ai_addrlen);
		if (serverinf == SOCKET_ERROR) {
			closesocket(clientsock);
			clientsock = INVALID_SOCKET;
			continue;
		}
		else {
			char nodelaything[] = { 1 };
			if (setsockopt(clientsock, IPPROTO_TCP, TCP_NODELAY, nodelaything, 1)) {
				printf("Setsockopt failed with error: %d\n", WSAGetLastError());
				return INVALID_SOCKET;
			}
			printf("Connected!\n");
		}
		break;
	}
	/*
	SOCKET clientsock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (clientsock == INVALID_SOCKET) {
	printf("socket function failed with error: %ld\n", WSAGetLastError());
	return INVALID_SOCKET;
	}
	sockaddr_in clientService;
	clientService.sin_family = AF_INET;
	clientService.sin_addr.s_addr = inet_addr("127.0.0.1");
	clientService.sin_port = htons(5029);
	int serverinf;
	serverinf = connect(clientsock, (SOCKADDR*) & clientService, sizeof(clientService));
	if (serverinf == SOCKET_ERROR) {
	printf("Connect failed: %ld\n", WSAGetLastError());
	closesocket(clientsock);
	clientsock = INVALID_SOCKET;
	return INVALID_SOCKET;
	}*/
	return clientsock;
}

void Serv_Connect(void *serv) {
	SOCKET server = *(SOCKET*)serv;
	while (1) {
		Server_Connect(server);
	}
}

// fun
float GetFloatBuff(char *buff, int offs) {
	float *retval = (float*)(buff + offs);
	return *retval;
}

int GetIntBuff(char *buff, int offs) {
	int *retval = (int*)(buff + offs);
	return *retval;
}

bool ShouldMoveToHost = false;

// Parse our data
void Net_ParseBuffs() {
	int i = 0;
	while (i < MAXCLIENTS) {
		if (sockets[i].used == 1) {
			// Need to parse everything
			int arraypos = 0;
			WaitForSingleObject(buffmutexes[i], INFINITE);
			while (arraypos < databuffsizes[i]) {
				// Determine the first int to determine the function to use
				switch (GetIntBuff(databuffs[i], arraypos)) {
				case 0:
					// RESERVED
					PlayerJoinEventSvRecv(databuffs[i] + arraypos + 4);
					arraypos += PlayerJoinEventSvSize + 4;
					break;
				case 1:
					// RESERVED
					break;
				case 2:
					// Player events
					if (host == true) {
						PlayerEventRecvFuncs[GetIntBuff(databuffs[i], arraypos + 4)]((unsigned char*)databuffs[i] + arraypos + 8, i);
					}
					else {
						PlayerEventRecvFuncs[GetIntBuff(databuffs[i], arraypos + 4)]((unsigned char*)databuffs[i] + arraypos + 8, CliNum);
						if (ShouldMoveToHost == true && GetIntBuff(databuffs[i], arraypos + 4) == PlayerUpdateEvent) {
							memcpy(&(player->x), &players[CliNum].x, sizeof(int));
							memcpy(&(player->y), &players[CliNum].y, sizeof(int));
							ShouldMoveToHost = false;
						}
					}
					// relay data
					if (host == true) {
						int buffsize = sizeof(char)*PlayerEventRecvSizes[GetIntBuff(databuffs[i], arraypos + 4)] + 16;
						char *temparray = (char*)malloc(buffsize);
						memcpy(temparray + 8, databuffs[i] + arraypos, buffsize - 8);
						int temp = 3;
						memcpy(temparray, &temp, 4);
						memcpy(temparray + 4, &i, 4);
						Net_AddToOut(temparray, buffsize);
					}
					arraypos += sizeof(char)*PlayerEventRecvSizes[GetIntBuff(databuffs[i], arraypos + 4)] + 8;
					break;
				case 3:
				{
					int pnode = GetIntBuff(databuffs[i], arraypos + 4);
					// Relayed data
					arraypos += 8;
					switch (GetIntBuff(databuffs[i], arraypos)) {
					case 2:
						if (host == false && CliNum != pnode) {
							PlayerEventRecvFuncs[GetIntBuff(databuffs[i], arraypos + 4)]((unsigned char*)databuffs[i] + arraypos + 8, pnode);
						}
						arraypos += sizeof(char)*PlayerEventRecvSizes[GetIntBuff(databuffs[i], arraypos + 4)] + 8;
						break;
					}
				}
				break;
				case 4:
					if (host != 1) {
						// Someone has joined so we must tell everyone
						PlayerJoinEventOthersRecv(databuffs[i] + arraypos + 4);
					}
					arraypos += PlayerJoinEventOthersSize + 4;
					break;
				case 5:
					if (host != 1) {
						// Someone has left so we must tell everyone
						PlayerDisconnectRecv(databuffs[i] + arraypos + 4);
					}
					arraypos += PlayerDisconnectSize + 4;
					break;
				case 6:
					// special tsc execute command
					if (host != 1) {
						TscExec = 1;
						// If we're dead then respawn
						if (host == 0 && player->hp == 0) {
							player->x = players[CliNum].x;
							player->y = players[CliNum].y;
							player->hp = max(player->maxHealth / 4 , 1);
							player->hide = false;
						}
						game.tsc->StartScript(GetIntBuff(databuffs[i],arraypos+4));
					}
					arraypos += 8;
					break;
				case 7:
					// Sync serialized object spawning
					if (host == 0) {
						objargs obj;
						memcpy(&obj, databuffs[i] + arraypos + 8, sizeof(objargs));
						int ser;
						memcpy(&ser, databuffs[i] + arraypos + 4, sizeof(int));
						// We are loading from the start of the level, so store it for when we load a level
						if (obj.onLoad == true) {
							nextloadobjs[nextloadid] = obj;
							nextloadobjsser[nextloadid] = ser;
							nextloadid++;
						}
						else {
							netobjs[ser].obj = CreateObject(obj.x, obj.y, obj.type, obj.xinertia, obj.yinertia, obj.dir, NULL, 0, 1);
							netobjs[ser].valid = true;
							netobjs[ser].obj->serialization = ser;
						}
					}
					arraypos += sizeof(objargs) + (sizeof(int) * 2);

					break;
				case 8:
					// Sync serialized object step
					int id;
					memcpy(&id, databuffs[i] + arraypos + 4, sizeof(int));
					int obj;
					memcpy(&obj, databuffs[i] + arraypos + 8, sizeof(int));
					// Only if we're not host, AND a function exists there! Jumping to arbitrary memory is a very bad idea.
					if (host == 0 && id < OBJ_LAST && ObjSyncTickFuncsRecv[id] != NULL && netobjs[obj].valid == true) {
						ObjSyncTickFuncsRecv[id](databuffs[i] + arraypos + 12, obj);
					}
					arraypos += ObjSyncTickSizes[id] + 12;
					break;
				case 9:
					// Sync serialized object death
					if (host == 0) {
						int id;
						memcpy(&id, databuffs[i] + arraypos + 4, sizeof(int));
						if (id < MAX_OBJECTS && netobjs[id].valid == true) {
							netobjs[id].obj->OnDeath(true);
						}
					}
					arraypos += 8;
					break;
				case 10:
					// Sync serialized object removal
					if (host == 0) {
						int id;
						memcpy(&id, databuffs[i] + arraypos + 4, sizeof(int));
						if (id < MAX_OBJECTS && netobjs[id].valid == true) {
							netobjs[id].obj->Delete(1);
						}
					}
					arraypos += 8;
					break;
				case 11:
					// Sync serialized object removal
					if (host == 0) {
						int id;
						memcpy(&id, databuffs[i] + arraypos + 4, sizeof(int));
						if (id < MAX_OBJECTS && netobjs[id].valid == true) {
							netobjs[id].obj->Kill(true);
						}
					}
					arraypos += 8;
					break;
				case 12:
					// An object has changed! We need to serialize it.
					if (host == 0) {
						int id2 = 0;
						memcpy(&id2, databuffs[i] + arraypos + 4, sizeof(short));
						Object *o = ID2Lookup[id2];
						int newtype;
						memcpy(&newtype, databuffs[i] + arraypos + 8, sizeof(int));
						if (newtype < OBJ_LAST) {
							o->ChangeType(newtype, true);
							unsigned int ser;
							memcpy(&ser, databuffs[i] + arraypos + 12, sizeof(int));
							if (ser < MAX_OBJECTS) {
								o->serialization = ser;
								netobjs[ser].obj = o;
								netobjs[ser].valid = true;
							}
						}
					}
					arraypos += 16;
					break;
				case 13: {
					// Host has reloaded save, adjust
					if (host == 0) {
						int i = 0;
						arraypos += 4;
						// change map
						memcpy(&game.switchstage.mapno, databuffs[i] + arraypos, sizeof(int));
						memcpy(&(player->inventory), databuffs[i] + arraypos + sizeof(int), sizeof(int) * MAX_INVENTORY);
						memcpy(&(player->ninventory), databuffs[i] + arraypos + (sizeof(int) * (MAX_INVENTORY + 1)), sizeof(int));
						memcpy(&(player->weapons), databuffs[i] + arraypos + (sizeof(int) * (MAX_INVENTORY + 2)), sizeof(Weapon) * WPN_COUNT);
						memcpy(&game.flags, databuffs[i] + arraypos + (sizeof(int) * (MAX_INVENTORY + 2)) + (sizeof(Weapon) * WPN_COUNT), NUM_GAMEFLAGS);
						memcpy(&(player->maxHealth), databuffs[i] + arraypos + (sizeof(int) * (MAX_INVENTORY + 2)) + (sizeof(Weapon) * WPN_COUNT) + NUM_GAMEFLAGS, sizeof(int));
						for (i = 0; i<NUM_TELEPORTER_SLOTS; i++)
						{
							int slotno, scriptno;
							memcpy(&slotno, databuffs[i] + arraypos + (sizeof(int) * (MAX_INVENTORY + 3)) + (sizeof(Weapon) * WPN_COUNT) + NUM_GAMEFLAGS + ((i * 2) * sizeof(int)), sizeof(int));
							memcpy(&scriptno, databuffs[i] + arraypos + (sizeof(int) * (MAX_INVENTORY + 4)) + (sizeof(Weapon) * WPN_COUNT) + NUM_GAMEFLAGS + ((i * 2) * sizeof(int)), sizeof(int));
							if (slotno != 0 && scriptno != 0) {
								textbox.StageSelect.SetSlot(slotno, scriptno);
							}
						}
						player->invisible = false;
						player->movementmode = MOVEMODE_NORMAL;
						player->hide = false;
						player->hp = player->maxHealth; // fade
						// Note that we should move to the host
						ShouldMoveToHost = true;
						arraypos -= 4;
					}
					arraypos += (sizeof(int) * (3 + MAX_INVENTORY + (NUM_TELEPORTER_SLOTS * 2)) + (sizeof(Weapon) * WPN_COUNT) + NUM_GAMEFLAGS);
				}
				break;
				case 14:
				{
					// Host has opted to change rooms. Currently only used for teleporter, regular tsc handles the rest
					if (host == 0) {
						int parm[4];
						memcpy(parm, databuffs[i] + arraypos + 4, sizeof(int) * 4);

						bool waslocked = (player->inputs_locked || game.frozen);

						stat("******* Executing <TRA to stage %d", parm[0]);
						game.switchstage.mapno = parm[0];
						game.switchstage.eventonentry = parm[1];
						game.switchstage.playerx = parm[2];
						game.switchstage.playery = parm[3];

						if (game.switchstage.mapno != 0)
						{
							// KEY is maintained across TRA as if the TRA
							// were a jump instead of a restart; but if the
							// game is in PRI then it is downgraded to a KEY.
							// See entrance to Yamashita Farm.
							if (waslocked)
							{
								player->inputs_locked = true;
								game.frozen = false;
							}
						}
					}
					arraypos += sizeof(int) * 5;
				}
					break;
				default:
					// Something terribly, terribly wrong has happened. Or someone's doing something malicious. Either way, kill it
					arraypos = databuffsizes[i];
					break;
				}
			}
		}
		databuffsizes[i] = 0;
		ReleaseMutex(buffmutexes[i]);
		i++;
	}
}

// Send out data at end of frame
void Net_SendData() {
	memmove(outbuff + 4, outbuff, outbuffsize);
	memcpy(outbuff, &outbuffsize, sizeof(int));
	if (host == true) {
		Packet_Send_Host(server, outbuff, outbuffsize + 4);
	}
	else {
		Packet_Send(client, outbuff, outbuffsize + 4);
	}
	outbuffsize = 0;
}

// Add to out buff
void Net_AddToOut(char *buff, int buffsize) {
	// We need control over this!
	WaitForSingleObject(outbuffmutex, INFINITE);
	memcpy(outbuff + outbuffsize, buff, buffsize);
	outbuffsize += buffsize;
	ReleaseMutex(outbuffmutex);
}

// for player functions
int Net_RegisterPlayerEventSend(char *(*func)(), int buffsize) {
	//PlayerEventSendFuncs = (char *(**)())realloc(PlayerEventSendFuncs, sizeof(void(*)(void))*PlayerEventSendNum + 1);
	//PlayerEventSendSizes = (int*)realloc(PlayerEventSendSizes, sizeof(int)*PlayerEventSendNum + 1);
	PlayerEventSendFuncs[PlayerEventSendNum] = func;
	PlayerEventSendSizes[PlayerEventSendNum] = buffsize;
	PlayerEventSendNum++;
	return PlayerEventSendNum - 1;
}

// for player functions
int Net_RegisterPlayerEventRecv(void(*func)(unsigned char*, int), int buffsize) {
	//PlayerEventRecvFuncs = (void (**) (unsigned char*, int))realloc(PlayerEventRecvFuncs, sizeof(void(*)(void))*PlayerEventRecvNum + 1);
	//PlayerEventRecvSizes = (int*)realloc(PlayerEventRecvSizes, sizeof(int)*PlayerEventRecvNum + 1);
	PlayerEventRecvFuncs[PlayerEventRecvNum] = func;
	PlayerEventRecvSizes[PlayerEventRecvNum] = buffsize;
	PlayerEventRecvNum++;
	return PlayerEventRecvNum - 1;
}

void Net_FirePlayerEvent(int ev) {
	int evs = PlayerEventSendSizes[ev];
	char *evbuff = (char*)malloc(sizeof(char)*(8 + evs));
	char *retarry = PlayerEventSendFuncs[ev]();
	if (evs != 0) {
		memcpy(evbuff + 8, retarry, evs);
		free(retarry);
	}
	int tmp = 2;
	memcpy(evbuff, &tmp, sizeof(int));
	memcpy(evbuff + 4, &ev, sizeof(int));
	Net_AddToOut(evbuff, evs + 8);
}


// Register functions to fire when joining
void Net_RegisterConnectEventSvSend(char *(*func)(), int buffsize) {
	PlayerJoinEventSvSend = func;
	PlayerJoinEventSvSize = buffsize;
}

// Register functions to fire when joining
void Net_RegisterConnectEventSvRecv(void(*func)(char*)) {
	PlayerJoinEventSvRecv = func;
}

// Register functions to fire when joining
void Net_RegisterConnectEventClSend(char *(*func)(), int buffsize) {
	PlayerJoinEventClSend = func;
	PlayerJoinEventClSize = buffsize;
}

// Register functions to fire when joining
void Net_RegisterConnectEventClRecv(void(*func)(char*)) {
	PlayerJoinEventClRecv = func;
}

// Register functions to fire to other clients when joining
void Net_RegisterConnectEventOthersSend(char *(*func)(int), int buffsize) {
	PlayerJoinEventOthersSend = func;
	PlayerJoinEventOthersSize = buffsize;
}

// Register functions to fire to other clients when joining
void Net_RegisterConnectEventOthersRecv(void (*func)(char*)) {
	PlayerJoinEventOthersRecv = func;
}

// Register functions to fire to other clients when one leaves
void Net_RegisterDisconnectSend(char *(*func)(int), int buffsize) {
	PlayerDisconnectSend = func;
	PlayerDisconnectSize = buffsize;
}

void Net_RegisterDisconnectRecv(void(*func)(char*)) {
	PlayerDisconnectRecv = func;
}

// Call this function to parse all networking related activity
void Net_Step() {
	// ALWAYS parse data every frame
	Net_ParseBuffs();
	// TODO: proper timing
	Net_SendData();
}