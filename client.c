#include <stdio.h>
#include <time.h>
#include <sys/time.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>

#define LENGTH 2048

// Global variables
volatile sig_atomic_t flag = 0;
int sockfd = 0;
char name[32];

// waktu untuk perhitungan skor
struct timeval waktu;
time_t mulai, selesai;
int elapsed;

// menghapus \n
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

void catch_ctrl_c_and_exit(int sig)
{
	flag = 1;
}

void send_msg_handler()
{
	char message[LENGTH] = {};
	char buffer[LENGTH + 32] = {};
	char tampungan[100];
	int i = 0;

	while (1)
	{

		//inisiasi waktu mulai
		gettimeofday(&waktu, NULL);
		mulai = waktu.tv_sec;

		fgets(message, LENGTH, stdin); //ambil input

		//inisiasi waktu akhir
		gettimeofday(&waktu, NULL);
		selesai = waktu.tv_sec;

		//menghitung selisih waktu mulai dan akhir
		elapsed = difftime(selesai, mulai);

		str_trim_lf(message, LENGTH);

		sprintf(buffer, "%d", elapsed); // convert int to string
		strcat(message, "-");
		strcat(message, buffer);

		if (strcmp(message, "exit") == 0)
		{
			break;
		}
		else
		{
			//convert integer to string in C

			if (i == 0)
			{
				strcpy(tampungan, strcat(name, ": "));
				i++;
			}
			else
			{
				strcpy(tampungan, name);
			}
			strcpy(tampungan, strcat(tampungan, message));
			sprintf(buffer, "%s\n", tampungan);
			send(sockfd, buffer, strlen(buffer), 0);
		}

		bzero(message, LENGTH);
		bzero(buffer, LENGTH + 32);
	}
	catch_ctrl_c_and_exit(2);
}

void recv_msg_handler()
{
	char message[LENGTH] = {};
	while (1)
	{
		int receive = recv(sockfd, message, LENGTH, 0);
		if (receive > 0)
		{
			printf("%s", message);
		}
		else if (receive == 0)
		{
			break;
		}
		else
		{
			// -1
		}
		memset(message, 0, sizeof(message));
	}
}

int main(int argc, char **argv)
{
	if (argc != 2)
	{
		printf("Usage: %s <port>\n", argv[0]);
		return EXIT_FAILURE;
	}

	char *ip = "128.199.244.249";
	// char *ip = "127.0.0.1";
	int port = atoi(argv[1]); // convert string to int

	signal(SIGINT, catch_ctrl_c_and_exit); // menerima signal ctrl+c

	printf("Please enter your name: ");
	fgets(name, 32, stdin);
	str_trim_lf(name, strlen(name));

	if (strlen(name) > 32 || strlen(name) < 2)
	{
		printf("Name tidak boleh lebih dari 32 karakter dan kurang dari 2 karakter.\n");
		return EXIT_FAILURE;
	}

	struct sockaddr_in server_addr;

	/* Socket settings */
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = inet_addr(ip); // ip address yang digunakan

	/*htons() Fungsi htons() berasal dari host (h), 
	menjadi (to), network (n), short (s).*/
	server_addr.sin_port = htons(port); // port yang digunakan

	// Connect to Server
	int err = connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr));
	if (err == -1)
	{
		printf("ERROR: connect\n");
		return EXIT_FAILURE;
	}

	// Send name
	send(sockfd, name, 32, 0);

	printf("=== Selamat datang di client side kuis Kahoot ===\n");

	//generate thread send message
	pthread_t send_msg_thread;
	if (pthread_create(&send_msg_thread, NULL, (void *)send_msg_handler, NULL) != 0)
	{
		printf("ERROR: pthread\n");
		return EXIT_FAILURE;
	}

	//generate thread receive message
	pthread_t recv_msg_thread;
	if (pthread_create(&recv_msg_thread, NULL, (void *)recv_msg_handler, NULL) != 0)
	{
		printf("ERROR: pthread\n");
		return EXIT_FAILURE;
	}

	while (1)
	{
		if (flag) // kalau signal ctrl + c masuk
		{
			printf("\nBye\n");
			break;
		}
	}

	//menutup socket
	close(sockfd);

	return EXIT_SUCCESS;
}
