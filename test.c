// http://www.xevious7.com/52 의 코드를 베이스로 하여 책과 검색으로 살을 붙여나갔다.
// 내가 생각지도 못한 에러처리와 문법이 많았으며 주석을 새로 달고 정리하였다.
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>

//epoll을 위해서 사용
//epoll함수는 리눅스에서만 지원되는데 select에 비해 상태변화 이벤트 관리가 편리하다.
#include <sys/epoll.h>

#define MAX_CLIENT   1000
#define DEFAULT_PORT 7777
#define MAX_EVENTS   1000

int g_svr_sockfd;	// 연결하고 넘겨주므로 하나만 있으면 된다.
int g_svr_port;		// 포트도 마찬가지

// 클라이언트의 소켓과 IP를 관리하는 구조체
struct {
	int  cli_sockfd;	//FileDescriptor
	char cli_ip[20];	//IP
} g_client[MAX_CLIENT];

int g_epoll_fd;                /* epoll fd */

struct epoll_event g_events[MAX_EVENTS];

/* function prototype */
void init_data0(void);            /* initialize data. */
void init_server0(int svr_port);  /* server socket bind/listen */
void epoll_init(void);            /* epoll fd create */
void epoll_cli_add(int cli_fd);   /* client fd add to epoll set */
void userpool_add(int cli_fd,char *cli_ip);
void error_handling(char *buf);

// 클라이언트 구조체 배열 초기화 함수
void init_data0(void)
{
	// cpu의 레지스터에 변수 할당[사실 여기에서 처음 봤다.]
	// 성능을 위해 사용하고 공간이 얼마 없어 부족하면 auto변수로 할당
	register int i;
	for(i = 0 ; i < MAX_CLIENT ; i++)
		g_client[i].cli_sockfd = -1;
}


void init_server0(int svr_port)
{
	struct sockaddr_in serv_addr;

	// IPv4 형식으로 TCP프로토콜을 이용하는 소켓 생성.
	// 마지막 인자는 특정 프로토콜 지정시 사용. 보통0
	if( (g_svr_sockfd = socket(AF_INET,SOCK_STREAM,0)) < 0 ) 
		error_handling("[Server] Socket() Fail");

	// 주소를 초기화 하고 값을 담아준다.
	memset( &serv_addr , 0 , sizeof(serv_addr)) ;	//0으로 초기화
	serv_addr.sin_family = AF_INET;					//Ipv4 주소체계
	// 뻔한 서버측 아이피를 지정해주는 이유는 하나의 컴퓨터에 2개 이상 ip할당 가능
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);	//InAddr_Any로 서버주소 지정
	serv_addr.sin_port = htons(svr_port);			//포트지정

	// 소켓 옵션 설정 
	// setsockopt(int sock, int level, int optname, void *optval, socklen_t *optlen)
	int nSocketOpt = 1;
	// 여기에서 SO_REUSEADDR은 TIME-OUT상황에서 reuse를 통해
	// 소켓에 할당되어 있는 port번호를 새로 시작할 수 있다.
	if( setsockopt(g_svr_sockfd, SOL_SOCKET, SO_REUSEADDR, &nSocketOpt, sizeof(nSocketOpt)) < 0 )
	{
		close(g_svr_sockfd); 
		error_handling("[Server] setsockopt() Fail");
	}

	// 이제 소켓에 주소정보 할당
	if(bind(g_svr_sockfd,(struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
	{
		close(g_svr_sockfd);
		error_handling("[Server] bind() Fail");
	}

	listen(g_svr_sockfd,15);	//2번째 인자는 대기큐!

	printf("[Server] Server Initialize. Port = %d\n", svr_port);
} 

// Event poll을 초기화 한다.
void epoll_init(void)
{
	// 이벤트 관리용 구조체
	struct epoll_event events;

	g_epoll_fd = epoll_create(MAX_EVENTS);
	if(g_epoll_fd < 0)
	{
		close(g_svr_sockfd);
		error_handling("[Server] epoll_create() Fail");
	}

	// 수신할 데이터가 존재하는 상황 발생시 EPOLLIN
	events.events = EPOLLIN;
	events.data.fd = g_svr_sockfd;

	// 파일 디스크립터 등록
	if( epoll_ctl(g_epoll_fd, EPOLL_CTL_ADD, g_svr_sockfd, &events) < 0 )
	{
		close(g_svr_sockfd);
		close(g_epoll_fd);
		error_handling("[Server] epoll_ctl() Fail");
	}

	printf("[Server] epoll setting complete\n");
}
/*------------------------------- end of function epoll_init */

void epoll_cli_add(int cli_fd)
{

	struct epoll_event events;

	/* event control set for read event */
	events.events = EPOLLIN;
	events.data.fd = cli_fd;

	if( epoll_ctl(g_epoll_fd, EPOLL_CTL_ADD, cli_fd, &events) < 0 )
	{
		printf("[ETEST] Epoll control fails.in epoll_cli_add\n");
	}

}

void userpool_add(int cli_fd,char *cli_ip)
{
	/* get empty element */
	register int i;

	for( i = 0 ; i < MAX_CLIENT ; i++ )
	{
		if(g_client[i].cli_sockfd == -1) break;
	}
	if( i >= MAX_CLIENT ) close(cli_fd);

	g_client[i].cli_sockfd = cli_fd;
	memset(&g_client[i].cli_ip[0],0,20);
	strcpy(&g_client[i].cli_ip[0],cli_ip);

}

void userpool_delete(int cli_fd)
{
	register int i;

	for( i = 0 ; i < MAX_CLIENT ; i++)
	{
		if(g_client[i].cli_sockfd == cli_fd)
		{
			g_client[i].cli_sockfd = -1;
			break;
		}
	}
}

void userpool_send(char *buffer)
{
	register int i;
	int len;

	len = strlen(buffer);

	for( i = 0 ; i < MAX_CLIENT ; i ++)
	{
		if(g_client[i].cli_sockfd != -1 )
		{
			len = send(g_client[i].cli_sockfd, buffer, len,0);
			/* more precise code needed here */
		}
	}

}


void client_recv(int event_fd)
{
	char r_buffer[1024]; /* for test.  packet size limit 1K */
	int len;
	/* there need to be more precise code here */ 
	/* for example , packet check(protocol needed) , real recv size check , etc. */

	/* read from socket */
	len = recv(event_fd,r_buffer,1024,0);
	if( len < 0 || len == 0 )
	{
		userpool_delete(event_fd);
		close(event_fd); /* epoll set fd also deleted automatically by this call as a spec */
		return;
	}

	userpool_send(r_buffer);

}

void server_process(void)
{
	struct sockaddr_in cli_addr;
	int i,nfds;
	int cli_sockfd;
	int cli_len = sizeof(cli_addr);
	
	// 이벤트가 발생한 파일 디스크립터 수를 반환한다.
	// 감지된 이벤트는 차곡차곡 버퍼에 넣는다.
	nfds = epoll_wait(g_epoll_fd, g_events, MAX_EVENTS, 100);

	// 이벤트가 없는 경우
	if(nfds == 0) 
		return;
	// 에러처리
	if(nfds < 0) 
	{
		error_handling("[Server] epoll_wait() Fail");
	}

	// 이벤트가 감지된 경우 이벤트 개수만큼 돌린다.
	for( i = 0 ; i < nfds ; i++ )
	{
		// wait에서 넣은 버퍼를 뒤져서 확인
		if(g_events[i].data.fd == g_svr_sockfd)
		{
			// 확인된 소켓으로 연결 수락
			cli_sockfd = accept(g_svr_sockfd, (struct sockaddr *)&cli_addr,(socklen_t *)&cli_len);
			if(cli_sockfd < 0) 
			{
			 	error_handling("[Server] accept() Fail");
			}
			else
			{
				printf("[Server] connet success fd:%d, ip:%s\n",cli_sockfd,inet_ntoa(cli_addr.sin_addr));
				userpool_add(cli_sockfd,inet_ntoa(cli_addr.sin_addr));
				epoll_cli_add(cli_sockfd);
			}
			continue;
		}
		// 서버 소켓이 아닐경우 클라이언트 소켓을 뒤진다.
		client_recv(g_events[i].data.fd);   
	}
}

void error_handling(char *buf)
{
	fputs(buf, stderr);
	fputc('\n', stderr);
	exit(1);
}

/*------------------------------- end of function server_process */

void end_server(int sig)
{
	close(g_svr_sockfd); /* close server socket */
	printf("[ETEST][SHUTDOWN] Server closed by signal %d\n",sig);
	exit(0);
}

int main( int argc , char *argv[])
{
	//포트를 지정했는지 확인
	if(argc < 2)
	{
		g_svr_port = DEFAULT_PORT;
		printf("Default port : %d\n", DEFAULT_PORT);
	}
	else //지정했다면
	{
		g_svr_port = atoi(argv[1]);
		//well known 못쓰게 해주고
		if(g_svr_port == 0){
			printf("Usage : %s <port>\n", argv[0]);
			exit(1);
		}else if(g_svr_port < 1024)
			error_handling("[Server] Well-Known포트를 지정하였습니다. [port < 1024]");
	}

	printf("[Server] ServerStart\n");

	// 초기화들 시키고
	init_data0();  
	init_server0(g_svr_port);
	
	epoll_init();

	while(1)
		server_process();
}
