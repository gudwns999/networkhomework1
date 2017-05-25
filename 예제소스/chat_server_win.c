/*
* chat_server_win.c
* VidualStudio���� console���α׷����� �ۼ��� multithread-DLL�� �����ϰ�
*  ���϶��̺귯��(ws2_32.lib)�� �߰��� ��.
*/

#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <process.h>

#define BUFSIZE 100

DWORD WINAPI ClientConn(void *arg);
void SendMSG(char* message, int len);
void ErrorHandling(char *message);

int clntNumber = 0;

typedef struct memberList {
	char userId[20];
}MEMBERLIST;

SOCKET clntSocks[10];
HANDLE hMutex;

MEMBERLIST member[11];

int main(int argc, char **argv)
{
	int i = 0;
	WSADATA wsaData;
	SOCKET servSock;
	SOCKET clntSock;

	SOCKADDR_IN servAddr;
	SOCKADDR_IN clntAddr;
	int clntAddrSize;

	HANDLE hThread;
	DWORD dwThreadID;

	if (argc != 2) {
		printf("Usage : %s <port>\n", argv[0]);
		exit(1);
	}
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) /* Load Winsock 2.2 DLL */
		ErrorHandling("WSAStartup() error!");

	hMutex = CreateMutex(NULL, FALSE, NULL);
	if (hMutex == NULL) {
		ErrorHandling("CreateMutex() error");
	}

	servSock = socket(PF_INET, SOCK_STREAM, 0);
	if (servSock == INVALID_SOCKET)
		ErrorHandling("socket() error");

	memset(&servAddr, 0, sizeof(servAddr));
	servAddr.sin_family = AF_INET;
	servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servAddr.sin_port = htons(atoi(argv[1]));

	if (bind(servSock, (SOCKADDR*)&servAddr, sizeof(servAddr)) == SOCKET_ERROR)
		ErrorHandling("bind() error");

	if (listen(servSock, 5) == SOCKET_ERROR)
		ErrorHandling("listen() error");
	printf("���� �ʱ�ȭ\n");
	while (1) {
		clntAddrSize = sizeof(clntAddr);
		clntSock = accept(servSock, (SOCKADDR*)&clntAddr, &clntAddrSize);
		if (clntSock == INVALID_SOCKET)
			ErrorHandling("accept() error");

		WaitForSingleObject(hMutex, INFINITE);
		clntSocks[clntNumber++] = clntSock;
		
		ReleaseMutex(hMutex);
		printf("���ο� ����, Ŭ���̾�Ʈ IP : %s \n", inet_ntoa(clntAddr.sin_addr));

		hThread = (HANDLE)_beginthreadex(NULL, 0, ClientConn, (void*)clntSock, 0, (unsigned *)&dwThreadID);
		if (hThread == 0) {
			ErrorHandling("������ ���� ����");
		}
	}

	WSACleanup();
	return 0;
}

DWORD WINAPI ClientConn(void *arg)
{
	SOCKET clntSock = (SOCKET)arg;
	int strLen = 0;
	char message[BUFSIZE];
	char byeMent[100];
	char memberList[1024]="������\n";
	//�ӼӸ� ���� ���
	char *p2pToID;
	//�ӼӸ� ���� ���
	char *p2pFromID;
	//�ӼӸ� ����
	char *p2pMessage;
	//���������� �������� �ӼӸ�
	char whisperMessage[BUFSIZE + 20];
	int i;

	while ((strLen = recv(clntSock, message, BUFSIZE+1, 0)) != 0) {
		//�����ܿ��� ����� �ϴ� ��ɵ�.
		//@@�� ������ ���� Ư���� ������� �б����־����.
		if (!strncmp(message, "@@", 2)) {
			//@@join�� ������ ��. Ŭ���̾�Ʈ ȭ�鿡 �ѷ��� �Ӹ� �ƴ϶� ������ ����ü�� ���̵� ����.
			if (!strncmp(message, "@@join", 6)) {
				char *name;	//join�ڿ� �ִ� ȸ���̸��� ������ ��.
				char newMember[100] = "defalut";	//(ȸ��)���� ä�ù濡 �����̽��ϴٸ� �ϱ� ���� �ʿ�.
				printf("ȸ������: ");
				message[strLen] = 0;
				printf("%s /", message);
				//ȸ���� ��ϵ� �� ������ �ִ� ����ڵ鿡�� ȸ������ ����� �˸���.
				name = strtok(message, " ");
				name = strtok(NULL, " ");
				strcpy(newMember, name);
				strcat(newMember, "���� ä�ù濡 �����̽��ϴ�.\n");
				strcpy(member[clntNumber].userId, name);
				printf("%s�� ��� ���\n", member[clntNumber].userId);

				for (i = 0; i < clntNumber; i++) {
					send(clntSocks[i], newMember, sizeof(newMember), 0);
				}
			}
			//@@out�� ������ �� �߻��ϴ� �ϵ�.
			else if (!strncmp(message, "@@out", 5)) {
				//����ȣ���� �Ͼ�� while���� ���������� �ؿ��� ó���Ѵ�.
				break;
			}
			//@@member ��ɾ ������ �� �߻��ϴ� �ϵ�.
			else if (!strncmp(message, "@@member", 8)) {
				printf("������: ");
				message[strLen] = 0;
				printf("%s\n", message);
				for (i = 1; i <= clntNumber; i++) {
					printf("%s \n",member[i].userId);
					strcat(memberList, member[i].userId);
					strcat(memberList, "\n");
				}
				send(clntSock, memberList, sizeof(memberList), 0);
			}
			//�ӼӸ� ����� �� �� Ư����.
			else if (!strncmp(message, "@@talk", 6)) {
				//�ӼӸ��� @@talk ����ID ������ID �޼������� �������� ���޵�. strtok�� ������ �߶��ָ� ���ϴ� ������ ���ϴ� ���� ���� �� ����.
				p2pToID = strtok(message, " ");
				p2pToID = strtok(NULL, " ");
				p2pFromID = strtok(NULL, " ");
				p2pMessage = strtok(NULL, "\n");
				//����߿� ID�� �ش��ϴ� �ڰ� �ִ��� ã�´�.
				for (i = 0; i < clntNumber; i++) {
					//�ش��ϴ� ���̵� ã��
					if (!strcmp(member[i + 1].userId, p2pToID)) {
						strcpy(whisperMessage, "��б�: <");
						strcat(whisperMessage, p2pFromID);
						strcat(whisperMessage, "> :");
						strcat(whisperMessage, p2pMessage);
						strcat(whisperMessage, "\n");
						send(clntSocks[i], whisperMessage, strlen(whisperMessage) + 1, 0);
						printf("%s���� %s�Կ��� �ӼӸ��� ���½��ϴ�.\n", p2pFromID, p2pToID);
					}
					//������ �������� �ش��ϴ� ���̵� ��ã��.
					if (i == (clntNumber - 1)) {
						if (strcmp(member[i + 1].userId, p2pToID)) {
							send(clntSock, "�ش��ϴ� ���̵� �����ϴ�.\n", 100, 0);
						}
					}
				}
			}
		}
		else {
			//�����ܿ��� �ѷ��ֱ� ���ؼ� �̷��� ������Ѵ�. ���⼭ ���� ���.
			message[strLen] = 0;
			printf("receive: %s", message);
			SendMSG(message, strLen);
		}
	}


	WaitForSingleObject(hMutex, INFINITE);
	for (i = 0; i<clntNumber; i++) {   // Ŭ���̾�Ʈ ���� �����
		//���� ������ ã�´�.
		if (clntSock == clntSocks[i]) {
			printf("%s Ż��\n", member[i + 1].userId);
			strcpy(byeMent, member[i + 1].userId);
			strcat(byeMent, "���� Ż���ϼ̽��ϴ�.\n");
			for (; i<clntNumber - 1; i++){
				//���� ���� �޺κ� ������ ��ĭ�� ������.
				clntSocks[i] = clntSocks[i + 1];
				strcpy(member[i + 1].userId, member[i + 2].userId);
			}
			break;
		}

	}
	clntNumber--;
	//���� ���� �������鿡�� �����ٴ°� �˷���.
	for (i = 0; i < clntNumber; i++) {
		send(clntSocks[i], byeMent, sizeof(byeMent), 0);
	}
	ReleaseMutex(hMutex);
	closesocket(clntSock);

	return 0;
}
void SendMSG(char* message, int len)
{
	int i;
	WaitForSingleObject(hMutex, INFINITE);
	for (i = 0; i<clntNumber; i++)
		send(clntSocks[i], message, len, 0);
	ReleaseMutex(hMutex);
}
void ErrorHandling(char *message)
{
	fputs(message, stderr);
	fputc('\n', stderr);
	exit(1);
}
