#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h> 
#include <sys/uio.h>
#include <unistd.h>
#include <stdlib.h>

#include "clue.h"


int client_connect(void);
int game_update(int sock, int *player_id);
int game_play(int sock, int player_id);
// int ui_update(Player_packet* );
// int change_player_position(WINDOW* window, Player_packet* player_pakcet);
// int set_my_amazing_cards(WINDOW* window, Player_packet* player_pakcet);
// int set_our_amazing_jeahyeon_history();
// int set_our_amazing_jeahyeon_log();
int roll_and_go(int sock, int player_id); int roll_dice(void);
int return_player_choice(int dice_value);
int return_player_position(int choice, int* y, int* x);
int set_dice_in_packet(Player_packet* packet, int dice_value, int choice_value);
int set_player_in_packet(Player_packet* packet, int player_id, int y, int x);
int sig_recv(int sock);


int main(){
	
	int sock = client_connect();

	int player_id;
	if(game_update(sock, &player_id) == -1){
		return -1;
	}

	int result = game_play(sock, player_id);
	return 0;	
}


int client_connect(void){
	int sock = socket(PF_INET, SOCK_STREAM, 0); // tcp/ip 
	if(sock == -1){
		perror("socket");
		return -1;
	}

	struct sockaddr_in addr = {0,};
	addr.sin_family = AF_INET;
	addr.sin_port = htons(8080);
	addr.sin_addr.s_addr = inet_addr("127.0.0.1"); // 서버주소
	
	if(connect(sock, (struct sockaddr *)&addr, sizeof(addr)) == -1){
		perror("connect");
		return -1;
	}

	return sock;
}

int game_play(int sock, int player_id){
	
	// 이후 클라이언트는 sig_recv로 대기한다. 
	// 서버는 SIG_TURN 또는 SIG_WAIT을 보낼 것이다.
	while(1){
		int sig;
		int type= SIGNAL;
		packet_recv(sock,(char*)&sig,&type);
		if(sig == SIG_TURN){
			printf("sigturn을 받았습니다.\n");
			printf("player_id: %d\n", player_id);
			roll_and_go(sock, player_id);
		}
	}
}

int roll_and_go(int sock, int player_id){

	int dice_value = 0;
	int choice_value = 0;
	int x,y;

	// 주사위값 얻어냄
	dice_value = roll_dice();

	// 선택값 얻어냄
	choice_value = return_player_choice(dice_value);

	// 위치값 얻어냄
	if(return_player_position(choice_value, &y, &x) == -1){
		return -1;	 
	}

	Player_packet packet = {0,};
	set_dice_in_packet(&packet, dice_value, choice_value);
	set_player_in_packet(&packet, player_id, y, x);

	printf("dice: %d\n", (unsigned char)packet.dice);
	printf("position: %hu\n", packet.position);

	int type = PACKET;
	packet_send(sock, (char*)&packet, &type);

	/*
	int nWrite = write(sock, &packet, sizeof(packet)); 
	if(nWrite <= 0 ){
		perror("write");
		return -1;
	}
	*/

	return 0;
}

int roll_dice(void){
	return (rand()%6);
}

int return_player_choice(int dice_value){
	
	// 선택값을 리턴하는 함수
	int choice_value = -1;
	while(1){
		printf("나온값의 수: %d\n  이하의 수들중 하나의 값을 고르세요: ",dice_value);
		scanf("%d",&choice_value);
		if(choice_value <= dice_value){
			break;	
		}
	}
	return choice_value;
}

int return_player_position(int choice, int* y, int*x){
	*x = 1;
	*y = 2;
	// 이부분은 ui와 연동이 되어야 함.

	return 0;
}

int set_dice_in_packet(Player_packet* packet, int dice_value, int choice_value){
	printf("dice_value: %d\n", dice_value);
	printf("choice_value: %d\n", choice_value);
	
	packet->dice |= (unsigned char)(dice_value << 3);
	packet->dice |= (unsigned char)(choice_value);
	return 0;
}

int set_player_in_packet(Player_packet* packet, int player_id, int y, int x){
	printf("x: %d\n", x);
	printf("y: %d\n", y);

	x <<= 2;
	packet->position |= (unsigned short)(x << ((3 - player_id) * 4));
	packet->position |= (unsigned short)(y << ((3 - player_id) * 4));
	return 0;
}

int game_update(int sock, int *player_id){
	
	//서버로부터 초기화 패킷을 받아 게임을 초기화
	//클라이언트는 자신패킷을 가지고 있지 않음 .
	//따라서 매번 서버에서 패킷을 받아서 파싱해야 함.
	Player_packet packet;

	// 패킷을 받는 것을 함수로 해야함.
	// 여기에는 받은 패킷을 파싱하는 함수가 있어야 함.
	int type;
	packet_recv(sock,(char*)&packet, &type);

	printf("플레이어 %d :업데이트 시작!!\n", PLAYER_ID(packet.info));
	printf("phase: %d\n",PLAYER_PHASE(packet.info)); 
	printf("isTurn: %d\n",PLAYER_ISTURN(packet.info)); 
	printf("position: %d\n",(unsigned short)PLAYER_POSITION(packet.position, 0)); 
	printf("position: %d\n",(unsigned short)PLAYER_POSITION(packet.position, 1)); 
	printf("position: %d\n",(unsigned short)PLAYER_POSITION(packet.position, 2)); 
	printf("position: %d\n",(unsigned short)PLAYER_POSITION(packet.position, 3)); 
	printf("플레이어 %d : 업데이트 완료!!\n", PLAYER_ID(packet.info));

	// 여기에서부터 초기화 업데이트
	// 	ui_update(player); // 여기에선 리턴값을 어떻게 설정할지 몰라서 if처리 안했음

	*player_id = PLAYER_ID(packet.info);
	return 0;	
}

// int ui_update(Player_packet* player_packet){
// 	change_player_position();
// 	set_my_amazing_cards();
// 	set_our_amazing_jeahyeon_history();
// 	set_our_amazing_jeahyeon_log();
// 
// 	return 0;
// }
// int change_player_position(WINDOW* window, Player_packet* player_packet){
// 
// }
// int set_my_amazing_cards(WINDOW* window, Player_packet* player_packet){
// 
// }
// int set_our_amazing_jeahyeon_history(){
// 
// }
// int set_our_amazing_jeahyeon_log(){
// 
// }


