#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <time.h>
#include "clue.h"

#define SCENE (1) 					// 현장
#define CRIMINAL (2) 				// 범인
#define WEAPON (3) 					// 흉기
#define SHUFFLE_CNT (10) 			// 섞는 횟수
#define DISTRIBUTE_CARD_CNT (16) 	// 분배되는 카드 개수

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
	for(int player_num = 0; player_num < PLAYER_CNT; player_num++) {
		if (player_packets[player_num] != NULL) {
			player_packets[player_num]->position = position_bit;	
		}
	}

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
	player_packets[0]->info |= 0x8;

	// 플레이어의 식별 번호를 패킷에 할당한다.
	for(int player_num = 0; player_num < PLAYER_CNT; player_num++) {
		player_packets[player_num]->info |= player_num;
		printf("PLAYER_ID: %d\n", PLAYER_ID(player_packets[player_num]->info));
	}

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
	for(int kind = 1; kind <= 3; kind++)
	{
		int maxCnt = (kind == WEAPON) ? 7 : 6;

		for(int card = 0; card < maxCnt; card++)
		{
			int card_value = (kind << 3) | card;
			if(card_value == answer[kind-1])
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
	if(player_packets == NULL) {
		fprintf(stderr, "game_set_turn : argument is null\n");
		return -1;
	}

	Player_packet* temp_packet = 0;

	for(int player_num = 0; player_num < PLAYER_CNT; player_num++)
	{
		if(player_packets[player_num] != NULL)
		{
			temp_packet = player_packets[player_num];
			break;
		}
	}	


	int cur_turn_player = PLAYER_TURN_PLAYER(temp_packet->info) >> 4;
	printf("cur_turn_player: %d\n", cur_turn_player);

	int next_turn_player;

	char turn_bit = 0x8;
	for (int i = 1; i < PLAYER_CNT; i++) {
		next_turn_player = (cur_turn_player + i) % PLAYER_CNT;
		if (player_packets[next_turn_player] != NULL) {
			if (player_packets[cur_turn_player] != NULL) {
				player_packets[cur_turn_player]->info ^= turn_bit;
			}
			player_packets[next_turn_player]->info ^= turn_bit;
			break;
		}
	}

	printf("next_turn_player: %d\n", next_turn_player);

	for (int i = 0; i < PLAYER_CNT; i++) {
		if (player_packets[i] != NULL) {
			player_packets[i]->info &= 0xcf;
			player_packets[i]->info |= next_turn_player << 4;
		}
	}

	return 0;
}

int game_set_phase(Player_packet** player_packets)
{
	if(player_packets == NULL)
	{
		fprintf(stderr, "game_set_phase");
		return -1;
	}

	char phase_bit = 0x4;
	for(int player_num = 0; player_num < PLAYER_CNT; player_num++) {
		if(player_packets[player_num] != NULL)
		{
			player_packets[player_num]->info ^= phase_bit;
		}
	}
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
	char select_bit = PLAYER_SELECT_VALUE(player_packet->dice);

	for(int player_num = 0; player_num < PLAYER_CNT; player_num++) {
		if(player_packets[player_num] != NULL)
		{
			player_packets[player_num]->dice = dice_bit;
			player_packets[player_num]->dice |= select_bit;
		}
	}

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
	{
		if(player_packets[player_num] != NULL)
		{
			player_packets[player_num]->infer = infer_bit;
		}
	}

	return 0;
}

int game_set_clue(Player_packet** player_packets, Player_packet* player_packet)
{
	if(player_packets == NULL || player_packet == NULL)
	{
		fprintf(stderr, "game_set_clue");
		return -1;
	}

	printf("set_clue1\n");

	char clue_bit = player_packet->clue;
	for(int player_num = 0; player_num < PLAYER_CNT; player_num++) {
		if (player_packets[player_num] != NULL) 
		{
			player_packets[player_num]->clue = clue_bit;
		}
	}

	printf("set_clue2\n");

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

	int ret = game_set_phase(player_packets);
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
	printf("code1\n");

	if(ret == -1)
	{
		fprintf(stderr, "game_init_player_data : game_set_clue error\n");
		return -1;
	}

	printf("code2\n");
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

	printf("code3\n");

	game_set_turn(player_packets);

	printf("code4\n");

	return 0;
}

// 모든 플레이어에게 각자의 플레이어 패킷을 보내는 함수
int game_route_packet(Player_packet** player_packets, int* players)
{
	if(players == NULL || player_packets == NULL)
	{
		fprintf(stderr, "game_route_packets : argument is null\n");
		return -1;
	}

	int type = PACKET;
	for(int player_num = 0; player_num < PLAYER_CNT; player_num++)
	{
		if(player_packets[player_num] != NULL) {
			packet_send(players[player_num], (char*)player_packets[player_num], &type);
		}
	}

	return 0;
}

int game_route_cards(Player_packet* player_packet, int* players)
{
	if(player_packet == NULL || players == NULL)
	{
		fprintf(stderr, "game_route_cards : argument is null\n");
		return -1;
	}

	Player_packet dead_cards = {0,};
	dead_cards.cards[0] = player_packet->cards[0];
	dead_cards.cards[1] = player_packet->cards[1];
	dead_cards.cards[2] = player_packet->cards[2];
	dead_cards.cards[3] = player_packet->cards[3];

	int type = PACKET;
	for(int player_num = 0; player_num < PLAYER_CNT; player_num++) {
		if(players[player_num] != -1) {
			packet_send(players[player_num], (char*)&dead_cards, &type);
		}
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

	// 서버가 관리할 플레이어들의 패킷 할당
	Player_packet** player_packets = calloc(PLAYER_CNT, sizeof(Player_packet*));
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

	// step 1) 
	game_route_packet(player_packets, players);
	return player_packets;
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

	for(int kind = 1; kind <= 3; kind++)
	{
		if(kind == WEAPON)
			answer[kind-1] = (kind << 3) | (rand() % 7);
		else
			answer[kind-1] = (kind << 3) | (rand() % 6);	
		printf("%d\n", answer[kind-1]);
	}	

	printf("zz4\n");
	return answer;
}

// 턴플레이어에게는 target_signal을,
// 다른 플레이어에게는 other_signal을 전송
int game_send_signal(int *players, int target_signal, int other_signal, int turn_player) 
{
	// 턴 플레이어에게 헤더를 보낸 후 시작 신호를 보낸다.
	// 나머지 플레이어에게 헤더를 보낸 후 대기 신호를 보낸다.
	for(int player_num = 0; player_num < PLAYER_CNT; player_num++)
	{
		if (players[player_num] != -1) {
			int type = SIGNAL;
			int signal;
			if(player_num == turn_player)
			{
				signal = target_signal;	
				printf("player%d : signal %d\n", player_num, signal);
				packet_send(players[player_num], (char*)&signal, &type);
			}
			else
			{
				signal = other_signal;
				printf("player%d : signal %d\n", player_num, signal);
				packet_send(players[player_num], (char*)&signal, &type);
			}
		}
	}
}

int game_roll_and_go(Player_packet** player_packets, int *players)
{
	if (player_packets == NULL || players == NULL)
	{
		fprintf(stderr, "game_roll_and_go : argument is null\n");
		return -1;
	}	

	// 턴 플레이어를 찾는다.
	int turn_player;
	for(int player_num = 0; player_num < PLAYER_CNT; player_num++)
	{
		if(player_packets[player_num] != NULL)
		{
			turn_player = PLAYER_TURN_PLAYER(player_packets[player_num]->info) >> 4;
		}
	}

	// 턴플에게 SIG_TURN을 나머지에게는 SIG_WAIT을 전송
	game_send_signal(players, SIG_TURN, SIG_WAIT, turn_player);

	// 턴 플레이어로부터 주사위+선택값, 위치값이 변경된 패킷을 받는다.
	Player_packet packet = {0,};
	packet_recv(players[turn_player], (char *)&packet, NULL);

	// 각 플레이어의 패킷에 반영한다.
	game_set_dice(player_packets, &packet);	
	game_set_position(player_packets, &packet);

	// 다음 단계(추리)로 변경한다.
	game_set_phase(player_packets);

	// 모든 플레이어에게 턴 플레이어의 주사위+선택 값을 전송
	game_route_packet(player_packets, players);

	printf("\n-----턴플레이어에게 전달받은 패킷 출력-----\n");
	
	printf("turn's POSITION: "), leejinsoo(PLAYER_POSITION(player_packets[turn_player]->position, turn_player), 2);
	printf("DICE_VALUE: "), leejinsoo(PLAYER_DICE_VALUE(player_packets[turn_player]->dice) >> 3, 1);
	printf("SELECT_VALUE: "), leejinsoo(PLAYER_SELECT_VALUE(player_packets[turn_player]->dice), 1);

	return 0;
}


int game_player_out(Player_packet **player_packets, int *players, int player) {
	
	if (player_packets == NULL) {
		fprintf(stderr, "game_player_out: argument is null\n");
		return -1;
	}

	printf("free_player_num: %d\n", player);
	free(player_packets[player]);

	player_packets[player] = NULL;
	players[player] = -1;
	printf("%d 플레이어가 죽었습니다.\n", player);
	return 0;
}

int game_ROOM_OF_TRUTH(Player_packet **player_packets, int *players, int player, char *answer) {

	if (player_packets == NULL || players == NULL || answer == NULL) 
	{
		fprintf(stderr, "game_ROOM_OF_TRUTH: argument is null\n");
		return -1;
	}

	unsigned char infer[3];
	infer[0] = (unsigned char)(PLAYER_INFER_SCENE(player_packets[player]->infer) >> 10);
	infer[1] = (unsigned char)(PLAYER_INFER_CRIMINAL(player_packets[player]->infer) >> 5);
	infer[2] = (unsigned char)(PLAYER_INFER_WEAPON(player_packets[player]->infer));
	if ((answer[0] == infer[0]) && (answer[1] == infer[1]) && (answer[2] == infer[2])) {
		// 패킷을 보내야돼!!!(누가 우승했는지)
		game_send_signal(players, SIG_WIN, SIG_WIN, player);
		return SIG_WIN;
	}
	else {
		game_send_signal(players, SIG_DIE, SIG_DIE, player);
		return SIG_DIE;
	}
}

// 반환값
// SIG_WIN : 턴플레이어가 이김
// SIG_DIE : 턴플레이어가 찍~
int game_infer(Player_packet **player_packets, int *players, char *answer) 
{
	if (player_packets == NULL || players == NULL || answer == NULL) {
		fprintf(stderr, "game_infer: argument is null\n");
		return -1;
	}

	// 턴 플레이어를 찾는다.
	int turn_player;
	for(int player_num = 0; player_num < PLAYER_CNT; player_num++)
	{
		if(player_packets[player_num] != NULL)
		{
			turn_player = PLAYER_TURN_PLAYER(player_packets[player_num]->info) >> 4;
		}
	}
	
	printf("turn_player: %d\n", turn_player);

	// 턴플레이어에게 SIG_INFR 전송
	// 나머지 플레이어에게는 SIG_WAIT 전송
	game_send_signal(players, SIG_INFR, SIG_WAIT, turn_player);

	// 턴플의 추리패킷을 서버가 받음
	Player_packet infer_packet;
	packet_recv(players[turn_player], (char*)&infer_packet, NULL);

	for(int i=0; i<3; i++)
		printf("%d ", answer[i]);

	// 모든 플레이어의 패킷에 추리정보를 세팅
	game_set_infer(player_packets, &infer_packet);
	unsigned short scene = PLAYER_INFER_SCENE(infer_packet.infer) >> 10;
	unsigned short crime = PLAYER_INFER_CRIMINAL(infer_packet.infer) >> 5;
	unsigned short weapon = PLAYER_INFER_WEAPON(infer_packet.infer);

	printf("scene: "), leejinsoo(scene, 1);
	printf("crime: "), leejinsoo(crime, 1);
	printf("weapon: "), leejinsoo(weapon, 1);

	// 모든 플레이어에게 추리정보가 담긴 패킷을 전송
	game_route_packet(player_packets, players);

	// 턴플이 진실의 방에서 추리를 했는지를 확인
	unsigned short position_bit = PLAYER_POSITION(player_packets[turn_player]->position, turn_player);
	position_bit >>= (4 * (3 - turn_player));
	
	// x:3, y:3
	if(position_bit == 0xF)
	{
		int ret = game_ROOM_OF_TRUTH(player_packets, players, turn_player, answer);
		// 게임종료(답 맞음)
		if (ret == SIG_WIN) {
			return SIG_WIN;
		}
		// 턴플 죽음(답 틀림)
		else if (ret == SIG_DIE) {
			// 자기패를 모든 플레이어에게 라우트
			game_route_cards(player_packets[turn_player], players);
			game_player_out(player_packets, players, turn_player);

			return SIG_DIE;
		}
		else {
			// 오류
			return -1;
		}
	}
	// 턴플의 다음 순서부터 한 명씩 단서 요청(시그널을 보내는 방식으로 요청)
	// 최초의 sig는 SIG_TURN으로 설정

	int sig = SIG_TURN;
	int clue_player;
	Player_packet packet = {0,};

	for (int i = 1; i < PLAYER_CNT; i++) {
		int target = (turn_player + i) % PLAYER_CNT;
		int type = SIGNAL;	
		packet_send(players[target], (char *)&sig, &type);

		// 플레이어는 단서가 있든 없든 SIG_TURN 요청이 오면 무조건 패킷을 보냄
		if (sig == SIG_TURN) {
			// 단서 패킷을 받음
			packet_recv(players[target], (char *)&packet, NULL);

			printf("info: "), leejinsoo(packet.info, 1);
			printf("position: "), leejinsoo(packet.position, 2);
			printf("cards: "), leejinsoo(packet.cards[0], 1), leejinsoo(packet.cards[1], 1), leejinsoo(packet.cards[2], 1), leejinsoo(packet.cards[3], 1);
			printf("dice: "), leejinsoo(packet.dice, 1);
			printf("infer: "), leejinsoo(packet.infer, 2);
			printf("after_clue: "), leejinsoo(packet.clue, 1);

			printf("\n\n");
			if (packet.clue != 0) {
				printf("SIG_DONE으로 세팅됨!!\n");
				clue_player = target;
				sig = SIG_DONE;
			}
		}
	}
	
	// 다른 플레이어 중 한명이 단서를 낸 경우        : sig => SIG_DONE
	// 다른 플레이어 중 아무도 단서를 내지 않은 경우 : sig => SIG_TURN

	//--------- 아래는 턴플레이어를 처리 ---------//

	// sig가 SIG_TURN : 한바퀴 돌아서 턴 플레이어가 단서를 제출해야함
	// sig가 SIG_DONE : 다른 플레이어가 단서를 제출함
	int player = clue_player;
	int type = SIGNAL;
	packet_send(players[turn_player], (char *)&sig, &type);

	// 다른 플레이어 중 아무도 단서를 내지 않은 경우
	// 턴 플레이어가 단서를 제출해야함
	if (sig == SIG_TURN) {

		// 단서 세팅 패킷을 턴 플레이어로 변경
		player = turn_player;

		// 턴플레이어로부터 단서 정보를 기다림
		packet_recv(players[turn_player], (char *)&packet, NULL);
		printf("턴플레이어가 보낸 단서 정보 : "), leejinsoo(packet.clue, 1);
	}

	// 모든 플레이어의 단서를 세팅
	printf("누군가 낸 단서 : "), leejinsoo(player_packets[player]->clue, 1);
	game_set_clue(player_packets, &packet);

	// 모든 플레이어에게 단서가 담긴 패킷 전송
	game_route_packet(player_packets, players);

	return 0;
}

int END_GAME(Player_packet **player_packets, int *players, char *answer, int ssock) {

	if (player_packets == NULL || answer == NULL) {
		fprintf(stderr, "END_GAME: argument is null\n");
		return -1;
	}

	for (int i = 0; i < PLAYER_CNT; i++) {
		if (player_packets[i] == NULL) {
			continue;
		}
		free(player_packets[i]);
		player_packets[i] = NULL;
	}

	free(players);
	free(answer);
	close(ssock);
	return 0;
}

int main()
{
	int ssock = server_open();			 // 서버를 연다.
	int* players = server_accept(ssock); // 플레이어 4명을 받는다.
	char* answer = game_set_answer();    // 정답 카드를 설정한다.
	
	// 플레이어 게임 정보를 초기화한 뒤 각 플레이어에게 전송
	Player_packet** player_packets = game_init(players, answer);

	// 게임이 끝날 때까지 반복
	while(1)
	{
		printf("나, 메인\n");
		if (game_roll_and_go(player_packets, players) == -1) {
			return -1;
		}

		if (game_infer(player_packets, players, answer) == SIG_WIN) {
			break;
		}

		printf("test code\n");
		// 턴 플레이어를 변경(비트 세팅)
		game_next_turn(player_packets);
	}

	// 끝
	END_GAME(player_packets, players, answer, ssock);
	return 0;
}
