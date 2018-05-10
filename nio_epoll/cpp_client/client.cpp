#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>

#include <iostream>
using namespace std;

#ifndef _BLOCK
#define _BLOCK
#endif


#define MAX_BUF	512

char *IP = NULL;
int PORT = 0;

int my_send(int fd,const void *data,size_t size)
{
#ifdef _BLOCK
	int ret = write(fd,data,size);
	return ret;
#else
	//
#endif
}

int my_receive(int fd,void *data,size_t size)
{
#ifdef _BLOCK
	int ret = read(fd,data,size);
	return ret;
#else
	//
#endif
}

//函数
//功能:设置socket为非阻塞的
static int
make_socket_non_blocking (int sfd)
{
    int flags, s;

    //得到文件状态标志
    flags = fcntl (sfd, F_GETFL, 0);
    if (flags == -1) {
        perror ("fcntl");
        return -1;
    }

    //设置文件状态标志
    flags |= O_NONBLOCK;
    s = fcntl (sfd, F_SETFL, flags);
    if (s == -1) {
        perror ("fcntl");
        return -1;
    }

    return 0;
}


int main(int argc,char **argv)
{
    if(argc != 3){
        printf("Uasge: ip port\n");
        return -1;
    }

    IP = argv[1];
    PORT = atoi(argv[2]);

    cout << IP << "   "<< PORT <<endl;
    struct sockaddr_in  cli;
    int sockfd;
    sockfd = socket(AF_INET,SOCK_STREAM,0);
    memset(&cli,0,sizeof(cli));
    cli.sin_family = AF_INET;
    cli.sin_port = htons(PORT);
    cli.sin_addr.s_addr = inet_addr(IP);

    if(-1 == connect(sockfd,(struct sockaddr *)&cli,sizeof(cli))){
        printf("%s\n",strerror(errno));
        return 2;
    }else{
    	cout << "Connect to " << IP << ":" << PORT <<"!" << endl;
    }

	return 0;
}
