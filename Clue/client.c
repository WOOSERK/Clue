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

//#define ADDRESS ("192.168.30.31")
#define ADDRESS ("127.0.0.1")
int client_connect(void);
int game_init(int sock, int *player_id, WINDOW ***windows,Player_packet *packet);    
int game_update(int sock, WINDOW ***windows,Cursor *cursor);
int game_play(int sock, int player_id,Player_packet *packet);	 
int roll_and_go(int sock, int player_id,WINDOW ***windows,Player_packet *packet,Cursor *cursor);
int roll_dice(void);
int return_player_choice(int dice_value);
int return_player_position(int choice, int* y, int* x);
int set_dice_in_packet(Player_packet* packet, int dice_value, int choice_value);
int set_player_in_packet(Player_packet* packet, int player_id, int y, int x);
int sig_recv(int sock);
int game_infer(int sock, int player_id,Cursor* cursor,WINDOW ***windows, pthread_t tid);
int send_clue(int sock, Player_packet *packet,Cursor *cursor,WINDOW ***windows);
int print_clue(Player_packet *packet,Cursor *cursor);


int main(){

	int sock = client_connect();
	if (sock == -1) {
		return -1;
	}

	initscr();
	refresh();
	start_color();
	color_init();

	WINDOW ***windows;	

	Player_packet packet={0,};
	
	int player_id;
	if(game_init(sock, &player_id, windows,&packet) == -1) {
		return -1;
	}

	int result = game_play(sock, player_id, &packet);
	if (result == SIG_DIE) {
		printf("당신은 사건을 해결하지 못했다...\n");
	}
	else if (result == SIG_WIN) {
		printf("사건이 해결되었다!!\n");
	}

	endwin();
	return 0;	
}


int client_connect(void){
int sock = socket(PF_INET, SOCK_STREAM, 0); 
	if(sock == -1){
		perror("socket");
		return -1;
	}

	struct sockaddr_in addr = {0,};
	addr.sin_family = AF_INET;
	addr.sin_port = htons(8080);
	addr.sin_addr.s_addr = inet_addr(ADDRESS); // 서버주소
	
	if(connect(sock, (struct sockaddr *)&addr, sizeof(addr)) == -1){
		perror("connect");
		return -1;
	}

	return sock;
}

int game_play(int sock, int player_id, Player_packet *packet) {  	
	
	WINDOW ***windows = display_init(packet->cards,player_id);	
	Cursor cursor;
	cursor.history = windows[6];
	cursor.log = windows[7];
	cursor.history_cnt = 0;
	cursor.log_cnt = 0;
	cursor.command = windows[13][2]; 
	// 이후 클라이언트는 packet_recv로 대기한다. 
	// 서버는 SIG_TURN 또는 SIG_WAIT을 보낼 것이다.
	

	while(1){
		int sig;
		int type;

		int turn_player = (int)PLAYER_TURN_PLAYER(packet->info) >>4;
		blink_player(windows[12],turn_player);

		char buf[32];
		sprintf(buf,"game start");
		str_add(cursor.log_str,(cursor.log_cnt)++,buf);
		history_log_print(cursor.log,cursor.log_str,cursor.log_cnt-1,sizeof(cursor.log_str)/sizeof(cursor.log_str[0]));
		move_command();

		if (packet_recv(sock,(char*)&sig, &type) == -1) {
			return -1;
		}

		pthread_t pid;
		// 턴플레이어의 경우
		if (sig == SIG_TURN){
			roll_and_go(sock, player_id, windows,packet,&cursor);
		}
		else
		{
			pthread_create(&pid,NULL,move_cursor,(void*)&cursor);
		}

		// 턴 플레이어의 roll_and_go 정보를 모든 플레이어가 받아서 업데이트
		if (game_update(sock, windows, &cursor) == -1) {
			return -1;
		}

		int result = game_infer(sock, player_id,&cursor,windows, pid);
		if (result == SIG_WIN) {
			return SIG_WIN;
		}
		else if (result == SIG_DIE) {
			return SIG_DIE;
		}
	
 		// 해당 패킷은 다음 턴이 어떤 플레이어인지 말해주는 용도
		// 이 패킷을 가지고 다음 턴의 플레이어가 누구인지 화면에 출력해야함
		packet_recv(sock, (char*)packet, NULL);
	}
	return 0;
}

int roll_and_go(int sock, int player_id, WINDOW ***windows, Player_packet *packet,Cursor *cursor) {
	
	WINDOW **player ;
	return_player_horse(windows, &player, player_id);

	int x,y;
	int dice_value = 0;
	int choice_value = 0;

	unsigned short position =(unsigned short)(PLAYER_POSITION(packet->position,player_id)) >> (4 * (3 - player_id));

	return_yx(position, &y, &x);

	// 주사위값 얻어냄
	char dice_buf[BUFSIZ];
	sprintf(dice_buf,"press 'y' to roll dice ");
	str_add(cursor->log_str,(cursor->log_cnt)++,dice_buf);
	history_log_print(cursor->log, cursor->log_str, cursor->log_cnt-1, sizeof(cursor->log_str)/sizeof(cursor->log_str[0]));
	move_command();

	dice_cursor(windows[12][5]);
	dice_value = roll_dice();
	dice_num_print(windows[13][1],dice_value);

	if( (y == 3&&x==2) || dice_value != 1){
		char location_buf[BUFSIZ];
		sprintf(location_buf,"press arrow keys and 's' to select location ");
		str_add(cursor->log_str,(cursor->log_cnt)++,location_buf);
		history_log_print(cursor->log, cursor->log_str, cursor->log_cnt-1, sizeof(cursor->log_str)/sizeof(cursor->log_str[0]));
		move_command();
		choice_value = map_cursor(player,windows[12],player_id,dice_value,y,x);
	}
	else{
		choice_value = PLAYER_SELECT_VALUE(packet->dice); 
	}
	// 위치값 얻어냄
	if(return_player_position(choice_value, &y, &x) == -1){
		return -1;	 
	}

	set_dice_in_packet(packet, dice_value, choice_value);  
	set_player_in_packet(packet, player_id, y, x);

	int type = PACKET;

	// 서버에게 roll_and_go 정보를 전송
	packet_send(sock, (char*)packet, &type);

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
		if(choice_value <= dice_value){
			break;	
		}
	}
	return choice_value;
}

int return_player_position(int choice, int* y, int*x){
	switch(choice)
	{
		case 0:
			*y = 0;
			*x = 0;
			break;
		case 1:
			*y = 0;
			*x = 1;
			break;
		case 2:
			*y = 0;
			*x = 2;
			break;
		case 3:
			*y = 1;
			*x = 0; 
			break;
		case 4:
			*y = 1;
			*x = 1;
			break;
		case 5:
			*y = 1;
			*x = 2;
			break;
		case 6:
			*y = 3;
			*x = 3;
			break;
		default:
			break;
	}
	// 이부분은 ui와 연동이 되어야 함.

	return 0;
}

int set_dice_in_packet(Player_packet* packet, int dice_value, int choice_value){
	packet->dice |= (unsigned char)(dice_value << 3);
	packet->dice |= (unsigned char)(choice_value);
	return 0;
}

int set_player_in_packet(Player_packet* packet, int player_id, int y, int x){
	unsigned short position_bit;
	switch(player_id)
	{
		case 0:
			position_bit = 0x0fff;
			break;
		case 1:
			position_bit = 0xf0ff;
			break;
		case 2:
			position_bit = 0xff0f;
			break;
		case 3:
			position_bit = 0xfff0;
			break;
		default:
			break;
	}

	x <<= 2;
	packet->position &= position_bit;
	packet->position |= (unsigned short)(x << ((3 - player_id) * 4));
	packet->position |= (unsigned short)(y << ((3 - player_id) * 4));
	return 0;
}

int game_init(int sock, int *player_id, WINDOW ***windows, Player_packet *packet) {

	packet_recv(sock,(char*)packet, NULL);
	
	scenario();

	*player_id = PLAYER_ID(packet->info);
	
	windows = display_init(packet->cards,*player_id);	 
	return 0;
}

int game_update(int sock, WINDOW ***windows, Cursor *cursor){
	
	// 서버로부터 초기화 패킷을 받아 게임을 초기화
	// 클라이언트는 자신패킷을 가지고 있지 않음 .
	// 따라서 매번 서버에서 패킷을 받아서 파싱해야 함.
	Player_packet packet={0,};
	
	// 패킷을 받는 것을 함수로 해야함.
	// 여기에는 받은 패킷을 파싱하는 함수가 있어야 함.
	int type;
	packet_recv(sock,(char*)&packet, &type);

	// 여기에서부터 초기화 업데이트
	int turn_player = PLAYER_TURN_PLAYER(packet.info) >> 4;
	int room_num = PLAYER_SELECT_VALUE(packet.dice) ;

	if( room_num !=7){
		WINDOW **player;
		return_player_horse(windows,&player,turn_player); // 플레이어 윈도우 반환
		horse_update(player,room_num,turn_player); // 위에서 반환받은 윈도우에 말 표시
	}

	char buf[128];
	int turn_player_dice = (int)PLAYER_SELECT_VALUE(packet.dice);
	sprintf(buf,"player %d went %s room",turn_player+1,parse_room_name(room_num));
	str_add(cursor->log_str,(cursor->log_cnt)++,buf);
	history_log_print(cursor->log, cursor->log_str, cursor->log_cnt-1, sizeof(cursor->log_str)/sizeof(cursor->log_str[0]));
	move_command();
	
	return 0;	
}

// packet변수에 서버로부터 라우팅된 단서 패킷이 세팅됨

int send_clue(int sock, Player_packet *packet,Cursor *cursor,WINDOW ***windows) {     						
	unsigned short scene = PLAYER_INFER_SCENE(packet->infer) >> 10;
	unsigned short crime = PLAYER_INFER_CRIMINAL(packet->infer) >> 5;
	unsigned short weapon = PLAYER_INFER_WEAPON(packet->infer);

	if (packet == NULL) {
		fprintf(stderr, "send_clue : argument is null\n");
		return -1;
	}

	// 자신이 가지고 있는 카드가 추리 정보에 있을 경우, my_cards 배열에 넣음
	char my_cards[3] = {0,};
	int cnt = 0;

	for (int i = 0; i < 4; i++){
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
	int player_id = (int)PLAYER_ID(packet->info);

	// 플레이어가 단서가 없는 경우
	// 단서 비트를 0으로 세팅
	if (cnt == 0) {
		packet->clue = 0x0;
	}
	// 플레이어가 단서가 있는 경우
	else {
		char select_clue;
		int flag=0;
		while(1){
			select_clue = clue_cursor(windows[5],packet->cards);	
			for(int i = 0 ; i < 4 ; i ++){
				if(select_clue == my_cards[i]){
					flag =1;
				}
			}
			if(flag == 1){
				break;
			}
			char buf[128];
			sprintf(buf,"Invalid clue.");
			str_add(cursor->log_str,(cursor->log_cnt)++,buf);
			history_log_print(cursor->log, cursor->log_str, cursor->log_cnt-1, sizeof(cursor->log_str)/sizeof(cursor->log_str[0]));
			move_command();
		}

		packet->clue = select_clue|(player_id<<5);
	}

	// 플레이어가 자기 카드 한장 또는 0을 서버에 전송 
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
int print_clue(Player_packet *packet,Cursor *cursor) {
	
	if (packet == NULL) {
		fprintf(stderr, "send_clue : argument is null\n");
		return -1;
	}
	
	char clue_buf[128];
	int player_id = PLAYER_ID(packet->info);
	int turn_player_id = PLAYER_TURN_PLAYER(packet->info) >> 4;
	int clue_player_id = PLAYER_CLUE_PLAYER(packet->clue) >> 5;
	char clue = PLAYER_CLUE(packet->clue);
	int clue_cate = PARSE_CATE(clue);
	int clue_card = PARSE_CARD(clue);
	char* clue_str = parse_card(clue_cate,clue_card);
	// 아무도 단서를 내지 않은 경우
	if (packet->clue == 0) {
		sprintf(clue_buf,"no one has it"); 
		str_add(cursor->log_str,(cursor->log_cnt)++,clue_buf);
		history_log_print(cursor->log, cursor->log_str, cursor->log_cnt-1, sizeof(cursor->log_str)/sizeof(cursor->log_str[0]));
		move_command();
	
		//printf("아무도 단서를 내지 않음!!\n");
	}
	// 턴플레이어가 아닌 다른 플레이어가 단서를 제출한 경우
	else if (clue_player_id != turn_player_id) {
		if (player_id == turn_player_id) {

		}
		else {
			sprintf(clue_buf,"player%d: send to player%d",clue_player_id+1,turn_player_id+1); 
			str_add(cursor->log_str,(cursor->log_cnt)++,clue_buf);
			history_log_print(cursor->log, cursor->log_str, cursor->log_cnt-1, sizeof(cursor->log_str)/sizeof(cursor->log_str[0]));
			move_command();
			//printf("%d 플레이어가 %d 플레이어에게 단서를 제출함!!\n", clue_player_id, turn_player_id);
		}
	}
	// 턴플레이어가 단서를 제출한 경우
	else {
		sprintf(clue_buf,"turn player(%d) send %s",clue_player_id+1,clue_str); 
		str_add(cursor->log_str,(cursor->log_cnt)++,clue_buf);
		history_log_print(cursor->log, cursor->log_str, cursor->log_cnt-1, sizeof(cursor->log_str)/sizeof(cursor->log_str[0]));
		move_command();
		//printf("%d(턴플레이어)가 자신의 카드 %d을 제출!!\n", turn_player_id, clue);
	}

	return 0;
}

int game_infer(int sock, int player_id, Cursor* cursor, WINDOW ***windows, pthread_t pid) {
	
	int sig;
	int type;

	char *scene_str;
	char *crime_str;
	char *weapon_str;
	// 턴플은 SIG_INFR, 나머지는 SIG_WAIT을 기다림
	packet_recv(sock, (char*)&sig, NULL);


	// 플레이어가 SIG_INFR를 받은 경우 : 턴플레이어
	if (sig == SIG_INFR) {
		
		char infer_buf[128];
		sprintf(infer_buf,"player %d start ",player_id+1);
		str_add(cursor->log_str,(cursor->log_cnt)++,infer_buf);
		history_log_print(cursor->log, cursor->log_str, cursor->log_cnt-1, sizeof(cursor->log_str)/sizeof(cursor->log_str[0]));
		move_command();

		// 범인, 흉기(장소는 이미 그 자리로 이동을 했으므로 자동 카운트)를 
		// 정하는 함수를 만든뒤 패킷에 담는다. 그런 다음에 packet_send를 한다.
		Player_packet packet = {0,};

		// 플레이어가 진실의 방에 있는 지를 확인하기 위해 서버로부터 패킷을 받아옴
		if (packet_recv(sock, (char*)&packet, NULL) == -1) {
			return -1;
		}

		// 경안이랑 같이 할테니 기다리도록....
		unsigned short position_bit = PLAYER_POSITION(packet.position, player_id);
		position_bit >>= (4 * (3 - player_id));

		// 진실의 방(x:3, y:3)이라면

		short scene, crime, weapon;
		while(1){	
			/// 진실의 방이 아닌 경우 인자가 1인 함수 실행 안하고 셀렉트 다이스 값으로 room을 셋팅합니다.
			if(position_bit == 0xF)
			{
				if ((scene = infer_cursor(1)) == -1) { continue; }
			}
			else{
				scene = PLAYER_SELECT_VALUE(packet.dice); 
			}
			if ((crime = infer_cursor(2)) == -1) { continue; }
			if ((weapon = infer_cursor(3)) == -1) { continue; }
			break;
		}

		packet.infer = ((scene |= 0x0008) << 10) | ((crime |= 0x0010) << 5) | (weapon |= 0x0018); 

		// 자신의 추리 정보를 서버에 전송
		type = PACKET;
		

		if (packet_send(sock, (char*)&packet, &type) == -1){
			return -1;
		}

		// 서버로부터 라우팅된 추리정보가 담긴 턴플레이어(나)의 패킷을 받음
		if (packet_recv(sock, (char*)&packet, NULL) == -1) {
			return -1;
		}
		int scene_cate = PARSE_CATE(scene);
		int scene_card = PARSE_CARD(scene);
		int crime_cate = PARSE_CATE(crime);
		int crime_card = PARSE_CARD(crime);
		int weapon_cate = PARSE_CATE(weapon);
		int weapon_card = PARSE_CARD(weapon);
		scene_str = parse_card(scene_cate, scene_card);
		crime_str = parse_card(crime_cate, crime_card);
		weapon_str = parse_card(weapon_cate, weapon_card);

		char buf[128];
		sprintf(buf,"your infer : %s killed him with %s in %s", crime_str, weapon_str, scene_str);
		str_add(cursor->history_str,(cursor->history_cnt)++,buf);
		history_log_print(cursor->history, cursor->history_str, cursor->history_cnt-1, sizeof(cursor->history_str)/sizeof(cursor->history_str[0]));
		move_command();

		// 서버로부터 전송되는 단서 신호를 대기
		// SIG_TURN or SIG_DONE
		if (packet_recv(sock, (char*)&sig, NULL) == -1){
			return -1;
		}

		// SIG_DONE이 전송됨
		// 턴플을 제외한 나머지 플레이어 중 하나가 단서를 제출한 경우
		if (sig == SIG_DONE){

			// 서버로부터 단서 패킷을 받음
			if(packet_recv(sock, (char*)&packet, NULL) == -1){
				return -1;
			}
			int clue_player = (int)PLAYER_CLUE_PLAYER(packet.clue)>>5;// 클루 플레이어 뽑아내기
			char clue_item = PLAYER_CLUE(packet.clue); // 단서 뽑아내기
			int clue_cate = PARSE_CATE(clue_item);
			int clue_card = PARSE_CARD(clue_item);

			char infer_buf[128];
			char* clue_name = parse_card(clue_cate,clue_card);
			sprintf(infer_buf,"player%d give a clue \" %s \" ",clue_player+1,clue_name); 
			str_add(cursor->history_str,(cursor->history_cnt)++,infer_buf);
			history_log_print(cursor->history, cursor->history_str, cursor->history_cnt-1, sizeof(cursor->history_str)/sizeof(cursor->history_str[0]));
			move_command();

			// UI 처리(단서 정보를 플레이어 화면에 출력)
			print_clue(&packet,cursor);
		}
		// SIG_TURN이 전송됨
		// 아무도 단서를 내지 않아 턴플이 단서를 내야되는 경우
		else if (sig == SIG_TURN) { 

			char infer_buf[128];
			sprintf(infer_buf," no one has clue. you have to show clue everyone "); 
			str_add(cursor->log_str,(cursor->log_cnt)++,infer_buf);
			history_log_print(cursor->log, cursor->log_str, cursor->log_cnt-1, sizeof(cursor->log_str)/sizeof(cursor->log_str[0]));

			move_command();
			send_clue(sock, &packet, cursor, windows);

			// UI 처리(단서 정보를 모든 플레이어 로그 화면에 출력)
			print_clue(&packet, cursor);

		}
		// SIG_DIE가 전송됨
		else if (sig == SIG_DIE) {
			char infer_buf[128];
			sprintf(infer_buf," you died .... "); 
			str_add(cursor->log_str,(cursor->log_cnt)++,infer_buf);
			history_log_print(cursor->log, cursor->log_str, cursor->log_cnt-1, sizeof(cursor->log_str)/sizeof(cursor->log_str[0]));
			move_command();

			// 죽은 플레이어의 카드가 공개됨. 안쓰지만 패킷은 받아두자!
			packet_recv(sock, (char*)&packet, NULL);
			return SIG_DIE;
		}
		// SIG_WIN이 전송됨
		else {     
			char buf[128];
			sprintf(buf,"you win!");
			str_add(cursor->log_str,(cursor->log_cnt)++,buf);
			history_log_print(cursor->log, cursor->log_str, cursor->log_cnt-1, sizeof(cursor->log_str)/sizeof(cursor->log_str[0]));
			move_command();
			return SIG_WIN;
		}

	}

	// 플레이어가 SIG_WAIT을 받은 경우 : 턴플레이어가 아닌 다른 플레이어
	else{   

		Player_packet packet = {0,};

		char buf1[128];
		sprintf(buf1,"please wait for inference");
		str_add(cursor->log_str,(cursor->log_cnt)++,buf1);
		history_log_print(cursor->log, cursor->log_str, cursor->log_cnt-1, sizeof(cursor->log_str)/sizeof(cursor->log_str[0]));
		move_command();
		// 서버로부터 라우팅된 추리정보가 담긴 턴플레이어의 패킷을 받음
		if (packet_recv(sock, (char*)&packet, NULL) == -1){
			return -1;
		}
		int scene = PLAYER_INFER_SCENE(packet.infer)>>10;
		int crime = PLAYER_INFER_CRIMINAL(packet.infer)>>5;
		int weapon = PLAYER_INFER_WEAPON(packet.infer);

		int scene_cate = PARSE_CATE(scene);
		int scene_card = PARSE_CARD(scene);
		int crime_cate = PARSE_CATE(crime);
		int crime_card = PARSE_CARD(crime);
		int weapon_cate = PARSE_CATE(weapon);
		int weapon_card = PARSE_CARD(weapon);
		scene_str = parse_card(scene_cate, scene_card);
		crime_str = parse_card(crime_cate, crime_card);
		weapon_str = parse_card(weapon_cate, weapon_card);

		int turn_player1 = PLAYER_TURN_PLAYER(packet.info)>>4;
		char buf3[128];
		sprintf(buf3,"player %d : kill \"%s\" with \"%s\" in \"%s\" ",turn_player1+1,crime_str,weapon_str,scene_str);
		str_add(cursor->history_str,(cursor->history_cnt)++,buf3);
		history_log_print(cursor->history, cursor->history_str, cursor->history_cnt-1, sizeof(cursor->history_str)/sizeof(cursor->history_str[0]));
		move_command();

		// 서버로부터 전송되는 단서 신호를 대기
		// SIG_TURN or SIG_DONE
		if (packet_recv(sock, (char*)&sig, NULL) == -1) {
			return -1;
		}

		pthread_cancel(pid);

		// 만약 SIG_TURN이 왔다면 내가 단서를 서버에 제출해야함
		// 만약 SIG_DONE이 왔다면 누가 이미 냈으므로 너는 넘어가라는 의미
		// 만약 SIG_DIE가 왔다면 턴플레이어가 죽음
		// 만약 SIG_WIN이 왔다면 게임이 끝남

		// SIG_TURN 을 받음
		char buf[BUFSIZ];
		if(sig == SIG_TURN){
			sprintf(buf," hey! send your clue ");
			str_add(cursor->log_str,(cursor->log_cnt)++,buf);
			history_log_print(cursor->log, cursor->log_str, cursor->log_cnt-1, sizeof(cursor->log_str)/sizeof(cursor->log_str[0]));
			move_command();

			send_clue(sock, &packet,cursor, windows);
			print_clue(&packet,cursor);
			return 0;
		}
		// SIG_DONE 을 받음
		else if (sig == SIG_DONE) { 
			// 서버로부터 단서 패킷을 받
			if(packet_recv(sock, (char *)&packet, NULL) == -1){
				return -1;
			}
			char clue_player = PLAYER_CLUE_PLAYER(packet.clue)>>5;
			char turn_player = PLAYER_TURN_PLAYER(packet.info)>>4;
			sprintf(buf,"player%d gave a clue to player%d",clue_player+1,turn_player+1);
			str_add(cursor->log_str,(cursor->log_cnt)++,buf);
			history_log_print(cursor->log, cursor->log_str, cursor->log_cnt-1, sizeof(cursor->log_str)/sizeof(cursor->log_str[0]));
			move_command();
			print_clue(&packet, cursor);
		}
		else if (sig == SIG_DIE) {
			sprintf(buf,"player %d die!\n", PLAYER_TURN_PLAYER(packet.info) >> 4);
			str_add(cursor->log_str,(cursor->log_cnt)++,buf);
			history_log_print(cursor->log, cursor->log_str, cursor->log_cnt-1, sizeof(cursor->log_str)/sizeof(cursor->log_str[0]));
			move_command();

			// 죽은 플레이어의 카드를 공개하기 위해 패킷을 받음
			packet_recv(sock, (char*)&packet, NULL);
			// 모든 플레이어 화면에 출력해야함
			print_clue(&packet, cursor);
			return SIG_DIE;
		}
		else {
			sprintf(buf,"player %d win!\n", PLAYER_TURN_PLAYER(packet.info) >> 4);
			str_add(cursor->log_str,(cursor->log_cnt)++,buf);
			history_log_print(cursor->log, cursor->log_str, cursor->log_cnt-1, sizeof(cursor->log_str)/sizeof(cursor->log_str[0]));
			move_command();
			return SIG_WIN;
		}
	}
	return 0;
}
