#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include "Serv.h"
#include "Proto.h"

/* ------------------------------------------------------ */
// Global Variables
int server_sock = 0, client_sock = 0;
int max_clients;
ClientList *initServer, *current;
ServerNickControl controler[30];
char buffer[SIZE];


/* ------------------------------------------------------ */
// Functions
void client_management(void *p_client);
void ctrl_c_exit(int sig);
void send_to_all_clients(ClientList *np, char tmp_buffer[]);
void send_to_a_client(ClientList *np, char tmp_buffer[]);
void send_arq(ClientList *np, char tmp_buffer[]);
int prepare_struct();
void str_trim_lf (char* arr, int length);

/* ------------------------------------------------------ */
int main() {

    signal(SIGINT, ctrl_c_exit);

    int sock;
    struct sockaddr_in server, client;

    /* Cria o socket de comunicacao */
    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if(server_sock<0) {
        perror("opening datagram socket");
        exit(1);
    }

    int serverLength = sizeof(server);
    int clientLength = sizeof(client);
    memset(&server, 0, serverLength);
    memset(&client, 0, clientLength);

    /* Associa */
    server.sin_family = PF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(1234);

    if (bind(server_sock,(struct sockaddr *)&server, serverLength) < 0) {
        perror("binding datagram socket");
        exit(1);
    }

    max_clients = prepare_struct();

    listen(server_sock, 5);

    if (getsockname(server_sock,(struct sockaddr *)&server, (socklen_t*) &serverLength) < 0) {
        perror("getting socket name");
        exit(1);
    }
    printf("Start Server on Port: %d\n", ntohs(server.sin_port));


    initServer = newNode(server_sock, inet_ntoa(server.sin_addr));
    current = initServer;

    while(1){
        client_sock = accept(server_sock, (struct sockaddr*) &client, (socklen_t*) &clientLength);

        getpeername(client_sock, (struct sockaddr*) &client, (socklen_t*) &clientLength);
        printf("Client %s:%d connected.\n", inet_ntoa(client.sin_addr), ntohs(client.sin_port));
        fflush(stdout);

        // Append linked list for clients
        ClientList *c = newNode(client_sock, inet_ntoa(client.sin_addr));
        c->prev = current;
        current->link = c;
        current = c;

        //Create a thread for every new client
        pthread_t id;
        if (pthread_create(&id, NULL, (void *)client_management, (void *)c) != 0) {
            perror("Create pthread error!\n");
            exit(EXIT_FAILURE);
        }
    }

}

/* ----------------------------------------------------------------------------- */

void client_management(void *p_client) {
    int quit = 0, i = 0, foundName = 0, off = 0;
    int receive;
    char nickname[LENGTH_NAME] = {};
    char dest[LENGTH_NAME] = {};
    char recv_buffer[LENGTH_MSG] = {};
    char service[LENGTH_NAME] = {};
    char send_buffer[LENGTH_SEND] = {};
    ClientList *new_client = (ClientList *)p_client;

    FILE *nickArq;

    // Get NickName
    if (recv(new_client->data, nickname, LENGTH_NAME, 0) <= 0 || strlen(nickname) < 2 || strlen(nickname) >= LENGTH_NAME-1) {
        printf("%s invalid nickname.\n", new_client->ip);
        quit = 1;
    } else {
        i = 0;
        while(i != max_clients){
            if(strcmp(controler[i].nick, nickname) == 0) {
                strcpy(controler[i].status, "online");
                foundName = 1;
                strncpy(new_client->name, nickname, LENGTH_NAME);
                printf("Welcome Back!!!\n%s(%s)(%d) join the chatroom.\n", new_client->name, new_client->ip, new_client->data);
                fflush(stdout);
            }
            i++;
        }
        if(foundName == 0){
            max_clients += 1;
            strncpy(controler[max_clients].nick, nickname, LENGTH_NAME);
            strncpy(new_client->name, nickname, LENGTH_NAME);
            strcpy(controler[max_clients].status, "online");
            controler[max_clients].id = max_clients;
            nickArq = fopen("nicks.txt", "a+");
            if(nickArq == NULL) {
                perror("Error in opening file");
                quit = 1;
            }
            fprintf(nickArq, " %s\n", nickname);
            fclose(nickArq);

            printf("%s(%s)(%d) join the chatroom.\n", new_client->name, new_client->ip, new_client->data);
            fflush(stdout);
        }
    }

    
    // Conversation
    while (1) {

        if (quit) {
            break;
        }

        int value = recv(new_client->data, service, LENGTH_MSG, 0);
        if (value > 0) {
            printf("recebi o serviço\n");
            fflush(stdout);
            printf("service: %s\n", service);
        }

        if(strcmp(service, "-all") == 0){
            printf("Msg for all clients\n");
            fflush(stdout);
            
            receive = recv(new_client->data, recv_buffer, LENGTH_MSG, 0);
            if (receive > 0) {
            if (strlen(recv_buffer) == 0) {
                continue;
            }

            sprintf(send_buffer, "%s：%s", new_client->name, recv_buffer);

            } else if (receive == 0 || strcmp(recv_buffer, "exit") == 0) {
                printf("%s (%d) leave the chatroom.\n", new_client->name, new_client->data);
                sprintf(send_buffer, "%s(%s) leave the chatroom.", new_client->name, new_client->ip);
                quit = 1;
            } else {
                printf("Fatal Error: -1\n");
                quit = 1;
            }

            send_to_all_clients(new_client, send_buffer);
            } else {
                if(strcmp(service, "-msg") == 0){
                printf("Msg for a unic client\n");
                fflush(stdout);

                recv(new_client->data, dest, LENGTH_NAME, 0);
                strncpy(new_client->destination, dest, LENGTH_NAME);


                receive = recv(new_client->data, recv_buffer, LENGTH_MSG, 0);
                if (receive > 0) {
                if (strlen(recv_buffer) == 0) {
                    continue;
                }
                    sprintf(send_buffer, "%s：%s", new_client->name, recv_buffer);
                } else if (receive == 0 || strcmp(recv_buffer, "exit") == 0) {
                    printf("%s (%d) leave the chatroom.\n", new_client->name, new_client->data);
                    sprintf(send_buffer, "%s(%s) leave the chatroom.", new_client->name, new_client->ip);
                    quit = 1;
                } else {
                    printf("Fatal Error: -1\n");
                    quit = 1;
                }

                send_to_a_client(new_client, send_buffer);
                } else {
                    if(strcmp(service, "-arq") == 0){
                        printf("Msg for a unic client\n");
                        fflush(stdout);

                        recv(new_client->data, dest, LENGTH_NAME, 0);
                        strncpy(new_client->destination, dest, LENGTH_NAME);

                        receive = recv(new_client->data, buffer, SIZE, 0);

                        if (receive > 0) {
                        if (strlen(buffer) == 0) {
                            continue;
                        }
                        } else if (receive == 0 || strcmp(buffer, "exit") == 0) {
                            printf("%s (%d) leave the chatroom.\n", new_client->name, new_client->data);
                            sprintf(buffer, "%s(%s) leave the chatroom.", new_client->name, new_client->ip);
                            quit = 1;
                        } else {
                            printf("Fatal Error: -1\n");
                            quit = 1;
                        }
                        send_arq(new_client, buffer);
                        }else{
                            if(strcmp(service, "-list") == 0){
                            i = 0;
                            while(i != max_clients){
                                if(strcmp(new_client->name, controler[i].nick) != 0){
                                    send(new_client->data, (char *)controler[i].nick, LENGTH_SEND, 0);
                                    send(new_client->data, (char *)controler[i].status, LENGTH_SEND, 0);
                                }
                                i++;
                            }
                    
                        } else {
                                
                        printf("Invalid Command");
                        fflush(stdout);    
                    }
                }  
            }
        }
    }

    // Remove Node
    close(new_client->data);
    if (new_client == current) { // remove an edge node
        current = new_client->prev;
        current->link = NULL;

    } else { // remove a middle node
        new_client->prev->link = new_client->link;
        new_client->link->prev = new_client->prev;
    }
    free(new_client);
    pthread_detach(pthread_self());
}

void ctrl_c_exit(int sig) {
    ClientList *tmp;
    char quit[LENGTH_MSG];
    while (initServer != NULL) {
        sprintf(quit, "quit");
        printf("\nClose socketfd: %d\n", initServer->data);
        send(initServer->data, quit, LENGTH_MSG, 0);
        close(initServer->data); // close all socket include server_sockfd
        tmp = initServer;
        initServer = initServer->link;
        free(tmp);
    }
    printf("Bye\n");
    exit(EXIT_SUCCESS);
}

void send_to_all_clients(ClientList *np, char tmp_buffer[]) {
    ClientList *tmp = initServer->link;
    while (tmp != NULL) {
        if (np->data != tmp->data) { // all clients except itself.
            printf("Send to sockfd %d: \"%s\" \n", tmp->data, tmp_buffer);
            send(tmp->data, tmp_buffer, LENGTH_SEND, 0);
        }
        tmp = tmp->link;
    }
}

void send_to_a_client(ClientList *np, char tmp_buffer[]) {
    ClientList *tmp = initServer->link;
    while (tmp != NULL) {
        if (strcmp(np->destination, tmp->name) == 0) {
            printf("Send to sockfd %d: \"%s\" \n", tmp->data, tmp_buffer);
            send(tmp->data, tmp_buffer, LENGTH_SEND, 0);
        }
        tmp = tmp->link;
    }
}

int prepare_struct(){
    char nickname[LENGTH_NAME] = {};
    int i;
    char c;

    FILE *nickArq;
    nickArq = fopen("nicks.txt", "r");
    if(nickArq == NULL) {
      perror("Error in opening file");
      exit(1);
   }
    while(c != EOF){
        c = fgetc(nickArq);
        fgets(nickname, LENGTH_NAME, nickArq);
        str_trim_lf(nickname, LENGTH_NAME);
        strncpy(controler[i].nick, nickname, LENGTH_NAME);
        strncpy(controler[i].status, "offline", LENGTH_NAME);
        controler[i].id = i;
        i++;
    }
    fclose(nickArq);

    return i;
}

void str_trim_lf (char* arr, int length) {
    int i;
    for (i = 0; i < length; i++) { // trim \n
        if (arr[i] == '\n') {
            arr[i] = '\0';
            break;
        }
    }
}

void send_arq(ClientList *np, char tmp_buffer[]) {
    ClientList *tmp = initServer->link;
    while (tmp != NULL) {
        if (strcmp(np->destination, tmp->name) == 0) {
            printf("Send to sockfd %d: \"%s\" \n", tmp->data, tmp_buffer);
            send(tmp->data, tmp_buffer, SIZE, 0);
        }
        tmp = tmp->link;
    }
}