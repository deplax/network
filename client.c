// 열혈강의 TCP/IP 책 423쪽 chat_clnt.c 파일 분석
// 헤더파일 인클루드
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h> 
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>

// 매크로 선언
#define BUF_SIZE 100
#define NAME_SIZE 20

// 함수 원형 선언
void * send_msg(void * arg);
void * recv_msg(void * arg);
void error_handling(char * msg);

// 전역변수 선언
char name[NAME_SIZE]="[DEFAULT]";	// 사용자가 이름을 지정하지 않았을 경우 부여될 이름
char msg[BUF_SIZE];					// 메시지용 버퍼
// 지금 버퍼크기가 100으로 작게 잡혀있는데
// 100자보다 긴 메시지를 보내면 여러개의 메시지가 발송된것차럼 처리된다

// main 함수 	
int main(int argc, char *argv[])
{
	// 소켓생성후 파일 디스크립트를 저장해둘 변수를 하나 생성한다
	int sock;

	// 구조체 sockaddr_in형 변수 serv_addr를 하나 생성한다
	// sockaddr_in은 IPv4의 주소정보를 담기 위해 정의된 구조체이다
	struct sockaddr_in serv_addr;

	// thread 변수를 사용하기 위한 변수 선언 
	// 메시지를 보내는 목적의 snd_thread와 메시지를 받는 목적의 rcv_thread가 있다.
	pthread_t snd_thread, rcv_thread;

	// 쓰레드의 main함수가 반환하는 값을 저장하기 위한 변수
	// pthread_join함수에서 두번째 인자에 반환값을 받고 싶은 값을 넘길 수 있는데
	// pthread_join의 함수원형이 int pthread_join(pthread_t thread, void **status) 이어서
	// thread의 main함수가 반환하는 값이 저장될 포인터 변수의 주소값을 전달해야 한다
	// 
	// 실제로는 우측과 같이 사용하는데 -> pthread_join(snd_thread, &thread_return);
	// 인자로 &를 붙여서 보내주는 이유는 함수 원형에서 더블포인터를 사용하였기 때문이다
	// 
	// 포인터의 주소를 넘긴다는것은 다음과 같은 의미가 된다
	// 포인터의 주소 -> 포인터 -> 포인터가 가리키고 있는 어떤 것
	// 
	// 따라서 포인터의 주소를 넘긴다는 것은 더블포인터를 사용하는것과 같다
	void * thread_return;

	// 인자의 갯수를 체크한다
	// argc, argv[] 는 각각 프로그램 실행시 넘긴 인자의 갯수와 그 인자의 내용들을 의미한다
	// 클라이언트는 (1) 클라이언트 프로그램 이름, (2) 서버 IP, (3) 포트번호, (4) 채팅방에서 사용할 이름
	// 이렇게 4가지 인자를 필요로 하기 때문에 argc가 4가 이니면 필요한 정보를 모두 얻지 못했기 때문에
	// 올바른 사용법을 안내하고 프로그램을 종료시킨다
	if(argc!=4) {
		printf("Usage : %s <IP> <port> <name>\n", argv[0]);
		exit(1);
	}

	// printf() 함수는 형식 문자열에 맞게 화면에 출력하는 기능을 함
	// sprintf() 함수는 화면 대신 문자 배열에 값을 복사하는 기능을 함
	sprintf(name, "[%s]", argv[3]);

	// 소켓을 하나 생성할껀데요 프로토콜 체계는 PF_INET(IPv4)형식면서 연결지향형 소켓이면 좋겠어요
	// (연결지향형 소켓을 생성했기 때문에 버퍼크기를 넘어서는 양의 메시지를 기록해도 손실없이 날라간다. 단, 여러개의 메시지가 발송된것차럼 처리된다)
	// 세번째 인자는 원래 하나의 프로토콜 체계 안에 데이터 전송 방식이 동일한 프로토콜이 둘 이상이 존재하는 경우
	// 이를 구체화 시켜주기 위해서 추가적인 인자를 전달하기위해 존재한다 
	// 채팅프로그램에서는 세번째 인자를 굳이 넣어주지 않아도 원하는 소켓을 생성할 수 있으므로 0을 넘긴다
	// 
	// PF_INET(IPv4) 프로토콜 체계에서 사용할 수 있는 세번째 인자에는 IPPROTO_TCP와 IPPROTO_UDP가 있는데
	// 각각 TCP소켓을 생성해 주세요, UDP 소켓을 생성해 주세요 라는 뜻이다
	sock=socket(PF_INET, SOCK_STREAM, 0);

	// memset을 이용하면 배열, 구조체, class(c++) 등을 한번에 초기화 할 수 있다
	// 찻반쩨 인자로 넘긴 해당 메모리 주소부터 3번째 인자로 넘긴 크기만큼 0으롤 초기화 해달라는 뜻이다
	memset(&serv_addr, 0, sizeof(serv_addr));

	// 위애서도 이미 언급된 적이 있지만 serv_addr의 타입인 sockaddr_in 구조체는 IPv4의 주소정보를 담기위해 정의된 구조체이다
	// sockaddr_in 구조체는 아래와 같이 생겼다
	// struct sockaddr_in
	// {
	// 		sa_family_t 	sin_family;		// 주소체계(Adress Family) -> IPv4, IPv6, Local통신 전용 등을 지정 가능
	// 		uint16_t		sin_port;		// 16비트 포트번호를 지정하는데 네트워크 바이트 순서로 저장해야 한다(빅엔디언, 리틀엔디언 등)
	// 		struct in_addr 	sin_addr;		// 32비트 주소 정보를 저장한다. 네트워크 바이트 순서로 저장해야 한다
	// 		char			sin_zero[8];	// 안쓴다고 한다. 자세한 내용은 70페이지에서 확인할 수 있다.
	// }
	// 그래서 결국 사용은 아래와 같이 한다
	// 
	// 나 IPv4 방식을 사용할꺼야
	serv_addr.sin_family=AF_INET;

	// 내가 사용할 주소를 지정할꺼야
	// 그 주소는 프로그램 실행시 넘겨주었던 첫번째 인자(127.0.0.1)인데 네트워크 바이트 순서로 달라고 했지?
	// 그래서 내가 inet_addr 함수를 이용해서 변환한 뒤에 넣어줄께 라는 뜻이다
	// 
	// inet_addr() 함수는 Dotted-Decimal Notation(10진 표기법)을 Big-Endian 32비트 값으로 변환 및 네트워크 바이트 순서로 변환하는 일을 한다
	// 성공시 Big Endian 32비트 값을, 실패하면 INADDR_NONE값을 반환한다
	// 
	// 이 함수를 사용해야 하는 이유는 sockaddr_in 구조체의 주소멤버인 in_addr_t의 데이터 타입이 unsigned long (32비트 정수형)이기 때문에
	// IP주소를 할당하기 위해 십진수 표현방식을 사용한 argv[1](=127.0.0.1)을 32비트 정수형 값으로 변환시켜줄 필요가 있기 때문이다
	serv_addr.sin_addr.s_addr=inet_addr(argv[1]);

	// htons에서 h는 host, n은 network 바이트 순서를 의미한다.
	// s는 short, l은 long을 의미한다
	// 
	// atoi()함수는 10진 정수 문자열을 정수로 변환하는 역할을 한다
	// 더 자세한 기능은 아래와 같다
	// 1. 문자열에서 10진 정수 숫자 문자 뒤의 일반 문자는 취소되며, 10진 정수 숫자 문자까지만 숫자로 변환됩니다.
	// 2. 10진 정수 숫자 문자 앞의 공백문자는 자동 제거되어 10진 정수 숫자 문자까지만 숫자로 변환됩니다.
	// 3. 공백 및 10진 정수 문자가 아닌 문자로 시작하면 0을 반환합니다.
	//
	// 간략히 말하면 Java의 trim 기능을 수행한 후 10진 정수 문자열에 대해서만 문자열에서 10진 정수로 변환해달라고 요청하는 것이다.
	// 공백 및 10진 정수 문자가 아닌 문자로 시작, 즉 문자열만 가득하면 0을 반환한다
	// 
	// 따라서 아래줄은 문자열 형태인 argv[2](=포트번호)에 포트번호 형태를 가진 문자열이 있는지 확인해보고
	// 포트번호 형태를 가진 문자열이 있다면 그 번호만 추출해서 정수형태의 포트번호로 바꾸어 주시고
	// 정수 형태로 변환된 포트번호를 host 바이트 순서에서 네트워크 바이트순서로 바꾸어 달라는 뜻이다
	serv_addr.sin_port=htons(atoi(argv[2]));

	// connect 함수는 성공하면 0 실패하면 -1을 반환한다
	// if문 엔에 connect를 넣어 놓았으므로 실패하게 되면 connet() error를 알릴 수 있게 된다
	// 
	// connect함수는 첫번째 인자로 소켓 파일 디스크립터를 전달하고
	// 두번째 인자로는 sockaddr_in형 변수 serv_addr를 전달하고 (형변환 및 주소값을 넘기는 이유는 connect함수 원형의 두번째 파라미터가 "struct sockaddr *serv_addr" 이기 때문임)
	// 즉 서버 주소 정보에 대한 포인터를 전달한다
	// 세번째 인자로는 struct sockaddr *serv_addr 포인터가 가르키는 구조체의 크기를 전달한다
	if(connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr))==-1)
		error_handling("connect() error");

	// 메시지를 발송하는 역할만 담당하는 쓰레드를 생성한다.
	// 찻번째 인자로 메시지 발송용 쓰레드ID를 저장할 변수인 snd_thread를 넘겨준다
	// 함수원형이 우측과 같이 생겼으므로 포인터를 전달해준다 (pthread_t *restrict thread)
	// 두번째 인자로 쓰레드에 부여할 특성 정보를 전달할 수 있다고 하는데 NULL을 넘겨서 기본적인 특성의 쓰레드가 생성되도록 한다
	// 세번째 인자로는 쓰레드가 생성된 후 해당 쓰레드의 main함수 역할을 할 함수의 주소(함수 포인터)를 전달한다
	// 네번째 인자로는 세번째 인자를 통해 등록된 함수가 호출될 때 전달할 인자의 정보를 담고있는 변수의 주소값을 전달한다
	pthread_create(&snd_thread, NULL, send_msg, (void*)&sock);
	// 윗 문장과 동일하여 설명 생략
	pthread_create(&rcv_thread, NULL, recv_msg, (void*)&sock);

	// 프로세스는 main함수가 종료되면 종료되어 버린다
	// main함수는 쓰레드를 생성하고 나면 마치 나는 더이상 관여할 일이 없다는 듯 코드상의 다음줄을 실행하러 내려간다
	// 따라서 쓰레드가 종료될 때 까지 프로세스가 종료되지 않도록 하기 위한 조치가 필요한데 이를 위해 필요한 것이 pthread_join이다
	// 첫번째 인자로는 기다릴 대상의 쓰레드ID를 넘기고 두번째 인자로는 해당 쓰레드의 main함수(지금 코드에서는 send_msg)가 반환하는 값이 저장될 포인터 변수의 주소값을 전달한다 
	pthread_join(snd_thread, &thread_return);
	// 윗 문장과 동일하여 설명 생략
	pthread_join(rcv_thread, &thread_return);
	// 소켓, 파일등을 열고 나면 반드시 닫아주어야 한다.
	// 따라서 실수로 close를 하지 않는 실수의 발생확률을 줄이기 위해 소켓 또는 파일을 생성한 직후 close()함수를 미리 작성해 두는 코딩 습관이 권장된다.
	close(sock);  
	return 0;
}

// send thread의 main함수 역할을 하는 함수
//
// 왜 함수 원형이 이렇게 복잡하게 생겼을까?
// pthread_create의 함수 원형은 아래와 같이 생겼다
// int pthread_create(---,---, void *(*start_routine)(void*), ---);
// 실제 사용은 아래와 같이 한다
// pthread_create(---, ---, send_msg, ---);
// 
// 함수원형이 왜 이렇게 생겼는지 의문이 들게한 원인은 아마도 아래의 문장 때문일 것이다.
// void *(*start_routine)(void*) 
// 
// 위 문장은 함수 포인터를 사용하기 위해서 생긴 것이다
//
// void *는 반환형이고
// *start_routine 은 포인터이고
// 매개변수가 void * 형태라는 뜻이다
// 
// 즉 pthread_create함수 원형은 반환형이 void * 이고 매개변수로 void * 형 변수 1개가 선언된 포인터형을 입력받고 싶은건데
// 그 마저도 앞에 *가 붙어있으니(*start_routine) 함수의 주소값을 넘겨주기를 바라고 있다.
// 
// 정리하자면 pthread_create 함수원형의 세번쨰 인자인 void *(*start_routine)(void*)는
// 반환형이 void * 이고 매개변수로 void * 형 변수 1개가 선언된 형태의 함수 주소값 저장을 위한 함수 포인터 변수를 선언한 것이고
// 그 함수 포인터 변수의 이름은 start_routine 인 셈이 된다.
// 
// 이후 과정은 보통 포인터들과 같이 진행하면 된다.
// 아래 예시와 같이 우리는 포인터에다가 주소값을 넣어주었다는 사실을 기억해보자
// int* pnum = &num
// 위 예제와 같이 포인터 변수를 초기화 시켜주기 위해선 당연히 주소값을 넣어 줄 필요가 있다
// 함수의 주소값이 무엇인가 찾아보았는데 함수 이름 그 자체가 함수의 주소값을 나타낸다고 한다.
// 따라서 pthread_create의 세번쨰 인자로 그저 함수 이름을 넣는것 만으로도 이 코드는 동작하게 되는 것이다.
void * send_msg(void * arg)   
{
	// pthread_create의 4번째 인자로 넘긴 값 (void*)&sock 이 void* arg 파라미터 형태로 들어온다
	// 파일디스크립터를 저장할 sock변수는 int형이니까 형변환 과정이 필요하다
	// 우선 void *형인 arg를 int *형으로 바꾸어주고
	// *를 붙여 값을 가져옴으로써 int형으로의 변환이 완료된다
	//
	// 전체 변환 과정
	// (void *)&sock -> 인자로 넘겨져서 (void*) arg 파라미터 형태로 받아짐
	// void* arg가 (int *)&arg 를 만나 int형 포인터로 변경됨
	// *(&arg) == arg -> 최종적으로 sock에 arg 대입;
	int sock=*((int*)arg);
	// 최종적으로 화면에 찍혀질 문자열 배열 선언 (배열 크기 선언부를 보았을 때 이름과 메시지 내용을 하나의 문자열 배열로 관리할것으로 예상됨)
	char name_msg[NAME_SIZE+BUF_SIZE];

	// 계속해서 메시지를 보낼 수 있도록 입력 대기
	while(1) 
	{
		// standard input스트립에서 문자열을 입력받는데 BUF_SIZE -1 개의 문자를 입력받을때 까지, EOF(End-of-File)에 도달할때까지
		// 또는 개행문자를 입력받을때까지 입력받아서 msg라는 문자열에 저장한다 
		fgets(msg, BUF_SIZE, stdin);

		// q나 Q를 입력받으면 종료
		// strcpm는 문자열 비교 함수
		if(!strcmp(msg,"q\n")||!strcmp(msg,"Q\n")) 
		{
			close(sock);
			exit(0);
		}
		// 위에서의 설명
		// printf() 함수는 형식 문자열에 맞게 화면에 출력하는 기능을 함
		// sprintf() 함수는 화면 대신 문자 배열에 값을 복사하는 기능을 함
		// 
		// 설명 추가
		// name_msg라는 문자열 배열에 2번째 인자로 넘긴 형식 문자열에 맞추어 이후 인자들을 문자열 배열에 작성해 주는 역할을 함
		// 쉽게 printf를 이용하면 화면에 출력할 내용들을 첫번째 인자로 넘긴 문자열 배열에 작성하는 것임
		sprintf(name_msg,"%s %s", name, msg);

		// name_msg문자열 배열에 있는 내용을 sock 소켓에 name_msg 길이만큼 보내주세요 라는 뜻으로
		// 리눅스에서 파일과 소켓을 동일하게 취급한다는 특성 때문에 send함수를 쓰지 않고도 소켓으로 데이터를 전송할 수 있다.
		// 성공시 전달한 바이트수, 실패시 -1이 return된다
		write(sock, name_msg, strlen(name_msg));

		// write대신 send를 쓰려면 다음과 같이 작성하면 된다
		// send(sock, name_msg, strlen(name_msg), 0);
		// 사실 네번째 인자에는 데이터 전송시 적용할 옵션을 정할 수 있는데 책에서는 대표적으로 긴급 데이터 전송에 관한 예제를 보이고 있다.
		// 채팅프로그램에서는 필요하지 않다고 판단되기 때문에 네번째 인자로 0을 넘기는 것이다 
	}
	return NULL;
}

// read thread main
void * recv_msg(void * arg)   
{
	// send_msg 함수의 내용과 동일하므로 설명 생략
	int sock=*((int*)arg);

	// 생략
	char name_msg[NAME_SIZE+BUF_SIZE];

	// read함수에서 return될 값을 저장하기 위한 목적의 변수.
	int str_len;

	// 계속해서 메시지를 수신할 수 있도록 대기
	while(1)
	{
		// sock으로 부터 수신된 데이터를 name_msg문자열 배열에 세번째 인자의 길이만큼 넣어주세요 라는 뜻이다.
		// -1이 붙은것은 \0을 제거하기 위함인것 같다
		// 
		// 리눅스에서 파일과 소켓을 동일하게 취급한다는 특성 때문에 recv함수를 쓰지 않고도 소켓으로 데이터를 수신할 수 있다.
		// 성공시 수신한 바이트수(단, 파일의 끝을 만나면 0), 실패시 -1이 return된다
		str_len=read(sock, name_msg, NAME_SIZE+BUF_SIZE-1);
		// read대신 recv를 쓰려면 다음과 같이 작성하면 된다
		// recv(sock, name_msg, strlen(name_msg), 0);
		// 사실 네번째 인자에는 데이터 전송시 적용할 옵션에 따라 대응할 인자를 넣어 주어야 하는데
		// 앞서 send함수에서 언급했듯이 채팅프로그램에서는 필요하지 않다고 판단되기 때문에 네번째 인자로 0을 넘기는 것이다

		// -1이 저장되어 있다는건 read에 실패했다는 뜻이므로 return
		if(str_len==-1) 
			return (void*)-1;

		// 배열은 0부터 데이터를 저장하니까 수신된 데이터를 차곡차곡 기록하고 나서 문자열 종료를 알리는 널 문자를 추가해줄 필요가 있음
		// 그래서 str_len-1까지 기록된 수신 데이터 바로 뒷 위치인 str_len에다가 0을 넣어줌으로써 문자열 종료를 알림
		name_msg[str_len]=0;

		// 표준출력 스트림에 name_msg문자열의 내용을 출력해 주세요
		fputs(name_msg, stdout);
	}
	return NULL;
}

// 정봉이 형과 send_msg 쓰레드가 끝나고 나면 recv_msg도 끝내야 하는것 아닌가에 대한 의문을 가졌었다.
// send_msg상의 exit() 호출은 send_msg 쓰레드의 종료를 뜻하지 프로세스 전체의 종료를 뜻하지 않기 때문에
// 분명히 프로세스는 죽지 않아야 하는데 왜죽지? 하고 논의한 결과 send_msg 쓰레드가 죽으면
// recv_msg쓰레드 상의 read함수에서 오류가 발생하여 -1이 return되어 종료된다는 사실을 알게 되었다.

// 에러처리 함수
void error_handling(char *msg)
{
	// 입력받은 문자열을 표준에러 출력스트림에 작성해 주세요
	fputs(msg, stderr);

	// 출력후에는 줄바꿈도 해주시구요 -> fputs가 아니라 fputc임에 유의해야 한다
	// 내가전에 ''를 ""로 썼다가 한참을 고생했다... 까먹지 말자..
	fputc('\n', stderr);

	// 오류를 사용자에게 알렸으니 이제 종료해주세요
	exit(1);
}
