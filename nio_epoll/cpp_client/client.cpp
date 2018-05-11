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

#define MAX_BUF	512

struct head_msg
{
    int  size;
    int  type;    
};

#define MSG_HEAD_SZIE   (sizeof(head_msg))

const char *IP_default = "127.0.0.1";
int PORT_default = 5000;

char *IP = NULL;
int PORT = 0;

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

void send_head_msg(int sockfd,int size,int type)
{
    head_msg hm;
    hm.size = size;
    hm.type = type;
    int ret = send(sockfd,(char *)&hm,MSG_HEAD_SZIE,0);
    cout << "send " << ret << "bytes" <<endl;
}

int main(int argc,char **argv)
{
    if(argc == 3){
        IP = argv[1];
        PORT = atoi(argv[2]);
        return -1;
    }else{
      IP = const_cast<char *>(IP_default);
      PORT = 5000;  
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
    cout << len <<endl;
    char *buf = (char *)malloc(len);
    m_info.SerializeToArray(buf,len);
    send_head_msg(sockfd,len,CMD_INFO);
    send(sockfd,buf,len,0);
    delete buf;

    m_info;
    m_info.set_data("zhengyifan da bao jian");
    len = m_info.ByteSizeLong();
    cout << len <<endl;
    send_head_msg(sockfd,len,CMD_INFO);
    buf = (char *)malloc(len);
    m_info.SerializeToArray(buf,len);
    send(sockfd,buf,len,0);
    delete buf;

    //sleep(10);

    close(sockfd);

	return 0;
}
