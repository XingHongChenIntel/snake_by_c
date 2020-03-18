#include<stdlib.h>
#include<stdio.h>
#include<string.h>
#include<unistd.h>
#include<termio.h>
#include<pthread.h>
#include<time.h>

#define Width 65
#define Heigh 25
static struct termios new_settings,stored_settings;
static int keyNumber;
static pthread_mutex_t mutex;
static pthread_mutex_t mutexsuspend;
static pthread_cond_t cond;
pthread_t inputpipe;
pthread_t paintpipe;
char pizz[Heigh][Width+1];

static unsigned int speed = 1000000;
int crush = 0;
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

void *inputmodel(void *arg){
	while(1){
		int temp = getchar();
		//printf("input chang to %d\n", temp);
		pthread_mutex_lock(&mutex);
		keyNumber = temp;
		//if(keyNumber == 27){
			//pthread_mutex_unlock(&mutex);
			//break;
		//}
		pthread_mutex_unlock(&mutex);
	}
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
		return 0;
	}
	return 0;
}

void printsnake(){
	snakebody* temp = head;
	int tempx=0;
	int tempy=0;
	int tailx=0;
	int taily=0;
	int checktag = stridexmap[keyvalue]+head->stridex | strideymap[keyvalue] +head->stridey;
	while(temp){
		if (temp == head){
			tempx = temp->stridex;
			tempy = temp->stridey;
			if (!checktag){
				temp->x += temp->stridex;
				temp->y += temp->stridey;
			}else{
				temp->stridex = stridexmap[keyvalue];
				temp->stridey = strideymap[keyvalue];
				temp->x += temp->stridex;
				temp->y += temp->stridey;
			}
			temp = temp->next;
			continue;
		}
		int bodystridex = temp->stridex;
		int bodystridey = temp->stridey;
		temp->stridex = tempx;
		temp->stridey = tempy;
		if(!temp->next){
			pizz[temp->x][temp->y] = ' ';
			tailx = temp->x;
			taily = temp->y;
		}
		temp->x += temp->stridex;
		temp->y += temp->stridey;
		tempx = bodystridex;
		tempy = bodystridey;
		temp = temp->next;
	}

	if (pizz[head->x][head->y] == 'o'){
		snakebody *tail = (snakebody*)malloc(sizeof(snakebody));
		tail->x = tailx;
		tail->y = taily;
		tail->stridex = tempx;
		tail->stridey = tempy;
		tail->next = NULL;
		temp = head;
		while(temp->next) {temp = temp->next;}
		temp->next = tail;
		while(genbean()){}
	}else if (pizz[head->x][head->y]!=' '){
		crush = 1;
	}

	temp = head->next;
	while(temp){
		if (temp->x == head->x && temp->y == head->y){
                	crush = 1;
			break;
		}
		temp = temp->next;
	}
	
	if (crush){
		char *endlog = "                 your  are   loser !!!!!!!!           ";
		memcpy(pizz[Heigh/2], endlog, strlen(endlog));
	}else{
        	temp = head;
		while(temp){
                	pizz[temp->x][temp->y] = 'O';
                	temp = temp->next;
        	}
	}
	for(int i=0; i<Heigh; i++){
		printf("%s\n",pizz[i]);
	}

}

void initpizz(){
	initkeyboard();
	keyNumber = 0;
	keyvalue = left;
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
	int startx = Heigh/2;
	int starty = Width/2;
	head = (snakebody *)malloc(sizeof(snakebody));
	head->x = startx;
	head->y = starty;
	head->stridex = 0;
	head->stridey = -1;
	head->next = NULL;
	snakebody* temp = head;
	for (int i=1; i<5; i++){
		snakebody* body = (snakebody*)malloc(sizeof(snakebody));
		body->x = startx;
		body->y = starty+i;
		body->stridex = 0;
		body->stridey = -1;
		body->next = NULL;
		temp->next = body;
		temp = body;
	}
	temp = head;
	for(int i=0; i<5; i++){
		int x = temp->x;
		int y = temp->y;
		pizz[x][y] = 'O';
		temp = temp->next;
	}
	while(genbean()){}
}

void *paint(void *arg){
	//char pizz[Width][Heigh];
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

int main(int argv, char *argc[]){
	int ret;
	pthread_mutex_init(&mutex, NULL);
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
	if (!strcmp(diffcuty, "easy")) speed = 1000000;
	if (!strcmp(diffcuty, "medium")) speed = 500000;
	if (!strcmp(diffcuty, "hard")) speed = 200000;
	if (!strcmp(diffcuty, "master")) speed = 100000;
	while(!crush){
		if(ret = pthread_create(&inputpipe, NULL, inputmodel, NULL)){
			printf("creat inputpipe thread failed, %s\n", strerror(ret));
			return ret;
		}
		if(ret = pthread_create(&paintpipe, NULL, paint, NULL)){
			printf("creat painpipe thread failed, %s\n", strerror(ret));
			return ret;
		}
		pthread_join(paintpipe, NULL);
		pthread_cancel(inputpipe);
		if(!crush){
			break;
		}
		int record = 0;
		snakebody *temp;
		while(head){
			temp = head->next;
			free(head);
			head=temp;
			record++;
		}
		printf("your record is %d\n", record);
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
	snakebody *temp;
	while(head){
		temp = head->next;
		free(head);
		head = temp;
		
	}
	closekeyboard();

	return 0;
}
