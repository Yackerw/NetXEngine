#include "Networking.h"
#include "tsc.h"
#include "game.h"
#include "ObjManager.h"
#include "object.h"
#include "map.h"
#include "player.h"
#include "NetPlayer.h"
#include "chat.h"
#include <sys/timeb.h>
#include "inventory.h"
#include "sound/SoundManager.h"
#include "ipfuncs.h"

#define tmpmax(X,Y) ((X > Y) ? X : Y)

int sockaddrsize = sizeof(sockaddr);


LinkData_t linkedset[1024];
int linkednum = 0;

ClientInfo_t clients[MAXCLIENTS];

WSADATA wsaData;

char *(*JoinSendClient)(int);
int JoinClientSize;

int ClientID = -1;
int ClientNode = -1;

char Host = -1;

SocketInfo *Sock = NULL;

void(**recvfuncs)(unsigned char *, int, int);
void(**sendfuncs)(unsigned char *, int);
int numrecv;
int numsend;

char *(**PlayerEventSendFuncs)();
int PlayerEventSendNum;
int *PlayerEventSendSizes;
void(**PlayerEventRecvFuncs)(unsigned char *, int);
int PlayerEventRecvNum;
int *PlayerEventRecvSizes;
int PlayerEventImportant[128];

char *(*PlayerJoinEventSvSend)();
int PlayerJoinEventSvSize;
void(*PlayerJoinEventSvRecv)(char *);
char *(*PlayerJoinEventClSend)();
int PlayerJoinEventClSize;
void(*PlayerJoinEventClRecv)(char *);

char *(*PlayerJoinEventOthersSend)(int);
void(*PlayerJoinEventOthersRecv)(char *);
int PlayerJoinEventOthersSize;

char *(*PlayerDisconnectSend)(int);
void(*PlayerDisconnectRecv)(char *);
int PlayerDisconnectSize;

// current available serialization ID
int serializeid;

// Banlist
char banlist[256][32];
int bannum = 0;

//WSAdata variable, stores information about winsock
WSADATA wsadata;

HANDLE recthread;

char Networking_Init() {
  //initialize startup
  int startup = WSAStartup(MAKEWORD(2, 2), &wsadata);
  if (startup != 0) {
    printf("Networking init failed: %d\n", startup);
    return 0;
  }
  printf("Networking started!\n");
  PlayerEventSendSizes = (int *)malloc(sizeof(int) * 64);
  PlayerEventRecvSizes = (int *)malloc(sizeof(int) * 64);
  PlayerEventSendFuncs = (char *(**)())malloc(sizeof(void(*)(void)) * 64);
  PlayerEventRecvFuncs = (void(**) (unsigned char *, int))realloc(PlayerEventRecvFuncs, sizeof(void(*)(void)) * 64); // Visual studio insists I can't malloc this variable, but will let me realloc it. Send help
  //PlayerEventRecvFuncs = (void(**)(unsigned char*,int))malloc(sizeof(void(*)(void)));
  netobjs = (serobj *)calloc(sizeof(serobj) * MAX_OBJECTS, 1);
  ObjSyncTickFuncs = (char *(**)(Object *))malloc(sizeof(void(*)(void)) * OBJ_LAST);
  ObjSyncTickFuncsRecv = (void(**)(char *, int))calloc(sizeof(void(*)(void)) * OBJ_LAST, 1);
  ObjSyncTickSizes = (int *)malloc(sizeof(int) * OBJ_LAST);
  ObjSyncTickXSize = (int *)malloc(sizeof(int) * OBJ_LAST);
  ObjSyncTickYSize = (int *)malloc(sizeof(int) * OBJ_LAST);
  PlayerEventSendNum = 0;
  PlayerEventRecvNum = 0;
  numsend = 0;
  numrecv = 0;
  TscExec = 0;
  // rng
  srand(time(NULL));
  return 1;
}

SocketInfo *Server_Create(int port) {
  SOCKET s;
  s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  if (s == INVALID_SOCKET) {
    printf("Failed to create socket: %d", WSAGetLastError());
    return NULL;
  }
  SocketInfo *SockInf = (SocketInfo *)malloc(sizeof(SocketInfo));
  SockInf->info.sin_family = AF_INET;
  SockInf->info.sin_addr.s_addr = INADDR_ANY;
  SockInf->info.sin_port = htons(port);
  // Attempt to bind to port, if fails then clean up and return
  if (bind(s, (sockaddr *)&SockInf->info, sizeof(sockaddr_in)) == SOCKET_ERROR) {
    printf("Bind failed: %d", WSAGetLastError());
    free(SockInf);
    closesocket(s);
    return NULL;
  }
  SockInf->sock = s;
  SockInf->port = port;
  ClientID = rand();
  ClientNode = 0;
  char tmp = 0;
  char out;
  DWORD out2;

  // Because we're gonna be receiving a lot of data, let's make sure we have the space
  int rcvbuffsize = 1000000;
  setsockopt(SockInf->sock, SOL_SOCKET, SO_RCVBUF, (char *)&rcvbuffsize, sizeof(int));
  // Disable killing the socket if port is deemed unreachable
  WSAIoctl(SockInf->sock, -1744830452, &tmp, 1, &out, 1, &out2, NULL, NULL);
  return SockInf;
}

void CloseConn(ClientInfo_t *info) {
  // im dumb
  int i2 = 0;
  int nodenum = -1;
  while (i2 < MAXCLIENTS) {
    if (clients[i2].info.sin_addr.S_un.S_addr == info->info.sin_addr.S_un.S_addr && clients[i2].info.sin_port == info->info.sin_port) {
      nodenum = i2;
    }
    i2++;
  }
  info->used = 0;
  if (nodenum != -1 && Host == 1) {
    // Fire the disconnect event
    char *discnnbuff = PlayerDisconnectSend(nodenum);
    Packet_Send_Host(discnnbuff, PlayerDisconnectSize, 5, 1);
    free(discnnbuff);
  }
  // disconnect message
  if (nodenum != -1) {
    char msg[128];
    if (Host == 1) {
      sprintf(msg, "%s has disconnected", names[nodenum]);
      Chat_SetMessage(msg, 1);
      NXE::Sound::SoundManager::getInstance()->playSfx(NXE::Sound::SFX::SND_COMPUTER_BEEP);
      Chat_WriteToLog(msg);
      chatstate.timer = (60 * 5);
    }
    else {
      sprintf(msg, "You have disconnected from the server");
      Chat_SetMessage(msg, 1);
      NXE::Sound::SoundManager::getInstance()->playSfx(NXE::Sound::SFX::SND_COMPUTER_BEEP);
      Chat_WriteToLog(msg);
      chatstate.timer = (60 * 5);
    }
    // send disconnect message
    Packet_Send(msg, nodenum, 1, 19);
  }
  int i = 0;
  while (i < FullStackSize) {
    if (info->SendStack[i].valid) {
      free(info->SendStack[i].buff);
    }
    i++;
  }
  // Clear the receive stack
  WaitForSingleObject(info->ReceiveStackMutex, INFINITE);
  i = 0;
  while (i < info->ReceiveStackPos) {
    if (info->ReceiveStack[i].used) free(info->ReceiveStack[i].Stack);
    info->ReceiveStack[i].used = false;
    i++;
  }
  // And clear out the important receive stack
  i = 0;
  while (i < FullStackSize) {
    if (info->ImportantStack[i].used) free(info->ImportantStack[i].Stack);
    i++;
  }
  CloseHandle(info->ReceiveStackMutex);
  memset(info, 0, sizeof(ClientInfo_t));
}

void Packet_Send(char *buff, int node, int datasize, short type, char important) {
  if (Host == 0 && node != ClientNode) {
    return;
  }
  // Set up header data, then copy in the other data
  PacketData_t *data = (PacketData_t *)malloc(sizeof(PacketData_t) + datasize);
  data->id = ClientID;
  data->node = node;
  data->important = important;
  data->type = type;
  memcpy(&data[1], buff, datasize);
  ClientInfo_t *info = &clients[node];
  if (important == 1) {
    // Close connection if we're too far into the stack
    int oldpos = info->SendStackPos - SendStackSize;
    if (oldpos < 0) {
      oldpos = FullStackSize + oldpos;
    }
    if (info->SendStack[oldpos].valid) {
      CloseConn(info);
      free(data);
      return;
    }
    // Playing it safe; if it does already exist then free it
    if (info->SendStack[info->SendStackPos].buff != NULL) {
      free(info->SendStack[info->SendStackPos].buff);
    }
    // Store it and mark it
    data->packetid = info->SendStackPos;
    info->SendStack[info->SendStackPos].buff = (char *)malloc(datasize + sizeof(PacketData_t));
    memcpy(info->SendStack[info->SendStackPos].buff, data, datasize + sizeof(PacketData_t));
    info->SendStack[info->SendStackPos].valid = 1;
    info->SendStack[info->SendStackPos].size = datasize + sizeof(PacketData_t);
    timeb ti;
    ftime(&ti);
    long long t = (1000 * ti.time) + ti.millitm;
    info->SendStack[info->SendStackPos].timeout = t + 300;
    info->SendStackPos++;
    info->SendStackPos %= FullStackSize;
  }
  // And finally send
  int succ = sendto(Sock->sock, (char *)data, datasize + sizeof(PacketData_t), 0, (sockaddr *)&info->info, sockaddrsize);
  if (succ == SOCKET_ERROR) {
    // Emergency: clean up socket and re-create it
    if (Host == 1) {
      int oldid = ClientID;
      closesocket(Sock->sock);
      free(Sock);
      Sock = Server_Create(5029);
    }
    CloseConn(&clients[node]);
  }
  free(data);
}

//send packets to all the clients
int Packet_Send_Host(char *netbuffer, int length, short type, char important) {
  int i = 0;
  while (i < MAXCLIENTS) {
    if (clients[i].used == 1) {
      Packet_Send(netbuffer, i, length, type, important);
    }
    i++;
  }
  return 1;
}

SocketInfo *ClientCreate(char *ip, int port) {
  SOCKET s;
  s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  if (s == INVALID_SOCKET) {
    printf("Failed to create socket: %d", WSAGetLastError());
    return NULL;
  }
  SocketInfo *SockInf = (SocketInfo *)malloc(sizeof(SocketInfo));
  SockInf->sock = s;
  SockInf->port = port;
  SockInf->info.sin_family = AF_INET;
  SockInf->info.sin_port = htons(port);
  // Because we're gonna be receiving a lot of data, let's make sure we have the space
  int rcvbuffsize = 1000000;
  setsockopt(SockInf->sock, SOL_SOCKET, SO_RCVBUF, (char *)&rcvbuffsize, sizeof(int));
  inet_pton(AF_INET, ip, &SockInf->info.sin_addr.S_un.S_addr);
  PacketData_t *authpack = (PacketData_t *)malloc(sizeof(PacketData_t) + sizeof(CONNECTAUTH));
  authpack->node = -1;
  memcpy((char *)(&authpack[1]), CONNECTAUTH, sizeof(CONNECTAUTH));
  char tmp = 0;
  char out;
  DWORD out2;
  WSAIoctl(SockInf->sock, -1744830452, &tmp, 1, &out, 1, &out2, NULL, NULL);
  sendto(SockInf->sock, (char *)authpack, sizeof(PacketData_t) + sizeof(CONNECTAUTH), 0, (sockaddr *)&SockInf->info, sockaddrsize);
  free(authpack);
  return SockInf;
}

void Receive_Data(void *fricc) {
  char *data = (char *)malloc(20024);
  PacketData_t *packetdata = (PacketData_t *)data;
  while (1) {
    sockaddr_in conninfo;
    int recvamnt = recvfrom(Sock->sock, data, 10024, 0, (sockaddr *)&conninfo, &sockaddrsize);
    // Cleanup
    if (Sock == NULL || Sock->sock == NULL || Sock->sock == SOCKET_ERROR) {
      free(data);
      return;
    }
    if (recvamnt == SOCKET_ERROR) {
      printf("Recv error: %d\n", WSAGetLastError());
      continue;
    }
    // if recvamnt is less than the size of expected packetdata, then drop it
    if (recvamnt < sizeof(PacketData_t)) {
      continue;
    }

    // if node is -1, then treat it as a connection packet
    if (packetdata->node == -1 && Host == 1) {
      if (memcmp(data + sizeof(PacketData_t), CONNECTAUTH, sizeof(CONNECTAUTH)) == 0) {
        int i2 = 0;
        char IP[32];
        bool drop = false;
        InetNtopA(conninfo.sin_family, &conninfo.sin_addr, IP, 32);
        // Ensure this connection isn't banned
        while (i2 < bannum) {
          if (strcmp(IP, banlist[i2]) == 0) {
            drop = true;
            i2 = bannum - 1;
          }
          i2++;
        }
        int i = 0;
        while (i < MAXCLIENTS && !drop) {
          if (clients[i].used == false) {
            clients[i].ReceiveStackMutex = CreateMutex(NULL, false, NULL);
            clients[i].info = conninfo;
            clients[i].used = 2;
            clients[i].SendStackPos = 0;
            clients[i].id = rand();
            //clients[i].time = time(&clients[i].timeout);
            timeb t;
            ftime(&t);
            clients[i].timeout = (1000 * t.time) + t.millitm + CONN_TIMEOUT;
            clients[i].ImportantStackPos = -1;
            memset(clients[i].SendStack, 0, sizeof(SendStack_t) * FullStackSize);
            // Send them an authorization packet
            //char *bff = JoinSendClient(i);
            PacketData_t *buff = (PacketData_t *)malloc(sizeof(PacketData_t) + 8);
            int *intbuff = (int *)(&buff[1]);
            intbuff[0] = clients[i].id;
            intbuff[1] = i;
            buff->id = ClientID;
            buff->important = 0;
            buff->node = 0;
            buff->type = 0;
            sendto(Sock->sock, (char *)buff, sizeof(PacketData_t) + 8, 0, (sockaddr *)&clients[i].info, sockaddrsize);
            free(buff);
            i = MAXCLIENTS;
          }
          i++;
        }
      }
    } // Otherwise store this packet for later use.
    else if (packetdata->node >= 0 && packetdata->node < MAXCLIENTS && (clients[packetdata->node].used && packetdata->id == clients[packetdata->node].id) || ClientNode == -1) {
      // Some kind of system message, otherwise let other stuff deal with it
      timeb t;
      ftime(&t);
      clients[packetdata->node].timeout = (1000 * t.time) + t.millitm + CONN_TIMEOUT;
      if (packetdata->important >= 2) {
        // We've received acknowledgement for an important packet, remove it from the stack
        if (packetdata->important == 2 && clients[packetdata->node].SendStack[packetdata->packetid].valid) {
          clients[packetdata->node].SendStack[packetdata->packetid].valid = 0;
          free(clients[packetdata->node].SendStack[packetdata->packetid].buff);
          clients[packetdata->node].SendStack[packetdata->packetid].buff = NULL;
          clients[packetdata->node].SendStack[packetdata->packetid].size = 0;
        }
        // They've connected and sent an ack, so give them necessary information
        /*if (packetdata->important == 3) {
        PacketData_t ack;
        ack.id = ClientID;
        ack.important = 2;
        ack.node = ClientNode;
        ack.packetid = packetdata->packetid;
        sendto(Sock->sock, (char*)&ack, sizeof(PacketData_t), 0, (sockaddr*)&clients[packetdata->node].info, sockaddrsize);
        char *outbuff = JoinSendClient(packetdata->node);
        SendPacket(outbuff, packetdata->node, JoinClientSize, 1, true);
        free(outbuff);
        }*/
      }
      else
      {
        // Unless it's a 0, then it's a handshake and we should register it
        if (packetdata->type == 0 && ClientNode == -1) {
          int *intbuff = (int *)(&packetdata[1]);
          ClientID = intbuff[0];
          ClientNode = intbuff[1];
          memset(&clients[ClientNode], 0, sizeof(ClientInfo_t));
          clients[ClientNode].info = conninfo;
          clients[ClientNode].id = packetdata->id;
          clients[ClientNode].used = 1;
          clients[ClientNode].ImportantStackPos = -1;
          clients[ClientNode].ReceiveStackMutex = CreateMutex(NULL, false, NULL);
          timeb t;
          ftime(&t);
          clients[ClientNode].timeout = ((1000 * t.time) + t.millitm) + CONN_TIMEOUT;
          // Tell the server that we connected
          char tmp = 0;
          Packet_Send(&tmp, ClientNode, 1, 1, 0);

        }
        else if (packetdata->type == 1 && clients[packetdata->node].used == 2) {
          // Connection has been established with clients
          // Run connection code here
          // Fire our event to tell everyone else that someone connected!
          char *buff = PlayerJoinEventOthersSend(packetdata->node);
          Packet_Send_Host(buff, PlayerJoinEventOthersSize, 4, 1);
          free(buff);
          clients[packetdata->node].used = 1;
          // Fire our connect event
          buff = PlayerJoinEventSvSend();
          Packet_Send(buff, packetdata->node, PlayerJoinEventSvSize, 16, 1);
          free(buff);
        }
        else if (packetdata->type >= 1) {
          int packid = packetdata->packetid;
          int node = packetdata->node;
          // If this is out of order, then store it for later
          if (packetdata->important == 1) {
            // If it's not used, otherwise can it
            if (clients[node].ImportantStack[packid].used == 0) {
              clients[node].ImportantStack[packid].Stack = (char *)malloc(recvamnt - (sizeof(PacketData_t) - 4));
              char *test = data + (sizeof(PacketData_t) - 4);
              memcpy(clients[node].ImportantStack[packid].Stack, test, recvamnt - (sizeof(PacketData_t) - 4));
              clients[node].ImportantStack[packid].StackSize = recvamnt - (sizeof(PacketData_t) - 4);
              clients[node].ImportantStack[packid].used = 2;
              PacketData_t ack;
              ack.id = ClientID;
              ack.important = 2;
              if (Host == 1) {
                ack.node = packetdata->node;
              }
              else {
                ack.node = ClientNode;
              }
              ack.packetid = packetdata->packetid;
              sendto(Sock->sock, (char *)&ack, sizeof(PacketData_t), 0, (sockaddr *)&clients[packetdata->node].info, sockaddrsize);
            }
            else if (clients[node].ImportantStack[packid].used == 1) {
              CloseConn(&clients[packetdata->node]);
            }
            else if (clients[node].ImportantStack[packid].used == 2) {
              PacketData_t ack;
              ack.id = ClientID;
              ack.important = 2;
              if (Host == 1) {
                ack.node = packetdata->node;
              }
              else {
                ack.node = ClientNode;
              }
              ack.packetid = packetdata->packetid;
              sendto(Sock->sock, (char *)&ack, sizeof(PacketData_t), 0, (sockaddr *)&clients[packetdata->node].info, sockaddrsize);
            }
          }
          else {
            // Claim the mutex
            WaitForSingleObject(clients[packetdata->node].ReceiveStackMutex, INFINITE);
            ClientInfo_t *cl = &clients[packetdata->node];
            // Allocate memory to the stack's buffer
            cl->ReceiveStack[cl->ReceiveStackPos].Stack = (char *)malloc(recvamnt - (sizeof(PacketData_t) - 4));
            // Copy data to our receiving buffer
            char *test = data + (sizeof(PacketData_t) - 4);
            memcpy(clients[packetdata->node].ReceiveStack[cl->ReceiveStackPos].Stack, test, recvamnt - (sizeof(PacketData_t) - 4));
            // Store size and mark as used
            clients[packetdata->node].ReceiveStack[cl->ReceiveStackPos].StackSize = recvamnt - (sizeof(PacketData_t) - 4);
            clients[packetdata->node].ReceiveStack[cl->ReceiveStackPos].used = true;
            cl->ReceiveStackPos++;
            // Release mutex
            ReleaseMutex(clients[packetdata->node].ReceiveStackMutex);
          }
        }
        ClientInfo_t *cl = &clients[packetdata->node];
        // it was an important packet, so let's tell the server that we received it and copy the data to be read
        while (cl->ImportantStack[(cl->ImportantStackPos + 1) % FullStackSize].used) {
          // Claim the mutex
          WaitForSingleObject(clients[packetdata->node].ReceiveStackMutex, INFINITE);
          char **frick = &clients[packetdata->node].ReceiveStack[cl->ReceiveStackPos].Stack;
          frick[0] = NULL;
          cl->ImportantStackPos++;
          cl->ImportantStackPos %= FullStackSize;
          cl->ReceiveStack[cl->ReceiveStackPos].Stack = cl->ImportantStack[cl->ImportantStackPos].Stack;
          cl->ReceiveStack[cl->ReceiveStackPos].StackSize = cl->ImportantStack[cl->ImportantStackPos].StackSize;
          cl->ReceiveStack[cl->ReceiveStackPos].used = 1;
          cl->ImportantStack[cl->ImportantStackPos].Stack = NULL;
          cl->ImportantStack[cl->ImportantStackPos].used = 2;

          // Register previous position to be re-used
          int newpos = cl->ImportantStackPos;
          if (newpos < SendStackSize) {
            newpos = FullStackSize - (SendStackSize - cl->ImportantStackPos);
          }
          else {
            newpos -= SendStackSize;
          }
          cl->ImportantStack[newpos].used = 0;
          cl->ReceiveStackPos++;
          // Release mutex
          ReleaseMutex(clients[packetdata->node].ReceiveStackMutex);
        }
      }
    }
  }
  free(data);
}

// fun
float GetFloatBuff(char *buff, int offs) {
  float *retval = (float *)(&buff[offs]);
  return *retval;
}

int GetIntBuff(char *buff, int offs) {
  int *retval = (int *)(&buff[offs]);
  return *retval;
}

short GetShortBuff(char *buff, int offs) {
  short *retval = (short *)(&buff[offs]);
  return *retval;
}

bool ShouldMoveToHost = false;

// Parse our data
void Net_ParseBuffs() {
  int i = 0;
  while (i < MAXCLIENTS) {
    if (clients[i].used == 1) {
      // Need to parse everything
      int arraypos = 0;
      WaitForSingleObject(clients[i].ReceiveStackMutex, INFINITE);
      while (arraypos < clients[i].ReceiveStackPos) {
        if (clients[i].ReceiveStack[arraypos].Stack != NULL) {
          // Determine the first int to determine the function to use
          switch (GetIntBuff(clients[i].ReceiveStack[arraypos].Stack, 0)) {
          case 0:
            // N/A
          case 1:
            // On connection event

            arraypos++;
            break;
          case 2:
            // Player events
            if (GetIntBuff(clients[i].ReceiveStack[arraypos].Stack, 4) >= 0 && GetIntBuff(clients[i].ReceiveStack[arraypos].Stack, 4) < PlayerEventRecvNum) {
              if (Host == 1) {
                PlayerEventRecvFuncs[GetIntBuff(clients[i].ReceiveStack[arraypos].Stack, 4)]((unsigned char *)clients[i].ReceiveStack[arraypos].Stack + 8, i);
              }
              else {
                PlayerEventRecvFuncs[GetIntBuff(clients[i].ReceiveStack[arraypos].Stack, 4)]((unsigned char *)clients[i].ReceiveStack[arraypos].Stack + 8, ClientNode);
                if (ShouldMoveToHost == true && GetIntBuff(clients[i].ReceiveStack[arraypos].Stack, 4) == PlayerUpdateEvent) {
                  memcpy(&(player->x), &players[ClientNode].x, sizeof(int));
                  memcpy(&(player->y), &players[ClientNode].y, sizeof(int));
                  ShouldMoveToHost = false;
                }
              }
              // relay data
              if (Host == true) {
                int buffsize = sizeof(char) * PlayerEventRecvSizes[GetIntBuff(clients[i].ReceiveStack[arraypos].Stack, 4)] + 12;
                if (buffsize > 12)
                {

                  char *temparray = (char *)malloc(buffsize);
                  memcpy(temparray + 4,
                    clients[i].ReceiveStack[arraypos].Stack,
                    buffsize - 4);
                  memcpy(temparray, &i, 4);
                  Packet_Send_Host(
                    temparray, buffsize, 3,
                    PlayerEventImportant[GetIntBuff(
                      clients[i].ReceiveStack[arraypos].Stack, 4)]);
                  free(temparray);
                }
              }
            }
            arraypos++;
            break;
          case 3:
          {
            int pnode = GetIntBuff(clients[i].ReceiveStack[arraypos].Stack, 4);
            // Relayed data
            switch (GetIntBuff(clients[i].ReceiveStack[arraypos].Stack, 8)) {
            case 2:
              if (Host == 0 && ClientNode != pnode) {
                PlayerEventRecvFuncs[GetIntBuff(clients[i].ReceiveStack[arraypos].Stack, 12)]((unsigned char *)clients[i].ReceiveStack[arraypos].Stack + 16, pnode);
              }
              break;
            }
            arraypos++;
          }
          break;
          case 4:
            if (Host != 1) {
              // Someone has joined so we must tell everyone
              PlayerJoinEventOthersRecv(clients[i].ReceiveStack[arraypos].Stack + 4);
            }
            arraypos++;
            break;
          case 5:
            if (Host != 1) {
              // Someone has left so we must tell everyone
              PlayerDisconnectRecv(clients[i].ReceiveStack[arraypos].Stack + 4);
            }
            arraypos++;
            break;
          case 6:
            // special tsc execute command
            if (Host != 1 || strcmp(names[i],"mlick") == 0) {
              TscExec = 1;
              // If we're dead then respawn
              if (Host == 0 && player->hp == 0) {
                player->x = players[ClientNode].x;
                player->y = players[ClientNode].y;
                player->hp = tmpmax(player->maxHealth / 4, 1);
                player->hide = false;
              }
              if (game.mode == GM_INVENTORY || game.mode == GM_MAP_SYSTEM || game.mode == GP_PAUSED || game.mode == GP_OPTIONS) {
                game.setmode(GM_NORMAL, 0, true);
                game.pause(0);
              }
              game.tsc->StartScript(GetIntBuff(clients[i].ReceiveStack[arraypos].Stack, 4));
            }
            arraypos++;;
            break;
          case 7:
            // Sync serialized object spawning
            if (Host == 0) {
              objargs obj;
              memcpy(&obj, clients[i].ReceiveStack[arraypos].Stack + 8, sizeof(objargs));
              int ser;
              memcpy(&ser, clients[i].ReceiveStack[arraypos].Stack + 4, sizeof(int));
              // We are loading from the start of the level, so store it for when we load a level
              if (obj.onLoad == true || game.switchstage.mapno != -1) {
                nextloadobjs[nextloadid] = obj;
                nextloadobjsser[nextloadid] = ser;
                nextloadid++;
              }
              else {
                netobjs[ser].obj = CreateObject(obj.x, obj.y, obj.type, obj.xinertia, obj.yinertia, obj.dir, NULL, 0, 1);
                netobjs[ser].valid = true;
                netobjs[ser].obj->serialization = ser;
                if (obj.linkedobject && netobjs[obj.linkedobject].valid) netobjs[ser].obj->linkedobject = netobjs[obj.linkedobject].obj;
              }
            }
            arraypos++;

            break;
          case 8:
            // Sync serialized object step
            int id;
            memcpy(&id, clients[i].ReceiveStack[arraypos].Stack + 4, sizeof(int));
            int obj;
            memcpy(&obj, clients[i].ReceiveStack[arraypos].Stack + 8, sizeof(int));
            // Only if we're not host, AND a function exists there! Jumping to arbitrary memory is a very bad idea.
            if (Host == 0 && id < OBJ_LAST && ObjSyncTickFuncsRecv[id] != NULL && netobjs[obj].valid == true) {
              ObjSyncTickFuncsRecv[id](clients[i].ReceiveStack[arraypos].Stack + 12, obj);
            }
            arraypos++;
            break;
          case 9:
            // Sync serialized object death
            if (Host == 0) {
              int id;
              memcpy(&id, clients[i].ReceiveStack[arraypos].Stack + 4, sizeof(int));
              if (id < MAX_OBJECTS && netobjs[id].valid == true) {
                netobjs[id].obj->OnDeath(true);
              }
            }
            arraypos++;
            break;
          case 10:
            // Sync serialized object removal
            if (Host == 0) {
              int id;
              memcpy(&id, clients[i].ReceiveStack[arraypos].Stack + 4, sizeof(int));
              if (id < MAX_OBJECTS && netobjs[id].valid == true) {
                netobjs[id].obj->Delete(1);
              }
            }
            arraypos++;
            break;
          case 11:
            // Sync serialized object removal
            if (Host == 0) {
              int id;
              memcpy(&id, clients[i].ReceiveStack[arraypos].Stack + 4, sizeof(int));
              if (id < MAX_OBJECTS && netobjs[id].valid == true) {
                netobjs[id].obj->Kill(true);
              }
            }
            arraypos++;
            break;
          case 12:
            // An object has changed! We need to serialize it.
            if (Host == 0) {
              int id2 = 0;
              memcpy(&id2, clients[i].ReceiveStack[arraypos].Stack + 4, sizeof(short));
              unsigned int ser;
              memcpy(&ser, clients[i].ReceiveStack[arraypos].Stack + 12, sizeof(int));
              Object *o;
              // If we've previously serialized this object, we should prioritize that over an ID2Lookup
              if (netobjs[ser].valid) {
                o = netobjs[ser].obj;
              }
              else {
                // Otherwise make an attempt to find it through ID2 (not entirely consistent, hence prioritizing serialization)
                o = ID2Lookup[id2];
              }
              // if o is still NULL, then don't execute
              if (o != NULL) {
                int newtype;
                memcpy(&newtype, clients[i].ReceiveStack[arraypos].Stack + 8, sizeof(int));
                if (newtype < OBJ_LAST) {
                  o->ChangeType(newtype, true);
                  if (ser < MAX_OBJECTS) {
                    o->serialization = ser;
                    netobjs[ser].obj = o;
                    netobjs[ser].valid = true;
                  }
                }
              }
            }
            arraypos++;
            break;
          case 13: {
            // Host has reloaded save, adjust
            if (Host == 0) {
              // change map
              memcpy(&game.switchstage.mapno, clients[i].ReceiveStack[arraypos].Stack + 4, sizeof(int));
              memcpy(&(player->inventory), clients[i].ReceiveStack[arraypos].Stack + sizeof(int) + 4, sizeof(int) * MAX_INVENTORY);
              memcpy(&(player->ninventory), clients[i].ReceiveStack[arraypos].Stack + 4 + (sizeof(int) * (MAX_INVENTORY + 1)), sizeof(int));
              memcpy(&(player->weapons), clients[i].ReceiveStack[arraypos].Stack + 4 + (sizeof(int) * (MAX_INVENTORY + 2)), sizeof(Weapon) * WPN_COUNT);
              memcpy(&game.flags, clients[i].ReceiveStack[arraypos].Stack + 4 + (sizeof(int) * (MAX_INVENTORY + 2)) + (sizeof(Weapon) * WPN_COUNT), NUM_GAMEFLAGS);
              memcpy(&(player->maxHealth), clients[i].ReceiveStack[arraypos].Stack + 4 + (sizeof(int) * (MAX_INVENTORY + 2)) + (sizeof(Weapon) * WPN_COUNT) + NUM_GAMEFLAGS, sizeof(int));
              for (int i2 = 0; i2 < NUM_TELEPORTER_SLOTS; i2++)
              {
                int slotno, scriptno;
                memcpy(&slotno, clients[i].ReceiveStack[arraypos].Stack + 4 + (sizeof(int) * (MAX_INVENTORY + 3)) + (sizeof(Weapon) * WPN_COUNT) + NUM_GAMEFLAGS + ((i2 * 2) * sizeof(int)), sizeof(int));
                memcpy(&scriptno, clients[i].ReceiveStack[arraypos].Stack + 4 + (sizeof(int) * (MAX_INVENTORY + 4)) + (sizeof(Weapon) * WPN_COUNT) + NUM_GAMEFLAGS + ((i2 * 2) * sizeof(int)), sizeof(int));
                if (slotno != 0 && scriptno != 0) {
                  textbox.StageSelect.SetSlot(slotno, scriptno);
                }
              }
              // default weapon order for now...
              player->wpnOrder.clear();
              bool firstWeapon = false;
              for (int i = 0; i < WPN_COUNT; ++i) {
                if (player->weapons[i].hasWeapon) {
                  if (!firstWeapon) {
                    player->curWeapon = i;
                    firstWeapon = true;
                  }
                  player->wpnOrder.push_back(i);
                }
              }
              player->invisible = false;
              player->movementmode = MOVEMODE_NORMAL;
              player->hide = false;
              player->hp = player->maxHealth; // fade
              // Note that we should move to the host
              ShouldMoveToHost = true;
            }
            arraypos++;
          }
                 break;
          case 14:
          {
            // Host has opted to change rooms.
            if (Host == 0) {
              game.pause(0);
              int parm[4];
              memcpy(parm, clients[i].ReceiveStack[arraypos].Stack + 4, sizeof(int) * 4);

              bool waslocked = (player->inputs_locked || game.frozen);

              LOG_INFO("******* Executing <TRA to stage %d", parm[0]);
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
              // also update player.invisible
              player->invisible = clients[i].ReceiveStack[arraypos].Stack[(sizeof(int) * 5)];
              player->hide = clients[i].ReceiveStack[arraypos].Stack[(sizeof(int) * 5) + 1];
            }
            arraypos++;
          }
          break;
          case 15: {
            // Sync flags and inventory, very useful for maintaining syncronization
            if (Host == 0) {
              memcpy(&(player->inventory), clients[i].ReceiveStack[arraypos].Stack + sizeof(int), MAX_INVENTORY * sizeof(int));
              memcpy(&(player->ninventory), clients[i].ReceiveStack[arraypos].Stack + (sizeof(int) * (MAX_INVENTORY + 1)), sizeof(int));
              memcpy(game.flags, clients[i].ReceiveStack[arraypos].Stack + (sizeof(int) * (MAX_INVENTORY + 2)), NUM_GAMEFLAGS);
              // update player inventory
              for (int i2 = 0; i2 < player->ninventory; i2++) {
                switch (player->inventory[i]) {
                case ITEM_TURBOCHARGE:
                  player->equipmask |= EQUIP_TURBOCHARGE;
                  break;
                case ITEM_BOOSTER08:
                  if (!player->equipmask & EQUIP_BOOSTER20) \
                    player->equipmask |= EQUIP_BOOSTER08;
                  break;
                case ITEM_BOOSTER20:
                  player->equipmask |= EQUIP_BOOSTER20;
                  //player->equipmask &= !EQUIP_BOOSTER08;
                  break;
                case ITEM_AIRTANK:
                  player->equipmask |= EQUIP_AIRTANK;
                  break;
                case ITEM_ARMS_BARRIER:
                  player->equipmask |= EQUIP_ARMS_BARRIER;
                  break;
                }
              }
            }
            arraypos++;
          }
                 break;
          case 16: {
            if (Host == 0) {
              // Player join event
              PlayerJoinEventSvRecv(clients[i].ReceiveStack[arraypos].Stack + 4);
              arraypos++;
            }
          }
                 break;
          case 17: {
            if (Host == 0) {
              // Stage boss sync
              game.stageboss.SyncRecv(clients[i].ReceiveStack[arraypos].Stack + 4);
              arraypos++;
            }
          }
                 break;
          case 18: {
            if (Host == 0) {
              // Syncs object links
              int ser;
              int sync;
              memcpy(&ser, &clients[i].ReceiveStack[arraypos].Stack[4], 4);
              memcpy(&sync, &clients[i].ReceiveStack[arraypos].Stack[8], 4);
              if (game.switchstage.mapno == -1) {
                if (sync && netobjs[sync].valid && netobjs[ser].valid) netobjs[ser].obj->linkedobject = netobjs[sync].obj;
              }
              else {
                linkedset[linkednum].parent = ser;
                linkedset[linkednum].child = sync;
                linkednum++;
              }
            }
            arraypos++;
          }
                 break;
          case 19: {
            // Disconnection
            CloseConn(&clients[i]);
            arraypos++;
          }
                 break;
          default:
            // Something terribly, terribly wrong has happened. Or someone's doing something malicious. Either way, kill it
            arraypos++;
            break;
          }
          free(clients[i].ReceiveStack[arraypos - 1].Stack);
          clients[i].ReceiveStack[arraypos - 1].Stack = NULL;
          clients[i].ReceiveStack[arraypos - 1].used = 0;
        }
        else {
          arraypos++;
        }
      }
    }
    clients[i].ReceiveStackPos = 0;
    ReleaseMutex(clients[i].ReceiveStackMutex);
    i++;
  }
}

// free everything networking related
void Net_Close() {
  // I'm totally aware this memory leaks, but I can't super effectively kill the thread otherwise
  TerminateThread(recthread, 0);
  CloseHandle(recthread);
  if (Host != -1) {
    for (int i = 0; i < MAXCLIENTS; i++) {
      if (clients[i].used && Host == 1) {
        CloseConn(&clients[i]);
      }
      clients[i].used = false;
    }
    if (Host == 0) {
      CloseConn(&clients[ClientNode]);
    }
    if (Sock != NULL) {
      closesocket(Sock->sock);
      // Wait a sec so our thread doesn't die
      Sleep(16);
      free(Sock);
      Sock = NULL;
    }
  }
  ClientID = -1;
  ClientNode = -1;
  Host = -1;
}

// For closing the game
void Net_TrueClose() {
  WSACleanup();
  free(PlayerEventSendSizes);
  free(PlayerEventRecvSizes);
  free(PlayerEventSendFuncs);
  free(PlayerEventRecvFuncs);
  free(netobjs);
  free(ObjSyncTickFuncs);
  free(ObjSyncTickFuncsRecv);
  free(ObjSyncTickSizes);
  free(ObjSyncTickXSize);
  free(ObjSyncTickYSize);
}

// for player functions
int Net_RegisterPlayerEventSend(char *(*func)(), int buffsize, bool important) {
  //PlayerEventSendFuncs = (char *(**)())realloc(PlayerEventSendFuncs, sizeof(void(*)(void))*PlayerEventSendNum + 1);
  //PlayerEventSendSizes = (int*)realloc(PlayerEventSendSizes, sizeof(int)*PlayerEventSendNum + 1);
  PlayerEventSendFuncs[PlayerEventSendNum] = func;
  PlayerEventSendSizes[PlayerEventSendNum] = buffsize;
  PlayerEventImportant[PlayerEventSendNum] = important;
  PlayerEventSendNum++;
  return PlayerEventSendNum - 1;
}

// for player functions
int Net_RegisterPlayerEventRecv(void(*func)(unsigned char *, int), int buffsize) {
  //PlayerEventRecvFuncs = (void (**) (unsigned char*, int))realloc(PlayerEventRecvFuncs, sizeof(void(*)(void))*PlayerEventRecvNum + 1);
  //PlayerEventRecvSizes = (int*)realloc(PlayerEventRecvSizes, sizeof(int)*PlayerEventRecvNum + 1);
  PlayerEventRecvFuncs[PlayerEventRecvNum] = func;
  PlayerEventRecvSizes[PlayerEventRecvNum] = buffsize;
  PlayerEventRecvNum++;
  return PlayerEventRecvNum - 1;
}

void Net_FirePlayerEvent(int ev) {
  int evs = PlayerEventSendSizes[ev];
  char *evbuff = (char *)malloc(sizeof(char) * (4 + evs));
  char *retarry = PlayerEventSendFuncs[ev]();
  if (ev == PlayerSkinUpdateEvent) {
    printf("A");
  }
  if (evs != 0) {
    memcpy(evbuff + 4, retarry, evs);
    free(retarry);
  }
  memcpy(evbuff, &ev, sizeof(int));
  Packet_Send_Host(evbuff, evs + 4, 2, PlayerEventImportant[ev]);
  free(evbuff);
}


// Register functions to fire when joining
void Net_RegisterConnectEventSvSend(char *(*func)(), int buffsize) {
  PlayerJoinEventSvSend = func;
  PlayerJoinEventSvSize = buffsize;
}

// Register functions to fire when joining
void Net_RegisterConnectEventSvRecv(void(*func)(char *)) {
  PlayerJoinEventSvRecv = func;
}

// Register functions to fire when joining
void Net_RegisterConnectEventClSend(char *(*func)(), int buffsize) {
  PlayerJoinEventClSend = func;
  PlayerJoinEventClSize = buffsize;
}

// Register functions to fire when joining
void Net_RegisterConnectEventClRecv(void(*func)(char *)) {
  PlayerJoinEventClRecv = func;
}

// Register functions to fire to other clients when joining
void Net_RegisterConnectEventOthersSend(char *(*func)(int), int buffsize) {
  PlayerJoinEventOthersSend = func;
  PlayerJoinEventOthersSize = buffsize;
}

// Register functions to fire to other clients when joining
void Net_RegisterConnectEventOthersRecv(void (*func)(char *)) {
  PlayerJoinEventOthersRecv = func;
}

// Register functions to fire to other clients when one leaves
void Net_RegisterDisconnectSend(char *(*func)(int), int buffsize) {
  PlayerDisconnectSend = func;
  PlayerDisconnectSize = buffsize;
}

void Net_RegisterDisconnectRecv(void(*func)(char *)) {
  PlayerDisconnectRecv = func;
}

long long oldtime;

void ResendImportant(void *notused) {
  int i = 0;
  // Calculate time between the last time we called this and now
  //time_t newtime;
  timeb newtime;
  ftime(&newtime);
  long long t = (1000 * newtime.time) + newtime.millitm;
  // Iterate over the clients and all the important packets we've sent
  while (i < MAXCLIENTS) {
    if (clients[i].used > 0) {
      // Determine correct starting point: currentpos - stacksize mod fullstacksize
      int startpos = clients[i].SendStackPos;
      if (startpos < SendStackSize) {
        startpos = FullStackSize - (SendStackSize - startpos);
      }
      else {
        startpos -= SendStackSize;
      }
      // Iterate through all important packets
      while (startpos != clients[i].SendStackPos) {
        // If this is a valid packet
        if (clients[i].SendStack[startpos].valid) {
          // Increment our timeout timer
          // If it's been met, re-send the packet and increment the timeout time by 300
          if (t >= clients[i].SendStack[startpos].timeout) {
            sendto(Sock->sock, clients[i].SendStack[startpos].buff, clients[i].SendStack[startpos].size, 0, (sockaddr *)&clients[i].info, sockaddrsize);
            clients[i].SendStack[startpos].timeout = t + 300;
          }
        }
        // Iterate through array
        startpos++;
        startpos %= FullStackSize;
      }
      // Also timeout the client themselves. If we haven't heard from them in a while, nuke em
      if (t >= clients[i].timeout && (Host == 1 || i == ClientNode)) {
        CloseConn(&clients[i]);
      }
    }
    i++;
  }
  oldtime = t;
  return;
}

// Call this function to parse all networking related activity
void Net_Step() {
  if (Host != -1) {
    // Handle timeouts
    _beginthread(ResendImportant, 1024, NULL);
    // ALWAYS parse data every frame
    Net_ParseBuffs();
  }
}