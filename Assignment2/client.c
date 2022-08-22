#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <time.h>
#include <fcntl.h>

#define PORT 8080


void readServerMessage(int serv_sockfd, char* msg, int len){
    int temp = read(serv_sockfd, msg, len);
    if(temp < 0){
        printf("Error reading message\n");
        exit(1);
    }
}

void printBoard(int* board){
    char temp[10];
    for(int i=0;i<10;i++){
        if(board[i] == 0) temp[i] = '_';
        else if(board[i]%2) temp[i] = 'O';
        else temp[i] = 'X';
    }
    temp[9] = '\0';
    printf("\n");
    for(int i=0;i<3;i++){
        printf("%c | %c | %c\n", temp[i*3], temp[i*3 + 1], temp[i*3 + 2]);
    }
}

void getInput1(int serv_sockfd){
    char move[5];
    int pos = 0;
    memset(move, '\0', 5*sizeof(char));
    printf("Enter (ROW, COL) for placing your mark: ");

    time_t end = time(0) + 15;
    int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);
    while(time(0) < end){
        int c = getchar();
        if(c != EOF && c != 10 && pos < 4){
            move[pos++] = c;
        }
        if(c == 10){
            break;
        }
    }
    if(pos < 3){
        printf("Too late\n");
        send(serv_sockfd, "TIME", 4*sizeof(char), 0);
    }else{
        int temp = send(serv_sockfd, move, 4*sizeof(char), 0);
        if(temp < 0){
            printf("Error sending input to server\n");
            exit(1);
        }
    }
}

void getInput2(int serv_sockfd){
    char replay[5];
    memset(replay, '\0', 5*sizeof(char));
    int pos = 0;
    while(1){
        int c = getchar();
        if(c != EOF && c!= 10 && pos < 4){
            replay[pos++] = c; 
        }
        if(c == 10){
            break;
        }
    }
    send(serv_sockfd, replay, 5*sizeof(char), 0);
}

int main(){
    int serv_sockfd, temp;
    struct sockaddr_in serv_addr;

    serv_sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(serv_sockfd < 0){
        printf("Error creating socket\n");
        exit(1);
    }

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    serv_addr.sin_port = htons(PORT);

    temp = connect(serv_sockfd, (struct sockaddr*) &serv_addr, sizeof(serv_addr));
    if(temp < 0){
        printf("Error connecting to server\n");
        exit(1);
    }

    int player_id = 0, game_id = 0;
    readServerMessage(serv_sockfd, &player_id, sizeof(int));
    if(player_id % 2){
        printf("Connected to the game server. Your player ID is %d. Waiting for a partner to join . . .\n",player_id);
    }else{
        printf("Connected to the game server. Your player ID is %d. Your partner's ID is %d. Your symbol is 'X'\n",player_id, player_id-1);
    }

    readServerMessage(serv_sockfd, &game_id, sizeof(int));
    if(player_id % 2){
        printf("Your partner's ID is %d. Your symbol is 'O'.\n",player_id+1);
    }
    printf("The game ID is %d. Starting the game ...\n",game_id);

    int* board = (int*)malloc(9*sizeof(int));
    printBoard(board);

    while(1){
        char msg[5];
        readServerMessage(serv_sockfd, msg, 5*sizeof(char));
        if(!strcmp(msg,"WAIT")){
            printf("Waiting for opponent to play\n");
        }else if(!strcmp(msg,"TURN")){
            getInput1(serv_sockfd);
        }else if(!strcmp(msg,"INV")){
            printf("Invalid move. ");
            getInput1(serv_sockfd);
        }else if(!strcmp(msg,"UPD")){
            int move, value;
            readServerMessage(serv_sockfd, &move, sizeof(int));
            readServerMessage(serv_sockfd, &value, sizeof(int));
            board[move] = value+1;
            printBoard(board);
        }else if(!strcmp(msg,"WIN")){
            char replay[5];
            printf("You win\n");
            printf("Do you wish to replay the game?\n");
            getInput2(serv_sockfd);
            readServerMessage(serv_sockfd, replay, 5*sizeof(char));
            if(!strcmp(replay,"CONT")){
                memset(board, 0, 9*sizeof(int));
            }else{
                break;
            }
        }else if(!strcmp(msg,"LOSE")){
            char replay[5];
            printf("You lose\n");
            printf("Do you wish to replay the game?\n");
            getInput2(serv_sockfd);
            readServerMessage(serv_sockfd, replay, 5*sizeof(char));
            if(!strcmp(replay,"CONT")){
                memset(board, 0, 9*sizeof(int));
            }else{
                break;
            }
        }else if(!strcmp(msg,"DRAW")){
            printf("Draw\n");
            printf("Game over\n");
            break;
        }else if(!strcmp(msg,"DISC")){
            printf("Sorry, your partner disconnected\n");
            break;
        }else if(!strcmp(msg,"TIM1")){
            char replay[5];
            printf("You took too long to play. Do you wish to play again?\n");
            getInput2(serv_sockfd);
            readServerMessage(serv_sockfd, replay, 5*sizeof(char));
            if(!strcmp(replay,"CONT")){
                memset(board, 0, 9*sizeof(int));
            }else{
                break;
            }
        }else if(!strcmp(msg,"TIM2")){
            char replay[5];
            printf("You're partner took too long to play. Do you wish to play again?\n");
            getInput2(serv_sockfd);
            readServerMessage(serv_sockfd, replay, 5*sizeof(char));
            if(!strcmp(replay,"CONT")){
                memset(board, 0, 9*sizeof(int));
            }else{
                break;
            }
        }else if(!strcmp(msg,"PRNT")){
            printBoard(board);
        }
    }
    close(serv_sockfd);
    return 0;
}