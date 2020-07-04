#include <unistd.h>
#include <string.h>
#include <time.h>
#define PACKET (1)
#define SIGNAL (2)
#define PACKET_SIZE (sizeof(Player_packet))
#define SIGNAL_SIZE (sizeof(int))
#define HEADER_SIZE (sizeof(Header))
#define SIG_TURN (1)
#define SIG_WAIT (2)
#define SIG_INFR (3)
#define SIG_DONE (4)
#define PLAYER_ID(player_info) ((player_info) & ((unsigned char)0x3) 
#define PLAYER_PHASE(player_info) ((player_info) & ((unsigned char)0x4))
#define PLAYER_ISTURN(player_info) ((player_info) & ((unsigned char)0x8))
#define PLAYER_POSITION(player_position, num) ((player_position) & ((unsigned short)0xf000 >> (num >> 2)))
#define PLAYER_SELECT_VALUE(player_select) ((player_select) & ((unsigned char)0x7))
#define PLAYER_DICE_VALUE(player_dice) ((player_dice) & ((unsigned char)0x38))
#define PLAYER_INFER_SCENE(player_infer) ((player_infer) & ((unsigned short)0x7c00))
#define PLAYER_INFER_WEAPON(player_infer) ((player_infer) & ((unsigned short)0x30e0))
#define PLAYER_INFER_CRIMINAL(player_infer) ((player_infer) & ((unsigned short)0x1f))

typedef struct header
{
	int type;
	int len;
} Header;

typedef struct player
{
	char info;	
	short position;
	char cards[4];
	char dice;
	short infer;
	char clue;
} Player_packet;

// 성공하면 0, 실패하면 -1을 리턴
int packet_send(int sock, char* packet, int* type)
{
	if(packet == NULL || type == NULL)
	{
		fprintf(stderr, "packet_send : argument is null\n");
		return -1;
	}

	// 1. 헤더 보냄
	Header header;
	header.type = *type;
	switch(*type)
	{
		// Player_packet을 전송할 경우
		case PACKET:
			header.len = sizeof(Player_packet);
			break;
		case SIGNAL:
			header.len = sizeof(int);
			break;
		default:
			break;
	}

	int ret = write(sock, &header, sizeof(Header));
	if(ret == -1)
	{
		perror("packet_send");
		return -1;
	}	
	
	// 2. 패킷 보냄
	// 성공적으로 전송할 때까지 반복
	while(1)
	{
		int nwritten = write(sock, packet, header.len);

		// 패킷을 전송하다 오류가 나면
		if(nwritten == -1)
		{
			perror("packet_send");
			return -1;
		}
		// 패킷을 성공적으로 전송했으면
		else if(nwritten == header.len)
			break;
		// 패킷이 정상적으로 가지 않았으면 다시 보냄
		else
			continue;
	}

	return 0;
}

// 성공하면 0, 실패하면 -1을 리턴. 인자 중 type은 반환에 사용
int packet_recv(int sock, char* packet, int* type)
{
	if(packet == NULL || type == NULL)
	{
		fprintf(stderr, "packet_recv : argument is null\n");
		return -1;
	}

	char buf[BUFSIZ];
	Header header;		
	// 1. 헤더를 받음
	while(1)
	{
		int nread = read(sock, &header, sizeof(header));
		// 헤더를 읽다가 오류가 나면
		if(nread == -1)
		{
			perror("packet_recv");
			return -1;
		}
		// 헤더를 성공적으로 읽었으면 다음
		else
			break;
	}

	// 2. 패킷을 받음
	// 성공적으로 받을 때까지 반복
	while(1)
	{
		int nread = read(sock, packet, header.len);
		if(nread == -1)
		{
			perror("packet_recv");
			return -1;
		}
		else
			break;
	}

	*type = header.type;
	return 0;
}
