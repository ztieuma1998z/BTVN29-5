// ChatServer.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <stdio.h>
#include <winsock2.h>

#pragma comment(lib, "ws2_32")
#pragma warning(disable:4996)

typedef struct Client {
	SOCKET client;
	bool isRegistered;
	char* id;
}Client;

CRITICAL_SECTION cs;
const char* welcomeMess = "HELLO. PLEASE ENTER YOUR ID BY THIS SYNTAX \"[CONNECT] id\" \n";
const char* connectOK = "[CONNECT] OK\n";
const char* connectError = "[CONNECT] ERROR ";
const char* connectErrorWrongSyntax = "[CONNECT] ERROR Wrong syntax\n";
const char* disConnOK = "[DISCONNECT] OK\n";
const char* disConnError = "[DISCONNECT] ERROR ";
const char* error = "[ERROR] ";
const char* sendOK = "[SEND] OK\n";
const char* sendError = "[SEND] ERROR ";
const char* userConn = "[USER] CONNECT ";
const char* userDisConn = "[USER] DISCONNECT ";

Client clients[64];
int numberClients = 0;


//void removeClient(SOCKET*, int*, int);
DWORD WINAPI ClientThread(LPVOID);

bool checkIdExist(char* id) {
	for (int i = 0; i < numberClients; i++)
	{
		if (strcmp(clients[i].id, id) == 0)
		{
			return true;
		}
	}
	return false;
}

int checkIdOnline(char* id) {
	if (numberClients == 0)
	{
		return -2;
	}
	for (int i = 0; i < numberClients; i++)
	{
		if (strcmp(clients[i].id, id) == 0)
		{
			if (clients[i].isRegistered == true) {
				return -1;//status id is registered
			}
			else {
				return i;//status id is not registered
			}
		}
	}
	return -2;//not found
}

int main()
{
	InitializeCriticalSection(&cs);

	WSADATA wsa;
	WSAStartup(MAKEWORD(2, 2), &wsa);

	SOCKADDR_IN addr;
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	addr.sin_port = htons(9000);

	SOCKET listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	bind(listener, (SOCKADDR*)&addr, sizeof(addr));
	listen(listener, 5);

	while (true)
	{
		printf("Waiting for new clients...\n");
		SOCKET client = accept(listener, NULL, NULL);
		CreateThread(0, 0, ClientThread, &client, 0, 0);
		send(client, welcomeMess, strlen(welcomeMess), 0);
	}

	// Chap nhan ket noi va truyen nhan du lieu
	DeleteCriticalSection(&cs);
}

int Login(SOCKET);
int List(SOCKET);
int SendAll(SOCKET, char*, char*);
int SendUser(SOCKET, char*, char*);
int Disconnect(SOCKET);

DWORD WINAPI ClientThread(LPVOID lpParam) {
	SOCKET client = *(SOCKET*)lpParam;
	printf("New client is accepted: %d.\n", client);
	char buf[256];
	int ret;
	char cmd[16], id[32], content[32];
	Login(client);
	while (true)
	{
		ret = recv(client, buf, sizeof(buf), 0);
		if (ret <= 0) {
			char mess[32];
			strcpy(mess, sendError);
			strcat(mess, "\n\0");
			send(client, mess, strlen(mess), 0);
		}
		buf[ret] = 0;
		printf("Received: %s\n", buf);
		ret = sscanf(buf, "%s %s %s", cmd, id, content);
		printf("Received: %s\n", cmd, id, content);
		if (strcmp(cmd, "[LIST]") == 0) {
			printf("%d, \n", ret);
			if (ret != 1)
			{
				char mess[32];
				strcpy(mess, error);
				strcat(mess, "Wrong Syntax");
				strcat(mess, "\n\0");
				send(client, mess, strlen(mess), 0);
			}
			else {
				List(client);
			}
		}
		else if (strcmp(cmd, "[SEND]") == 0)
		{
			printf("%d, \n", ret);
			if (ret != 3) {
				char mess[32];
				strcpy(mess, sendError);
				strcat(mess, "Wrong Syntax");
				strcat(mess, "\n\0");
				send(client, mess, strlen(mess), 0);
			}
			if (strcmp(id, "ALL") == 0)
			{
				SendAll(client, id, content);
			}
			else {
				SendUser(client, id, content);
			}
		}
		else if (strcmp(cmd, "[DISCONNECT]") == 0)
		{
			if (ret != 1) {
				char mess[32];
				strcpy(mess, disConnError);
				strcat(mess, "Wrong Syntax");
				strcat(mess, "\n\0");
				send(client, mess, strlen(mess), 0);
			}
			else {
				Disconnect(client);
			}
		}
		else {
			char mess[32];
			strcpy(mess, error);
			strcat(mess, "Command is not found");
			strcat(mess, "\n\0");
			send(client, mess, strlen(mess), 0);
		}
	}
	return 0;
}


int Login(SOCKET client) {
	char buf[256];
	char cmd[16], id[32], tmp[32];
	int ret;

	while (true)
	{
		ret = recv(client, buf, sizeof(buf), 0);
		if (ret <= 0) {
			send(client, connectError, strlen(connectError), 0);
			closesocket(client);
			return 0;
		}
		buf[ret] = 0;
		ret = sscanf(buf, "%s %s %s", cmd, id, tmp);
		if (ret == 2) {
			if (strcmp(cmd, "[CONNECT]") == 0) {
				int checkID = checkIdOnline(id);
				if (numberClients > 0 && checkID == -1) {
					char mess[64];
					strcpy(mess, connectError);
					strcat(mess, "Id is exist");
					send(client, mess, strlen(mess), 0);
				}
				else {
					send(client, connectOK, strlen(connectOK), 0);
					printf("New client logs in successful: %d.\n", client);
					EnterCriticalSection(&cs);
					//Save the client;
					if (checkID != -1 && checkID != -2)
					{
						clients[checkID].isRegistered = true;
					}
					else {
						char mess[32];
						strcpy(mess, userConn);
						strcat(mess, " ");
						strcat(mess, id);
						strcat(mess, "\n,\0");
						if (numberClients > 0) {
							for (int i = 0; i < numberClients; i++)
							{
								if (clients[i].isRegistered == true) {
									send(clients[i].client, mess, strlen(mess), 0);
								}
							}
						}
						clients[numberClients].id = (char*)malloc(strlen(id) + 1);
						memcpy(clients[numberClients].id, id, strlen(id) + 1);
						clients[numberClients].client = client;
						clients[numberClients].isRegistered = true;
					}
					//End Saving
					numberClients++;
					LeaveCriticalSection(&cs);
					break;
					return 1;
				}
			}
			else {
				send(client, connectErrorWrongSyntax, strlen(connectErrorWrongSyntax), 0);
			}
		}
		else
		{
			send(client, connectErrorWrongSyntax, strlen(connectErrorWrongSyntax), 0);
		}
	}
}

int List(SOCKET client) {
	char sendBuf[2048] = "[LIST] OK ";
	for (int i = 0; i < numberClients; i++)
	{
		if (client != clients[i].client && clients[i].isRegistered == true)
		{
			char* clientId = clients[i].id;
			strcat(sendBuf, clientId);
			strcat(sendBuf, "\n");
		}
	}
	strcat(sendBuf, "\0");
	printf("buf: %s\n", sendBuf);
	send(client, sendBuf, strlen(sendBuf), 0);
	return 1;
}

int SendAll(SOCKET client, char* id, char* content) {
	char sendBuf[2048] = "[MESSAGE] ALL ";
	strcat(sendBuf, content);
	strcat(sendBuf, "\n\0");
	printf("buf: %s\n", sendBuf);
	if (numberClients <= 1) {
		char mess[64];
		strcpy(mess, sendError);
		strcat(mess, "All user are offline");
		strcat(mess, "\n\0");
		send(client, mess, strlen(mess), 0);
		return 0;
	}
	for (int i = 0; i < numberClients; i++)
	{
		if (client != clients[i].client && clients[i].isRegistered == true) {
			send(clients[i].client, sendBuf, strlen(sendBuf), 0);
		}
	}
	return 1;
}

int SendUser(SOCKET client, char* id, char* content) {
	char sendBuf[2048] = "[MESSAGE] ";
	strcat(sendBuf, id);
	strcat(sendBuf, " ");
	strcat(sendBuf, content);
	strcat(sendBuf, "\n\0");
	printf("buf: %s\n", sendBuf);
	for (int i = 0; i < numberClients; i++)
	{
		if (strcmp(id, clients[i].id) == 0 && clients[i].client != client) {
			if (clients[i].isRegistered == true) {
				send(clients[i].client, sendBuf, strlen(sendBuf), 0);
			}
			else {
				char mess[32];
				strcpy(mess, sendError);
				strcat(mess, "User is offline");
				strcat(mess, "\n\0");
				send(client, mess, strlen(mess), 0);
				return 0;
			}
		}
	}
	return 1;
}

int Disconnect(SOCKET client) {
	char id[16];
	for (int i = 0; i < numberClients; i++)
	{
		if (clients[i].client == client) {
			strcpy(id, clients[i].id);
		}
	}
	char mess[32];
	strcpy(mess, userDisConn);
	strcat(mess, " ");
	strcat(mess, id);
	strcat(mess, "\n,\0");
	for (int i = 0; i < numberClients; i++)
	{
		if (clients[i].isRegistered == true && client != clients[i].client) {
			send(clients[i].client, mess, strlen(mess), 0);
		}
	}
	for (int i = 0; i < numberClients; i++)
	{
		if (clients[i].client == client) {
			clients[i].isRegistered = false;
		}
	}
	return 1;
}

// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file
