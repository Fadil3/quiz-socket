#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include <sys/types.h>
#include <signal.h>

#define MAX_CLIENTS 3 // maksimal pemain yang bisa gabung
#define BUFFER_SZ 2048

static _Atomic unsigned int cli_count = 0;
static int uid = 10;

/* Client structure */
typedef struct
{
	struct sockaddr_in address;
	int sockfd;
	int uid;
	char name[32];
	int skor;
	int ans;
} client_t;

int start = 0;
char correct[2];
int next = 0;
int soal = 0;
client_t *clients[MAX_CLIENTS];

pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;

void inc_next()
{
	pthread_mutex_lock(&clients_mutex);
	next++;
	pthread_mutex_unlock(&clients_mutex);
}
void str_overwrite_stdout()
{
	printf("\r%s", "> ");
	fflush(stdout);
}

// menghapus panjang string
void str_trim_lf(char *arr, int length)
{
	int i;
	for (i = 0; i < length; i++)
	{ // trim \n
		if (arr[i] == '\n')
		{
			arr[i] = '\0';
			break;
		}
	}
}

void print_client_addr(struct sockaddr_in addr)
{
	printf("%d.%d.%d.%d",
		   addr.sin_addr.s_addr & 0xff,
		   (addr.sin_addr.s_addr & 0xff00) >> 8,
		   (addr.sin_addr.s_addr & 0xff0000) >> 16,
		   (addr.sin_addr.s_addr & 0xff000000) >> 24);
}

/* Add clients to queue */
void queue_add(client_t *cl)
{
	pthread_mutex_lock(&clients_mutex);

	for (int i = 0; i < MAX_CLIENTS; ++i)
	{
		if (!clients[i])
		{
			clients[i] = cl;
			break;
		}
	}

	pthread_mutex_unlock(&clients_mutex);
}

/* Remove clients to queue */
void queue_remove(int uid)
{
	pthread_mutex_lock(&clients_mutex);

	for (int i = 0; i < MAX_CLIENTS; ++i)
	{
		if (clients[i])
		{
			if (clients[i]->uid == uid)
			{
				clients[i] = NULL;
				break;
			}
		}
	}

	pthread_mutex_unlock(&clients_mutex);
}

/* Send message to all clients except sender */
void send_message(char *s, int uid)
{
	pthread_mutex_lock(&clients_mutex);

	for (int i = 0; i < MAX_CLIENTS; ++i)
	{
		if (clients[i])
		{
			if (clients[i]->uid == uid)
			{
				if (write(clients[i]->sockfd, s, strlen(s)) < 0)
				{
					perror("ERROR: write to descriptor failed");
					break;
				}
			}
		}
	}

	pthread_mutex_unlock(&clients_mutex);
}

/*
 *  strtoken.c
 *
 *  Copyleft (C) 2015  Sun Dro (a.k.a. kala13x)
 *
 * This source is thread safe alternative of the strtok().
 * See usage of the get_token() below at main() function.
 */
/*
 * get_token() - Function breaks a string into a sequence of zero or more 
 * nonempty tokens. On the first call to get_token() func the string to be 
 * parsed should be specified in psrc. In each subsequent call that should 
 * parse the same string, psrc must be NULL. The delimit argument specifies 
 * a set of bytes that delimit the tokens in the parsed string. Functions 
 * return a pointer to the next token, or NULL if there are no more tokens.
 */

// LINK REPOSITORY : https://gist.github.com/kala13x/20ee37e9a4c3d094d9ac
char *get_token(char *psrc, const char *delimit, void *psave)
{
	static char sret[512];
	register char *ptr = psave;
	memset(sret, 0, sizeof(sret));

	if (psrc != NULL)
		strcpy(ptr, psrc);
	if (ptr == NULL)
		return NULL;

	int i = 0, nlength = strlen(ptr);
	for (i = 0; i < nlength; i++)
	{
		if (ptr[i] == delimit[0])
			break;
		if (ptr[i] == delimit[1])
		{
			ptr = NULL;
			break;
		}
		sret[i] = ptr[i];
	}
	if (ptr != NULL)
		strcpy(ptr, &ptr[i + 1]);

	return sret;
}

int count_score(int elapsed)
{
	int skor = 0;
	skor += 50 + 10 * (30 - elapsed);
	return skor;
}

/* Handle all communication with the client */
void *handle_client(void *arg)
{
	char buff_out[BUFFER_SZ];
	char name[32];
	int leave_flag = 0;
	int elapsed = 0;
	char answer[50];
	char saveptr[32];
	char delimit[2] = {'-', '\0'};
	int temp_skor = 0;
	char kirim_skor[50];

	cli_count++;
	client_t *cli = (client_t *)arg;

	// Name
	if (recv(cli->sockfd, name, 32, 0) <= 0 || strlen(name) < 2 || strlen(name) >= 32 - 1)
	{
		printf("Didn't enter the name.\n");
		leave_flag = 1;
	}
	else
	{
		strcpy(cli->name, name);
		sprintf(buff_out, "%s has joined\n", cli->name);
		printf("%s", buff_out);
		send_message(buff_out, cli->uid);
	}

	bzero(buff_out, BUFFER_SZ);

	while (1)
	{
		if (leave_flag)
		{
			break;
		}

		int receive = recv(cli->sockfd, buff_out, BUFFER_SZ, 0);
		if (receive > 0)
		{
			if (strlen(buff_out) > 0)
			{
				if (start == 1)
				{
					printf("buff out %s\n", buff_out);
					char *ptr = get_token(buff_out, delimit, &saveptr);
					printf("1: %s\n", ptr);

					ptr = get_token(NULL, delimit, &saveptr);
					printf("2: %s\n", ptr);

					int x = atoi(ptr);
					printf("elapsed %d\n", x);

					if (cli->ans < soal)
					{

						inc_next();
						cli->ans++;
						if (correct[0] == buff_out[strlen(cli->name) + 2])
						{
							temp_skor = count_score(x);
							printf("skor %d\n", temp_skor);
							cli->skor += temp_skor;

							//kirim skor ke lawan
							send_message(cli->name, cli->uid);
							send_message(" ", cli->uid);
							sprintf(kirim_skor, "%d", cli->skor); // convert int to string
							send_message(kirim_skor, cli->uid);
							send_message("\n", cli->uid);
							printf("%d", cli->skor);

							send_message("jawaban kamu benar\n", cli->uid);
						}
						else
						{
							send_message("\njawaban salah\n", cli->uid);
						}
					}
					else
					{
						send_message("anda sudah menjawab soal, tunggu player lain\n", cli->uid);
					}
				}
			}
		}
		else if (receive == 0 || strcmp(buff_out, "exit") == 0)
		{
			sprintf(buff_out, "%s has left\n", cli->name);
			printf("%s", buff_out);
			send_message(buff_out, cli->uid);
			leave_flag = 1;
		}
		else
		{
			printf("ERROR: -1\n");
			leave_flag = 1;
		}

		bzero(buff_out, BUFFER_SZ);
	}

	/* Delete client from queue and yield thread */
	close(cli->sockfd);
	queue_remove(cli->uid);
	free(cli);
	// realloc(cli, 0);
	cli_count--;
	pthread_detach(pthread_self());

	return NULL;
}

//printf("masuk\n");
char pertanyaan[30][20];
char option[5][100];
// char abjad[4][4] = {"A.", "B.", "C.", "D."};
char temp[100];
char jeda[3] = {"\n"};

FILE *fsoal;

void send_question(){
	int j = 0;
	for (int i = 0; i < MAX_CLIENTS; ++i)
	{
		if (clients[i])
		{
			j = 0;

			while (strcmp(pertanyaan[j], "##") != 0)
			{
				strcat(pertanyaan[j], " ");
				if (write(clients[i]->sockfd, pertanyaan[j], strlen(pertanyaan[j])) < 0)
				{
					perror("ERROR: write to descriptor failed");
					break;
				}
				j++;
			}
			write(clients[i]->sockfd, jeda, strlen(jeda));

			for (int k = 0; k < 4; k++)
			{
				write(clients[i]->sockfd, option[k], strlen(option[k]));
			}
		}
	}
}

void copy_question(){
	int j = 0;

	fscanf(fsoal, "%s", pertanyaan[j]);
	while (strcmp(pertanyaan[j], "##") != 0)
	{
		j++;
		fscanf(fsoal, "%s", pertanyaan[j]);
	}
	for (j = 0; j < 4; j++)
	{
		fscanf(fsoal, "%s", option[j]);
		strcat(option[j], "\n");
	}

	fscanf(fsoal, " %s", correct);
	//fclose(fsoal);
}
void next_question(){
	for (int i = 0; i < MAX_CLIENTS; ++i){
		if (clients[i]){
			char *pesan;
			pesan = "Soal Selanjutnya...\n";
			write(clients[i]->sockfd, pesan, strlen(pesan));
		}
	}

}

void quiz2(){
	fsoal = fopen("soal2.txt", "r");
	copy_question();
	send_question();
	fclose(fsoal);
	soal++;
}
void quiz3(){
	fsoal = fopen("soal3.txt", "r");
	copy_question();
	send_question();
	fclose(fsoal);
	soal++;
}
void quiz4(){
	fsoal = fopen("soal4.txt", "r");
	copy_question();
	send_question();
	fclose(fsoal);
	soal++;
}
void quiz5(){
	fsoal = fopen("soal5.txt", "r");
	copy_question();
	send_question();
	fclose(fsoal);
	soal++;
}
void viewscoreboard(){
	char *pesan;
	for (int i = 0; i < MAX_CLIENTS; ++i){
		if (clients[i]){
			if (soal == 5){
				pesan = "====Papan Skor Akhir===\n";
				write(clients[i]->sockfd, pesan, strlen(pesan));
			}else{
				pesan = "====Papan Skor Sementara===\n";
				write(clients[i]->sockfd, pesan, strlen(pesan));
			}
			if(clients[0]->skor > clients[1]->skor){
				write(clients[i]->sockfd, clients[0]->name, strlen(clients[0]->name));
				pesan = " : ";
				write(clients[i]->sockfd, pesan, strlen(pesan));
				char temp[32];
				sprintf(temp,"%d",clients[0]->skor);
				write(clients[i]->sockfd, temp, strlen(temp));
				pesan = "\n";
				write(clients[i]->sockfd, pesan, strlen(pesan));
				write(clients[i]->sockfd, clients[1]->name, strlen(clients[1]->name));
				pesan = " : ";
				write(clients[i]->sockfd, pesan, strlen(pesan));
				sprintf(temp,"%d",clients[1]->skor);
				write(clients[i]->sockfd, temp, strlen(temp));
				pesan = "\n";
				write(clients[i]->sockfd, pesan, strlen(pesan));
				if (soal == 5){
					pesan = "Pemenanganya adalah ";
					write(clients[i]->sockfd, pesan, strlen(pesan));
					write(clients[i]->sockfd, clients[0]->name, strlen(clients[0]->name));
					pesan = "\n";
					write(clients[i]->sockfd, pesan, strlen(pesan));
				}
			}else{
				write(clients[i]->sockfd, clients[1]->name, strlen(clients[1]->name));
				pesan = " : ";
				write(clients[i]->sockfd, pesan, strlen(pesan));
				char temp[32];
				sprintf(temp,"%d",clients[1]->skor);
				write(clients[i]->sockfd, temp, strlen(temp));
				pesan = "\n";
				write(clients[i]->sockfd, pesan, strlen(pesan));

				write(clients[i]->sockfd, clients[0]->name, strlen(clients[0]->name));
				pesan = " : ";
				write(clients[i]->sockfd, pesan, strlen(pesan));
				sprintf(temp,"%d",clients[0]->skor);
				write(clients[i]->sockfd, temp, strlen(temp));
				pesan = "\n";
				write(clients[i]->sockfd, pesan, strlen(pesan));
				if (soal == 5){
					pesan = "Pemenanganya adalah ";
					write(clients[i]->sockfd, pesan, strlen(pesan));
					write(clients[i]->sockfd, clients[1]->name, strlen(clients[1]->name));
					pesan = "\n";
					write(clients[i]->sockfd, pesan, strlen(pesan));
				}
			}
		}
	}
	printf("vskr\n");
}
void *handle_quiz(void *arg){
	while(1){
		if(next == 2 && soal == 1){
			next_question();
			sleep(1);
			quiz2();
			next = 0;
		}
		if(next == 2 && soal == 2){
			viewscoreboard();
			next_question();
			sleep(2);
			quiz3();
			next = 0;
		}
		if(next == 2 && soal == 3){
			viewscoreboard();
			next_question();
			sleep(2);
			quiz4();
			next = 0;
		}
		if(next == 2 && soal == 4){
			viewscoreboard();
			next_question();
			sleep(1);
			quiz5();
			next = 0;
		}
		if(next == 2 && soal == 5){
			sleep(2);
			viewscoreboard();
			next = 0;
		}
	}
}
pthread_t qid;

void quiz(){
	for (int i = 0; i < MAX_CLIENTS; ++i){
		if (clients[i]){
			char *pesan;
			pesan = "Quiz akan dimulai\n";
			write(clients[i]->sockfd, pesan, strlen(pesan));
		}
	}
	sleep(1);
	fsoal = fopen("soal1.txt", "r");
	copy_question();
	send_question();
	fclose(fsoal);
	soal++;
	pthread_create(&qid, NULL, &handle_quiz, NULL);
}


int main(int argc, char **argv)
{
	int tes = 0;
	char *pesan;
	if (argc != 2)
	{
		printf("Usage: %s <port>\n", argv[0]);
		return EXIT_FAILURE;
	}

	char *ip = "127.0.0.1"; // inisialisasi ip yang digunakan

	int port = atoi(argv[1]); // convert string to integer

	int option = 1;
	int listenfd = 0, connfd = 0;

	struct sockaddr_in serv_addr; //server addres
	struct sockaddr_in cli_addr;  //client addres

	pthread_t tid;

	/* Socket settings */
	listenfd = socket(AF_INET, SOCK_STREAM, 0);
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = inet_addr(ip);

	serv_addr.sin_port = htons(port); //bind port and ip

	if (setsockopt(listenfd, SOL_SOCKET, (SO_REUSEPORT | SO_REUSEADDR), (char *)&option, sizeof(option)) < 0)
	{
		perror("ERROR: setsockopt failed");
		return EXIT_FAILURE;
	}

	/* Bind */
	if (bind(listenfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
	{
		perror("ERROR: Socket binding failed");
		return EXIT_FAILURE;
	}

	/* Listen */
	if (listen(listenfd, 10) < 0)
	{
		perror("ERROR: Socket listening failed");
		return EXIT_FAILURE;
	}

	printf("=== Selamat datang di server side kuis Kahoot ===\n");

	while (1)
	{
		socklen_t clilen = sizeof(cli_addr);
		connfd = accept(listenfd, (struct sockaddr *)&cli_addr, &clilen);

		/* Check if max clients is reached */
		if ((cli_count + 1) == MAX_CLIENTS)
		{
			printf("Peserta sudah penuh. Silahkan coba beberapa saat lagi.");
			print_client_addr(cli_addr);
			printf(":%d\n", cli_addr.sin_port);
			close(connfd);

			continue;
		}
		pesan = "semua player telah bergabung.\n";

		/* Client settings */
		client_t *cli = (client_t *)malloc(sizeof(client_t));
		cli->address = cli_addr;
		cli->sockfd = connfd;
		cli->uid = uid++;
		cli->ans = 0;

		/* Add client to the queue and fork thread */
		queue_add(cli);
		pthread_create(&tid, NULL, &handle_client, (void *)cli);

		/* Reduce CPU usage */
		sleep(1);
		if (cli_count == 2)
		{
			quiz();
			start = 1;
		}
	}

	return EXIT_SUCCESS;
}