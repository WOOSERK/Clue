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
char position_exchanger(short position, char info);
int clue_you_got(char* clue);


int main(){
	
	int sock = client_connect();

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
	addr.sin_addr.s_addr = inet_addr("127.0.0.1"); // 서버주소
	
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
		if(sig == SIG_TURN){
			printf("내 턴입니다!!\n");
			roll_and_go(sock, player_id);
		}

		// 턴 플레이어의 정보를 모든 플레이어가 받아서 업데이트
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
	packet_recv(sock,(char*)&packet, NULL);
	// ui_update(player); // 여기에선 리턴값을 어떻게 설정할지 몰라서 if처리 안했음

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
	// ui_update(player); // 여기에선 리턴값을 어떻게 설정할지 몰라서 if처리 안했음

	printf("-----아래 정보로 화면 업데이트 진행-----\n");
	for (int i = 0; i < PLAYER_CNT; i++) {
		printf("%d's POSITION: %d\n", i, PLAYER_POSITION(packet.position, i));
	}
	printf("DICE_VALUE: %d\n", PLAYER_DICE_VALUE(packet.dice));
	printf("SELECT_VALUE: %d\n\n", PLAYER_SELECT_VALUE(packet.dice));

	return 0;	
}

int game_infer(int sock, int player_id) {
	
	int sig;
	int type_;
	packet_recv(sock, (char*)&sig, NULL);

	// 턴플레이어가 추리신호를 받은 경우
	if(sig == SIG_INFR){
		
		printf("siginfr을 받았습니다. 추리하세요.\n");
		
		// 범인, 흉기(장소는 이미 그 자리로 이동을 했으므로 자동 카운트)를 
		// 정하는 함수를 만든뒤 패킷에 담는다. 그런다음에 packet_send를 한다.
		Player_packet packet;
	
		// 추리 정보 선택
		// 이부분은 윈도우와의 연계가 필요함 일단은 예비로 설정.
		type_ = PACKET;
		unsigned short crime;
		unsigned short weapon;
		printf("범인을 선택하세요(0~5): ");
		scanf("%hd", &crime);
		printf("무기를 선택하세요(0~6): ");
		scanf("%hd", &weapon);
		unsigned short place =(unsigned short)(position_exchanger(packet.position, packet.info));
		packet.infer = (place << 10) | ((crime | 0x0010) << 5) | (weapon | 0x0018); 

		// 자신의 추리 정보 서버에 전송
		if (packet_send(sock, (char*)&packet, &type_) == -1){
			return -1;
		}

		// 서버로부터 전송되는 단서 신호를 대기
		// 서버는 단서를 보내기 전 반드시 신호를 먼저 보내야함
		if (packet_recv(sock, (char*)&sig, NULL) == -1){
			return -1;
		}

		// 턴플을 제외한 나머지 플레이어 중 하나가 단서를 제출한 경우
		if (sig == SIG_WAIT){

			// 서버로부터 단서를 받음
			if(packet_recv(sock, (char*)&packet, &type_) == -1){
				return -1;
			}

			//이 부분도 클루를 보여주는 ui와 연동
			printf("단서는 %d 입니다.\n", packet.clue);

			// 여기는 packet.clue와 단서와의 매핑을 해야함.
			clue_you_got(&(packet.clue));			
			return 0;
		}
		
		// 아무도 단서를 내지 않아서 턴플이 내야되는 경우
		else{ 
			printf("아무도 없네요... 니가 내세요.\n");
			type_ = PACKET;
			char my_cards[3]= {0,};
			for(int i =0; i< 4; i++){
				if(crime  == packet.cards[i]){
					my_cards[i] = packet.cards[i];		
				}
				else if((weapon >> 5) == packet.cards[i]){
					my_cards[i] = packet.cards[i];
				}
				else if(position_exchanger(packet.position, packet.info) == packet.cards[i]){
					//현재 나의 아이디를 뽑아와서 나의 포지션을 추출해낸후, 
					// 그 포지션(즉 내가 있는 장소)와 내가 갖고있는 카드와 비교한다.
					// 있다면 my_cards에 포함시킨다.
					my_cards[i] = packet.cards[i]; 
				}
				// 자기 카드 중에서 단서 후보카드만 보여줌
				printf("%d: %d \t",i+1, (int)my_cards[i]);
			}
		
			// 단서가 없으면 해당 for문에서 game_infer함수 종료
			// 단서가 있으면 해당 for문을 탈출함
			for(int i = 0; i < 4; i++){
				if(my_cards[i] != 0)
					break;

				// 나(턴플)도 단서가 없는 경우
				if(i == 3){ 
					printf("나 또한 단서가 없습니다.\n");
					type_ = PACKET;
					packet.clue = 0x00;

					// 단서 비트를 0으로 세팅해서 서버에 전송
					if(packet_send(sock, (char*)&packet, &type_) == -1){
						return -1;
					}
					return 0;
				}
			}
			
			// 나(턴클)한테 단서가 있는 경우
			while(1){
				int select_clue;
				printf("\n무엇을 단서로 제출하시겠습니까? : ");
				scanf("%d", &select_clue);
				if(select_clue <1 || select_clue > 4 ) {
					printf("다시 입력하세요 없는 번호입니다.\n");
				}
				else {
					if(my_cards[select_clue-1] == 0)
						printf("없는 카드입니다 다시 입력하세요.\n");
					else{
						type_= PACKET;
						packet.clue = my_cards[select_clue-1];
						
						// 아무도 단서를 안냈으므로 턴클이 자기 카드 한장을 서버에 전송 
						if(packet_send(sock, (char*)&packet, &type_) == -1){
							return -1;
						}
						// 서버의 단서 정보 라우트를 기다림
						packet_recv(sock, (char*)&packet, NULL);

						// 서버에게 받은 정보 업데이트 
						// ui_update()
						break;
					}
				}
			}
			return 0;
		}
	}
	else{  //턴이 아닌 애들 
		printf("해당 턴플레이어가 아닙니다...\n");
		Player_packet packet;

		// 서버에서 전송되는 추리패킷을 기다림 
		if(packet_recv(sock, (char*)&packet, NULL) == -1){
			return -1;
		}

		// 단서 제출 턴 시그널을 대기함(SIG_TURN or SIG_WAIT)
		packet_recv(sock, (char*)&sig, NULL);


		// 내가 단서를 제출할 턴임
		// 만약 SIG_WAIT이 왔다면 누가 이미 냈으므로 너는 넘어가라는 의미
		if(sig == SIG_TURN){
			unsigned short crime = PLAYER_INFER_CRIMINAL(packet.infer);
			unsigned short weapon = PLAYER_INFER_WEAPON(packet.infer);
			unsigned short place = PLAYER_INFER_SCENE(packet.infer);

			// 서버로부터 받은 패킷으로부터 턴클이 추리한 내용을 저장한다.
			char my_cards[3]= {0,};
			for(int i =0; i< 4; i++){
				if((char)crime  == packet.cards[i]){
					my_cards[i] = packet.cards[i];		
				}
				else if((char)(weapon >> 5) == packet.cards[i]){
					my_cards[i] = packet.cards[i];
					return 0;	
				}
				else if(position_exchanger(packet.position, packet.info) == packet.cards[i]){
					my_cards[i] = packet.cards[i]; 
				}
			}

			// 단서가 없으면 해당 for문에서 game_infer함수 종료
			// 단서가 있으면 해당 for문을 탈출함
			for(int i = 0; i < 4; i++){

				// 단서가 있으면 for문 바로 탈출
				if(my_cards[i] != 0)
					break;

				//나는 단서가 없는 경우
				if(i == 3){ 
					printf("나는 단서가 없습니다.\n");
					type_ = PACKET;
					packet.clue = 0x00;

					// 단서가 없어도 단서 정보가 담긴 패킷을 전송은 해야됨
					if(packet_send(sock, (char*)&packet, &type_) == -1){
						return -1;
					}
					// 단서를 턴플이 냈는지 
					// 다른 플레이어가 턴플에게 전달했는지를 
					// 구분하는 시그널을 받음
					if(packet_recv(sock, (char*)&sig, NULL) == -1){
						return -1;
					}
					// SIG_TURN_PLYAER가 옴
					// 단서를 턴플이 낸 경우
					if(sig == SIG_TURN_PLAYER){

						// 턴플이 제출한 단서를 받는것을 대기
						if(packet_recv(sock, (char*)&packet, &type_) == -1){
							return -1;
						} 

						// 해당 패킷 파싱 여기서 해야됨

						printf("턴클로부터 공개적으로 모두에게 뿌린 단서는 ... 입니다.\n");
					}
					// SIG_WAIT이 옴
					// 단서를 다른 플레이어가 턴플에게 전송한 경우
					else{
						if(packet_recv(sock, (char*)&packet, &type_) == -1){
							return -1;
						}
						// 누가 보냈는지는 packet.clue에다가 써서 넣어놨고, 
						// 클루의 모양은 왼쪽부터 5비트가 단서의 내용이고
						// 그 다음의 2비트가 누가 냈는지 식별하기 위함이다.

						// 턴플레이어가 누군지는 packet.info에다가 써서 넣어놨고,
						// 왼쪽부터 4비트가 턴플레이어의 번호임
						printf("*플레이어가 턴플레이어(*번)에게 단서를 보여주었습니다\n");
					}
					return 0;
				}
			}

			// 단서가 있음
			// 자신의 단서를 제출
			while(1){
				int select_clue;
				printf("\n무엇을 단서로 제출하시겠습니까? : ");
				scanf("%d", &select_clue);
				if(select_clue <1 || select_clue > 4) {
					printf("다시 입력하세요 없는 번호입니다.\n");
				}
				else {
					if(my_cards[select_clue-1] == 0) {
						printf("없는 카드입니다 다시 입력하세요.\n");
					}
					else {
						type_= PACKET;

						// shift 해야됨
						packet.clue = my_cards[select_clue-1];

						// 서버에게 단서가 들어있는 패킷 전송
						// 나의 식별번호를 clue의 우측에서 4비트에 담아서 서버에 전송
						if(packet_send(sock, (char*)&packet, &type_) == -1){
							return -1;
						}

						// 시그널을 먼저 받아야하나?
						// 뭔가 받음
						packet_recv();

						// 문자열 출력
						printf("*번 플레이어가 턴플레이어(*번)에게 단서를 보여주었습니다\n");
						break;
					}
				}
			}
			return 0;
		}
		// 이미 단서는 넘겨졌으니 그냥 넘어가라.
		else{ 
			printf("이미 누가 단서를 제출했습니다. 넘어갑니다.\n");
			// sig널 받고 패킷을 받아서 누가 턴플에게 단서를 줬는지 파싱해야함. 
			type_ = SIGNAL;
			if(packet_recv(sock, &sig, &type_) == -1){
				return -1;
			}
			if(sig == SIG_WAIT){
				// 이 부분은 다시 생각해봐야 함.(왜 sigturn만 오는데 꼭 받아야 하는지.)
				type_ = PACKET;
				if(packet_recv(sock, &packet, &type_) == -1){
					return -1;
				}
				int player_who_gave_clue = packet.clue & 0x3;
				int player_who_get_clue = PLAYER_TURNPLAYER(packet.info);
				//그리고 턴플레이어도 빼와서 히스토리나 출력에
				// 누가 턴플레이어게 줫는지도 알려줘야 한다. 이부분은 내일 같이 상의.
			}		
		}
	}
}

char position_exchanger(short position, char info){
	char place_card;
	char i;
	for(i =3; i > ((char)3- PLAYER_ID(info)); i--) ;

	switch(PLAYER_POSITION(position, PLAYER_ID(info)) >> (i*4)){
		case 0x0: place_card = 0x8; break;
		case 0x1: place_card = 0x9; break;
		case 0x2: place_card = 0xa; break;
		case 0x4: place_card = 0xb; break;
		case 0x5: place_card = 0xc; break;
		case 0x6: place_card = 0xd; 
	}
	return place_card;
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

