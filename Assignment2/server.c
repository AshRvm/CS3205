#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>

#define PORT 8080
#define PLAY_LIM 50
#define BUFF_SIZE 15

pthread_mutex_t lock;
int num_players = 0;
int player_id = 1;
int game_id = 0;


void sendClientMessage(int cli_sockfd, char* msg, int len){
    int temp = send(cli_sockfd, msg, len, 0);
    if(temp < 0){
        printf("Error sending message to client\n");
        exit(1);
    }
}

int winCheck(int* board){
    // Colume check
    for(int i=0;i<3;i++){
        int sum = board[i] + board[i + 3] + board[i + 6];
        if(!(sum%3) && (sum>0) && (board[i] == board[i + 3])) return 1;
    }

    // Row check
    for(int i=0;i<3;i++){
        int sum = board[i*3] + board[1 + i*3] + board[2 + i*3];
        if(!(sum%3) && (sum>0) && (board[i*3] == board[1 + i*3])) return 1;
    }

    // Diagonal check
    for(int i=2;i<6;i+=2){
        int sum = board[4] + board[4+i] + board[4-i];
        if(!(sum%3) && (sum>0) && (board[4] == board[4-i])) return 1;
    }

    return 0;
}

void printBoard(int* board){
    for(int i=0;i<3;i++){
        for(int j=0;j<3;j++){
            printf("%d ",board[i*3 + j]);
        }
        printf("\n");
    }
}

void* ticTacToe(void* pair_sockfds){
    int curr_player = 0, is_game_running = 1, turn_count = 0;
    int* cli_sockfds = (int*) pair_sockfds;
    int* board = (int*)malloc(9*sizeof(int));
    memset(board, 0, 9*sizeof(int));

    while(is_game_running){
        int temp = 1;
        int temp1 = 1, temp2 = 1;
        int player_done = 0;
        int final_move = 0;
        int timeout = 0;
        int is_win = 0;
        temp1 = send(cli_sockfds[curr_player], "TURN", 5*sizeof(char), 0);
        temp2 = send(cli_sockfds[(curr_player+1)%2], "WAIT", 5*sizeof(char), 0);
        while(!player_done){
            char move[5];
            temp = read(cli_sockfds[curr_player], move, 4*sizeof(char));
            printf("%s\n",move);
            if(!strcmp(move, "TIME")){
                timeout = 1;
                is_game_running = 0;
                break;
            }
            if(temp <= 0 || temp1 <= 0 || temp2 <= 0){
                sendClientMessage(cli_sockfds[(curr_player+1)%2], "DISC", 5*sizeof(char));
                break;
            }
            int row = (int)move[0] - 48;
            int col = (int)move[2] - 48;
            int temp_move = (row-1)*3 + (col-1);
            if((row > 3 || col > 3 || row < 1 || col < 1) || (board[temp_move] != 0)){
                sendClientMessage(cli_sockfds[curr_player], "INV", 5*sizeof(char));
            }else{
                board[temp_move] = curr_player+1;
                player_done = 1;
                final_move = temp_move;
            }
        }
        if(temp <= 0){
            break;
        }

        if(!timeout){
            for(int i=0;i<2;i++){
                sendClientMessage(cli_sockfds[i], "UPD", 5*sizeof(char));
                sendClientMessage(cli_sockfds[i], &final_move, sizeof(int));
                sendClientMessage(cli_sockfds[i], &curr_player, sizeof(int));
            }
        }

        if(is_game_running){
            is_win = winCheck(board);
            is_game_running = 1 - is_win;
        }
        if(!is_game_running){
            char replay1[5], replay2[5];

            if(is_win){
                sendClientMessage(cli_sockfds[curr_player], "WIN", 5*sizeof(char));
                sendClientMessage(cli_sockfds[(curr_player+1)%2], "LOSE", 5*sizeof(char));
            }else if(timeout){
                sendClientMessage(cli_sockfds[curr_player], "TIM1", 5*sizeof(char));
                sendClientMessage(cli_sockfds[(curr_player+1)%2], "TIM2", 5*sizeof(char));
            }

            read(cli_sockfds[curr_player], replay1, 5*sizeof(char));
            read(cli_sockfds[(curr_player+1)%2], replay2, 5*sizeof(char));

            if(!strcmp(replay1,"YES") && !strcmp(replay2,"YES")){
                is_game_running = 1;
                curr_player = 1;
                turn_count = -1;
                memset(board, 0, 9*sizeof(int));
                sendClientMessage(cli_sockfds[curr_player], "CONT", 5*sizeof(char));
                sendClientMessage(cli_sockfds[curr_player], "PRNT", 5*sizeof(char));
                sendClientMessage(cli_sockfds[(curr_player+1)%2], "CONT", 5*sizeof(char));
                sendClientMessage(cli_sockfds[(curr_player+1)%2], "PRNT", 5*sizeof(char));
            }
        }else if(turn_count == 8){
            sendClientMessage(cli_sockfds[curr_player], "DRAW", 5*sizeof(char));
            sendClientMessage(cli_sockfds[(curr_player+1)%2], "DRAW", 5*sizeof(char));
            is_game_running = 0;
        }

        curr_player = (curr_player+1)%2;
        turn_count++;
    }
    printf("Game over\n");
    close(cli_sockfds[0]);
    close(cli_sockfds[1]);

    pthread_mutex_lock(&lock);
    num_players -= 2;
    pthread_mutex_unlock(&lock);

    free(cli_sockfds);

    pthread_exit(NULL);
}

int main(){
    int serv_sockfd, temp;
    struct sockaddr_in serv_addr;
    struct sockaddr_in cli_addr;

    pthread_mutex_init(&lock, NULL);

    serv_sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(serv_sockfd < 0){
        printf("Error creating socket\n");
        exit(1);
    }

    memset(&serv_addr, 0, sizeof(serv_sockfd));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(PORT);

    temp = bind(serv_sockfd, (struct sockaddr*) &serv_addr, sizeof(serv_addr));
    if(temp < 0){
        printf("Error binding\n");
        exit(1);
    }

    while(1){
        int players_waiting = 0;
        socklen_t len = sizeof(cli_addr);
        int* player_pair_sockfds = (int*)malloc(2*sizeof(int));
        while(players_waiting < 2){
            listen(serv_sockfd, PLAY_LIM - num_players);
            memset(&cli_addr, 0, sizeof(cli_addr));
            
            int cli_sockfd = accept(serv_sockfd, (struct sockaddr*) &cli_addr, &len);
            if(cli_sockfd < 0){
                printf("Error accepting connection\n");
                exit(1);
            }

            player_pair_sockfds[players_waiting] = cli_sockfd;
            sendClientMessage(cli_sockfd, &player_id, sizeof(int));

            pthread_mutex_lock(&lock);
            num_players++;
            player_id++;
            pthread_mutex_unlock(&lock);

            players_waiting++;
        }
        pthread_mutex_lock(&lock);
        game_id++;
        pthread_mutex_unlock(&lock);

        for(int i=0;i<2;i++){
            sendClientMessage(player_pair_sockfds[i], &game_id, sizeof(int));
        }

        pthread_t thread;
        temp = pthread_create(&thread, NULL, ticTacToe, (void*)player_pair_sockfds);
    }

    close(serv_sockfd);
    pthread_mutex_destroy(&lock);
    pthread_exit(NULL);
}