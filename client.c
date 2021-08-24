#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <stdio_ext.h>

/* Define the port to communication between clients when transferring the file (peer-to-peer) */
#define port_to_file 4444

/* Define the Buffer Size (bytes) to file transfer (default: 256 bytes)*/
#define buffer_size (256)

struct message
{
	int online;
	char username[20];
	char message[100];
	char receiver[20][20];
	int sizeoflist;
	char file_name[100];
	int port;
};
typedef struct message message;

struct file_data
{
	char file_name[100];
	char message[100];
	int port;
};
typedef struct file_data file_data;
file_data data_of_file;

void *recvmg(void *);
void *send_file();
void get_extension(char[], char[]);
int read_img(char[]);
char msg[1000];

void *recvmg(void *my_sock)
{
	int sock = *((int *)my_sock), len = 0;

	// client thread always ready to receive message
	while ((len = recv(sock, (char *)&data_of_file, sizeof(data_of_file), 0)) > 0)
	{
		data_of_file.message[len] = '\0';

		if (data_of_file.port != port_to_file)
			fputs(data_of_file.message, stdout);
		else
		{
			printf("\nReceiving the port of server to connect with the client: %s", data_of_file.message);
			int sock_file = 0, n_bytes_read = 0;
			char extension[20], file_name[100], *buffer = malloc(buffer_size);
			struct sockaddr_in client_address;
			FILE *output_file;

			get_extension(data_of_file.file_name, extension);
			strcpy(file_name, "new_file.");
			strcat(file_name, extension);
			file_name[strlen(file_name) - 1] = '\0';
			sock_file = socket(AF_INET, SOCK_STREAM, 0);
			client_address.sin_port = htons(data_of_file.port);
			client_address.sin_family = AF_INET;
			client_address.sin_addr.s_addr = htons(INADDR_ANY);

			if ((connect(sock_file, (struct sockaddr *)&client_address, sizeof(client_address))) == -1)
			{
				printf("\nConnection to socket failed!\n");
				exit(1);
			}
			output_file = fopen(file_name, "ab");
			if (output_file == NULL)
				printf("\nError in file read!\n");

			while ((n_bytes_read = read(sock_file, buffer, buffer_size)) > 0)
				fwrite(buffer, 1, n_bytes_read, output_file);
			if (n_bytes_read < 0)
				printf("\nError in file read or transfer\n");
			printf("\nFinished the file transfer or the reading with client!\n");
			fclose(output_file);
			close(sock_file);
			shutdown(sock_file, SHUT_WR);
		}
	}
}

void *send_file(char file_name[])
{
	int client_sock = 0, sock_file = 0, n_bytes_read = 0, i, loading = 0;
	long file_size, pos_curr, total_transferred = 0;
	char *buffer = malloc(buffer_size);
	struct sockaddr_in client_address;
	FILE *input_file;
	sock_file = socket(PF_INET, SOCK_STREAM, 0);
	client_address.sin_family = AF_INET;
	client_address.sin_port = htons(port_to_file);
	client_address.sin_addr.s_addr = htons(INADDR_ANY);
	if (bind(sock_file, (struct sockaddr *)&client_address, sizeof(client_address)) == -1)
		printf("\nCannot bind, error!\n");
	if (listen(sock_file, 20) == -1)
		printf("\nListening failed!\n");
	if ((client_sock = accept(sock_file, (struct sockaddr *)NULL, NULL)) < 0)
		printf("\nAccept failed!\n");

	file_name[strlen(file_name) - 1] = '\0';
	input_file = fopen(file_name, "rb");

	if (input_file == NULL)
		printf("\nError in file read!\n");

	// Get size of the file
	pos_curr = ftell(input_file);
	fseek(input_file, 0, SEEK_END);
	file_size = ftell(input_file);
	fseek(input_file, pos_curr, SEEK_SET);
	printf("\n[");

	while ((n_bytes_read = fread(buffer, 1, buffer_size, input_file)) > 0)
	{
		total_transferred = total_transferred + n_bytes_read;
		if (write(client_sock, buffer, n_bytes_read) < 0)
			printf("\nFile not sent!\n");
		//printf("\nFtotal_transferred: %ld bytes\n", total_transferred);
		loading = (int)((n_bytes_read * 500) / (file_size));
		for (i = 0; i < loading; i++)
		{
			printf("#");
			fflush(stdout);
		}
		if ((total_transferred + buffer_size) <= file_size)
			usleep(10000);
	}
	printf("] 100%% \n");
	if (feof(input_file))
		printf("\nFinished the file transfer or the sending with client!\n\n");
	if (ferror(input_file))
		printf("\nError in file read or transfer\n");
	fclose(input_file);
	close(client_sock);
	shutdown(client_sock, SHUT_WR);
}

void get_extension(char file_name[], char extension[])
{
	int i = 0, j = 0, name_tam = 0;
	while (file_name[i] != '.')
		i++;

	i++;
	while (file_name[i] != '\0')
	{
		extension[j] = file_name[i];
		i++;
		j++;
	}
	extension[j] = '\0';
}

int main(int argc, char *argv[])
{
	pthread_t recvt, send_file_t;
	int sock = 0, sock_file = 0, flag = 0, i = 0;
	char buffer_send_msg[1000], buffer_login[1000], user[20];

	message *login = (message *)buffer_login;
	message *send_msg = (message *)buffer_send_msg;
	send_msg->sizeoflist = 0;
	struct sockaddr_in server_ip;

	sock = socket(AF_INET, SOCK_STREAM, 0);
	server_ip.sin_port = htons(5050);
	server_ip.sin_family = AF_INET;
	server_ip.sin_addr.s_addr = htons(INADDR_ANY);

	if ((connect(sock, (struct sockaddr *)&server_ip, sizeof(server_ip))) == -1)
	{
		printf("\nConnection to socket failed!\n");
		exit(1);
	}

	printf("\nRegister your username: ");

	fgets(login->username, 20, stdin);
	login->online = 1;
	write(sock, buffer_login, sizeof(buffer_login));

	printf("\nTo specify the users you want send a message, type: 'specify';");
	printf("\nTo clear you contact chat, type 'reset';");
	printf("\nTo send a file to a user, type 'file';");
	printf("\nTo leave the chat, type: 'exit';\nTo see your contacts, type: 'contact';");
	printf("\nTo talk with anybody, type any message and press 'Enter'...\n\n");

	//creating a client thread which is always waiting for a message
	pthread_create(&recvt, NULL, (void *)recvmg, &sock);

	//ready to read a message from console
	while (1)
	{
		fgets(send_msg->message, 100, stdin);

		if (strncmp(send_msg->message, "specify", 7) == 0)
		{
			i = 0;
			send_msg->sizeoflist = 0;
			send_msg->online = 2;
			printf("\nType the username to add it in your userlist (Type -1 to exit).");
			printf("\nIf you don't remember an username, you can use the contact list command to check it.\n");
			do
			{
				fgets(user, 20, stdin);
				if (strncmp(user, "-1", 2) == 0)
					break;

				strcpy(send_msg->receiver[i], user);
				i++;
				send_msg->sizeoflist++;
			} while (1);
		}
		if (strncmp(send_msg->message, "file", 4) == 0)
		{
			send_msg->port = port_to_file;
			printf("\nType the file name 'with extension' that you want to send: ");
			fgets(send_msg->file_name, 20, stdin);
			send_msg->file_name[strlen(send_msg->file_name)] = '\0';

			//creating a client thread that will send a file
			pthread_create(&send_file_t, NULL, (void *)send_file, send_msg->file_name);

			printf("\nType the username that you want to send the file: ");
			fgets(send_msg->receiver[0], 20, stdin);
			send_msg->receiver[0][strlen(send_msg->receiver[0])] = '\0';

			if ((write(sock, buffer_send_msg, sizeof(buffer_send_msg))) < 0)
				printf("\nMessage not sent!\n");
			//thread is closed
			pthread_join(send_file_t, NULL);
		}
		if (strncmp(send_msg->message, "reset", 5) == 0)
		{
			send_msg->online = 1;
			send_msg->sizeoflist = 0;
		}
		if (strncmp(send_msg->message, "reset", 5) != 0 && strncmp(send_msg->message, "specify", 7) != 0 && strncmp(send_msg->message, "file", 4) != 0)
		{
			if ((write(sock, buffer_send_msg, sizeof(buffer_send_msg))) < 0)
				printf("\nMessage not sent!\n");
		}
		if (strncmp(send_msg->message, "exit", 4) == 0)
			break;
	}

	//thread is closed
	pthread_join(recvt, NULL);
	close(sock);
	return 0;
}
