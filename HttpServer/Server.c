/*
http://localhost:7777 로 테스트 할 수 있으며
index.html파일을 불러와 읽어 추가로 index.css와 bono.jpg를 불러옵니다.
index.html파일이 없을 경우 404에러를 출력합니다.
*/

#include <arpa/inet.h>
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

// 스레드 문제는 나중에 추가하자.
// 일단은 연결 1개만 받고 나중에 스레드로 밀어넣기 

#define PORT_NUM 7777
#define BUFFER_SIZE 4096
#define BUFFER_SIZE_PACKET 1024

#define OK_TEXT "HTTP/1.0 200 OK\r\nContent-Type:text/html\r\n\r\n"
#define OK_CSS "HTTP/1.0 200 OK\r\nContent-Type:text/css\r\n\r\n"
#define OK_IMAGE "HTTP/1.0 200 OK\r\nContent-Type:image/jpeg\r\n\r\n"
#define NOT_FOUND "HTTP/1.0 404 Not_Found\r\nContent-Type:text/html\r\n\r\n"
#define NOT_FOUND_MSG "<HTML><BODY>Page is not found 404!!</BODY></HTML>"



int main(int argc, char **argv)
{
	//fd 만들고 
	int sock_server, connfd, msg_len;
	// 소켓에 바인딩할 것들 만들고
	struct sockaddr_in servaddr,cliaddr;
	// 클라이언트 소켓 만들고
	socklen_t clilen;

	char msg[BUFFER_SIZE];
	char buf[BUFFER_SIZE_PACKET];
	//방식 집에 넣을 공간
	char command[BUFFER_SIZE_PACKET];
	//파일명 넣을 공간
	char file_name[BUFFER_SIZE_PACKET];
	char test[BUFFER_SIZE_PACKET];

	//각종 정보들 넣고
	sock_server = socket(AF_INET, SOCK_STREAM, 0);
	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(PORT_NUM);

	//소켓에 바인딩!
	bind(sock_server, (struct sockaddr *)&servaddr, sizeof(servaddr));
	printf("서버를 시작합니다!!\n");

	//일단 대기 큐 개수는 1024개로 해두고 나중에 상수화 시키자.
	listen(sock_server, 1024);

	while(1)
	{
		clilen = sizeof(cliaddr);
		connfd = accept(sock_server, (struct sockaddr *)&cliaddr, &clilen);

		//에러처리 해주고 나중에 함수 따로 빼서 묶자.
		if(connfd == -1)
		{
			printf("소켓을 생성할 수 없어서 종료합니다.\n");
			exit(1);
		}
		printf("Connect!! %d\n", connfd);

		// 연결 수락된게 있으면 가져온다.
		if (connfd > 0) {
			msg_len = recv(connfd, msg, BUFFER_SIZE, 0);
			// null로 닫아주고 저번에 이거 안해서 개판남
			msg[msg_len] = 0;
			// 받아온거 뿌려주고
			printf("=========================================\n%s=========================================\n", msg);

			
			// 분석해서 상황에 맞는 리스폰을 내놔야겠지
			sscanf(msg, "%s %s \n %s \n", command, file_name, test);
			printf("%s\n", command);
			printf("%s\n", file_name);
			printf("%s\n", test);

			int fd;

			//중복이 너무 많은데 이건 나중에 종합해서 함수로 빼자 
			if(strstr(file_name, ".jpg") != NULL){
				send(connfd, OK_IMAGE, strlen(OK_IMAGE), 0);
				fd = open("bono.jpg", O_RDONLY);
				while ((clilen = read(fd, buf, BUFFER_SIZE_PACKET)) > 0) {
					send(connfd, buf, clilen, 0);
				}	
			}
			if(strstr(file_name, ".css") != NULL){
				send(connfd, OK_CSS, strlen(OK_CSS), 0);
				fd = open("index.css", O_RDONLY);
				while ((clilen = read(fd, buf, BUFFER_SIZE_PACKET)) > 0) {
					send(connfd, buf, clilen, 0);
				}	
			}

			fd = open("index.html", O_RDONLY);
			if(fd < 0){
				send(connfd, NOT_FOUND, strlen(NOT_FOUND), 0);
				send(connfd, NOT_FOUND_MSG, strlen(NOT_FOUND_MSG), 0);
			}else{
				send(connfd, OK_TEXT, strlen(OK_TEXT), 0);
				while ((clilen = read(fd, buf, BUFFER_SIZE_PACKET)) > 0) {
					send(connfd, buf, clilen, 0);
				}
			}	
			
			// 닫자
			close(fd);
			close(connfd);
		}
	}
}

