#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h> 
#include <sys/uio.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>

#include "ncur.h"
#include "clue.h"


int client_connect(void);
int game_init(int sock, int *player_id);
int game_update(int sock);
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
int game_infer(int sock, int player_id);
int clue_you_got(char* clue);
int send_clue(int sock, Player_packet *packet);
int print_clue(Player_packet *packet);


int main(){
	
	int sock = client_connect();
	if (sock == -1) {
		return -1;
	}

	initscr();
	refresh();
	start_color();
	color_init();

	int player_id;
	if(game_init(sock, &player_id) == -1) {
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
	addr.sin_addr.s_addr = inet_addr("192.168.30.31"); // 서버주소
	
	if(connect(sock, (struct sockaddr *)&addr, sizeof(addr)) == -1){
		perror("connect");
		return -1;
	}

	return sock;
}

int game_play(int sock, int player_id){
	
	// 이후 클라이언트는 packet_recv로 대기한다. 
	// 서버는 SIG_TURN 또는 SIG_WAIT을 보낼 것이다.
	while(1){
		int sig;
		int type;
		packet_recv(sock,(char*)&sig, &type);

		// 턴플레이어의 경우
		if(sig == SIG_TURN){
			printf("내 턴입니다!!\n");
			roll_and_go(sock, player_id);
		}

		// 턴 플레이어의 roll_and_go 정보를 모든 플레이어가 받아서 업데이트
		game_update(sock);

		game_infer(sock, player_id);
	}
	return 0;
}

int roll_and_go(int sock, int player_id){

	int dice_value = 0;
	int choice_value = 0;
	int x,y;

	// 주사위값 얻어냄
	dice_value = roll_dice();

	// To. 경안
	// 경안아. 선택값과 위치값을 UI에서 선택해야한단다.
	// 아래 함수들을 참고하렴

	// 선택값 얻어냄
	choice_value = return_player_choice(dice_value);

	// 위치값 얻어냄
	if(return_player_position(choice_value, &y, &x) == -1){
		return -1;	 
	}

	Player_packet packet = {0,};
	set_dice_in_packet(&packet, dice_value, choice_value);
	set_player_in_packet(&packet, player_id, y, x);

	int type = PACKET;

	// 서버에게 roll_and_go 정보를 전송
	packet_send(sock, (char*)&packet, &type);

	return 0;
}

int roll_dice(void){
	srand((unsigned int)time(NULL));
	return (rand()%6) + 1;
}

int return_player_choice(int dice_value){
	
	// 선택값을 리턴하는 함수
	int choice_value = -1;
	while(1){
		printf("주사위값: %d\n선택값 입력: ", dice_value);
		scanf("%d", &choice_value);
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
	packet->dice |= (unsigned char)(dice_value << 3);
	packet->dice |= (unsigned char)(choice_value);
	return 0;
}

int set_player_in_packet(Player_packet* packet, int player_id, int y, int x){
	x <<= 2;
	packet->position |= (unsigned short)(x << ((3 - player_id) * 4));
	packet->position |= (unsigned short)(y << ((3 - player_id) * 4));
	return 0;
}

int game_init(int sock, int *player_id) {

	Player_packet packet;

	// step 1)
	packet_recv(sock,(char*)&packet, NULL);
	
	// ui_update(player); 
	display_init(packet.cards);	 // 경안₩

	*player_id = PLAYER_ID(packet.info);

	return 0;
}

int game_update(int sock){
	
	// 서버로부터 초기화 패킷을 받아 게임을 초기화
	// 클라이언트는 자신패킷을 가지고 있지 않음 .
	// 따라서 매번 서버에서 패킷을 받아서 파싱해야 함.
	Player_packet packet;

	// 패킷을 받는 것을 함수로 해야함.
	// 여기에는 받은 패킷을 파싱하는 함수가 있어야 함.
	int type;
	packet_recv(sock,(char*)&packet, &type);

	// 여기에서부터 초기화 업데이트

	// To. 경안
	// 업데이트 잘하렴
	// 우리는 너를 믿는단다
	// ui_update(player); 

	printf("-----아래 정보로 화면 업데이트 진행-----\n");
	for (int i = 0; i < PLAYER_CNT; i++) {
		printf("%d's POSITION: %d\n", i, PLAYER_POSITION(packet.position, i));
	}
	printf("DICE_VALUE: %d\n", PLAYER_DICE_VALUE(packet.dice));
	printf("SELECT_VALUE: %d\n\n", PLAYER_SELECT_VALUE(packet.dice));

	return 0;	
}


// packet변수에 서버로부터 라우팅된 단서 패킷이 세팅됨
int send_clue(int sock, Player_packet *packet) {

	unsigned short scene = PLAYER_INFER_SCENE(packet->infer) >> 10;
	unsigned short crime = PLAYER_INFER_CRIMINAL(packet->infer) >> 5;
	unsigned short weapon = PLAYER_INFER_WEAPON(packet->infer);

	//printf("scene(send_clue): %hd\n", scene);
	//printf("crime(send_clue): %hd\n", crime);
	//printf("weapon(send_clue): %hd\n", weapon);

	if (packet == NULL) {
		fprintf(stderr, "send_clue : argument is null\n");
		return -1;
	}

	//printf("packet_clue_before: %hd\n", packet->clue);
	// 자신이 가지고 있는 카드가 추리 정보에 있을 경우, my_cards 배열에 넣음
	char my_cards[3] = {0,};
	int cnt = 0;
	for (int i = 0; i < sizeof(my_cards); i++){
		printf("packet->cards[%d]: %d\n", i, packet->cards[i]);
		if (scene == packet->cards[i]) {
			my_cards[cnt++] = packet->cards[i];		
		}
		else if (crime == packet->cards[i]) {
			my_cards[cnt++] = packet->cards[i];		
		}
		else if (weapon == packet->cards[i]) {
			my_cards[cnt++] = packet->cards[i];		
		}
	}

	printf("-----DEBUG START-----\n");
	for (int i = 0; i < cnt; i++) {
		printf("%d ", my_cards[i]);
	}
	printf("\n-----DEBUG END-----\n");

	// 플레이어가 단서가 없는 경우
	if (cnt == 0) {
		// 단서 비트를 0으로 세팅
		printf("단서가 없습니다.\n");
		packet->clue = 0x0;
	}
	// 플레이어가 단서가 있는 경우
	else {
		int select_clue;
		printf("\n무엇을 단서로 제출하시겠습니까? : ");
		scanf("%d", &select_clue);
		packet->clue = my_cards[select_clue-1];
	}

	// 플레이어가 자기 카드 한장 또는 0을 서버에 전송 
	//printf("packet_clue: %hd\n", packet->clue);
	int type = PACKET;
	if (packet_send(sock, (char*)packet, &type) == -1){
		return -1;
	}

	// 서버의 단서 정보 라우트를 기다림
	// 결국 받아서 packet에 저장함
	if (packet_recv(sock, (char*)packet, NULL) == -1) {
		return -1;
	}

	return 0;
}

// 단서 정보를 플레이어 화면에 출력하는 함수
int print_clue(Player_packet *packet) {
	
	if (packet == NULL) {
		fprintf(stderr, "send_clue : argument is null\n");
		return -1;
	}

	int player_id = PLAYER_ID(packet->info);
	int turn_player_id = PLAYER_TURN_PLAYER(packet->info) >> 4;
	int clue_player_id = PLAYER_CLUE_PLAYER(packet->clue) >> 5;
	char clue = PLAYER_CLUE(packet->clue);

	// 아무도 단서를 내지 않은 경우
	if (packet->clue == 0) {
		printf("아무도 단서를 내지 않음!!\n");
	}
	// 턴플레이어가 아닌 다른 플레이어가 단서를 제출한 경우
	else if (clue_player_id != turn_player_id) {
		if (player_id == turn_player_id) {
			clue_you_got(&(packet->clue));
		}
		else {
			printf("%d 플레이어가 %d 플레이어에게 단서를 제출함!!\n", clue_player_id, turn_player_id);
		}
	}
	// 턴플레이어가 단서를 제출한 경우
	else {
		printf("%d(턴플레이어)가 자신의 카드 %d을 제출!!\n", turn_player_id, clue);
	}

	return 0;
}

int game_infer(int sock, int player_id) {
	
	int sig;
	int type;

	// 턴플은 SIG_INFR, 나머지는 SIG_WAIT을 기다림
	packet_recv(sock, (char*)&sig, NULL);

	// 플레이어가 SIG_INFR를 받은 경우 : 턴플레이어
	if (sig == SIG_INFR) {
		
		printf("siginfr을 받았습니다. 추리하세요.\n");
		
		// 범인, 흉기(장소는 이미 그 자리로 이동을 했으므로 자동 카운트)를 
		// 정하는 함수를 만든뒤 패킷에 담는다. 그런 다음에 packet_send를 한다.
		Player_packet packet = {0,};

		// To. 경안
		// 경환아. 우리다.
		// 거기는 어떠니. 
		// 이곳은 슬슬 싸늘하다.
		// 너의 시끄러운 입이 그립구나.
		// 혹시 이 글을 읽고 있다면 일단 나가라
		// 경안아. 장소는 블락하고 나머지는 선택할 수 있게 해라
		// 진실의 방이 아닐 경우에는 현장을 고를 때 커서를 고정시키렴

		// 추리 정보 선택
		// 이부분은 윈도우와의 연계가 필요함 일단은 예비로 설정.
		// UI 처리
		unsigned short crime;
		unsigned short weapon;
		unsigned short scene;
		printf("현장을 선택하세요(0-5): ");
		scanf("%hd", &scene);
		printf("범인을 선택하세요(0~5): ");
		scanf("%hd", &crime);
		printf("무기를 선택하세요(0~6): ");
		scanf("%hd", &weapon);
	
		// 추리정보를 패킷에 잘 담으렴
		// 아래 두줄은 변경해야됨(경안이가)
		// unsigned short place =(unsigned short)(position_exchanger(packet.position, packet.info));

		packet.infer = ((scene | 0x0008) << 10) | ((crime | 0x0010) << 5) | (weapon | 0x0018); 
		printf("packet.infer: %hu\n", packet.infer);

		// 자신의 추리 정보를 서버에 전송
		type = PACKET;
		if (packet_send(sock, (char*)&packet, &type) == -1){
			return -1;
		}

		// 서버로부터 라우팅된 추리정보가 담긴 턴플레이어(나)의 패킷을 받음
		if (packet_recv(sock, (char*)&packet, NULL) == -1) {
			return -1;
		}

		// 서버로부터 전송되는 단서 신호를 대기
		// SIG_TURN or SIG_DONE
		if (packet_recv(sock, (char*)&sig, NULL) == -1){
			return -1;
		}

		// SIG_DONE이 전송됨
		// 턴플을 제외한 나머지 플레이어 중 하나가 단서를 제출한 경우
		if (sig == SIG_DONE){
			printf("누군가 단서를 냈습니다. 턴플레이어는 단서를 받으세요\n");

			// 서버로부터 단서 패킷을 받음
			if(packet_recv(sock, (char*)&packet, NULL) == -1){
				return -1;
			}

			// UI 처리(단서 정보를 플레이어 화면에 출력)
			print_clue(&packet);
		}
		// SIG_TURN 이 전송됨
		// 아무도 단서를 내지 않아 턴플이 단서를 내야되는 경우
		else { 
			printf("턴플레이어가 단서를 내야 됩니다\n");
			send_clue(sock, &packet);
			print_clue(&packet);
		}
		return 0;
	}

	// 플레이어가 SIG_WAIT을 받은 경우 : 턴플레이어가 아닌 다른 플레이어
	else{   
		printf("나는 턴플레이어가 아닙니다...\n");
		Player_packet packet = {0,};

		// 서버로부터 라우팅된 추리정보가 담긴 턴플레이어의 패킷을 받음
		if (packet_recv(sock, (char*)&packet, NULL) == -1){
			return -1;
		}

		// 서버로부터 전송되는 단서 신호를 대기
		// SIG_TURN or SIG_DONE
		if (packet_recv(sock, (char*)&sig, NULL) == -1) {
			return -1;
		}

		// 만약 SIG_TURN이 왔다면 내가 단서를 서버에 제출해야함
		// 만약 SIG_DONE이 왔다면 누가 이미 냈으므로 너는 넘어가라는 의미

		// SIG_TURN 을 받음
		if(sig == SIG_TURN){
			send_clue(sock, &packet);
			print_clue(&packet);
			return 0;
		}
		// SIG_DONE 을 받음
		else{ 
			printf("이미 누가 단서를 제출했습니다.\n");

			// 서버로부터 단서 패킷을 받음
			if(packet_recv(sock, (char *)&packet, NULL) == -1){
				return -1;
			}
			print_clue(&packet);

			// To. 경안
			// 경환아. 네가 여기까지 왔다면 너는 이제 시작이다.
			// 우리는 할만큼 했다.
			// 쉽지 않은 길이었다.
			// 이제 너의 이야기다.
			// 자, 펼쳐라. 너의 꿈을...!
		}
	}
	return 0;
}

int clue_you_got(char* clue){
	if(*clue & 0x18){
		switch(*clue & 0x7){
			case 0x0: printf("단서는 문효원의 정치질\n"); break;
			case 0x1: printf("단서는 원창이의 주먹\n"); break;
			case 0x2: printf("단서는 우석이의 혀\n"); break;
			case 0x3: printf("단서는 주성이의 가\n"); break;
			case 0x4: printf("단서는 경안아닌 김경환\n"); break;
			case 0x5: printf("단서는 제현아닌 신재현\n"); break;
			case 0x6: printf("단서는 rihanna의 우산\n"); 
		}
	}else if(*clue & 0x08){
		switch(*clue & 0x7){
			case 0x0: printf("단서는 Kitchen\n"); break;
			case 0x1: printf("단서는 Classroom\n"); break;
			case 0x2: printf("단서는 Restroom\n"); break;
			case 0x3: printf("단서는 Office\n"); break;
			case 0x4: printf("단서는 Horse\n"); break;
			case 0x5: printf("단서는 TrainingRoom\n"); 
		}
	}
	else{
		switch(*clue & 0x7){
			case 0x0: printf("단서는 A\n"); break;
			case 0x1: printf("단서는 B\n"); break;
			case 0x2: printf("단서는 C\n"); break;
			case 0x3: printf("단서는 D\n"); break;
			case 0x4: printf("단서는 E\n"); break;
			case 0x5: printf("단서는 F\n"); 
		}
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

