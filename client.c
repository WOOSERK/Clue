//wonchang	
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
int game_play(int);
int client_connect(void);
int game_update(int sock);
// int ui_update(Player_packet* );
// int change_player_position(WINDOW* window, Player_packet* player_pakcet);
// int set_my_amazing_cards(WINDOW* window, Player_packet* player_pakcet);
// int set_our_amazing_jeahyeon_history();
// int set_our_amazing_jeahyeon_log();
int roll_and_go(int sock);
int roll_dice(void);
int return_player_choice(int dice_value);
int return_player_position(int choice, int* y, int* x);
int set_dice_in_packet(Player_packet* packet, int* num, int* choice);
int set_player_in_packet(Player_packet* packet, int* num, int* choice);
int sig_recv(int sock);

int main(){
	int sock = client_connect();
	if(game_update(sock) == -1){
		return -1;
	}
	int result = game_play(sock);
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
	addr.sin_addr.s_addr = inet_addr("192.168.30.6"); // 서버주소
	
	if(connect(sock, (struct sockaddr *)&addr, sizeof(addr)) == -1){
		perror("connect");
		return -1;
	}

	return sock;
}
int game_play(int sock){
	// 이후 클라이언트는 sig_recv로 대기한다. 
	// 서버는 SIG_TURN 또는 SIG_WAIT을 보낼 것이다.
	while(1){
		int sig;
		int type= SIGNAL;
		packet_recv(sock,(char*)&sig,&type);
		if(sig == SIG_TURN){
			printf("sigturn을 받았습니다.\n");
			roll_and_go(sock);
		}

		char buf[BUFSIZ];
		int nRead = read(sock, buf, sizeof(buf));

	}
}

int roll_and_go(int sock){
	int num = 0;
	int choice = 0;
	int x,y;
	
	num = roll_dice();
	choice = return_player_choice(num);
	if(return_player_position(choice, &y, &x) == -1){
		return -1;	 
	}
	Player_packet packet;
	set_dice_in_packet(&packet, &num, &choice);
	set_player_in_packet(&packet, &y, &x);

	int nWrite = write(sock,&packet, sizeof(packet)); 
	if(nWrite <= 0 ){
		perror("write");
		return -1;
	}
	return 0;
}
int roll_dice(void){
	return (rand()%6);
}
int return_player_choice(int dice_value){
	// 선택값을 리턴하는 함수
	int select_no = -1;
	while(1){
		printf("나온값의 수: %d\n  이하의 수들중 하나의 값을 고르세요: ",dice_value);
		scanf("%d",&select_no);
		if(select_no <= dice_value){
			break;	
		}
	}
	return select_no;
}

int return_player_position(int choice, int* y, int*x){
	*x = 2;
	*y = 2;
	// 이부분은 ui와 연동이 되어야 함.


	return 0;
}
int set_dice_in_packet(Player_packet* packet, int* num, int* choice){
	return 0;
}
int set_player_in_packet(Player_packet* packet, int* num, int* choice){
	return 0;
}
int game_update(int sock){
	//서버로부터 초기화 패킷을 받아 게임을 초기화
	//클라이언트는 자신패킷을 가지고 있지 않음 .
	//따라서 매번 서버에서 패킷을 받아서 파싱해야 함.
	Player_packet packet;

	// 패킷을 받는 것을 함수로 해야함.
	// 여기에는 받은 패킷을 파싱하는 함수가 있어야 함.
	int struct_ = STRUCTURE ;
	packet_recv(sock,(char*)&packet, &struct_);
	printf("id: %d\n", PLAYER_ID(packet.info));
	printf("phase: %d\n",PLAYER_PHASE(packet.info)); 
	printf("isTurn: %d\n",PLAYER_ISTURN(packet.info)); 
	printf("position: %d\n",PLAYER_POSITION(packet.position, 0)); 
	printf("position: %d\n",PLAYER_POSITION(packet.position, 1)); 
	


	// 여기에서부터 초기화 업데이트
// 	ui_update(player); // 여기에선 리턴값을 어떻게 설정할지 몰라서 if처리 안했음

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

