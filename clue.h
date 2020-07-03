#define ACK (1)
#define SIG_TURN (2)
#define SIG_WAIT (3)
#define SIG_INFR (4)
#define SIG_DONE (5)
#define PLAYER_ID(player_info) ((player_info) & ((char)0x3) 
#define PLAYER_PHASE(player_info) ((player_info) & ((char)0xc)
#define PLAYER_ISTURN(player_info) ((player_info) & ((char)0x10))
#define PLAYER_POSITION(player_position, num) ((player_position) & ((short)0xf000 >> (4*num)))
#define PLAYER_SELECT_VALUE(player_select) ((player_select) & ((char)0x7))
#define PLAYER_DICE_VALUE(player_dice) ((player_dice) & ((char)0x38))
#define PLAYER_INFER_CRIMINAL(player_infer) ((player_infer) & ((short)0x7c00))
#define PLAYER_INFER_WEAPON(player_infer) ((player_infer) & ((short)0x30e0))
#define PLAYER_INFER_SCENE(player_infer) ((player_infer) & ((short)0x1f))

typedef struct header
{
	int type;
	int len;
} Header;

typedef struct player
{
	char info;	
	short position;
	char cards[4];
	char dice;
	short infer;
	char clue;
} Player_packet;

int packet_send(int player, char* packet)
{

}

int packet_recv(int player, char* packet)
{

}

int wait_ack(int player)
{

}
