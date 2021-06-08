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
#include "Proto.h"

int sock = 0;
char nickname[LENGTH_NAME] = {};
int quit = 0, flag = 0;
FILE *fp;
char *filename = "env.txt";

void sendMSg();
void recvMsg();
void send_file(FILE *fp, int sockfd);
void str_trim_lf (char* arr, int length);
void str_overwrite_stdout();
void ctrl_c_exit(int sig);
void red();
void green();
void reset();

/* ------------------------------------------------------------------------ */

int main(int argc, char *argv[]){

    signal(SIGINT, ctrl_c_exit);

    printf("Select your nickname: ");
    if (fgets(nickname, LENGTH_NAME, stdin) != NULL) {
        str_trim_lf(nickname, LENGTH_NAME);
    }
    if (strlen(nickname) < 2 || strlen(nickname) >= LENGTH_NAME-1) {
        printf("\nName must be more than one and less than thirty characters.\n");
        exit(EXIT_FAILURE);
    }


/* Cria o socket */
sock = socket(AF_INET, SOCK_STREAM, 0);
if(sock<0){ /*se nao conseguir abrir o socket*/
  perror ("Abertura do Socket");
  exit(1);
}

struct sockaddr_in server, client;
int serverLength = sizeof(server);
int clientLength = sizeof(client);
memset(&server, 0, serverLength);
memset(&client, 0, clientLength);

/* Associa */
server.sin_family = PF_INET;
server.sin_addr.s_addr = inet_addr("127.0.0.1");
server.sin_port = htons(1234);


/* Solicita conexão */
if (connect(sock, (struct sockaddr*)&server, serverLength) < 0) {
  perror("Erro na conexão");
  exit(1);
}

getsockname(sock, (struct sockaddr*) &client, (socklen_t*) &clientLength);
getpeername(sock, (struct sockaddr*) &server, (socklen_t*) &serverLength);
printf("Connect to Server: %s:%d\n", inet_ntoa(server.sin_addr), ntohs(server.sin_port));
printf("You: %s:%d\n", inet_ntoa(client.sin_addr), ntohs(client.sin_port));

send(sock, nickname, LENGTH_NAME, 0);

pthread_t send_msg_thread;
if (pthread_create(&send_msg_thread, NULL, (void *) sendMSg, NULL) != 0) {
  printf ("Create pthread error!\n");
  exit(EXIT_FAILURE);
}

pthread_t recv_msg_thread;
if (pthread_create(&recv_msg_thread, NULL, (void *) recvMsg, NULL) != 0) {
  printf ("Create pthread error!\n");
  exit(EXIT_FAILURE);
}


while(1){ 
    if(quit){
        printf("\nBye\n");
        break;
    }
}

close(sock);
exit(0);
}

void sendMSg() {
    char message[LENGTH_MSG] = {};
    char service[LENGTH_NAME] = {};
    char dest[LENGTH_NAME] = {};
    int value;

    while (1) {

    printf("<< -quit     Quit chatroom\r\n");
    printf("<< -all      Send message to every active user\r\n");
    printf("<< -msg      Send private message\r\n");
    printf("<< -list     Show all clients and the status\r\n");
    printf("<< -help     Show help\r\n");
    printf("<< -arq      Send a file\r\n");

    str_overwrite_stdout();
    printf("Escolha sua opção: ");
    fgets(service, LENGTH_NAME, stdin);
    str_trim_lf(service, LENGTH_NAME);
    flag = 0;

    if(strcmp(service, "-help") == 0){
    
    } else{
        if(strcmp(service, "-all") == 0){
        send(sock, service, LENGTH_NAME, 0);

            str_overwrite_stdout();
            while (fgets(message, LENGTH_MSG, stdin) != NULL) {
                str_trim_lf(message, LENGTH_MSG);
                fflush(stdin);
                if (strlen(message) == 0) {
                    str_overwrite_stdout();
                } else {
                    break;
                }
            }
            send(sock, message, LENGTH_MSG, 0);

        } else {
            if(strcmp(service, "-msg") == 0){
            send(sock, service, LENGTH_NAME, 0);
            str_overwrite_stdout();
            printf("Para quem deseja mandar a mensagem: ");
            fflush(stdout);
            fgets(dest, LENGTH_NAME, stdin);
            str_trim_lf(dest, LENGTH_MSG);

            send(sock, dest, LENGTH_NAME, 0);

            str_overwrite_stdout();
            while (fgets(message, LENGTH_MSG, stdin) != NULL) {
                str_trim_lf(message, LENGTH_MSG);
                if (strlen(message) == 0) {
                    str_overwrite_stdout();
                } else {
                    break;
                }
            }
            send(sock, message, LENGTH_MSG, 0);
            if (strcmp(message, "exit") == 0) {
                break;
            }
            
            } else {
                if(strcmp(service, "-list") == 0){
                send(sock, service, LENGTH_NAME, 0);
                flag = 1;
                 
                } else {
                    if (strcmp(message, "-quit") == 0) {
                        quit = 1;
                        break;
                    } else {
                        if(strcmp(service, "-arq") == 0){
                            ////////////////////////////////////////////////////////
                                send(sock, service, LENGTH_NAME, 0);
                                str_overwrite_stdout();
                                printf("Para quem deseja mandar a mensagem: ");
                                fflush(stdout);
                                fgets(dest, LENGTH_NAME, stdin);
                                str_trim_lf(dest, LENGTH_MSG);

                                send(sock, dest, LENGTH_NAME, 0);

                                str_overwrite_stdout();
                            
                                fp = fopen(filename, "r");
                                if (fp == NULL) {
                                    perror("[-]Error in reading file.");
                                    exit(1);
                                }

                                send_file(fp, sock);
                                printf("[+]File data sent successfully.\n");
                                if (strcmp(message, "exit") == 0) {
                                    break;
                                }
                            } else {
                                
                            str_overwrite_stdout();
                            printf("invalid command \n");
                        }
                    }
                }
            }
        }
    }

    }
    ctrl_c_exit(2);
}

void recvMsg() {
    char receiveMessage[LENGTH_SEND] = {};
    char aux[LENGTH_SEND] = {};
    while (1) {

            int receive = recv(sock, receiveMessage, LENGTH_SEND, 0);
            if (receive > 0) {
                if(strcmp(receiveMessage, "quit") == 0){
                    quit = 1;
                    break;
                }
                if(strcmp(receiveMessage, "offline") == 0 || strcmp(receiveMessage, "online") == 0){
                    printf("\n %s: ", aux);
                    if(strcmp(receiveMessage, "offline") == 0){
                        red();
                        printf("%s", receiveMessage);
                        reset();
                    } else{
                        green();
                        printf("%s", receiveMessage);
                        reset();
                    }

                } else {
                    if(flag == 1){
                        strncpy(aux, receiveMessage, LENGTH_SEND);
                    } else{
                        printf("\n\r%s\n", receiveMessage);
                        str_overwrite_stdout();
                    }
                }                
            } else if (receive == 0) {
                break;
            } else { 
                // -1 
        }
    }
}


void ctrl_c_exit(int sig) {
    quit = 1;
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

void str_overwrite_stdout() {
    printf("\r%s", "> ");
    fflush(stdout);
}

void red () {
  printf("\033[1;31m");
}

void green () {
  printf("\033[0;32m");
}

void reset () {
  printf("\033[0m");
}

void send_file(FILE *fp, int sockfd){
  int n;
  char data[SIZE] = {0};

  while(fgets(data, SIZE, fp) != NULL) {
    if (send(sockfd, data, sizeof(data), 0) == -1) {
      perror("[-]Error in sending file.");
      exit(1);
    }
    bzero(data, SIZE);
  }
}