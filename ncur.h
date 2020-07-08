#ifndef ncur_h
#define ncur_h
#include <ncurses.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#define NUM_ABS(a,b) ((a-b) < 0 ? ((-1)*(a-b)) : (a-b))
#define CAL_RANGE(cury,curx,y,x) (NUM_ABS(cury,y)+NUM_ABS(curx,x))
#define PARSE_CATE(card) (((card>>3)&0x3))
#define PARSE_CARD(card) (card&0x7)
#define MAPS_NUM (11)
#define LAYOUT_NUM (6)
#define INFER_RESULT (3)
#define SUSPECT_NUM (6)
#define PLACE_NUM (6)
#define WEAPON_NUM (7)
#define CHECK_NUM (76)
#define PLAYER_NUM (4)
#define HISTORY_MAX (7)
#define STR_ARR_MAX (1024)
#define COMMAND_X (70)
#define COMMAND_Y (1)

typedef struct Cursor{
	WINDOW **history; // 커서 제어를 위한 history 윈도우
	WINDOW **log; // 커서 제어를 위한 log 윈도우 
	char* history_str[STR_ARR_MAX]; // history 로그를 담기 위한 배열
	char* log_str[STR_ARR_MAX]; // log 로그를 담기위한 배열
	int history_cnt; // history 로그 개수를 카운트
	int log_cnt; // log 개수를 카운트
	WINDOW *command; // 사용자의 커맨드를 출력하는 함수
}Cursor;

/*
	windows[0](layout) : map,memo,clue,history,log,infer;
	windows[1](map) : outer,background,kitchen,hall,classroom,stair,looftop,toilet,office,livingroom,training
	windows[2](room_infer) : "Kitchen","ClassRoom","RestRoom","Training","Horse","Office"
	windows[3](suspect_infer) : "KH_Kim","WS_Kim","JS_Kim","WC_Park","JH_Shin","KA_Jeon"
	windows[4](weapon_infer) : "Knife","Umbrella","Punch","MacBook","Chair","Cable","ZUGA"
	windows[5](clue) : 1번단서, 2번단서, 3번단서, 4번단서, 선택한단서 보여주는 디스플레이,선택버튼
	windows[6](history):
	windows[7](log):
	windows[8](player1) : player1-> "Kitchen","ClassRoom","RestRoom","Training","Horse","Office","looftop
	windows[9](player2) : player2
	windows[10](player3) : player3
	windows[11](player4) : player4
	windows[12] : playerList -> player1,2,3,4 , select_room_display , dice_button , send_position
	windows[13](buttons) : 단서제출 버튼 , 주사위 숫자 보여주는 디스플레이, 커맨드 창
*/
/* 윈도우를 만드는 함수 height : 윈도우 세로길이 ,width : 윈도우 가로길이 , y : 시작 y좌표 , x : 시작 x 좌표, color: 윈도우 배경, 글자 색*/
static WINDOW* display_background(int height,int width,int y,int x,int color);
/* 해당 터미널의 최고x,y 값과 x,y의 60% 값 가져오기 maxx: 최고x 값, maxy : 최고 y값,height : maxy의 60%값 , width:maxx의 60%값*/
static void getmaxhw(int *maxy,int *maxx,int *height1, int *width1);
/* 윈도우의 타이틀 출력하기 arg : 추가할 윈도우 , args : 타이틀명*/
static void display_title(WINDOW* arg,char *args);
/* layout을 잡아주는 함수 반환값: 만들어진 윈도우의 배열들 (레이아웃 배열) , maxy : 최고y 값 , maxx : 최고 x값, height1 : maxy의 60%의 값 , wihdth : maxx의 60%값*/
WINDOW** make_layout(int maxy,int maxx,int height1,int width1);
/* 해당 방에 색을 결정하고 문과 방이름을 출력하는 함수, window : 출력할 방 , color : 방의 색, y,x : 문의 위치 y,x 좌표 , str : 문 형태, roomName : 방 이름*/
static void map_refresh(WINDOW* window,int color, int y, int x, char* str,char*roomName);
/* 맵을 만드는 함수 ( 방을 각각 만들고 그림을 그린다.) height,width : 맵의 시작위치*/
WINDOW** make_map(int height1,int width1);
/* 커서 제어시 x 좌표의 경계를 제어하는 함수*/
static void width_boundary(int *curx,int max_x,int min_x );
/* 커서 제어시 y 좌표의 경계를 제어하는 함수*/
static void height_boundary(int *cury,int max_y,int min_y);
/* 메모 위에서 커서가 이동하는 함수*/
void memo_cursor();
/* 추리 ui를 만드는 함수 , infer_name : 추리창에 표시 될 단서 이름, window_num : 추리창에 그릴 단서 개수 , startY : 시작  y 값, startX : 시작 x 값, height: 한 칸의 높이 , width : 한 칸의 넓이*/
WINDOW** make_infer_clue(char** infer_name,int window_num,int startY, int startX, int height, int width);
/* 메모에 단서목록을 출력하는 함수 window : 메모장 window , size_t : 전달될 아이템 배열의 크기, item : 출력할 단서들의 배열 , starty : 시작 y 값*/
static void display_memo_item(WINDOW *window,size_t size,char** item,int startY);
/* 버튼 만들기 height : 높이 , width : 넓이 , startY,startX : 시작x,y값, color : 윈도우 색상*/
WINDOW* make_button(int height,int width,int startY, int startX, int color, char*arg);
/* 메모를 만드는함수 : memo : 메모가 그려질 윈도우 , height: 시작 y값, width : 시작 x 값*/
WINDOW** make_memo(WINDOW* memo,int height,int width);
/* 추리칸에서 커서 움직이는 함수 row : 장소일경우 0 , 사람일경우 1, 흉기일경우 2 를 보낸다. */
int infer_cursor(int row);
/* 맵에 플레이어의 위치를 그린다. hegith : 높이 , width : 넓이 , startY,startX :시작좌표값 return : 플레이어가 선택한 단서값*/ 
WINDOW** display_player(int height, int width, int startY,int startX);
/* 현재 차례 플레이어 깜박임*/
void blink_player(WINDOW** player,int num);
/* 맵에 말을 생성 및 초기설정합니다. player_num : 현재 순번인 플레이어 번호*/
WINDOW** make_horse(int player_num);
/* 맵의 현재 상태를 업데이트합니다. player: 플레이어의 위치에 해당하는 윈도우 배열, room_num : 플레이어가 위치 할 방번호, player_num : 현재 순번의 플레이어*/
void horse_update(WINDOW **player, int room_num, int player_num);
/* 맵에서의 커서 이동, room : 플레이어 위치 배열 , player : 현재 순번의 플레이어(0-3) return : 플레이어가 선택한 방의 값*/
int map_cursor(WINDOW **player_room,WINDOW** playerList,int player,int range,int cury,int curx);
/* 맵 이동시 커서를 제어하는 함수 choice_room : 턴클이 선택한 방번호, range : 이동 가능한 거리(2-4), cury,curx : 현재 플레이어 말의 위치[-1,-1의경우 모든방 이동 가능]) return ->> 1 : 이등가능 -1 : 이동불가능*/
int move_limit(int choice_room,int range,int cury,int curx);
/* 스레드에서 계속 돌릴 커서 제어 함수*/
void* move_cursor(void* arg);
/* 카테고리와 카드번호를 주면 보유 카드의 string으로 이름을 반환한다. */
char* parse_card(int category,int card_num);
/* 보유 단서를 출력하는 함수 category : 어떤 카드 종류인지 보냄(1-3) , card_num : 카드에 해당하는 숫자를 보냄 , num : 단서의 순번을 보낸다.*/
WINDOW* make_clue(int category,int card_num,int num);
/* 현재 커서가 가리키는 카드를 display에 출력한다 display_card : 카드를 출력할 디스플레이, card : 카드의 정보,*/
void display_select_card(WINDOW* display_card, char card);
/* clue 영역에서 커서를 움직인다. window : clue 영억의 윈도우, cards: 클라이언트가 가진 카드 배열);*/
char clue_cursor(WINDOW** window,char* cards);
/* color_pair 정의 */
void color_init();
/*맵 커서 이동시 커서의 위치에 해당하는 방이름을 디스플레이에 출력, display_map : 디스플레이 , cnt : 출력할 방 번호*/
void display_select_map(WINDOW *display_map,int cnt);
// 방번호를 넣으면 yx를 인자를 통해 반환한다. room_num : 변화된 방번호, y,x : 좌표를 받을 인자 return : 0 : 정상종료 , -1 : 오류*/
int calc_yx(int room_num,int *cury, int *curx);
/* 주사위 돌리기 버튼 위로 커서를 이동시킨다. dice_button : 주사위 굴리기 버튼 */
void dice_cursor(WINDOW* dice_button);
/* history 영역 윈도우 만들기 */
WINDOW** make_history();
/* log 영역 윈도우 만들기 */
WINDOW** make_log();
/* history or log 영역 출력하기 ( 새로고침) 함수 window : history or log  윈도우, str: 출력할 문자열 배열 , start : 시작 인덱스, str_size 배열의 총 크기*/
void history_log_print(WINDOW **window, char**str, int start,int str_size);
/* history or log 영역에서 커서를 움직이는 함수 , window  : 커서가 움직이는 영역, str: 출력 할 문구 , start : 출력 시작 인덱스 , str_size : str의 크기*/
void history_log_cursor(WINDOW **window,char **str, int str_size);
/*로그 또는 히스토리에 내역(?) 을 추가, str_arry : 로그 또는 히스토리를 담은 배열 , add_idx : 추가할 방 번호, str :추가할 문구*/
int str_add(char **str_arry, int add_idx,char* str);
/*사용자에게 커맨드를 입력받고 보여주는 함수 , command : 커맨드를 출력 할 함수*/
int input_command(WINDOW *command);
/* 커서를 커맨드 입력 창으로 움직인다.*/
void move_command();
/* 주사위 값을 출력한다.  window : 주사위 숫자를 출력 할 함수, num : 나온 숫자*/ 
void dice_num_print(WINDOW *window, int num);
// 디스플레이 초기화
WINDOW*** display_init(char *card); 
// 카드 배열을 보내면 파싱을 해서 숫자로 뽑아낸다.
int* parse_card_num(char* cards);
// position 패킷을 보내면 yx에 값을 세팅해서 준다.
void return_yx(unsigned short position,int *y, int *x);
// player_id 에 해당하는 말 윈도우 반환
void return_player_horse(WINDOW ***window,WINDOW **player,int player_id);
/*방 번호를 전달하면 방의 이름을 반환하는 함수*/
char* parse_room_name(int room_num);
#endif
