#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <time.h>
#include "clue.h"

#define SCENE (0) // 현장
#define WEAPON (1) // 흉기
#define CRIMINAL (2) // 범인
#define SHUFFLE_CNT (10) // 섞는 횟수
#define DISTRIBUTE_CARD_CNT (16) // 분배되는 카드 개수
#define PLAYER_CNT (4) // 플레이어는 4명

// jusung


// 서버 여는(서버 소켓 만드는) 함수
int server_open()
{
	int ssock = socket(PF_INET, SOCK_STREAM, 0);
	if(ssock == -1)
	{
		perror("socket");
		return -1;
	}

	struct sockaddr_in saddr = {0, };
	saddr.sin_family = AF_INET;
	saddr.sin_port = htons(8080);
	saddr.sin_addr.s_addr = htonl(INADDR_ANY);

	int value = 1;
	if(setsockopt(ssock, SOL_SOCKET, SO_REUSEADDR, &value, sizeof(value)) == -1)
	{
		perror("setsockopt");
		return -1;
	}

	if(bind(ssock, (struct sockaddr *)&saddr, sizeof(saddr)) == -1)
	{
		perror("bind");
		return -1;
	}

	if(listen(ssock, 10) == -1)
	{
		perror("listen");
		return -1;
	}

	printf("[server] CLUE running...\n");

	return ssock;
}

// 플레이어 4명을 받는 함수
int* server_accept(int ssock)
{
	int* players = calloc(PLAYER_CNT, sizeof(int));
	if(players == NULL)
	{
		fprintf(stderr, "server_accept : dynamic allocation error\n");
		return NULL;
	}

	for(int player_num = 0; player_num < PLAYER_CNT; player_num++)
	{
		struct sockaddr_in caddr = {0,};
		int caddr_len = sizeof(caddr);

		// 중간에 플레이어가 나갈 경우 카운트 다시 설정해야 한다.)
		int csock = accept(ssock, (struct sockaddr *)&caddr, &caddr_len);
		if(csock < 0)
		{
			free(players);
			perror("accept");
			return NULL;
		}

		players[player_num] = csock;
		printf("[server] %s is connected...\n", inet_ntoa(caddr.sin_addr));
	}

	return players;
}

int game_set_position(Player_packet** player_packets, Player_packet* player_packet)
{
	if(player_packets == NULL)
	{
		fprintf(stderr, "game_set_position : argument is null\n");
		return -1;
	}

	short position_bit = player_packet->position;
	for(int player_num = 0; player_num < PLAYER_CNT; player_num++)
		player_packets[player_num]->position = position_bit;	

	return 0;
}

// 각 플레이어의 식별 번호를 패킷에 할당하고 처음 위치에 배치하는 함수
int game_init_players(Player_packet** player_packets)
{
	if(player_packets == NULL)
	{
		fprintf(stderr, "game_init_players : argument is null\n");
		return -1;
	}

	Player_packet position_packet = {0,};
	position_packet.position = 0x5555;
	// 플레이어의 식별 번호를 패킷에 할당한다.
	for(int player_num = 0; player_num < PLAYER_CNT; player_num++)
		player_packets[player_num]->info |= player_num;

	// 모든 플레이어를 x:1, y:1로 설정
	game_set_position(player_packets, &position_packet);

	return 0;
}

// 카드를 플레이어들에게 분배하는 함수
int game_init_cards(Player_packet** player_packets, char* answer)
{
	if(player_packets == NULL || answer == NULL)
	{
		fprintf(stderr, "game_init_cards : argument is null\n");
		return -1;
	}

	int cards[DISTRIBUTE_CARD_CNT];
	int cnt = 0;	

	// 정답이 아닌 나머지 카드 설정 
	for(int kind = 0; kind < 3; kind++)
	{
		int maxCnt = (kind == WEAPON) ? 7 : 6;

		for(int card = 0; card < maxCnt; card++)
		{
			int card_value = (kind << 3) | card;
			if(card_value == answer[kind])
				continue;

			cards[cnt++] = (kind << 3) | card;
		}	
	}	

	// 카드10(SHUFFLE_CNT)번 섞음
	for(int i = 0; i < SHUFFLE_CNT; i++)
	{
		// 섞을 두 개의 카드 선택
		int one = rand() % DISTRIBUTE_CARD_CNT;		
		int two = rand() % DISTRIBUTE_CARD_CNT; 
	
		// 섞음
		int tmp = cards[one];
		cards[one] = cards[two];
		cards[two] = tmp;
	}	

	// 분배 
	for(int i = 0; i < PLAYER_CNT; i++)
	{
		player_packets[i]->cards[0] = cards[4*i]; 
		player_packets[i]->cards[1] = cards[4*i + 1]; 
		player_packets[i]->cards[2] = cards[4*i + 2];
		player_packets[i]->cards[3] = cards[4*i + 3];
	}
}

// 다음 플레이어 턴으로 바꾸고 리턴하는 함수
int game_set_turn(Player_packet** player_packets)
{
	if(player_packets == NULL)
	{
		fprintf(stderr, "game_set_turn : argument is null\n");
		return -1;
	}

	int player_num = 0;
	char turn_bit = 0x8;
	for(; player_num < PLAYER_CNT; player_num++)
	{
		// 해당 플레이어가 턴이었으면 다음 플레이어의 턴으로 변경
		if(PLAYER_ISTURN(player_packets[player_num]->info))
		{
			player_packets[player_num]->info ^= turn_bit;
			player_packets[(player_num+1) % PLAYER_CNT]->info ^= turn_bit;
			break;
		}
		else
			continue;
	}

	return player_num;
}

int game_set_phase(Player_packet** player_packets, Player_packet* player_packet)
{
	if(player_packets == NULL || player_packet == NULL)
	{
		fprintf(stderr, "game_set_phase");
		return -1;
	}

	char phase_bit = PLAYER_PHASE(player_packet->info);
	for(int player_num = 0; player_num < PLAYER_CNT; player_num++)
		player_packets[player_num]->info = phase_bit;

	return 0;
}

int game_set_dice(Player_packet** player_packets, Player_packet* player_packet)
{
	if(player_packets == NULL || player_packet == NULL)
	{
		fprintf(stderr, "game_set_dice");
		return -1;
	}

	char dice_bit = PLAYER_DICE_VALUE(player_packet->dice);
	for(int player_num = 0; player_num < PLAYER_CNT; player_num++)
		player_packets[player_num]->dice = dice_bit;

	return 0;
}

int game_set_infer(Player_packet** player_packets, Player_packet* player_packet)
{
	if(player_packets == NULL || player_packet == NULL)
	{
		fprintf(stderr, "game_set_infer");
		return -1;
	}

	short infer_bit = player_packet->infer;
	for(int player_num = 0; player_num < PLAYER_CNT; player_num++)
		player_packets[player_num]->infer = infer_bit;

	return 0;
}

int game_set_clue(Player_packet** player_packets, Player_packet* player_packet)
{
	if(player_packets == NULL || player_packet == NULL)
	{
		fprintf(stderr, "game_set_clue");
		return -1;
	}

	char clue_bit = player_packet->clue;
	for(int player_num = 0; player_num < PLAYER_CNT; player_num++)
		player_packets[player_num]->clue = clue_bit;

	return 0;
}

int game_init_player_data(Player_packet** player_packets)
{
	if(player_packets == NULL)
	{
		fprintf(stderr, "game_init_player_data : argument is null\n");
		return -1;
	}

	Player_packet player_packet = {0,};

	int ret = game_set_phase(player_packets, &player_packet);
	if(ret == -1)
	{
		fprintf(stderr, "game_init_player_data : game_set_phase error\n");
		return -1;
	}

	ret = game_set_dice(player_packets, &player_packet);
	if(ret == -1)
	{
		fprintf(stderr, "game_init_player_data : game_set_dice error\n");
		return -1;
	}

	ret = game_set_infer(player_packets, &player_packet);
	if(ret == -1)
	{
		fprintf(stderr, "game_init_player_data : game_set_infer error\n");
		return -1;
	}

	ret = game_set_clue(player_packets, &player_packet);
	if(ret == -1)
	{
		fprintf(stderr, "game_init_player_data : game_set_clue error\n");
		return -1;
	}

	return 0;
}

int game_next_turn(Player_packet** player_packets)
{
	if(player_packets == NULL)
	{
		fprintf(stderr, "game_next_turn : argument is null\n");
		return -1;
	}

	int ret = game_init_player_data(player_packets);
	if(ret == -1)
	{
		fprintf(stderr, "game_next_turn : game_init_player_data error\n");
		return -1;
	}
	
	int turn_player = game_set_turn(player_packets);
	
	return turn_player;
}

// 모든 플레이어에게 패킷을 보내는 함수
int game_route(int* players, Player_packet** player_packets)
{
	if(players == NULL || player_packets == NULL)
	{
		fprintf(stderr, "game_route : argument is null\n");
		return -1;
	}

	for(int player_num = 0; player_num < PLAYER_CNT; player_num++)
	{
		// 헤더 보내야함
		write(players[player_num], player_packets[player_num], sizeof(player_packets[player_num]));
	}	

	return 0;
}

Player_packet** game_init(int* players, char* answer)
{
	if(players == NULL || answer == NULL)
	{
		fprintf(stderr, "game_init : argument is null\n");
		return NULL;
	}
	
	Player_packet** player_packets = calloc(PLAYER_CNT, sizeof(Player_packet));
	if(player_packets == NULL)
	{
		fprintf(stderr, "game_init : dynamic allocation error\n");
		return NULL;
	}

	for(int player_num = 0; player_num < PLAYER_CNT; player_num++)
	{
		player_packets[player_num] = calloc(1, sizeof(Player_packet));
		if(player_packets[player_num] == NULL)
		{
			for(int i=0; i<player_num; i++)
				free(player_packets[i]);

			free(player_packets);
			fprintf(stderr, "game_init : dynamic allocation error\n");
			return NULL;
		}
	}

	game_init_cards(player_packets, answer);
	game_init_players(player_packets);

	// route
}

char* game_set_answer()
{
	char* answer = calloc(3, sizeof(char));
	if(answer == NULL)
	{
		fprintf(stderr, "game_set_answer : dynamic allocation error\n");
		return NULL;
	}

	srand(time(NULL));

	for(int kind = 0; kind < 3; kind++)
	{
		if(kind == WEAPON)
			answer[kind] = (kind << 3) | (rand() % 7);
		else
			answer[kind] = (kind << 3) | (rand() % 6);	
	}	

	return answer;
}

int main()
{
	int ssock = server_open();	
	int* players = server_accept(ssock);
	char* answer = game_set_answer();	
	Player_packet** player_packets = game_init(players, answer);

	int turn_player = 0;
	while(1)
	{
		// 루프 끝
	}

	return 0;
}

