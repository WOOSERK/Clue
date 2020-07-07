#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <time.h>
#include "clue.h"

#define SCENE (1) // 현장
#define CRIMINAL (2) // 범인
#define WEAPON (3) // 흉기
#define SHUFFLE_CNT (10) // 섞는 횟수
#define DISTRIBUTE_CARD_CNT (16) // 분배되는 카드 개수


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
		player_packets[player_num]->position = position_bit;	
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
		player_packets[player_num]->info ^= phase_bit;
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
		player_packets[player_num]->dice = dice_bit;
		player_packets[player_num]->dice |= select_bit;
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

	game_set_turn(player_packets);
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
		packet_send(players[player_num], (char*)player_packets[player_num], &type);

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
		packet_send(players[player_num], (char*)&dead_cards, &type);
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
			answer[kind] = (kind << 3) | (rand() % 7);
		else
			answer[kind] = (kind << 3) | (rand() % 6);	
	}	

	return answer;
}

// signal_ : SIG_TURN or SIG_INFR
int game_send_signal(int *players, int signal_, int turn_player) 
{
	// 턴 플레이어에게 헤더를 보낸 후 시작 신호를 보낸다.
	// 나머지 플레이어에게 헤더를 보낸 후 대기 신호를 보낸다.
	for(int player_num = 0; player_num < PLAYER_CNT; player_num++)
	{
		int type = SIGNAL;
		if(player_num == turn_player)
		{
			int signal = signal_;	
			packet_send(players[player_num], (char*)&signal, &type);
		}
		else
		{
			int signal = SIG_WAIT;
			packet_send(players[player_num], (char*)&signal, &type);
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
		if(PLAYER_ISTURN(player_packets[player_num]->info))
		{
			turn_player = player_num;	
			break;
		}
	}

	game_send_signal(players, SIG_TURN, turn_player);

	// 턴 플레이어로부터 주사위+선택값, 위치값이 변경된 패킷을 받는다.
	char buf[BUFSIZ];
	packet_recv(players[turn_player], buf, NULL);

	// 각 플레이어의 패킷에 반영한다.
	game_set_dice(player_packets, (Player_packet*)buf);	
	game_set_position(player_packets, (Player_packet*)buf);

	// 다음 단계(추리)로 변경한다.
	game_set_phase(player_packets);

	// 모든 플레이어에게 턴 플레이어의 주사위+선택 값을 전송
	game_route_packet(player_packets, players);

	printf("\n-----턴플레이어에게 전달받은 패킷 출력-----\n");
	printf("turn's POSITION: %d\n", PLAYER_POSITION(player_packets[0]->position, turn_player));
	printf("DICE_VALUE: %d\n", PLAYER_DICE_VALUE(player_packets[0]->dice));
	printf("SELECT_VALUE: %d\n", PLAYER_SELECT_VALUE(player_packets[0]->dice));

	return 0;
}


int game_player_out(Player_packet **player_packets, int *players, int player) {
	
	if (player_packets == NULL) {
		fprintf(stderr, "game_player_out: argument is null\n");
		return -1;
	}

	free(player_packets[player]);
	player_packets[player] = NULL;
	players[player] = -1;
	return 0;
}

int game_ROOM_OF_TRUTH(Player_packet **player_packets, int *players, int player, char *answer) {

	if (player_packets == NULL || answer == NULL) 
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
		return 1;
	}
	else {
		game_player_out(player_packets, players, player);
		return 2;
	}
}

int game_infer(Player_packet **player_packets, int *players, char *answer) 
{
	if (player_packets == NULL || players == NULL) {
		fprintf(stderr, "game_infer: argument is null\n");
		return -1;
	}

	// 턴 플레이어를 찾는다.
	int turn_player;
	for(int player_num = 0; player_num < PLAYER_CNT; player_num++)
	{
		if(PLAYER_ISTURN(player_packets[player_num]->info))
		{
			turn_player = player_num;	
			break;
		}
	}
	printf("turn: %d\n", turn_player);

	// 턴플레이어에게 SIG_INFR 전송!!
	// 나머지 플레이어에게는 SIG_WAIT 전송!!
	game_send_signal(players, SIG_INFR, turn_player);

	// 턴플의 추리패킷을 서버가 받음
	Player_packet infer_packet;
	packet_recv(players[turn_player], (char*)&infer_packet, NULL);
	printf("turn's infer: %d\n", infer_packet.infer);

	// 턴플이 진실의 방에서 추리를 했는지를 확인
	unsigned short position_bit = PLAYER_POSITION(player_packets[turn_player]->position, turn_player);
	position_bit >>= (4 * (3 - turn_player));
	
	// x:3, y:3
	if(position_bit == 0xF)
	{
		int ret = game_ROOM_OF_TRUTH(player_packets, players, turn_player, answer);
		if (ret == 1) {
			// 게임종료(답 맞음)
			return 1;
		}
		else if (ret == 2) {
			// 턴플 죽음(답 틀림)
			// 자기패를 모든 플레이어 패킷에 세팅한 뒤 라우트
			game_route_cards(player_packets[turn_player], players);
			return 2;
		}
		else {
			// 오류
			return -1;
		}
	}

	// 모든 플레이어의 패킷에 추리정보를 반영
	game_set_infer(player_packets, player_packets[turn_player]);

	// 턴플을 제외한 나머지 플레이어에게 추리정보가 담긴 패킷을 전송
	for (int i = 1; i < PLAYER_CNT; i++) {
		int target = (turn_player + i) % PLAYER_CNT;
		int type = PACKET;
		packet_send(players[target], (char *)player_packets[target], &type);
	}

	int value = SIG_TURN;
	int clue_player;
	for (int i = 1; i < PLAYER_CNT; i++) {
		int target = (turn_player + i) % PLAYER_CNT;
		int type = SIGNAL;	
		packet_send(players[target], (char *)&value, &type);

		// 아직 아무도 단서를 내지 않음
		if (value == SIG_TURN) {

			// 단서 패킷을 받음
			Player_packet packet;
			packet_recv(players[target], (char *)&packet, NULL);

			// 단서 유무 확인
			// 단서를 냄
			if (packet.clue != 0) {
				clue_player = target;
				value = SIG_DONE;
			}
		}
	}

	// 결국 아무도 안내서 결국 턴플이 냄
	if (value == SIG_TURN) {
		printf("SIG_TURN\n");
		int type = SIGNAL;
		packet_send(players[turn_player], (char *)&value, &type);

		Player_packet packet;

		// 턴플레이어로부터 단서 정보를 기달
		packet_recv(players[turn_player], (char *)&packet, NULL);
		printf("턴플레이어가 보낸 자신의 카드 : %d\n", packet.clue);

		// 만약 턴플이 보낸 단서가 0인 경우 답임. 처리해주어야함

		// 모든 플레이어의 패킷에 턴플레이어의 단서 정보를 세팅
		game_set_clue(player_packets, player_packets[turn_player]);

		// 다른 플레이어들에게 턴플의 단서를 보여주기 전에 준비 신호(SIG_TURN_PLAYER)를 보냄
		int sig = SIG_TURN_PLAYER;
		for (int i = 1; i < PLAYER_CNT; i++) {
			int target = (turn_player + i) % PLAYER_CNT;
			int type = SIGNAL;	
			packet_send(players[target], (char *)&sig, &type);
		}

		// 모든 플레이어에게 추리정보를 전송
		game_route_packet(player_packets, players);
	}
	// 다른 플레이어가 단서를 제출한 경우
	else {
		printf("SIG_WAIT\n");
		int sig = SIG_WAIT;
		
		// 모든 플레이어에게 SIG_WAIT을 제출
		// 왜? 
		for (int i = 0; i < PLAYER_CNT; i++) {
			int target = (turn_player + i) % PLAYER_CNT;
			int type = SIGNAL;	
			packet_send(players[target], (char *)&sig, &type);
		}
		
		int type = PACKET;
		player_packets[turn_player]->clue = player_packets[clue_player]->clue;

		// clue_player를 제외한 다른 나머지 패킷들은 clue비트가 0으로 세팅되어 있음
		// 따라서 통일성있게 turn플레이어를 제외한 나머지 플레이어의 clue는 0으로 세팅
		player_packets[clue_player]->clue = 0;
		game_route_packet(player_packets, players);
	}

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
		game_roll_and_go(player_packets, players);
		if (game_infer(player_packets, players, answer) == 1) {
			break;
		}

		// 턴 플레이어를 변경(비트 세팅)
		game_next_turn(player_packets);
	}

	// 끝
	END_GAME(player_packets, players, answer, ssock);
	return 0;
}


