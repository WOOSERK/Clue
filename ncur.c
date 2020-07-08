#include "ncur.h"

/*
void run(){
	initscr();
	refresh();
	start_color();
	color_init();
	char my_card[4] = {8+2,16+4,16+3,24+5};
	WINDOW ***windows = display_init(my_card);
	move_command();

	Cursor cursor;
	cursor.history = make_history();
	cursor.log = make_log();
	cursor.command = make_button(3,25,COMMAND_Y,COMMAND_X,2,"Your Command : ");
	getchar();
	cursor.history_str[0]="000";
	cursor.history_str[1]="111";
	cursor.history_str[2]="222";
	cursor.history_str[3]="333";
	cursor.log_str[0]="000";
	cursor.history_cnt=4;
	cursor.log_cnt=1;
	
	pthread_t pid;
	pthread_create(&pid,NULL,move_cursor,(void*)&cursor);
	pthread_join(pid,NULL);


	clue_cursor(windows[5],my_card);
	getchar();
	
	int room, suspect, weapon;
	while(1){
		if((room = infer_cursor(1)) == -1 ){continue;}
		if((suspect = infer_cursor(2)) == -1 ){continue;}
		if((weapon = infer_cursor(3)) == -1 ){continue;}
		break;
	}

	int map_value[2][3]={{0,1,2},{3,4,5}};
	int curx,cury;
	int room_num = map_cursor(windows[8],windows[12],0,2,-1,-1);
	horse_update(windows[8],room_num,0);	

	calc_yx(room_num,&cury,&curx);

	room_num = map_cursor(windows[8],windows[12],0,2,cury,curx);
	horse_update(windows[8],room_num,0);

	room_num = map_cursor(windows[9],windows[12],1,2,-1,-1);
	horse_update(windows[9],room_num,1);	

	dice_cursor(windows[12][5]);
	endwin();
}
*/
void color_init(){
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
	mvwaddstr(arg,0,(titlex/2)-4,args);
	wrefresh(arg);
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
	maps[0] = newwin(21,129,3,2) ; // outer
	maps[1] = newwin(17,102,5,6)  ; // bg
	maps[2] = newwin(6,13,5,6) ; // kitchen
	maps[3] = newwin(5,102,11,6)  ; // hall
	maps[4] = newwin(6,33,5,23) ; // classroom
	maps[5] = newwin(3,10,9,73)  ; // stair
	maps[6] = newwin(4,18,5,69)  ; // looftop
	maps[7] = newwin(6,13,5,95)  ; // tilet
	maps[8] = newwin(6,53,16,6)  ; // office
	maps[9] = newwin(6,13,16,59)  ; // livingroom
	maps[10] = newwin(6,13,16,72)  ; // training

	wbkgd(maps[0],COLOR_PAIR(10));
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
					attron(COLOR_PAIR(9)|A_BOLD);
					mvprintw(cury,curx,"%c",ch);
					move(cury,curx);
					break;
				}
				else if( ch == 'q'){
					return ;
				}
				else{
					break ;
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
	display_background(1,70,1,width+1,10);
	display_background(1,70,13,width+1,10);
	display_background(1,70,25,width+1,10);
	display_background(1,70,39,width+1,10);

	for(int i = 0 ; i < 38 ; i+=2){
		display_background(1,5,2+i,width+20,9);
		display_background(1,5,2+i,width+33,9);
		display_background(1,5,2+i,width+46,9);
		display_background(1,5,2+i,width+59,9);
	}

	wrefresh(memo);
	return memo_item;
}

int infer_cursor(int row){
	row--;
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
						move((height*0.65)+6,maxx + 21 );
						while(1){
							ch = getch();
							if( ch == 'y' || ch == 'Y'){
								move_command();
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
	box(button,'|','~');
	mvwaddstr(button,1,2,arg);
	wrefresh(button);
	return button;
}

WINDOW** display_player(int height, int width, int startY, int startX){
	startY-=1;
	WINDOW** players = malloc(sizeof(WINDOW*)*7);
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
	players[4]=display_background(3,15,startY+8,startX,9);
	display_title(players[4],"select(s)");
	players[5]=make_button(3,20,startY+12,startX-3,3,"Roll_the_dice(y)");
	players[6]=make_button(3,25,startY+16,startX-8,2,"Send_position_?(y/n)");
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

int map_cursor(WINDOW **player_room,WINDOW** playerList,int player,int range,int cury,int curx){
	wmove(player_room[6],0,0);
	wrefresh(player_room[6]);
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
				display_select_map(playerList[4],cnt);
				display_title(playerList[4],"select(s)");
				wmove(player_room[cnt],0,0);
				wrefresh(player_room[cnt]);
				break;
			case KEY_LEFT:
				cnt--;
				cnt = cnt < 0 ? 0 : cnt;
				display_select_map(playerList[4],cnt);
				display_title(playerList[4],"select(s)");
				wmove(player_room[cnt],0,0);
				wrefresh(player_room[cnt]);
				break;
			default:
				if( ch == 's'){
					if(move_limit(cnt,range-1,cury,curx) == -1){break;}
					wmove(playerList[6],1,2);
					wrefresh(playerList[6]);
					ch = getch();
					if(ch == 'y' || ch == 'Y'){
						werase(player_room[7]);
						wrefresh(player_room[7]);
						move_command();
						return cnt; 
					}
					else{
						wmove(player_room[cnt],0,0);
						wrefresh(player_room[cnt]);
						break;
					}
				}
		}
	}
}
int move_limit(int choice_room,int range,int cury,int curx){
	int map_value[2][3]={{0,1,2},{3,4,5}};
	int map[6]={0,};
	if(cury == 3 && curx == 2 )return 1;
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

char* parse_card(int category,int card_num){
	char *str;
	if(category == 1){
		char *str1[] = {"Kitchen","ClassRoom","RestRoom","Training","Horse","Office","RoomOfTurth"};
		str = str1[card_num];
	}
	else if(category == 2) {
		char *str1[] ={"KH_Kim","WS_Kim","JS_Kim","WC_Park","JH_Shin","KA_Jeon"};
		str = str1[card_num];
	}
	else if(category == 3){
		char *str1[] = {"Knife","Umbrella","Punch","MacBook","Chair","Cable","ZUGA"};
		str = str1[card_num];
	}
	return str;
}


WINDOW* make_clue(int category,int card_number,int num){
	char* card_name = parse_card(category,card_number);
	int color_num = num+1;
	num *=3;
	int maxx,maxy,height,width;
	getmaxhw(&maxy,&maxx,&height,&width);
	height +=2;
	width = 4;	
	WINDOW *card = newwin(3,20,height+num,width);
	wbkgd(card,COLOR_PAIR(color_num));
	box(card,'|','~');
	mvwaddstr(card,1,5,card_name);
	wrefresh(card);
	return card;
}

char clue_cursor(WINDOW** window,char* cards){
	noecho();
	cbreak();
	keypad(stdscr,TRUE);
	int cnt=0;
	int ch;
	wmove(window[0],1,3);
	display_select_card(window[4],cards[cnt]);
	wrefresh(window[0]);
	while(1){
		ch = getch();
		switch(ch){
			case KEY_UP:
				cnt--;
				cnt = cnt<0?0:cnt;
				wmove(window[cnt],1,3);
				display_select_card(window[4],cards[cnt]);
				wrefresh(window[cnt]);
				break;
			case KEY_DOWN:
				cnt++;
				cnt = cnt>3?3:cnt;
				wmove(window[cnt],1,3);
				display_select_card(window[4],cards[cnt]);
				wrefresh(window[cnt]);
				break;
			default:
				if( ch == 's' || ch == 'S'){
					wmove(window[5],1,2);
					wrefresh(window[5]);
					ch = getch();
					if(ch == 'y' || ch == 'Y'){
						move_command();
						return cards[cnt];
					}
					if( ch == 'n' || ch == 'N'){
						wmove(window[0],1,3);
						wrefresh(window[0]);
						break;	
					}
					else{
						wmove(window[0],1,3);
						wrefresh(window[0]);
						break;
					}
				}
		}
	}
}

void display_select_card(WINDOW* display_card, char card){
	int cate,card_num;
	cate = PARSE_CATE(card);
	card_num = PARSE_CARD(card);
	char* cardstr = parse_card(cate,card_num);
	werase(display_card);
	char buf[32];
	sprintf(buf,"%s",cardstr);
	mvwaddstr(display_card,1,3,buf);
	display_title(display_card,"select(s)");
	wrefresh(display_card);
}

void display_select_map(WINDOW *display_map,int cnt){
	char *str1[] = {"Kitchen","ClassRoom","RestRoom","Office","Horse","Training","RoomOfTurth"};
	werase(display_map);
	mvwaddstr(display_map,1,1,str1[cnt]);
	wrefresh(display_map);
}

int calc_yx(int room_num, int *cury, int *curx){
	int map_value[2][3]={{0,1,2},{3,4,5}};
	for( int y = 0 ; y < 2 ; y++){
		for(int x = 0 ; x < 3 ; x++){
			if(room_num == map_value[y][x]){
				*cury = y;
				*curx = x;
				return 0;
			}
		}
	}
	return -1;
}

void dice_cursor(WINDOW* dice_button){
	wmove(dice_button,1,2);
	wrefresh(dice_button);
	int ch;
	while(1){
		ch = getch();
		if(ch == 'y' || ch == 'Y'){
			return ;
		}
	}
}


WINDOW** make_history(){
	int startx,starty,maxx,maxy;
	getmaxhw(&maxy,&maxx,&starty,&startx);
	startx /= 2;
	startx += 15;
	starty += 1;
	WINDOW *background = display_background(21,44,starty,startx-5,10);
	box(background,'|',' ');
	wrefresh(background);
	WINDOW **history = malloc(sizeof(WINDOW*)*8);
	for(int i = 0 ; i < 21 ; i+=3){
		history[i/3] = display_background(2,34,starty+i,startx,1);
	}
	return history;
}

WINDOW** make_log(){
	int startx,starty,maxx,maxy;
	getmaxhw(&maxy,&maxx,&starty,&startx);
	startx += 10;
	starty += 1;
	WINDOW *background = display_background(21,44,starty,startx-5,10);
	box(background,'|',' ');
	wrefresh(background);
	WINDOW **log = malloc(sizeof(WINDOW*)*8);
	for(int i = 0 ; i < 21 ; i+=3){
		log[i/3] = display_background(2,34,starty+i,startx,1);
	}
	return log;
}

void history_log_cursor(WINDOW **window,char **str, int str_size){
	int start_idx=0;
	noecho();
	cbreak();
	keypad(stdscr,TRUE);
	int ch;
	int cnt=HISTORY_MAX-1;
	history_log_print(window,str,start_idx,str_size);
	wmove(window[HISTORY_MAX-1],1,1);
	wrefresh(window[HISTORY_MAX-1]);
	while(1){
		ch = getch();
		switch(ch){
			case KEY_DOWN:
				cnt ++;
				if(cnt >= HISTORY_MAX){
					start_idx++;
					start_idx = (start_idx+6) < str_size ? start_idx : --start_idx;
					cnt = HISTORY_MAX-1;
					history_log_print(window,str,start_idx,str_size);
				}
				wmove(window[cnt],1,1);
				wrefresh(window[cnt]);
				break;
			case KEY_UP:
				cnt--;
				if(cnt < 0){
					start_idx--;
					start_idx = start_idx < 0 ? 0 : start_idx;
					cnt = 0;
					history_log_print(window,str,start_idx,str_size);
				}
				wmove(window[cnt],1,1);
				wrefresh(window[cnt]);
				break;
			default:
				if(ch == 'q' || ch == 'Q'){
					return ;
				}
				break;
		}
	}
}

void history_log_print(WINDOW** display, char**str, int start_idx,int  str_size){
	start_idx += str_size < 7 ? str_size-1 : 7;
	for(int i = HISTORY_MAX-1 ; i >= 0 ; i--){
		if(0 <= start_idx){
			werase(display[i]);
			wattrset(display[i],A_BOLD);
			mvwaddstr(display[i],1,1,str[start_idx--]);
			wrefresh(display[i]);
		}
	}
	return ;
}

int str_add(char **str_arry, int add_idx,char* str){
	size_t len = strlen(str);
	if( add_idx < STR_ARR_MAX){
		str_arry[add_idx] = malloc(sizeof(char)*len);
		strcpy(str_arry[add_idx],str);
		return 1;
	}
	return -1;
}

void* move_cursor(void *arg){
	Cursor cursor = *(Cursor*)arg;
	int ch ; 
	while(1){
		ch = input_command(cursor.command); 
		switch(ch){
			case 'L':
			case 'l':
				history_log_cursor(cursor.log,cursor.log_str,cursor.log_cnt);
				break;
			case 'H':
			case 'h':
				history_log_cursor(cursor.history,cursor.history_str,cursor.history_cnt);
				break;
			case 'M':
			case 'm':
				memo_cursor();
				break;
			default:
				if(ch == 'q'){
					return NULL;
				}
				break;
		}
	}
}

int input_command(WINDOW *command){
	noecho();
	cbreak();
	keypad(stdscr,TRUE);
	wmove(command,1,18);
	wrefresh(command);
	int ch = getch();
	mvwprintw(command,1,18,"  ");
	wattrset(command,A_BOLD|COLOR_PAIR(1));
	mvwprintw(command,1,18,"%c",ch);
	wrefresh(command);
	refresh();
	return ch;
}

void move_command(){
	move(COMMAND_Y+1,COMMAND_X+18);
	refresh();
}

void dice_num_print(WINDOW *window, int num){
	wattrset(window,A_BOLD|A_BLINK|COLOR_PAIR(1));
	mvwprintw(window,1,9,"%d",num);
	wrefresh(window);
	move_command();
}

WINDOW*** display_init(char* card){
	char *suspect_str[] ={"KH_Kim","WS_Kim","JS_Kim","WC_Park","JH_Shin","KA_Jeon"};
	char *room_str[] = {"Kitchen","ClassRoom","RestRoom","Training","Horse","Office"};
	char *weapon_str[] = {"Knife","Umbrella","Punch","MacBook","Chair","Cable","ZUGA"};
	int maxx,maxy,width1,height1;
	getmaxhw(&maxy,&maxx,&height1,&width1);
	/* 아래 window 배열 저장 순서 : layout,maps,places,suspect,weapon*/
	WINDOW ***windows = malloc(sizeof(WINDOW**)*13);; 
	windows[0] = make_layout(maxy,maxx,height1,width1); // 레이아웃 잡기
	windows[1] = make_map(height1,width1); // 맵 그리기
	display_background(4*3,14*9,(height1*0.65)+1,3,10);
	windows[2] = make_infer_clue(room_str,6,(height1*0.65)+1,3,4,14); // 장소추리칸 그리기
	windows[3] = make_infer_clue(suspect_str,6,(height1*0.65)+5,3,4,14); // 범인 추리칸 그리기
	windows[4] = make_infer_clue(weapon_str,7,(height1*0.65)+9,3,4,14); // 흉기 추리칸 그리기

	make_memo(windows[0][1],height1,width1);	

	int *my_card = parse_card_num(card);
	WINDOW *clues[6]; // 1번단서 , 2번단서, 3번단서, 4번단서, 선택한 단서 보여주기, 선택버튼
	display_background(14,50,height1+4,2,10);
	for(int i=0 ; i < 4 ; i ++){
		clues[i] = make_clue(my_card[i*2],my_card[i*2+1],i+1);
	}
	clues[4] = make_button(3,23,height1+7,25,11,"");
	clues[5] = make_button(3,23,height1+11,25,7,"select_clue_?(y/n)");
	windows[5] = clues;

	windows[6] = make_history();
	windows[7] = make_log();

	windows[8] = make_horse(0);
	windows[9] = make_horse(1);
	windows[10] = make_horse(2);
	windows[11] = make_horse(3);
	windows[12] = display_player(1,10,5,110);

	WINDOW *buttons[3];
	buttons[0] = make_button(3,20,(height1*0.65)+5,(6*14)+24,2,"Send infer(y/n)"); // 버튼만들기
	buttons[1] = make_button(3,12,16,95,3,"dice :");
	buttons[2] =  make_button(3,25,COMMAND_Y,COMMAND_X,2,"Your Command : ");

	windows[13] = buttons;

	return windows;
}

int* parse_card_num(char* cards){
	int *my_card = malloc(sizeof(int)*8);
	for(int i = 0 ; i < 4 ; i++){
		my_card[i*2]=PARSE_CATE(cards[i]);
		my_card[(i*2)+1]=PARSE_CARD(cards[i]);
	}
	return my_card;
}

void return_yx(unsigned short position,int *y, int *x){
	*y = position>>2;
	*x = position&0x3;
}

void return_player_horse(WINDOW ***windows,WINDOW**player, int player_id){
	switch(player_id){
		case 0:
			player = windows[8];
			break;
		case 1:
			player = windows[9];
			break;
		case 2:
			player = windows[10];
			break;
		case 3:
			player = windows[11];
			break;
		default:
			break;
	}
}

char* parse_room_name(int room_num){
	switch(room_num){
		case 0:
			return "Kitchen";
		case 1:
			return "ClassRoom";
		case 2:
			return "RestRoom";
		case 3:
			return "Office";
		case 4:
			return "Horse";
		case 5:
			return "Training";
		case 6:
			return "RoomOfTruth";
	}
	return NULL;
}

