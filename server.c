#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <unistd.h>

pthread_mutex_t mutex;
int n = 0;
int m = 0;
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

struct client
{
    int sock;
    char username[20];
    int online;
};
typedef struct client user;

struct thread
{
    int sock;
    char name[20];
};
typedef struct thread thread;

struct unreaded_message
{
    char name[20];
    char message[100];
};
typedef struct unreaded_message unreaded_message;

struct file_data
{
    char file_name[100];
    char message[100];
    int port;
};
typedef struct file_data file_data;
file_data data_of_file;

unreaded_message list_unreaded_message[100];
user client[50];

void concatenate_user(char [], char [], char []);
void send_to_all(message *, int , char *);
void send_to_specific(message *, int , char *);
void send_file_data(message *, int , char *);
void send_to_offline(message *, int );

void concatenate_user(char text[], char user[], char res[])
{
    res[0] = '\0';
    strcat(res, "User ");
    strcat(res, user);
    res[strlen(res) - 1] = '\0';
    strcat(res, " said: ");
    strcat(res, text);
    res[strlen(res)] = '\0';
}

void send_to_all(message *msg, int curr, char *name)
{
    int i;
    pthread_mutex_lock(&mutex);
    send_to_offline(msg, curr);
    concatenate_user(msg->message, name, data_of_file.message);

    for (i = 0; i < n; i++)
    {
        if (client[i].sock != curr && client[i].online == 1)
        {
            if (send(client[i].sock, (char *)&data_of_file, sizeof(data_of_file), 0) < 0)
            {
                printf("\nSending failure\n");
                continue;
            }
        }
    }
    pthread_mutex_unlock(&mutex);
}

void send_to_specific(message *msg, int curr, char *name)
{
    int i;
    pthread_mutex_lock(&mutex);
    send_to_offline(msg, curr);
    concatenate_user(msg->message, name, data_of_file.message);

    for (i = 0; i < msg->sizeoflist; i++)
    {
        for (int j = 0; j < n; j++)
        {
            if (strcmp(msg->receiver[i], client[j].username) == 0 && client[j].sock != curr)
            {
                if (client[j].online == 1)
                {
                    if (send(client[j].sock, (char *)&data_of_file, sizeof(data_of_file), 0) < 0)
                    {
                        printf("\nSending failure\n");
                        continue;
                    }
                }
                else
                {
                    strcpy(list_unreaded_message[m].name, client[j].username);
                    strcpy(list_unreaded_message[m].message, data_of_file.message);
                    printf("\n Message save. Because is offline the user: %s", list_unreaded_message[m].name);
                    m++;
                }
            }
        }
    }
    pthread_mutex_unlock(&mutex);
}

void send_file_data(message *msg, int curr, char *name)
{
    pthread_mutex_lock(&mutex);

    int i = 0;
    strcpy(data_of_file.file_name, msg->file_name);
    strcpy(data_of_file.message, msg->username);
    strcpy(data_of_file.message, name);
    data_of_file.port = msg->port;

    for (i = 0; i < n; i++)
    {
        if (strcmp(msg->receiver[0], client[i].username) == 0 && client[i].sock != curr)
        {
            if (client[i].online == 1)
            {
                printf("\nSending the port to the client: %s", client[i].username);
                if (send(client[i].sock, (char *)&data_of_file, sizeof(data_of_file), 0) < 0)
                {
                    printf("\nSending failure \n");
                    continue;
                }
            }
        }
    }
    pthread_mutex_unlock(&mutex);
}

void send_to_offline(message *msg, int curr)
{
    for (int i = 0; i < m; i++)
    {
        for (int j = 0; j < n; j++)
        {
            strcpy(data_of_file.message, list_unreaded_message[i].message);
            if (strcmp(client[j].username, list_unreaded_message[i].name) == 0 && client[j].online == 1)
            {
                if (send(client[j].sock, (char *)&data_of_file, sizeof(data_of_file), 0) < 0)
                {
                    printf("\nSending failure\n");
                    continue;
                }
                strcpy(list_unreaded_message[i].name, "-1");
            }
        }
    }
}

void send_contact(int curr)
{
    int i = 0, size;
    pthread_mutex_lock(&mutex);
    //  char msg[100];
    strcpy(data_of_file.message, "\n\t     Contacts \n---------------------------------- \n");
    send(curr, (char *)&data_of_file, sizeof(data_of_file), 0);
    for (int i = 0; i < n; i++)
    {
        if (client[i].sock != curr)
        {
            strcpy(data_of_file.message, "User: ");
            strcat(data_of_file.message, client[i].username);
            data_of_file.message[strlen(data_of_file.message) - 1] = '\0';
            send(curr, (char *)&data_of_file, sizeof(data_of_file), 0);
            if (client[i].online == 1)
            {
                strcpy(data_of_file.message, " - Online");
            }
            else
            {
                strcpy(data_of_file.message, " - Offline");
            }
            strcat(data_of_file.message, "\n---------------------------------- \n");
            send(curr, (char *)&data_of_file, sizeof(data_of_file), 0);
        }
    }
    pthread_mutex_unlock(&mutex);
}

void disconnect_user(int sock)
{
    int i = 0;
    FILE *fptr;
    pthread_mutex_lock(&mutex);
    while (sock != client[i].sock)
    {
        i++;
    }
    client[i].online = 0;
    fptr = fopen("data.txt", "w");
    fwrite(&client, sizeof(user), n, fptr);
    fclose(fptr);
    close(sock);
    pthread_mutex_unlock(&mutex);
    pthread_cancel(pthread_self());
}

void *recv_msg(void *client_name)
{
    thread *client_data = (thread *)client_name;
    int sock = client_data->sock;
    char name[20];
    strcpy(name, client_data->name);
    char msg[1000];
    int len;
    int tamanho;
    while ((len = recv(sock, msg, 1000, 0)) > 0)
    {
        data_of_file.port = 5050;
        message *rcv_msg = (message *)msg;
        msg[len] = ' ';
        if (strncmp(rcv_msg->message, "exit", 4) == 0)
        {
            disconnect_user(sock);
        }
        else
        {
            if (strncmp(rcv_msg->message, "contact", 7) == 0)
            {
                send_contact(sock);
            }
            else
            {
                if (strncmp(rcv_msg->message, "file", 4) == 0)
                {
                    send_file_data(rcv_msg, sock, name);
                }
                else
                {
                    if (rcv_msg->online == 2)
                    {
                        send_to_specific(rcv_msg, sock, name);
                    }
                    else
                    {
                        send_to_all(rcv_msg, sock, name);
                    }
                }
            }
        }
    }
}

int main()
{
    struct sockaddr_in server_ip;
    pthread_t recv_t;
    user client_aux;
    int sock = 0, client_sock = 0, i = 0, flag = 0;
    char buffer[1000], username[20];
    FILE *fptr;
    message *login_content;
    thread client_name;
    sock = socket(PF_INET, SOCK_STREAM, 0);
    server_ip.sin_family = AF_INET;
    server_ip.sin_port = htons(5050);
    server_ip.sin_addr.s_addr = htons(INADDR_ANY);
    if (bind(sock, (struct sockaddr *)&server_ip, sizeof(server_ip)) == -1)
        printf("\nCannot bind, error! \n");
    else
        printf("\nServer Startedn\n");
    if (listen(sock, 20) == -1)
        printf("\nListening failed\n");
    while (1)
    {
        if ((client_sock = accept(sock, (struct sockaddr *)NULL, NULL)) < 0)
            printf("\nAccept failed \n");
        client_name.sock = client_sock;
        pthread_mutex_lock(&mutex);
        recv(client_sock, buffer, 1000, 0);
        login_content = (message *)buffer;
        strcpy(client_name.name, login_content->username);
        strcpy(username, login_content->username);
        username[strlen(username)-1] = '\0';
        printf("\nThe user %s is registered!\n", username);
        fptr = fopen("data.txt", "r+");
        while (fread(&client_aux, sizeof(client_aux), 1, fptr))
        {
            if (strcmp(login_content->username, client_aux.username) == 0)
            {
                flag = 1;
                client_aux.online = 1;
                client_aux.sock = client_sock;
            }
            client[i] = client_aux;
            i++;
        }
        if (flag == 0)
        {
            strcpy(client_aux.username, login_content->username);
            client_aux.sock = client_sock;
            client_aux.online = 1;
            client[i] = client_aux;
            i++;
            fwrite(&client_aux, sizeof(client_aux), 1, fptr);
            n++;
        }
        fclose(fptr);

        if (flag != 0)
        {
            fptr = fopen("data.txt", "w");
            fwrite(&client, sizeof(client_aux), i, fptr);
            fclose(fptr);
        }
        i = 0;
        flag = 0;
        // creating a thread for each client
        pthread_create(&recv_t, NULL, (void *)recv_msg, &client_name);
        pthread_mutex_unlock(&mutex);
    }
    return 0;
}
