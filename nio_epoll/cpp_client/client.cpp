#include "x.pb.h"

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

#define HEAD_SIZE   10
#define MAX_BUF	512

const char *IP_default = "127.0.0.1";
int PORT_default = 5000;

char *IP = NULL;
int PORT;

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

void send_head_msg(int sockfd,int size,uint32_t type)
{
    head_msg  hm;
    hm.set_size(size);
    hm.set_type(type);
    int len = hm.ByteSizeLong();
    char *buf = (char *)malloc(len);
    hm.SerializeToArray(buf,len);
    int ret = send(sockfd,buf,len,0);
    delete buf;
}

int main(int argc,char **argv)
{
    if(argc == 3){
        IP = argv[1];
        PORT = atoi(argv[2]);
    }else{
        IP = const_cast<char *>(IP_default);
        PORT = PORT_default;
    }

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

    info m_info;
    m_info.set_data("hello,world");
    int len = m_info.ByteSizeLong();
    send_head_msg(sockfd,len,CMD_INFO);
    char *buf = (char *)malloc(len);
    int ret = send(sockfd,buf,len,0);
    cout << "send data " << ret <<endl;
    delete buf;

    m_info.set_data("fuck you a XX okok ok ok hello china");
    len = m_info.ByteSizeLong();
    send_head_msg(sockfd,len,CMD_INFO);
    buf = (char *)malloc(len);
    ret = send(sockfd,buf,len,0);
    cout << "send data " << ret <<endl;
    delete buf;
	return 0;
}
