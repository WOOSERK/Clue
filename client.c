// hohohoho

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h> 
#include <sys/uio.h>
#include <unistd.h>
#include "clue.h"

int client_connect(void);
int game_init(int);
int game_play(int);
int client_connect(void);
int game_init(int sock);
int ack_to_server(int sock);
int ui_update(Player_packet* );
// int change_player_position(WINDOW* window, Player_packet* player_pakcet);
// int set_my_amazing_cards(WINDOW* window, Player_packet* player_pakcet);
// int set_our_amazing_jeahyeon_history();
// int set_our_amazing_jeahyeon_log();
int sig_recv(int sock);

int main(){
	int sock = client_connect();
	if(game_init(sock) == -1){
		return -1;
	}
	int result = game_play(sock);
}

int game_play(int sock){
	// 이후 클라이언트는 sig_recv로 대기한다. 
	// 서버는 SIG_TURN 또는 SIG_WAIT을 보낼 것이다.
	while(1){
		int sig = sig_recv(sock);
		if(sig == '1'){
// 			roll_and_go();
		}
		char buf[BUFSIZ];
		int nRead = read(sock, buf, sizeof(buf));

	}
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
	addr.sin_addr.s_addr = inet_addr("192.168.194.139"); // 서버주소
	
	if(connect(sock, (struct sockaddr *)&addr, sizeof(addr)) == -1){
		perror("connect");
		return -1;
	}

	return sock;
}

int game_init(int sock){
	//서버로부터 초기화 패킷을 받아 게임을 초기화
	//클라이언트는 자신패킷을 가지고 있지 않음 .
	//따라서 매번 서버에서 패킷을 받아서 파싱해야 함.

	// 패킷을 받는 것을 함수로 해야함.
	char buf[BUFSIZ];
	int nRead = read(sock, buf, sizeof(buf));
	if(nRead < 0){
		perror("read");
		return -1;
	}	
	else if(nRead == 0){
		return -1;
	}
	buf[nRead] = '\0';
	if(ack_to_server(sock) == -1){
		return -1;
	}
	// 여기에는 받은 패킷을 파싱하는 함수가 있어야 함.

	// 여기에서부터 초기화 업데이트
// 	ui_update(player); // 여기에선 리턴값을 어떻게 설정할지 몰라서 if처리 안했음

	

}
int ack_to_server(int sock){
	int ack;
	ack = '1';
	int nWrite = write(sock, &ack, sizeof(int));
	if(nWrite == -1){
		perror("write");
		return -1;
	}
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
int sig_recv(int sock){
	int sig;
	int nRead = read(sock, &sig, sizeof(int));
	if(nRead < 0){
		perror("read");
		return -1;
	}
	return sig; 
}
