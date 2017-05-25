/*
* chat_client_win.c
* VidualStudio에서 console프로그램으로 작성시 multithread-DLL로 설정하고
*  소켓라이브러리(ws2_32.lib)를 추가할 것.
*/

#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <process.h>
#include <Windows.h>

#define BUFSIZE 100
#define NAMESIZE 20

DWORD WINAPI SendMSG(void *arg);
DWORD WINAPI RecvMSG(void *arg);
void ErrorHandling(char *message);

char name[NAMESIZE] = "[Default]";
char message[BUFSIZE];
//회원가입
//int joinFun();
//int flagKey;
//첫시작을 알린다.
int flagStart=0;

int main(int argc, char **argv)
{
	WSADATA wsaData;
	SOCKET sock;
	SOCKADDR_IN servAddr;

	HANDLE hThread1, hThread2;
	DWORD dwThreadID1, dwThreadID2;

	if (argc != 3) {
		printf("Usage : %s <IP> <port> <name>\n", argv[0]);
		exit(1);
	}
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) /* Load Winsock 2.2 DLL */
		ErrorHandling("WSAStartup() error!");
	sock = socket(PF_INET, SOCK_STREAM, 0);
	if (sock == INVALID_SOCKET)
		ErrorHandling("socket() error");
//	flagKey = 0;
	printf("유저ID를 입력하세요\n");
	scanf("%s", &name);
/*	
	scanf("%s", &name);
	fflush(stdin);
	printf("회원가입 방법: @@join id\n");

	while (true)
	{
		joinFun();
		if (flagKey == 1)break;
	}
*/
	memset(&servAddr, 0, sizeof(servAddr));
	servAddr.sin_family = AF_INET;
	servAddr.sin_addr.s_addr = inet_addr(argv[1]);
	servAddr.sin_port = htons(atoi(argv[2]));
//	if (flagKey == 1)
//	{
		if (connect(sock, (SOCKADDR*)&servAddr, sizeof(servAddr)) == SOCKET_ERROR)
			ErrorHandling("connect() error");
//	}
	hThread1 = (HANDLE)_beginthreadex(NULL, 0, SendMSG, (void*)sock, 0, (unsigned *)&dwThreadID1);
	hThread2 = (HANDLE)_beginthreadex(NULL, 0, RecvMSG, (void*)sock, 0, (unsigned *)&dwThreadID2);
	if (hThread1 == 0 || hThread2 == 0) {
		ErrorHandling("쓰레드 생성 오류");
	}

	WaitForSingleObject(hThread1, INFINITE);
	WaitForSingleObject(hThread2, INFINITE);

	closesocket(sock);
	return 0;
}

DWORD WINAPI SendMSG(void *arg) // 메시지 전송 쓰레드 실행 함수
{
	SOCKET sock = (SOCKET)arg;
	char nameMessage[NAMESIZE + BUFSIZE];
	char talkMessage[6 + NAMESIZE + NAMESIZE + BUFSIZE];
	char *p2pID;
	char *p2pMessage;

	while (1) {
		fgets(message, BUFSIZE, stdin);
		sprintf(nameMessage, "[%s] %s", name, message);
			if(flagStart==0){
				char joinMember[30];
				strcpy(joinMember, "@@join ");
				strcat(joinMember, name);
				send(sock, joinMember, strlen(joinMember), 0);
				flagStart = 1;
			}
			else {
				if (!strcmp(message, "&quit\n")) {  // '&quit' 입력시 종료
					send(sock, "@@out", 6, 0);
					printf("채팅을 종료하셨습니다.");
					closesocket(sock);
					exit(0);
				}
				else if (!strcmp(message, "&list\n")) {
					send(sock, "@@member", 8, 0);
				}
				//귓말 기능은 특별하게 구현해주어야 함.
				else if (!strncmp(message, "&p2p", 4)) {
					p2pID = strtok(message, " ");
					p2pID = strtok(NULL, " ");
					p2pMessage = strtok(NULL,"\n");
					//talkMessage양식
					strcpy(talkMessage, "@@talk ");
					strcat(talkMessage, p2pID);
					strcat(talkMessage, " ");
					strcat(talkMessage, name);
					strcat(talkMessage, " ");
					strcat(talkMessage, p2pMessage);
					send(sock, talkMessage, strlen(talkMessage)+1, 0);
				}
				else send(sock, nameMessage, strlen(nameMessage), 0);
			}
	}
}

DWORD WINAPI RecvMSG(void *arg) /* 메시지 수신 쓰레드 실행 함수 */
{
	SOCKET sock = (SOCKET)arg;
	char nameMessage[NAMESIZE + BUFSIZE];
	int strLen;
		while (1) {
			strLen = recv(sock, nameMessage, NAMESIZE + BUFSIZE - 1, 0);
			if (strLen == -1) return 1;
			nameMessage[strLen] = 0;
			fputs(nameMessage, stdout);
		}
}
void ErrorHandling(char *message)
{
	fputs(message, stderr);
	fputc('\n', stderr);
	exit(1);
}
/*
int joinFun() {
	char joinString[100];
	gets(joinString);
	char *p;
	p = strtok(joinString, " ");
	p = strtok(NULL, " ");
	if (flagKey == 2) {
		if (!strncmp(joinString, "@@join", 6)) {
			
			if (!strcmp(p, name)) {
				printf("Wait...\n");
				Sleep(1000);
				printf("%s님 서버와의 연결을 시작합니다.\n", name);
				flagKey = 1;
			}
			else {
				printf("아이디가 없습니다.\n");
			}
		}
		else {
			printf("회원가입하세요\n");
		}
	}
	else flagKey = 2;
	return flagKey;
}
*/