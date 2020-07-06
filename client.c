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
//추가한 함수는 아직 안썼음

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
		int type_ = SIGNAL;
		packet_recv(sock,(char*)&sig,&type_);
		if(sig == SIG_TURN){
			printf("sigturn을 받았습니다.\n");
			roll_and_go(sock);
		}
		// 그러면 이단계에서 update를 한번 더 해준다.
		if(game_update(sock) == -1){
			return -1;
		}
		// 만약 문자열을 받을 거면 따로 해줘야함.
		// game_update에 해줘도 됨. 이건 말해서 같이 정해햐 할 듯.
		// 이제가 본격적인 infer
		int return_value = game_infer(sock);
		if(return_vlaue == -1){
			return -1;
		}
		else if(return_value == 99){
			//ROOM_OF_TRUTH에서 답을 맞힌 경우.
			// 아니면 어차피 ROOM_OF_TRUTH는 roll_and_go에서 정해지므로
			// 따로 함수를 둬도 될것 같다.
			break;
		}
		// 해당턴이 끝나고 
	}
	return 0;
}

int game_infer(int sock){
	//먼저 sig_recv를 받는다. 
	int sig;
	int type_ = SIGNAL;
	packet_recv(sock, (char*)&sig, &type_);
	if(sig == SIG_INFR){
		printf("siginfr을 받았습니다. 추리하세요.\n");
		// 범인, 흉기(장소는 이미 그 자리로 이동을 했으므로 자동 카운트)를 
		// 정하는 함수를 만든뒤 패킷에 담는다. 그런다음에 packet_send를 한다.
		Player_packet packet;
		// 이부분은 윈도우와의 연계가 필요함 일단은 예비로 설정.
		type_ = PACKET;
		unsigned short crime;
		unsigned short weapon;
		printf("범인을 선택하세요(0~5): ");
		scanf("%d\n", &crime);
		printf("무기를 선택하세요(0~6): ");
		scanf("%d\n", &weapon);
		unsigned short place =(unsigned short)(position_exchanger(packet.position, packet.info));
		packet.infer = (place << 10) | ((weapon << 5) | 0x0100) | (crime | 0x0018); 
		// 계산 확인
		if(packet_send(sock, (char*)&packet, &type_) == -1){
			return -1;
		}
		// 추정한 단서를 보낸후 단서를 기다리는 중
		if(packet_recv(sock, (char*)&sig, &type_) == -1){
			return -1;
		}
		if( sig == SIG_WAIT){
			if(packet_recv(sock, (char*)&packet, &type_) == -1){
				return -1;
			}
			//이 부분도 클루를 보여주는 ui와 연동
			printf("단서는 %d 입니다.\n", packet.clue);
			// 여기는 packet.clue와 단서와의 매핑을 해야함.
			clue_you_got(&(packet.clue));			
			return 0;
		}
		else{ // 다 돌았는데 아무도 해당 단서가 없었을 때
			printf("아무도 없네요... 니가 내세요.\n");
			type_ = PACKET;
			char my_cards[3]= {0,};
			for(int i =0; i< 4; i++){
				if(crime  == packet.cards[i]){
					my_cards[i] = packet.cards[i];		
				}else if((weapon >> 5) == packet.cards[i]){
					my_cards[i] = packet.cards[i];
				}
				else if(position_exchanger(packet.position, packet.info) == packet.cards[i]){
					//현재 나의 아이디를 뽑아와서 나의 포지션을 추출해낸후, 
					// 그 포지션(즉 내가 있는 장소)와 내가 갖고있는 카드와 비교한다.
					// 있다면 my_cards에 포함시킨다.
					my_cards[i] = packet.cards[i]; 
				}
				printf("%d: %d \t",i+1, (int)my_cards[i]);
			}
			
			for(int i =0 ; i< 4; i++){
				if(my_cards[i] != 0)
					break;
				if( i == 3){ //나도 단서가 없는 경우
					printf("나 또한 단서가 없습니다.\n");
					type_ = PACKET;
					packet.clue = 0x00;
					if(packet_send(sock, (char*)&packet, &type_) == -1){
						return -1;
					}
					return 0;
				}
			}
			
			// 나한테 단서가 있는 경우
			while(1){
				int select_clue;
				printf("\n무엇을 단서로 제출하시겠습니까? : ");
				scanf("%d", &select_clue);
				if(select_clue <1 || select_clue > 4 )
					printf("다시 입력하세요 없는 번호입니다.\n");
				else{
					if(my_cards[select_clue-1] == 0)
						printf("없는 카드입니다 다시 입력하세요.\n");
					else{
						type_= PACKET;
						packet.clue = my_cards[select_clue-1];
						if(packet_send(sock, (char*)&packet, &type_) == -1){
							return -1;
						}
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
		type_ = PACKET;
		if(packet_recv(sock, (char*)&packet, &type_) == -1){
				return -1;
		}
		type_ = SIGNAL;
		packet_recv(sock, (char*)&sig, &type_);

		if(sig == SIG_TURN){
			unsigned short crime = PLAYER_INFER_CRIMINAL(packet.infer);
			unsigned short weapon = PLAYER_INFER_WEAPON(packet.infer);
			unsigned short place = PLAYER_INFER_SCENE(packet.infer);
			// 서버로부터 받은 패킷으로부터 턴클이 추리한 내용을 저장한다.
			char my_cards[3]= {0,};
			for(int i =0; i< 4; i++){
				if((char)crime  == packet.cards[i]){
					my_cards[i] = packet.cards[i];		
				}else if((char)(weapon >> 5) == packet.cards[i]){
					my_cards[i] = packet.cards[i];
			return 0;	}
				else if(position_exchanger(packet.position, packet.info) == packet.cards[i]){
					my_cards[i] = packet.cards[i]; 
				}
			}
			for(int i =0 ; i< 4; i++){
				if(my_cards[i] != 0)
					break;
				if( i == 3){ //나도 단서가 없는 경우
					printf("나 또한 단서가 없습니다.\n");
					type_ = PACKET;
					packet.clue = 0x00;
					if(packet_send(sock, (char*)&packet, &type_) == -1){
						return -1;
					}
					type_ = SIGNAL;
					if(packet_recv(sock, (char*)&sig, &type_) = -10){
						return -1;
					}
					if( sig == SIG_TURN_PLAYER){
						if(packet_recv(sock, (char*)&packet, &type_) == -1){
							return -1;
						} // 이것은 턴플레이어턴으로 가서 
						printf("턴클로부터 공개적으로 모두에게 뿌린 단서는 ... 입니다.\n");
					}
					else{
						if(packet_recv(sock, (char*)&packet, &type_) == -1){
							return -1;
						}
						//누가 보냈는지는 packet.clue에다가 써서 넣어놨고, 
						// 클루의 모양은 왼쪽부터 5비트가 단서의 내용이고
						//그 다음의 2비트가 누가 냈는지 식별하기 위함이다.

					}
					return 0;
				}
			}

			while(1){
				int select_clue;
				printf("\n무엇을 단서로 제출하시겠습니까? : ");
				scanf("%d", &select_clue);
				if(select_clue <1 || select_clue > 4)
					printf("다시 입력하세요 없는 번호입니다.\n");
				else{
					if(my_cards[select_clue-1] == 0)
						printf("없는 카드입니다 다시 입력하세요.\n");
					else{
						type_= PACKET;
						packet.clue = my_cards[select_clue-1];
						if(packet_send(sock, (char*)&packet, &type_) == -1){
							return -1;
						}
						break;
					}
				}
			}
			return 0;
		}
		else{ // 이미 단서는 넘겨졌으니 그냥 넘어가라.
			printf("이미 누가 단서를 제출했습니다. 넘어갑니다.\n");
			sleep(2);
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
	srand((unsigned int)time(NULL));
	return (rand()%6+1);
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
	int struct_ = PACKET ;
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

