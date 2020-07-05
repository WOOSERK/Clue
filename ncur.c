#include <ncurses.h>
#include <stdlib.h>
#define NUM_ABS(a,b) ((a-b) < 0 ? ((-1)*(a-b)) : (a-b))
#define CAL_RANGE(cury,curx,y,x) (NUM_ABS(cury,y)+NUM_ABS(curx,x))
#define MAPS_NUM (11)
#define LAYOUT_NUM (6)
#define INFER_RESULT (3)
#define SUSPECT_NUM (6)
#define PLACE_NUM (6)
#define WEAPON_NUM (7)
#define CHECK_NUM (76)
#define PLAYER_NUM (4)

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
int map_cursor(WINDOW **room,int player,int range,int cury,int curx);
/* 맵 이동시 커서를 제어하는 함수 choice_room : 턴클이 선택한 방번호, range : 이동 가능한 거리(2-4), cury,curx : 현재 플레이어 말의 위치) return ->> 1 : 이등가능 -1 : 이동불가능*/
int move_limit(int choice_room,int range,int cury,int curx);

void run(){
	initscr();
	refresh();
	start_color();
	init_pair(1, COLOR_BLACK, COLOR_WHITE);
	init_pair(2, COLOR_RED, COLOR_WHITE);
	init_pair(3, COLOR_YELLOW, COLOR_WHITE);
	init_pair(4, COLOR_GREEN, COLOR_WHITE);
	init_pair(5, COLOR_BLUE, COLOR_WHITE);
	init_pair(6, COLOR_MAGENTA, COLOR_WHITE);
	init_pair(7, COLOR_CYAN, COLOR_WHITE);
	init_pair(8, COLOR_WHITE, COLOR_BLACK);
	init_pair(9, COLOR_BLACK,COLOR_YELLOW);
	init_pair(10, COLOR_BLACK,COLOR_CYAN);
	init_pair(11, COLOR_BLACK,COLOR_MAGENTA);
	init_pair(12,COLOR_BLACK,COLOR_GREEN);
	char *suspect_str[] ={"KH_Kim","WS_Kim","JS_Kim","WC_Park","JH_Shin","KA_Jeon"};
   char *room_str[] = {"Kitchen","ClassRoom","RestRoom","Training","Horse","Office"};
	char *weapon_str[] = {"Knife","Umbrella","Punch","MacBook","Chair","Cable","ZUGA"};

	int maxx,maxy,width1,height1;
	getmaxhw(&maxy,&maxx,&height1,&width1);

	WINDOW **windows[10]; //layout,maps,places,suspect,weapon
	WINDOW **playerList; // 현재 순서인 플레이어를 표기하기위한 디스플레이
	WINDOW **player1,**player2,**player3,**player4;
	windows[0] = make_layout(maxy,maxx,height1,width1); // 레이아웃 잡기
	windows[1] = make_map(height1,width1); // 맵 그리기
	playerList = display_player(1,13,5,115);
	windows[2] = make_infer_clue(room_str,6,(height1*0.65)+1,3,4,14); // 장소추리칸 그리기
	windows[3] = make_infer_clue(suspect_str,6,(height1*0.65)+5,3,4,14); // 범인 추리칸 그리기
	windows[4] = make_infer_clue(weapon_str,7,(height1*0.65)+9,3,4,14); // 흉기 추리칸 그리기
	WINDOW *button = make_button(3,20,(height1*0.65)+5,(6*14)+25,2,"Send infer(y/n)"); // 버튼만들기
	make_memo(windows[0][1],height1,width1);	
	
	

/*	 blink test
	for( int i = 0 ; i < 4 ; i++){
		blink_player(playerList,i);
		map_cursor(
		getchar();
	}
*/	

	/*
		infer 영역 커서 제어 테스트 코드
	while(1){
		if((room = infer_cursor(0)) == -1 ){continue;}
		if((suspect = infer_cursor(1)) == -1 ){continue;}
		if((weapon = infer_cursor(2)) == -1 ){continue;}
		break;
	}
	*/ 
	player1=make_horse(0);
	player2=make_horse(1);
	player3=make_horse(2);
	player4=make_horse(3);

	/*for(int i = 0 ; i < PLACE_NUM+1;i++){
		blink_player(playerList,0);
		horse_update(player1,i,0);
		getchar();
	}*/
	
	int map_value[2][3]={{0,1,2},{3,4,5}};
	int curx,cury;
	int room_num = map_cursor(player1,0,2,-1,-1);
	for(int y = 0 ; y < 2 ; y++){
		for ( int x = 0 ; x < 3 ; x++){
			if(map_value[y][x] == room_num){
				cury = y;
				curx = x;
			}
		}
	}
	mvprintw(1,1,"y= %dx =%d",cury,curx);
	horse_update(player1,room_num,0);	
	room_num = map_cursor(player1,0,2,cury,curx);
	mvprintw(1,2,"%d",room_num);
	horse_update(player1,room_num,0);	
	wrefresh(stdscr);
	getchar();
	endwin();
}

int main(int argc, char *argv[])
{
	run();
	return 0;
}


static WINDOW* display_background(int height,int width,int y,int x,int color){
    WINDOW* window; 
    window = newwin(height,width,y,x);
    wbkgd(window,COLOR_PAIR(color));
    wrefresh(window);
    return window;
}
static void getmaxhw(int *maxy,int *maxx,int *height1, int *width1){
    getmaxyx(stdscr,*maxy,*maxx);
    *height1 = *maxy * 0.65;
    *width1 = *maxx * 0.65;
}

static void display_title(WINDOW* arg,char *args){
    int titlex, titley;
    getmaxyx(arg,titley,titlex);
    box(arg,'|','=');
    mvwaddstr(arg,0,titlex/2,args);
}

WINDOW** make_layout(int maxy,int maxx,int height1,int width1){
    WINDOW **windows=malloc(sizeof(WINDOW*)*LAYOUT_NUM);; // map,memo,clue,history,log,infer;
    char* arg_str[] = {"Map","Memo(m)","Clue(c)","History(h)","Log(l)","Infer"};
    /* create four windows to fill the screen */
    windows[0] = newwin(height1*0.65, width1,0,0) ;  // map
    windows[1] = newwin(height1, maxx-width1, 0, width1); // memo
    windows[2] = newwin(maxy-height1, width1/2, height1, 0) ; // clue
    windows[3] = newwin(maxy-height1, width1/2, height1,width1/2 ); // history
    windows[4] = newwin(maxy-height1, maxx-width1, height1, width1) ; // log
    windows[5] = newwin(height1-(height1*0.65), width1, height1*0.65, 0)  ; // infer
    /* Write to each window */
    for(int i = 0 ; i < LAYOUT_NUM ;i++){
        wbkgd(windows[i],COLOR_PAIR(8));
        display_title(windows[i],arg_str[i]);
        wrefresh(windows[i]);
    }
    return windows;
}
static void map_refresh(WINDOW* window,int color, int y, int x, char* str,char*roomName){
    wbkgd(window,COLOR_PAIR(color));
    box(window,'|','-');
    mvwaddstr(window,2,1,roomName);
    mvwaddstr(window,y,x,str);
}

WINDOW** make_map(int height1,int width1){
    WINDOW **maps=malloc(sizeof(WINDOW*)*MAPS_NUM); //outer,background,kitchen,hall,classroom,stair,looftop,toilet,office,livingroom,training,
    maps[0] = newwin(21,110,3,5) ; // outer
    maps[1] = newwin(17,102,5,9)  ; // bg
    maps[2] = newwin(6,13,5,9) ; // kitchen
    maps[3] = newwin(5,102,11,9)  ; // hall
    maps[4] = newwin(6,33,5,26) ; // classroom
    maps[5] = newwin(3,10,9,76)  ; // stair
    maps[6] = newwin(4,18,5,72)  ; // looftop
    maps[7] = newwin(6,13,5,98)  ; // tilet
    maps[8] = newwin(6,53,16,9)  ; // office
    maps[9] = newwin(6,13,16,62)  ; // livingroom
    maps[10] = newwin(6,13,16,75)  ; // training
    
    wbkgd(maps[0],COLOR_PAIR(9));
    wbkgd(maps[1],COLOR_PAIR(1));
    char *roomName[] = {"Kitchen(s)","ClassRoom(s)","RoomOfTruth(s)","RestRoom(s)","Office(s)","Horse(s)","Training(s)"};
    map_refresh(maps[2],3,5,5,"| |",roomName[0]); // 주방
    map_refresh(maps[4],2,5,17,"| |",roomName[1]); //강의실
    map_refresh(maps[6],4,3,6,"|    |",roomName[2]); // 옥상
    map_refresh(maps[7],5,5,5,"| |",roomName[3]); // 화장실
    map_refresh(maps[8],6,0,15,"| |",roomName[4]); // 사무실
    map_refresh(maps[9],7,0,5,"| |",roomName[5]); // 거실
    map_refresh(maps[10],3,0,5,"| |",roomName[6]); // 체단실
    
    wbkgd(maps[5],COLOR_PAIR(1));
    box(maps[5],'|','-');
    mvwaddstr(maps[5],0,2,"==  ==");
    mvwaddstr(maps[5],1,2,"==  ==");
    mvwaddstr(maps[5],2,2,"==  ==");
    
    wbkgd(maps[3],COLOR_PAIR(1));
    
    box(maps[3],'|','-');
    mvwaddstr(maps[3],0,5,"| |");
    mvwaddstr(maps[3],0,34,"| |");
    mvwaddstr(maps[3],0,70,"|  |");
    mvwaddstr(maps[3],0,94,"| |");
    mvwaddstr(maps[3],4,15,"| |");
    mvwaddstr(maps[3],4,58,"| |");
    mvwaddstr(maps[3],4,71,"| |");
    
    mvwaddstr(maps[3],1,65,"|");
    mvwaddstr(maps[3],2,65,"|");
    mvwaddstr(maps[3],3,65,"|");
    
    mvwaddstr(maps[3],1,25,"|");
    mvwaddstr(maps[3],2,25,"|");
    mvwaddstr(maps[3],3,25,"|");
    
    for( int i = 0; i < MAPS_NUM ; i ++){
        wrefresh(maps[i]);
    }
    return maps;
}

static void width_boundary(int *curx,int max_x,int min_x ){
    if(*curx > max_x || *curx < min_x){
        if(*curx > max_x){
            *curx = max_x;
        }
        else{
            *curx = min_x;
        }
    }
}

static void height_boundary(int *cury,int max_y,int min_y){
    if( *cury > max_y || *cury < min_y){
        if(*cury > max_y){
            *cury = max_y;
        }
        else{
            *cury = min_y;
        }
    }
}
void memo_cursor(){
    noecho();
    cbreak();
    keypad(stdscr,TRUE);
    int maxx,maxy,height,width,startx,starty;
    getmaxhw(&maxy,&maxx,&height,&width);
    startx = width + 22;
    starty = 2;
    maxx = width + 61;
    maxy = starty + 36;
    move(starty,startx);
    int curx,cury;
    while(1){
        int ch = getch();
        getyx(stdscr,cury,curx);
        switch(ch){
            case KEY_DOWN:
                cury = cury + 2;
                height_boundary(&cury,maxy,starty);
                move(cury,curx);
                break;
            case KEY_UP:
                cury = cury - 2;
                height_boundary(&cury,maxy,starty);
                move(cury,curx);
                break;
            case KEY_RIGHT:
                curx = curx + 13;
                width_boundary(&curx,maxx,startx);
                move(cury,curx);
                break;
            case KEY_LEFT:
                curx = curx - 13;
                width_boundary(&curx,maxx,startx);
                move(cury,curx);
                break;
            default:
                if(ch == ' '|| ch =='o'|| ch == 'O' || ch =='x'|| ch=='X' || ch =='?'){
                    attron(COLOR_PAIR(10)|A_BOLD);
                    mvprintw(cury,curx,"%c",ch);
                    move(cury,curx);
                    break;
                }
                else{
                    return ;
                }
        }
    }
    return ;
}
WINDOW** make_infer_clue(char** infer_name,int window_num,int startY, int startX, int height, int width){
    WINDOW **places = malloc(sizeof(WINDOW*)*window_num+1);
    int cnt=0;
    int i;
    for(i = 0; i < (window_num*width) ; i+=width){
        WINDOW *place;
        place = newwin(height,width,startY,startX+i) ;
        wbkgd(place,COLOR_PAIR(1));
        box(place,'|','~');
        mvwprintw(place,2,1,"%s(s)",*infer_name++);
        places[cnt++]=place;
        wrefresh(place);
    }
    WINDOW *window = display_background(2,15,startY+1,i+5,11);
    places[cnt] = window;
    return places;
}
static void display_memo_item(WINDOW *window,size_t size,char** item,int startY){
    int i;
    for(i = 0 ; i < size ; i ++) {
        mvwaddstr(window,startY+i+i,3,item[i]);
    }
}

WINDOW** make_memo(WINDOW* memo,int height,int width){
    char buf[32];
    for(int cnt = 13 ; cnt <=52 ; cnt+=13){
        sprintf(buf,"player%d",cnt/13);//10글자
        mvwaddstr(memo,1,5+cnt,buf);
    }
    char *suspect_str[] ={"     KH_Kim","     WS_Kim","     JS_Kim","    WC_Park","    JH_Shin","    KA_Jeon"};
    char *room_str[] = {"  ClassRoom","   RestRoom","   Training","     Office","    Kitchen","      Horse","RoomOfTruth"};
    char *weapon_str[] = {"      Knife","   Umbrella","      Punch","    MacBook","      Chair","      Cable","       ZUGA"};
    WINDOW **memo_item = malloc(sizeof(WINDOW*)*3);
    memo_item[0] = display_background((SUSPECT_NUM*2)-1,70,2,width+1,1);
    memo_item[1] = display_background((PLACE_NUM*2)-1,70,(SUSPECT_NUM*2)+2,width+1,1);
    memo_item[2] = display_background((WEAPON_NUM*2)-1,70,(PLACE_NUM*2)+(SUSPECT_NUM*2)+2,width+1,1);
    int curY ;
    display_memo_item(memo,SUSPECT_NUM,suspect_str,2);
    display_memo_item(memo,PLACE_NUM,room_str,14);
    display_memo_item(memo,WEAPON_NUM,weapon_str,26);
    display_background(1,70,1,width+1,9);
    display_background(1,70,13,width+1,9);
    display_background(1,70,25,width+1,9);
    display_background(1,70,39,width+1,9);
    
    for(int i = 0 ; i < 38 ; i+=2){
        display_background(1,5,2+i,width+20,10);
        display_background(1,5,2+i,width+33,10);
        display_background(1,5,2+i,width+46,10);
        display_background(1,5,2+i,width+59,10);
    }
    
    wrefresh(memo);
    return memo_item;
}

int infer_cursor(int row){
    char ** str;
    int max_cnt;
    if(row == 0){
        char *str1[] = {"Kitchen","ClassRoom","RestRoom","Training","Horse","Office"};
        str = str1;
        max_cnt = SUSPECT_NUM;
    }
    else if(row == 1) {
        char *str1[] ={"KH_Kim","WS_Kim","JS_Kim","WC_Park","JH_Shin","KA_Jeon"};
        str = str1;
        max_cnt = PLACE_NUM;
    }
    else if(row == 2){
        char *str1[] = {"Knife","Umbrella","Punch","MacBook","Chair","Cable","ZUGA"};
        str = str1;
        max_cnt = WEAPON_NUM;
    }
    int maxx,maxy,width,height,startx,starty;
    int cnt = 0;
    getmaxhw(&maxy,&maxx,&height,&width);
    startx = 4;
    starty = height*0.65+3+(4*row);
    maxx = startx+(14*(max_cnt-1));
    noecho();
    cbreak();
    keypad(stdscr,TRUE);
    attron(COLOR_PAIR(11));
    mvprintw(starty,maxx+17,"%s",str[cnt]);
    move(starty,startx);
    int curx,cury;
    while(1){
        int ch = getch();
        getyx(stdscr,cury,curx);
        switch(ch){
            case KEY_RIGHT:
                cnt ++;
                cnt = cnt < max_cnt ? cnt : max_cnt-1;
                curx = curx + 14;
                width_boundary(&curx,maxx,startx);
                attron(COLOR_PAIR(11));
                mvprintw(cury,maxx+17,"           ");
                mvprintw(cury,maxx+17,"%s",str[cnt]);
                move(cury,curx);
                break;
            case KEY_LEFT:
                cnt --;
                cnt = cnt < 0 ? 0 : cnt;
                curx = curx - 14;
                width_boundary(&curx,maxx,startx);
                attron(COLOR_PAIR(11));
                mvprintw(cury,maxx+17,"           ");
                mvprintw(cury,maxx+17,"%s",str[cnt]);
                move(cury,curx);
                break;
            default:
                if(ch == 's'){
                    if(row==2){
                       move((height*0.65)+6,maxx + 23);
							  while(1){
								  ch = getch();
								  if( ch == 'y' || ch == 'Y'){
									  return cnt;
								  }
								  else if( ch == 'n' || ch == 'N'){
									  return -1;
								  }
								  else{
									  ;
								  }
							  }
                    }
                    return cnt;
                }
                return -1;
        }
    }
}
WINDOW* make_button(int height,int width,int startY,int startX,int color,char*arg){
	WINDOW *button = display_background(height,width,startY,startX,color);
	box(button,'|','-');
	mvwaddstr(button,1,2,arg);
	wrefresh(button);
	return button;
}
WINDOW** display_player(int height, int width, int startY, int startX){
	WINDOW** players = malloc(sizeof(WINDOW*)*4);
	char buf[16];
	for(int i = 0 ; i < PLAYER_NUM*2 ; i+=2 ) {
		sprintf(buf,"player%d",(i/2)+1);
		WINDOW* player;
		player = newwin(height,width,startY+i,startX);
		wbkgd(player,COLOR_PAIR(8));
		wattrset(player,A_BLINK);
		wattroff(player,A_BLINK);
		mvwprintw(player,0,1,"%s",buf);
		wrefresh(player);
		players[(i/2)] = player;
	}
	return players;
}
void blink_player(WINDOW** player, int num){
	char buf[16];
	for( int i = 0 ; i < PLAYER_NUM; i++){
		sprintf(buf,"player%d",i+1);
		if(i == num){
			wattron(player[i],A_BLINK);
		}
		else{
			wattroff(player[i],A_BLINK);
		}
		mvwprintw(player[i],0,1,"%s",buf);
		wrefresh(player[i]);
	}
}
WINDOW** make_horse(int player_num){
	WINDOW **maps = malloc(sizeof(WINDOW*)*PLACE_NUM+2);
   maps[0] = newwin(1,1,8,11+player_num); // kitchen
   maps[1] = newwin(1,1,8,28+player_num); // classroom
   maps[2] = newwin(1,1,8,100+player_num); // tilet
   maps[3] = newwin(1,1,19,11+player_num); // office
   maps[4] = newwin(1,1,19,64+player_num); // livingroom
   maps[5] = newwin(1,1,19,77+player_num); // training
   maps[6] = newwin(1,1,6,74+player_num); // looftop
   maps[7] = newwin(1,1,13,40+player_num);


	for(int i = 0 ; i <= PLACE_NUM+1 ; i++){
		wbkgd(maps[i],COLOR_PAIR(1));
	}
	mvwprintw(maps[7],0,0,"%d",player_num+1);
	wrefresh(maps[7]);
	return maps;
}
void horse_update(WINDOW** player,int room_num,int player_num){
	char buf[2];
	sprintf(buf,"%d",player_num+1);
	for(int i = 0 ; i < PLACE_NUM+1;i ++){
		werase(player[i]);
		if(room_num == i){
			mvwaddstr(player[i],0,0,buf);
		}
		wrefresh(player[i]);
	}
	return ;
}

int map_cursor(WINDOW **room,int player,int range,int cury,int curx){
	wmove(room[6],0,0);
	wrefresh(room[6]);
	noecho();
	cbreak();
	keypad(stdscr,TRUE);
	int ch;
	int cnt=PLACE_NUM;
	while(1){
		ch = getch();
		switch(ch){
			case KEY_RIGHT:
				cnt++;
				cnt = cnt > PLACE_NUM ? PLACE_NUM : cnt;
				wmove(room[cnt],0,0);
				wrefresh(room[cnt]);
				break;
			case KEY_LEFT:
				cnt--;
				cnt = cnt < 0 ? 0 : cnt;
				wmove(room[cnt],0,0);
				wrefresh(room[cnt]);
				break;
			default:
				if( ch == 's'){
					if(move_limit(cnt,range-1,cury,curx) == -1){;}
					else{
						werase(room[7]);
						wrefresh(room[7]);
						return cnt; 
					}	
				}
		}
	}
}
int move_limit(int choice_room,int range,int cury,int curx){
	int map_value[2][3]={{0,1,2},{3,4,5}};
	int map[6]={0,};
	if(cury == -1 && curx == -1 )return 1;
	for(int y = 0 ; y < 2; y ++){
		for( int x = 0 ; x < 3 ; x ++){
			if(range >=CAL_RANGE(cury,curx,y,x)){
				map[map_value[y][x]] = 1;
			}
		}
	}
	for(int i = 0 ; i < 6 ; i ++){
		if(choice_room == i){
			if(map[i] ==1){
				return 1;
			}
			else{
				return -1;
			}
		}
	}
	return -1;
}


