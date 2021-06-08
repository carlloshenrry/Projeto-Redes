
#ifndef LIST
#define LIST

typedef struct ClientNode {
    int data;
    struct ClientNode* prev;
    struct ClientNode* link;
    char ip[16];
    char name[31];
    char destination[31];
} ClientList;

typedef struct registerClients{

    char nick[31];
    char status[31];
    int id;
    char offlineMsg[201];

} ServerNickControl;

ClientList *newNode(int sockfd, char* ip) {
    ClientList *addNode = (ClientList *)malloc( sizeof(ClientList) );
    addNode->data = sockfd;
    addNode->prev = NULL;
    addNode->link = NULL;
    strncpy(addNode->ip, ip, 16);
    strncpy(addNode->name, "NULL", 5);
    return addNode;
}

#endif // LIST