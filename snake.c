#include<stdlib.h>
#include<stdio.h>
#include<string.h>
#include<unistd.h>
#include<termio.h>
#include<pthread.h>
#include<time.h>
#include<stdatomic.h>
#include<semaphore.h>
// #define NDEBUG
#include<assert.h>

// The Function declaration which belong to header file
typedef enum keydirect_E{
	up = 0,
	down = 1,
	left = 2,
	right = 3,
}keydirect_T;

typedef struct snakebody{
	int x;
	int y;
	int stridex;
	int stridey;
	struct snakebody *next;
}snakebody;

typedef struct mazemap{
	int width;
	int heigh;
	int doorx;
	int doory;
	int barrier;
}mazemap;

#define mazecount 9
#define specialNum 2

int initpizz(int No);
int destorypizz();

char **pizz;
snakebody *head;

// After c11, we can set the global var as atomic 
_Atomic keydirect_T keyvalue;
// keydirect_T keyvalue;

static int Width=60;
static int Heigh=25;

static struct termios new_settings,stored_settings;
// After c11, we can set the global var as atomic 
// static atomic_int keyNumber;
static int keyNumber;
static pthread_mutex_t mutex;
static pthread_t keyboardlisten;
static pthread_t workflow;
static unsigned int speed = 1000000;
static unsigned int initspeed = 1000000;

int crush = 0;
char specialbean[specialNum] = {'<', '>'};
int stridexmap[4] = {-1, 1, 0, 0};
int strideymap[4] = {0, 0, -1, 1};
mazemap maze[mazecount] = { {60, 25, 2, 0, 0}, {100, 50, 0, 70, 20}, {45, 25, 24, 10, 5}, 
							{45, 50, 15, 44, 30}, {100, 25, 0, 20, 50}, {70, 30, 40, 0, 60}, 
							{60, 30, 0, 10, 15}, {150, 50, 49, 70, 80}, {15, 20, 0, 1, 2}, 
};

void initkeyboard()
{
	tcgetattr(0,&stored_settings);
	new_settings = stored_settings;
	new_settings.c_lflag &= (~ICANON);
	new_settings.c_lflag &= (~ECHO);
	new_settings.c_cc[VTIME] = 0;
	new_settings.c_cc[VMIN] = 1;
	tcsetattr(0, TCSANOW, &new_settings);
}

void closekeyboard(){
	tcsetattr(0, TCSANOW, &stored_settings);
}

void printHead(){
		// \033c ---> Reset Device <ESC>c
        printf("\033c");
        printf("--------------------------hello, player-----------------------!\n");
        printf("-- Author: NPG-QAT xinghong                                  --\n");
        printf("-- Date:   2022-06-11                                        --\n");
        printf("--                                                           --\n");
        printf("--------------------------------------------------------------!\n");
		printf(" +++++++++++++++++++++++++++++++++++++++++++++++++++++++  \n");
		printf("                                                          \n");
		printf("              welcome to the stupid game                   \n");
		printf("                                                          \n");
		printf(" +++++++++++++++++++++++++++++++++++++++++++++++++++++++  \n");
		printf(" Ps : esc key can help you exit, and left arrow , up arrow , down arrow, right arrow help you contral snake!\n");
		printf(" Ps : 'o' -> bean, '<' -> slow, '>' -> fast \n");
	return;
}

void designGame(){
	printHead();
	printf(" please chose the diffcuty (easy / medium / hard / master) : ");
	char diffcuty[50];
	scanf("%s", diffcuty);
	if (!strcmp(diffcuty, "easy")) initspeed = 500000;
	if (!strcmp(diffcuty, "medium")) initspeed = 200000;
	if (!strcmp(diffcuty, "hard")) initspeed = 100000;
	if (!strcmp(diffcuty, "master")) initspeed = 50000;
}

int genbean(){
	srand((unsigned)time(NULL));
	int randx,randy;
	randx = rand()%(Heigh-2)+1;
	randy = rand()%(Width-2)+1;
	int bean = rand()%10;
	char spbean = 'o';
	if (bean < 6){
		spbean = 'o';
	}else if (bean<8)
	{
		spbean = '<';
	}else if(bean<10){
		spbean = '>';
	}
	
	if (pizz[randx][randy]!=' '){
		return 1;
	}else{
		pizz[randx][randy] = spbean;
	}
	return 0;
}

int convertsnake(){
	snakebody *cursor = head;
	keydirect_T initdir;
	switch (keyvalue)
	{
		case up:
			head->x = Heigh-2;
			head->y = Width/2;
			initdir = down;
			break;
		case down:
			head->x = 1;
			head->y = Width/2;
			initdir = up;
			break;
		case left:
			head->x = Heigh/2;
			head->y = Width-2;
			initdir = right;
			break;
		case right:
			head->x = Heigh/2;
			head->y = 1;
			initdir = left;
			break;
		default:
			break;
	}
	while(cursor->next){
		cursor->next->x = cursor->x + stridexmap[initdir];
		cursor->next->y = cursor->y + strideymap[initdir];
		cursor->next->stridex = stridexmap[keyvalue];
		cursor->next->stridey = strideymap[keyvalue];
		cursor = cursor->next; 
	}
	return 0;
}

int crushonwall(){
	int x = head->x;
	int y = head->y;
	if((x==0 || x== Heigh-1 || y == 0 || y == Width-1) && pizz[x][y] != '$'){
		crush = 1;
		return crush;
	}
	return 0;
}

int crushonbean(int x, int y, int stridex, int stridey){
	if (pizz[head->x][head->y] == 'o'){
		snakebody *tail = (snakebody*)malloc(sizeof(snakebody));
		tail->x = x;
		tail->y = y;
		tail->stridex = stridex;
		tail->stridey = stridey;
		tail->next = NULL;
		snakebody *cursor = head;
		while(cursor->next) {cursor = cursor->next;}
		cursor->next = tail;
		while(genbean()){}
		return 1;
	}
	return 0;
}

int crushonspbean(){
	if(pizz[head->x][head->y] == '<'){
		speed = speed*3/2;
		while(genbean()){}
	}else if(pizz[head->x][head->y] == '>'){
		speed = speed/2;
		while(genbean()){}
	}
	return 0;
}

int crushonbody(){
	snakebody *cursor = head->next;
	while(cursor){
		if (cursor->x == head->x && cursor->y == head->y){
            crush = 1;
			break;
		}
		cursor = cursor->next;
	}
	return crush;
}

int crushondoor(){
	int ret=0;
	if(pizz[head->x][head->y] == '$'){
		ret = destorypizz();
		assert(!ret);
		int mazeNO = rand()%mazecount;
		assert(mazeNO<mazecount);
		ret = initpizz(mazeNO);
		assert(!ret);
		ret = convertsnake();
		assert(!ret);
		while(genbean());
	}
	return 0;
}

int crushonbarrier(){
	if(pizz[head->x][head->y] == 'x'){
		crush = 1;
		return crush;
	}
	return 0;
}

void handlecrush(){
	if (crush){
		// char *endlog = "                 your  are   loser !!!!!!!!           ";
		char *log = "you loser!";
		if (Width>10){
			memcpy(&pizz[Heigh/2][Width/2-5], log, strlen(log));		
		}
	}else{
        snakebody *cursor = head;
		while(cursor){
			if (cursor->x > 0 && cursor->x < Heigh-1 && cursor->y >0 && cursor->y < Width-1){
				pizz[cursor->x][cursor->y] = 'O';			
			}
			cursor = cursor->next;
        }
	}
}

void movesnake(){
	snakebody* cursor = head;
	int newstridex=0;
	int newstridey=0;
	int tailx=0;
	int taily=0;
	int inputvalidate = stridexmap[keyvalue]+head->stridex | strideymap[keyvalue] +head->stridey;
	while(cursor){
		if (cursor == head){
			newstridex = cursor->stridex;
			newstridey = cursor->stridey;
			if (!inputvalidate){
				cursor->x += cursor->stridex;
				cursor->y += cursor->stridey;
			}else{
				cursor->stridex = stridexmap[keyvalue];
				cursor->stridey = strideymap[keyvalue];
				cursor->x += cursor->stridex;
				cursor->y += cursor->stridey;
			}
			cursor = cursor->next;
			continue;
		}
		int bodystridex = cursor->stridex;
		int bodystridey = cursor->stridey;
		cursor->stridex = newstridex;
		cursor->stridey = newstridey;
		if(!cursor->next){
			if (cursor->x > 0 && cursor->x < Heigh-1 && cursor->y >0 && cursor->y < Width-1){
				pizz[cursor->x][cursor->y] = ' ';		
			}
			tailx = cursor->x;
			taily = cursor->y;
		}
		cursor->x += cursor->stridex;
		cursor->y += cursor->stridey;
		newstridex = bodystridex;
		newstridey = bodystridey;
		cursor = cursor->next;
	}
	crushonbean(tailx, taily, newstridex, newstridey);
	crushonspbean();
	crushonwall();
	crushonbarrier();
	crushonbody();
	crushondoor();
	handlecrush();

	for(int i=0; i<Heigh; i++){
		printf("%s\n",pizz[i]);
	}
}

void initsnake(){
	keyNumber = 0;
	keyvalue = left;	
	int startx = Heigh/2;
	int starty = Width/2;
	snakebody* temp = head;
	for (int i=0; i<5; i++){
		snakebody* body = (snakebody*)malloc(sizeof(snakebody));
		body->x = startx;
		body->y = starty+i;
		body->stridex = 0;
		body->stridey = -1;
		body->next = NULL;
		if(!head){
			head = body;
			temp = body;
		}else{
			temp->next = body;
			temp = body;
		}
	}
	temp = head;
	while(temp){
		int x = temp->x;
		int y = temp->y;
		pizz[x][y] = 'O';
		temp = temp->next;		
	}
}

int initpizz(int No){
	speed = initspeed;
	Heigh = maze[No].heigh;
	Width = maze[No].width;
	pizz = (char **) malloc(sizeof(char *)*Heigh);
	for (int i=0; i<Heigh; i++){
		pizz[i] = (char*) malloc(sizeof(char)*(Width+1));
	}
	for(int i=0;i<Heigh;i++){
		for(int j=0;j<=Width;j++){
			if(j==Width){
				pizz[i][j] = '\0';
				continue;
			}
			if(i==0 || i== Heigh-1){
				pizz[i][j] = '-';
			}else if(j == 0 || j==Width-1){
				pizz[i][j] = '|';
			}else{
				pizz[i][j] = ' ';
			}	
		}
	}
	int doorx = maze[No].doorx;
	int doory = maze[No].doory;
	pizz[doorx][doory] = '$';
	for (int i=0; i<maze[No].barrier; i++){
		int randx,randy;
		randx = rand()%(Heigh-2)+1;
		randy = rand()%(Width-2)+1;
		pizz[randx][randy] = 'x';
	}
	return 0;
}

int destorypizz(){
	for(int i=0; i<Heigh; i++){
		free(pizz[i]);
	}
	free(pizz);
	return 0;
}

void *Controlflow(void *arg){
	int ret = 0;
	while(genbean());
	while(1){
		printHead();
		// Check if player escape
		if(keyNumber == 27){
			break;
		}
		movesnake();
		if(crush){
			break;
		}
		usleep(speed);
	}
}

int freesnake(){
	int record = 0;
	snakebody *temp;
	while(head){
		temp = head->next;
		free(head);
		head = temp;
		record++;
	}
	return record;
}

void *startlistenkeyboard(void *arg){
	// After C++11 standard, it recommand to use "__atomic"
	while(1){
		int temp = getchar();
		__sync_lock_test_and_set(&keyNumber, 30);
		keyNumber = temp;
		switch(keyNumber){
			case 65:
				__atomic_store_n(&keyvalue, up, 0);    
				break;
			case 66:
				__atomic_store_n(&keyvalue, down, 0);   
				break;
			case 68:
				__atomic_store_n(&keyvalue, left, 0);   
				break;
			case 67:
				__atomic_store_n(&keyvalue, right, 0);   
				break;
		}	
	}
}


int main(int argv, char *argc[]){
	pthread_mutex_init(&mutex, NULL);
	designGame();
	while(!crush){
		initkeyboard();
		initpizz(0);
		initsnake();

		int ret = 0;		
		if(ret = pthread_create(&keyboardlisten, NULL, startlistenkeyboard, NULL)){
			printf("creat inputpipe thread failed, %s\n", strerror(ret));
			return ret;
		}
		if(ret = pthread_create(&workflow, NULL, Controlflow, NULL)){
			printf("creat painpipe thread failed, %s\n", strerror(ret));
			return ret;
		}
		pthread_join(workflow, NULL);
		pthread_cancel(keyboardlisten);

		printf("your record is %d\n", freesnake());
		destorypizz();
		closekeyboard();

		// Exit the Game process
		if(!crush){
			// Use esc to exit the game
			break;
		}else{
			// player can restart game from here
			printf("you lose, please chose if you want start again(y or n) :");
			char yorn = getchar();
			if (yorn == 'y' || yorn == 'Y'){
				crush = 0;
			}else{
				break;
			}
		}
	}
	return 0;
}
