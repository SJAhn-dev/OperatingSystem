#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <wiringPi.h>
#include <semaphore.h>
#include <pthread.h>

#define LED_R	3
#define LED_Y	2
#define LED_G	0
#define SW_R	6
#define SW_Y	5
#define SW_G	4
#define SW_W	27

int status;
static int retval = 999;
int user_input_count = 0;
int stage_count = 1;
int end_flag = 0;
int seqArray[5] = { 0, 0, 0, 0, 0 };
int inputArr[5] = { 0, 0, 0, 0, 0 };
int blink_end = 0;
sem_t sem;
pthread_t pt[2];
void init(void);
void off(void);
void blink(void);
void blink_all(void);
void control_part(void);
void end_Checker(void);
void clearArray(void);

int main(void){
	puts("Welcom To Memory Game! Please wait a second!\n");
    init();
	sem_init(&sem, 0, 0);
	puts("Are you ready?\n");
	pthread_create(&pt[0], NULL, (void*)control_part, NULL);
	pthread_create(&pt[1], NULL, (void*)end_Checker, NULL);
	pthread_join(pt[0], (void*)&status);
	pthread_join(pt[1], (void*)&status);

	if(end_flag){
		puts("Game end!\n");
	}

    return 0;
}

void control_part(void) {
	blink();
	while(!end_flag) {
		if(stage_count!=1) { sem_wait(&sem); }
		if(end_flag) { pthread_exit((void*)&retval); }
		printf("Stage %d, Good Luck!\n", stage_count);

		for(int i=0; i<stage_count; i++){
			int randVal = rand()%3 + 1;
			if(randVal==1) { randVal--; }
			seqArray[i] = randVal;
			digitalWrite(randVal, 1);
			delay(200);
			digitalWrite(randVal, 0);
			delay(200);
		}
		
		int input_count = 0;
		int white_input = 0;
		while(!white_input){
			if(input_count>6) { 
				end_flag=2; 
				sem_post(&sem);
				pthread_exit((void*)&retval);
			}
			if(digitalRead(SW_R) == 0) {
				digitalWrite(LED_R, 1);
				inputArr[input_count] = LED_R;
				input_count++;
				while(!digitalRead(SW_R)) { delay(1); }
			}
			else if(digitalRead(SW_Y) == 0) {
				digitalWrite(LED_Y, 1);
				inputArr[input_count] = LED_Y;
				input_count++;
				while(!digitalRead(SW_Y)) { delay(1); }
			}
			else if(digitalRead(SW_G) == 0) {
				digitalWrite(LED_G, 1);
				inputArr[input_count] = LED_G;
				input_count++;
				while(!digitalRead(SW_G)) { delay(1); }
			}
			else if(digitalRead(SW_W) == 0) {
				white_input = 1;
				while(!digitalRead(SW_W)) { delay(1); }
			}
			off();
		}
		stage_count++;
		sem_post(&sem);
		sleep(1);
	}
}

void end_Checker(void) {
	while(!end_flag){
		sem_wait(&sem);
		if(end_flag==2) {
			puts("Too much Input!\n");
			blink_all();
			end_flag=1;
			while(1) { if(blink_end) { pthread_exit((void*)&retval); }}
		}
		int pass = 1;
		for(int i=0; i<=stage_count-2; i++){
			if(seqArray[i] != inputArr[i]){
				pass = 0;
			}
		}
		if(pass) {
			printf("Well Done!\n");
			if(stage_count>=6){
				end_flag = 1;
				sem_post(&sem);
				puts("You Win!!\n");
				blink();
				pthread_exit((void*)&retval);
			}
		} else {
			end_flag = 1;
			sem_post(&sem);
			puts("You Lose... Try Again\n");
			blink_all();
			pthread_exit((void*)&retval);
		}
		sem_post(&sem);
		sleep(1);
		if(end_flag) { pthread_exit((void*)&retval); }
	}
}

void clearArray(void) {
	for(int i=0; i<stage_count-1; i++){
		seqArray[i] = -1;
		inputArr[i] = -2;
	}
}

void init(void) {
    if(wiringPiSetup() == -1){
		puts("Setup Fail");
		exit(1);
    }
	pinMode(SW_R, INPUT);
	pinMode(SW_Y, INPUT);
	pinMode(SW_G, INPUT);
	pinMode(LED_R, OUTPUT);
	pinMode(LED_Y, OUTPUT);
	pinMode(LED_G, OUTPUT);
	off();
}

void off(void){
	digitalWrite(LED_R, 0);
	digitalWrite(LED_Y, 0);
	digitalWrite(LED_G, 0);
}

void blink(void){
	int i = 0;
	while(i<3){
		digitalWrite(LED_R, 1);
		delay(250);
		digitalWrite(LED_R, 0);

		digitalWrite(LED_Y, 1);
		delay(250);
		digitalWrite(LED_Y, 0);

		digitalWrite(LED_G, 1);
		delay(250);
		digitalWrite(LED_G, 0);
		i++;
	}
}

void blink_all(void){
	for(int i = 0; i<3; i++) {
		digitalWrite(LED_R, 1);
		digitalWrite(LED_Y, 1);
		digitalWrite(LED_G, 1);

		delay(500);

		digitalWrite(LED_R, 0);
		digitalWrite(LED_Y, 0);
		digitalWrite(LED_G, 0);

		delay(500);
	}
	blink_end=1;
}

