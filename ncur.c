#include <ncurses.h>
#include <stdlib.h>
#define MAPS_NUM (11)
#define LAYOUT_NUM (6)
#define INFER_RESULT (3)
#define SUSPECT_NUM (6)
#define PLACE_NUM (6)
#define WEAPON_NUM (7)
#define CHECK_NUM (76)

static void getmaxhw(int *maxy,int *maxx,int *height1, int *width1){
	getmaxyx(stdscr,*maxy,*maxx);
	*height1 = *maxy * 0.65;
	*width1 = *maxx * 0.65;
}

static void bomb(void)
{
	addstr("Unable to allocate memory for new window.\n");
	refresh();
	endwin();
	exit(1);
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
	if( (windows[0] = newwin(height1*0.65, width1,0,0)) == NULL) bomb();  // map
	if( (windows[1] = newwin(height1, maxx-width1, 0, width1)) == NULL) bomb(); // memo
	if( (windows[2] = newwin(maxy-height1, width1/2, height1, 0)) == NULL) bomb(); // clue
	if( (windows[3] = newwin(maxy-height1, width1/2, height1,width1/2 )) == NULL) bomb(); // history
	if( (windows[4] = newwin(maxy-height1, maxx-width1, height1, width1)) == NULL) bomb(); // log
	if( (windows[5] = newwin(height1-(height1*0.65), width1, height1*0.65, 0)) == NULL) bomb(); // infer
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
	mvwaddstr(window,2,3,roomName);
	mvwaddstr(window,y,x,str);
}

WINDOW** make_map(int height1,int width1){
	WINDOW **maps=malloc(sizeof(WINDOW*)*MAPS_NUM); //outer,background,kitchen,hall,classroom,stair,looftop,toilet,office,livingroom,training,
	if((maps[0] = newwin(21,110,3,5)) == NULL)bomb(); // outer
	if((maps[1] = newwin(17,102,5,9)) == NULL)bomb(); // bg
	if((maps[2] = newwin(6,13,5,9)) == NULL)bomb(); // kitchen
	if((maps[3] = newwin(5,102,11,9)) == NULL)bomb(); // hall
	if((maps[4] = newwin(6,33,5,26)) == NULL)bomb(); // classroom
	if((maps[5] = newwin(3,10,9,76)) == NULL)bomb(); // stair
	if((maps[6] = newwin(4,18,5,72)) == NULL)bomb(); // looftop
	if((maps[7] = newwin(6,13,5,98)) == NULL)bomb(); // tilet
	if((maps[8] = newwin(6,53,16,9)) == NULL)bomb();
	if((maps[9] = newwin(6,13,16,62)) == NULL)bomb();
	if((maps[10] = newwin(6,13,16,75)) == NULL)bomb();

	wbkgd(maps[0],COLOR_PAIR(9));
	wbkgd(maps[1],COLOR_PAIR(1));
	char *roomName[] = {"Kitchen","ClassRoom","RoomOfTruth","RestRoom","Office","Horse","Training"};
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
WINDOW** make_infer_clue(int window_num,int startY, int startX, int height, int width){
	WINDOW **places = malloc(sizeof(WINDOW*)*window_num);
	int cnt=0;
	for(int i = 0; i < (window_num*width) ; i+=width){
		WINDOW *place;
		if( (place = newwin(height,width,startY,startX+i)) == NULL) bomb();
		wbkgd(place,COLOR_PAIR(1));
		box(place,'|','~');
		places[cnt++]=place;	
		wrefresh(place);
	}
	return places;
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
				if(ch =='o'|| ch == 'O' || ch =='x'|| ch=='X' || ch =='?'){
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
static void display_memo_item(WINDOW *window,size_t size,char** item,int startY){
	int i;
	for(i = 0 ; i < size ; i ++) {
		mvwaddstr(window,startY+i+i,3,item[i]);
	}
}

static WINDOW* display_background(int height,int width,int y,int x,int color){
	WINDOW* window;
	window = newwin(height,width,y,x);
	wbkgd(window,COLOR_PAIR(color));
	wrefresh(window);
	return window;
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
	//make_checkField(width+21);

	wrefresh(memo);
	return memo_item;
}


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

	int maxx,maxy,width1,height1;
	getmaxhw(&maxy,&maxx,&height1,&width1);

	WINDOW **windows[10]; //layout,maps,places,suspect,weapon

	windows[0] = make_layout(maxy,maxx,height1,width1); // 레이아웃 잡기
	windows[1] = make_map(height1,width1); // 맵 그리기
	windows[2] = make_infer_clue(6,(height1*0.65)+1,3,4,14); // 장소추리칸 그리기
	windows[3] = make_infer_clue(6,(height1*0.65)+5,3,4,14); // 범인 추리칸 그리기
	windows[4] = make_infer_clue(7,(height1*0.65)+9,3,4,12); // 흉기 추리칸 그리기
	make_memo((windows[0])[1],height1,width1);	
	memo_cursor();
}

int main(int argc, char *argv[])
{
	run();
	endwin();
	return 0;
}


