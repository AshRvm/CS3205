#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <time.h>
#include <fcntl.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/ip_icmp.h>

#define PACKET_SIZE 64   
#define PORT 8080
#define RECV_TIMEOUT 1 

struct ping_pkt{
    struct icmphdr hdr;
    char msg[PACKET_SIZE-sizeof(struct icmphdr)];
};

unsigned short checksum(void *b, int len){
    unsigned short *buf = b;
    unsigned int sum=0;
    unsigned short result;
  
    for(sum = 0; len > 1; len -= 2){
        sum += *buf++;
    }
    if(len == 1){
        sum += *(unsigned char*)buf;
    }
    sum = (sum >> 16) + (sum & 0xFFFF);
    sum += (sum >> 16);
    result = ~sum;
    return result;
}

int isValidIpAddress(char *ipAddress){
    struct sockaddr_in sa;
    int result = inet_pton(AF_INET, ipAddress, &(sa.sin_addr));
    return result != 0;
}

void ping(int ping_sockfd, struct sockaddr_in *ping_addr, char *ping_ip){      
    struct ping_pkt packet;
    struct sockaddr_in addr;
    struct timespec time_start, time_end;
    long double rtt=0;
  
    bzero(&packet, sizeof(packet));
        
    packet.hdr.type = ICMP_ECHO;

    memset(packet.msg, 0, sizeof(packet.msg));
    packet.hdr.checksum = checksum(&packet, sizeof(packet));

    clock_gettime(CLOCK_MONOTONIC, &time_start);
    int temp = sendto(ping_sockfd, &packet, sizeof(packet), 0, (struct sockaddr*) ping_addr, sizeof(*ping_addr));
    if(temp < 0){
        printf("Request timed out or host unreacheable\n");
        return ;
    }

    int addr_len=sizeof(addr);
    temp = 0;

    time_t end = time(0) + RECV_TIMEOUT;
    int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);
    while(time(0) < end){
        temp = recvfrom(ping_sockfd, &packet, sizeof(packet), 0, (struct sockaddr*)&addr, &addr_len);
        if(temp < 0){
            printf("Request timed out or host unreacheable\n");
            return;
        }
        if(temp != 0){
            break;
        }
    }
    if(temp == 0){
        printf("Request timed out or host unreacheable\n");
        return;
    }

    clock_gettime(CLOCK_MONOTONIC, &time_end);        
    rtt = (time_end.tv_sec-time_start.tv_sec)*1000.0 + (time_end.tv_nsec - time_start.tv_nsec)/1000000.0;
    printf("Reply from %s. RTT = %Lf ms\n", ping_ip, rtt);
}

int main(int argc, char *argv[]){
    char *ip_addr;
    ip_addr = argv[1];
    if(!isValidIpAddress(ip_addr)){
        printf("Bad hostname\n");
    }else{
        int sockfd;
        struct sockaddr_in addr;
        struct hostent *host_entity = gethostbyname(ip_addr);

        addr.sin_family = host_entity->h_addrtype;
        addr.sin_addr.s_addr = *(long*)host_entity->h_addr;
        addr.sin_port = htons(PORT);
    
        sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
        ping(sockfd, &addr, argv[1]);
    }
    return 0;
}