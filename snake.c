#include<stdlib.h>
#include<stdio.h>
#include<string.h>
#include<unistd.h>
#include<termio.h>
#include<pthread.h>
#include<time.h>
#include<assert.h>

#define Width 65
#define Heigh 25

static struct termios new_settings,stored_settings;
static int keyNumber;
static pthread_mutex_t mutex;
static pthread_t keyboardlisten;
static pthread_t workflow;

static unsigned int speed = 1000000;
int crush = 0;
char pizz[Heigh][Width+1];

int stridexmap[4] = {-1, 1, 0, 0};
int strideymap[4] = {0, 0, -1, 1};

enum keydirect{
	up = 0,
	down = 1,
	left = 2,
	right = 3,
};

enum keydirect keyvalue;

typedef struct snakebody{
	int x;
	int y;
	int stridex;
	int stridey;
	struct snakebody *next;
}snakebody;

snakebody *head;

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
        printf("\033c");
        printf("--------------------------hello, player-----------------------!\n");
        printf("-- Author: flex cxh                                          --\n");
        printf("-- Date:   2020-03-11                                        --\n");
        printf("--                                                           --\n");
        printf("--------------------------------------------------------------!\n");
	return;
}

void designGame(){
	printHead();
	printf(" +++++++++++++++++++++++++++++++++++++++++++++++++++++++  \n");
	printf("                                                          \n");
	printf("              welcome to the stupid game                   \n");
	printf("                                                          \n");
	printf(" +++++++++++++++++++++++++++++++++++++++++++++++++++++++  \n");
	printf(" Ps : esc key can help you exit, and left arrow , up arrow , down arrow, right arrow help you contral snake!\n");
	printf(" please chose the diffcuty (easy / medium / hard / master) : ");
	char diffcuty[50];
	scanf("%s", diffcuty);
	if (!strcmp(diffcuty, "easy")) speed = 500000;
	if (!strcmp(diffcuty, "medium")) speed = 200000;
	if (!strcmp(diffcuty, "hard")) speed = 100000;
	if (!strcmp(diffcuty, "master")) speed = 50000;
}

int genbean(){
	srand((unsigned)time(NULL));
	int randx,randy;
	randx = rand()%(Heigh-2)+1;
	randy = rand()%(Width-2)+1;
	snakebody *temp = head;
	int checktag = 0;
	while(temp){
		if (randx == temp->x && randy == temp->y){
			checktag = 1;
			break;
		}
		temp = temp->next;
	}
	if (checktag){
		return 1;
	}else{
		pizz[randx][randy] = 'o';
	}
	return 0;
}

int crushonwall(){
	int x = head->x;
	int y = head->y;
	if(x==0 || x== Heigh-1 || y == 0 || y == Width-1){
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

void handlecrush(){
	if (crush){
		char *endlog = "                 your  are   loser !!!!!!!!           ";
		memcpy(pizz[Heigh/2], endlog, strlen(endlog));
	}else{
        snakebody *cursor = head;
		while(cursor){
			pizz[cursor->x][cursor->y] = 'O';
			cursor = cursor->next;
        }
	}
}

void printsnake(){
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
			pizz[cursor->x][cursor->y] = ' ';
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
	crushonwall();
	crushonbody();
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

void initpizz(){
	initkeyboard();
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
	initsnake();
	while(genbean()){}
}

void *Controlflow(void *arg){
	initpizz();
	while(1){
		pthread_mutex_lock(&mutex);
		printHead();
		if(keyNumber == 27){
			pthread_mutex_unlock(&mutex);
			break;
        }
		switch(keyNumber){
			case 65:
				keyvalue = up;
				break;
			case 66:
				keyvalue = down;
				break;
			case 68:
				keyvalue = left;
				break;
			case 67:
				keyvalue = right;
				break;
		}	
		printsnake();
		if(crush){
			closekeyboard();
			pthread_mutex_unlock(&mutex);
			break;
		}
		pthread_mutex_unlock(&mutex);
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
	while(1){
		int temp = getchar();
		pthread_mutex_lock(&mutex);
		keyNumber = temp;
		pthread_mutex_unlock(&mutex);
	}
}


int main(int argv, char *argc[]){
	pthread_mutex_init(&mutex, NULL);
	designGame();
	while(!crush){
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
		if(!crush){
			break;
		}
		printf("your record is %d\n", freesnake());
		while(crush){
			char yorn;
            printf("you lose, please chose if you want start again(y or n) :");
			scanf("%c", &yorn);
			if (yorn == 'y'){
				crush = 0;
				break;
			}else if(yorn == 'n'){
				break;
			}else{
				printf("please input legal charactor !\n");
				continue;
			}
		}
	}
	freesnake();
	closekeyboard();

	return 0;
}
