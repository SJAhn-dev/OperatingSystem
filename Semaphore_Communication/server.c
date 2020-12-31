#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <semaphore.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>

#define BUF_SIZE	100
#define RBUF_SIZE	120
#define INPUT_FILE	"./input_temp"
#define RESULT_FILE	"./result_temp"
#define LOG_FILE 	"./log_temp"
#define USR_LIST	"./usr_list.txt"
#define SERVER_PID 	"./server_pid.txt"

void init_server();
void game_control();
void adjust_server();
void signalHandler(int sig);
void before_end();
void clientOut(int sig);

int fd_i;
int fd_r;
int fd_l;
int* ptr_i;
int* ptr_r;
int* ptr_l;
char buf[BUF_SIZE];
sem_t* server_control_sem;	// Server Control Semaphore
sem_t* client_count_sem;	// client count
sem_t* client_turn_sem;
sem_t* enter_permission_sem;
int game_playing = 0; 		// 0 : wait_to_play, 1 : playing
int count = 1;
int playerCount = 0;
int client_out = 0;
char pass_string[30] = "PASS\n";
char npass_string[30] = "FAIL\n";

int slot = 0;
int client_pid[2];

int main(void)
{
	atexit(before_end);
	signal(SIGINT,signalHandler);
	signal(SIGALRM,clientOut);
	printf("plaese wait until server is read...:");
	init_server();
	printf("Done\n");
	printf("Server %d is ready for play\n", getpid());

	FILE* fp = fopen(USR_LIST, "w");
	fclose(fp);
	
	while(1)
	{
		slot = 0;
		while(playerCount<2)
		{
			int enteredPlayer;
			sem_getvalue(client_count_sem, &enteredPlayer);

			if(enteredPlayer > playerCount)
			{
				memset(buf, 0x00, BUF_SIZE);
				read(fd_i, buf, BUF_SIZE);
				client_pid[slot] = atoi(buf);
				printf("Client %d hi with slot %d\n", client_pid[slot], slot);
				slot++;
				playerCount++;
			}
			int permission;
			sem_getvalue(enter_permission_sem, &permission);
			if(permission<1 && playerCount<2) { sem_post(enter_permission_sem); }
			else if(permission==1 && playerCount==2) { sem_wait(enter_permission_sem); }
		}
		game_playing = 1;
		game_control();
		sleep(1);
		adjust_server();
	}
}

void init_server()
{
	count = 1;
	mkfifo(INPUT_FILE, 0666);
	fd_i = open(INPUT_FILE, O_RDWR, S_IRWXU);
	ptr_i = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED, fd_i, 0);

	mkfifo(RESULT_FILE, 0666);
	fd_r = open(RESULT_FILE, O_RDWR, S_IRWXU);
	ptr_r = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED, fd_r, 0);
	
	mkfifo(LOG_FILE, 0666);
	fd_l = open(LOG_FILE, O_RDWR, S_IRWXU);

	sem_unlink("server_control_sem");
	if((server_control_sem = sem_open("server_control_sem", O_CREAT, 0644, 0)) == SEM_FAILED)
	{
		perror("open");
		exit(1);
	}

	sem_unlink("client_count_sem");
	if((client_count_sem = sem_open("client_count_sem", O_CREAT, 0644, 0)) == SEM_FAILED)
	{
		perror("open");
		exit(1);
	}

	sem_unlink("client_turn_sem");
	if((client_turn_sem = sem_open("client_turn_sem", O_CREAT, 0644, 0)) == SEM_FAILED)
	{
		perror("open");
		exit(1);
	}

	sem_unlink("enter_permission_sem");
	if((enter_permission_sem = sem_open("enter_permission_sem", O_CREAT, 0644, 2)) == SEM_FAILED)
	{
		perror("open");
		exit(1);
	}

	FILE* fp_sv = fopen(SERVER_PID, "w");
	char server_pid[10];
	sprintf(server_pid, "%d\n", getpid());
	fputs(server_pid, fp_sv);
	fclose(fp_sv);
}

void adjust_server()
{
	//printf("Adjust Server Please wait...:");
	count = 1;
	playerCount = 0;
	client_out = 0;
	int valueCheck;
	
	sem_getvalue(server_control_sem, &valueCheck);
	if(valueCheck > 0) { sem_wait(server_control_sem); }

	sem_getvalue(client_count_sem, &valueCheck);
	if(valueCheck == 2) { sem_wait(client_count_sem); valueCheck--; }
	if(valueCheck == 1) { sem_wait(client_count_sem); }

	sem_getvalue(client_turn_sem, &valueCheck);
	if(valueCheck == 1) { sem_wait(client_turn_sem); }

	sem_getvalue(enter_permission_sem, &valueCheck);
	if(valueCheck == 2) { sem_wait(enter_permission_sem); valueCheck--; }
	if(valueCheck == 1) { sem_wait(enter_permission_sem); }
	//printf("Done\n");
}

void game_control()
{
	int input;
	while(game_playing)
	{
		sem_wait(server_control_sem);
		if(client_out == 1) { return; }
		sem_getvalue(client_count_sem, &playerCount);
		if(playerCount<2)
		{
			memset(buf, 0x00, BUF_SIZE);
			read(fd_l, buf, BUF_SIZE);
			printf("Client %s is disconnected.\n", buf);
			printf("One of clients is disconnected.\n");
			game_playing = 0;
			playerCount = 0;
			break;
		}
		
		memset(buf, 0x00, BUF_SIZE);
		read(fd_i, buf, BUF_SIZE);
		
		char result[RBUF_SIZE];
		strcpy(result, buf);

		char* input_pid;
		int input_buf;
		char* token = strtok(buf, " ");
		
		input_pid = token;
		token = strtok(NULL, " ");
		input_buf = atoi(token);

		printf("%s %d\n", input_pid, input_buf);
		
		if( ( (count%10 == 3 || count%10 == 6 || count%10 == 9) && input_buf==-1) ||
		    ( (count%10 != 3 && count%10 != 6 && count%10 != 9) && input_buf==count) )
		{
			strcat(result,pass_string);
			write(fd_r, result, RBUF_SIZE);
			count++;
		}
		else
		{
			strcat(result,npass_string);
			write(fd_r, result, RBUF_SIZE);
			printf("\n Player %d lose!\n", client_pid[slot%2]);
		}
		slot++;
	}
}

void signalHandler(int sig)
{
	printf("\nServer is closing...: ");
	
	FILE *fp_ul = fopen(USR_LIST,"r");

	while(!feof(fp_ul))
	{
		char usr_pid[10];
		fgets(usr_pid, sizeof(usr_pid), fp_ul);
		int targetPid = atoi(usr_pid);
		if(targetPid >0) { kill(targetPid, SIGQUIT); }
	}
	fclose(fp_ul);
	printf("Done\n");

	before_end();
}

void clientOut(int sig)
{
	for(int i=0; i<2; i++)
	{
		if(!kill(client_pid[i], SIGALRM))
		{
			printf("Client %d out.\n", client_pid[i]);
		}
	}
	playerCount = 0;
	game_playing = 0;
	client_out = 1;
	
	int server_control_value;
	sem_getvalue(server_control_sem, &server_control_value);
	if(server_control_value==0)
	{
		sem_post(server_control_sem);
	}
}

void before_end()
{
	sem_unlink("server_control_sem");
	sem_unlink("client_count_sem");
	sem_unlink("client_turn_sem");
	exit(1);		
}
