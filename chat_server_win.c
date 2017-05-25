/*
* chat_server_win.c
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
	printf("서버 초기화\n");
	while (1) {
		clntAddrSize = sizeof(clntAddr);
		clntSock = accept(servSock, (SOCKADDR*)&clntAddr, &clntAddrSize);
		if (clntSock == INVALID_SOCKET)
			ErrorHandling("accept() error");

		WaitForSingleObject(hMutex, INFINITE);
		clntSocks[clntNumber++] = clntSock;

		ReleaseMutex(hMutex);
		printf("새로운 연결, 클라이언트 IP : %s \n", inet_ntoa(clntAddr.sin_addr));

		hThread = (HANDLE)_beginthreadex(NULL, 0, ClientConn, (void*)clntSock, 0, (unsigned *)&dwThreadID);
		if (hThread == 0) {
			ErrorHandling("쓰레드 생성 오류");
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
	char memberList[1024] = "멤버목록\n";
	//귓속말 보낼 대상
	char *p2pToID;
	//귓속말 보낸 대상
	char *p2pFromID;
	//귓속말 내용
	char *p2pMessage;
	//최종적으로 보내어질 귓속말
	char whisperMessage[BUFSIZE + 20];
	int i;

	while ((strLen = recv(clntSock, message, BUFSIZE + 1, 0)) != 0) {
		//서버단에서 해줘야 하는 기능들.
		//@@이 들어왔을 때는 특별한 기능으로 분기해주어야함.
		if (!strncmp(message, "@@", 2)) {
			//@@join이 들어왔을 시. 클라이언트 화면에 뿌려줄 뿐만 아니라 가입자 구조체에 아이디 저장.
			if (!strncmp(message, "@@join", 6)) {
				char *name;	//join뒤에 있는 회원이름을 가져올 것.
				char newMember[100] = "defalut";	//(회원)님이 채팅방에 들어오셨습니다를 하기 위해 필요.
				printf("회원가입: ");
				message[strLen] = 0;
				printf("%s /", message);
				//회원이 등록될 시 접속해 있는 사용자들에게 회원가입 사실을 알린다.
				name = strtok(message, " ");
				name = strtok(NULL, " ");
				strcpy(newMember, name);
				strcat(newMember, "님이 채팅방에 들어오셨습니다.\n");
				strcpy(member[clntNumber].userId, name);
				printf("%s님 멤버 등록\n", member[clntNumber].userId);

				for (i = 0; i < clntNumber; i++) {
					send(clntSocks[i], newMember, sizeof(newMember), 0);
				}
			}
			//@@out이 들어왔을 때 발생하는 일들.
			else if (!strncmp(message, "@@out", 5)) {
				//종료호출이 일어나면 while문을 빠져나가고 밑에서 처리한다.
				break;
			}
			//@@member 명령어가 들어왔을 때 발생하는 일들.
			else if (!strncmp(message, "@@member", 8)) {
				printf("멤버목록: ");
				message[strLen] = 0;
				printf("%s\n", message);
				for (i = 1; i <= clntNumber; i++) {
					printf("%s \n", member[i].userId);
					strcat(memberList, member[i].userId);
					strcat(memberList, "\n");
				}
				send(clntSock, memberList, sizeof(memberList), 0);
			}
			//귓속말 기능은 좀 더 특별함.
			else if (!strncmp(message, "@@talk", 6)) {
				//귓속말은 @@talk 상대방ID 보낸자ID 메세지내용 포맷으로 전달됨. strtok로 적절히 잘라주면 원하는 변수에 원하는 값을 넣을 수 있음.
				p2pToID = strtok(message, " ");
				p2pToID = strtok(NULL, " ");
				p2pFromID = strtok(NULL, " ");
				p2pMessage = strtok(NULL, "\n");
				//멤버중에 ID에 해당하는 자가 있는지 찾는다.
				for (i = 0; i < clntNumber; i++) {
					//해당하는 아이디를 찾음
					if (!strcmp(member[i + 1].userId, p2pToID)) {
						strcpy(whisperMessage, "비밀글: <");
						strcat(whisperMessage, p2pFromID);
						strcat(whisperMessage, "> :");
						strcat(whisperMessage, p2pMessage);
						strcat(whisperMessage, "\n");
						send(clntSocks[i], whisperMessage, strlen(whisperMessage) + 1, 0);
						printf("%s님이 %s님에게 귓속말을 보냈습니다.\n", p2pFromID, p2pToID);
					}
				}
			}
		}
		else {
			//서버단에도 뿌려주기 위해선 이렇게 해줘야한다. 여기서 많이 헤맴.
			message[strLen] = 0;
			printf("receive: %s", message);
			SendMSG(message, strLen);
		}
	}


	WaitForSingleObject(hMutex, INFINITE);
	for (i = 0; i<clntNumber; i++) {   // 클라이언트 연결 종료시
									   //나간 소켓을 찾는다.
		if (clntSock == clntSocks[i]) {
			printf("%s 탈퇴\n", member[i + 1].userId);
			strcpy(byeMent, member[i + 1].userId);
			strcat(byeMent, "님이 탈퇴하셨습니다.\n");
			for (; i<clntNumber - 1; i++) {
				//나간 소켓 뒷부분 소켓을 한칸씩 땡겨줌.
				clntSocks[i] = clntSocks[i + 1];
				strcpy(member[i + 1].userId, member[i + 2].userId);
			}
			break;
		}

	}
	clntNumber--;
	//나간 소켓 나머지들에게 나갔다는걸 알려줌.
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
