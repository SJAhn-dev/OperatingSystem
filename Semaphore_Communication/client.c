#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <semaphore.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>

#define BUF_SIZE	60
#define RBUF_SIZE 	120
#define USR_BUFSIZE 10
#define INPUT_FILE	"./input_temp"
#define	RESULT_FILE	"./result_temp"
#define LOG_FILE	"./log_temp"
#define USR_LIST	"./usr_list.txt"
#define SERVER_PID	"./server_pid.txt"

void enter_server();
void print_error_connection();
void game_play();
void signalHandler(int sig);
void server_closed(int sig);
void another_clientout(int sig);
void wait_and_print();
void send_and_sleep();
void write_pid();
int get_serverPid();

int fd_i;
int fd_r;
int fd_l;
char buf[BUF_SIZE];
char rbuf[RBUF_SIZE];
sem_t* server_control_sem;
sem_t* client_count_sem;
sem_t* client_turn_sem;
sem_t* enter_permission_sem;
int count = 0;
int playerNo;
int playerId;
char playerId_c[10];
char pass_string[] = "PASS\n";
char client_pid[15] = "[";


int main(void)
{
	signal(SIGINT, signalHandler);
	signal(SIGQUIT, server_closed);
	signal(SIGALRM, another_clientout);
	printf("Waiting for server....\n");
	write_pid();
	enter_server();
	sem_wait(enter_permission_sem);
	//printf("Server Access Permission\n");
	game_play();
}

void enter_server()
{
	fd_i = open(INPUT_FILE, O_RDWR);
	fd_r = open(RESULT_FILE, O_RDWR);
	fd_l = open(LOG_FILE, O_RDWR);
	playerId = getpid();
	sprintf(playerId_c, "%d", playerId);

	if((server_control_sem = sem_open("server_control_sem", O_CREAT|O_EXCL)) != SEM_FAILED)
	{
		print_error_connection();
		exit(1);
	} else { server_control_sem = sem_open("server_control_sem", O_CREAT); }


	if((client_count_sem = sem_open("client_count_sem", O_CREAT|O_EXCL)) != SEM_FAILED)
	{
		print_error_connection();
		exit(1);
	} else { client_count_sem = sem_open("client_count_sem", O_CREAT); }

	if((client_turn_sem = sem_open("client_turn_sem", O_CREAT|O_EXCL)) != SEM_FAILED)
	{
		print_error_connection();
		exit(1);
	} else { client_turn_sem = sem_open("client_turn_sem", O_CREAT); }

	if((enter_permission_sem = sem_open("enter_permission_sem", O_CREAT|O_EXCL)) != SEM_FAILED)
	{
		print_error_connection();
		exit(1);
	} else { enter_permission_sem = sem_open("enter_permission_sem", O_CREAT); }

}

void game_play()
{
	int game_end = 0;
	int playerCount = 0;
	char print[3] = "] ";

	write(fd_i, playerId_c, sizeof(playerId_c));
	
	strcat(client_pid, playerId_c);
	strcat(client_pid, print);

	sem_post(client_count_sem);
	sem_getvalue(client_count_sem, &playerNo);
	
	while(playerCount!=2)
	{
		sem_getvalue(client_count_sem, &playerCount);
	}


	printf("Please wait for 3 seconds to synchronization.\n");

	printf("========== 3 6 9 G A M E ==========\n\n");
	printf("		Hello, player %d\n", playerId);
	printf("		Welcome to 369 game!\n\n");
	printf("======================================\n");
	printf("Please enter any key to start game\n");
	
	fgets(buf, BUF_SIZE, stdin);
	memset(buf, 0x00, BUF_SIZE);
	memset(rbuf, 0x00, RBUF_SIZE);
	
	if(playerNo==1)
	{
		while(!game_end)
		{
			wait_and_print();
			send_and_sleep();
		}
	}
	else
	{
		while(!game_end)
		{
			send_and_sleep();
			wait_and_print();
		}
	}
}

void send_and_sleep()
{
			int input_num;
			char input[68];
			printf("input num : ");
			memset(buf, 0x00, BUF_SIZE);
			fgets(buf, BUF_SIZE, stdin);

			strcpy(input, client_pid);
			strcat(input, buf);

			write(fd_i, input, strlen(input));

			printf("%s%s", client_pid, buf);

			sem_post(server_control_sem);
			sem_post(client_turn_sem);
			sleep(1);
}

void wait_and_print()
{
			int end_checker;
			sem_wait(client_turn_sem);

			sem_getvalue(client_count_sem, &end_checker);
			if(end_checker==1)
			{
				printf("You Lose!\n");
				printf("\nClient is closing...:");
				write(fd_l,playerId_c,BUF_SIZE);

				sem_trywait(client_count_sem);
				sem_post(server_control_sem);
				printf("Done\n");

				exit(0);
			}

			memset(buf, 0x00, BUF_SIZE);
			memset(rbuf, 0x00, RBUF_SIZE);
		
			read(fd_r, rbuf, RBUF_SIZE);
			
			char* result_input;
			char* result;

			char* token = strtok(rbuf, "\n");
			result_input = token;
			token = strtok(NULL, " ");
			result = token;

			printf("%s\n", rbuf);
			if(strcmp(pass_string,result))
			{
				printf("You Win!\n");
				sem_wait(client_count_sem);
				sem_post(client_turn_sem);
				exit(1);
			}

			memset(rbuf, 0x00, RBUF_SIZE);
}

void signalHandler(int sig)
{
	printf("\nClient is closing...: ");

	printf("Done\n");
	
	if(sig==SIGINT)
	{
		kill(get_serverPid(), SIGALRM);
		exit(1);
	}
}

void print_error_connection()
{
	printf("sem_open failed. (Number of players): No such file or directory\n");
	printf("An error is occured, tell admin..\n");
}

void server_closed(int sig)
{
	printf("Server is closed. Tell admin.\n");
	exit(0);
}

void write_pid()
{
	FILE* fp_ul = fopen(USR_LIST, "a+");
	char my_pid[10];
	sprintf(my_pid, "%d\n", getpid());
	fputs(my_pid, fp_ul);
	fclose(fp_ul);
}

void another_clientout(int sig)
{
	printf("You win!\n");
	printf("The other player is disconnected.\n");
	exit(0);
}

int get_serverPid()
{
	int serverPid;

	FILE* fp_sv = fopen(SERVER_PID,"r");

	char srv_pid[10];
	fgets(srv_pid, sizeof(srv_pid), fp_sv);
	serverPid = atoi(srv_pid);

	return serverPid;
}
