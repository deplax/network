#include <stdio.h>
#include <stdlib.h>
#include <unistd.h> 
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>

#define BUF_SIZE 100
#define NAME_SIZE 20

void * send_msg(void * arg);
void * recv_msg(void * arg);
void error_handling(char * msg);

char name[NAME_SIZE]="[DEFAULT]";
char msg[BUF_SIZE];

int main(int argc, char *argv[])
{
	int sock;
	struct sockaddr_in serv_addr;
	pthread_t snd_thread, rcv_thread;

	void * thread_return;

	char note[1024];

	if(argc < 3) {
		printf("[Client] Usage : %s <IP> <port> <name>\n", argv[0]);
		exit(1);
	}

	// 이름을 지정하지 않는 경우 Default가 들어간다.
	if(argc != 3){
		if(strlen(argv[3]) > NAME_SIZE)
			error_handling("[Client] name size max 20byte");
		sprintf(name, "[%s]", argv[3]);
		// 이름 길이로 인한 오버플로우를 막는다.
 	}

	// 소켓 열고
	sock=socket(PF_INET, SOCK_STREAM, 0);

	// 초기화 하고
	memset(&serv_addr, 0, sizeof(serv_addr));

	// 소켓에 바인딩할 주소 지정
	serv_addr.sin_family=AF_INET;
	serv_addr.sin_addr.s_addr=inet_addr(argv[1]);
	serv_addr.sin_port=htons(atoi(argv[2]));

	if(connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr))==-1)
		error_handling("[Client] connect() error");

	sprintf(note, "%s 님이 입장하셨습니다.\n", name);
	write(sock, note, strlen(note));
	
	// 보내기, 받기 스레드 생성
	pthread_create(&snd_thread, NULL, send_msg, (void*)&sock);
	pthread_create(&rcv_thread, NULL, recv_msg, (void*)&sock);

	// join을 통해 스레드가 모두 종료될 때까지 기다린다. 착하기도 하지
	pthread_join(snd_thread, &thread_return);
	pthread_join(rcv_thread, &thread_return);
	close(sock);  
	return 0;
}

// 메시지 발사!!!
void * send_msg(void * arg)   
{
	int sock=*((int*)arg);
	char name_msg[NAME_SIZE+BUF_SIZE];

	while(1) 
	{
		fgets(msg, BUF_SIZE, stdin);

		if(!strcmp(msg,"q\n")||!strcmp(msg,"Q\n")) 
		{
			close(sock);
			exit(0);
		}
		sprintf(name_msg,"%s %s", name, msg);

		write(sock, name_msg, strlen(name_msg));

	}
	return NULL;
}

// 메시지 받아오기
void * recv_msg(void * arg)   
{
	int sock=*((int*)arg);
	char name_msg[NAME_SIZE+BUF_SIZE];

	int str_len;

	while(1)
	{
		str_len=read(sock, name_msg, NAME_SIZE+BUF_SIZE-1);
		if(str_len==-1) 
			return (void*)-1;

		name_msg[str_len]=0;

		fputs(name_msg, stdout);
	}
	return NULL;
}

void error_handling(char *msg)
{
	fputs(msg, stderr);
	fputc('\n', stderr);
	exit(1);
}
