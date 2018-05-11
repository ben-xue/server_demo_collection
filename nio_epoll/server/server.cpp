#include "x.pb.h"

#include <google/protobuf/message_lite.h>

#include <signal.h>
#include <netdb.h>

#include <sstream>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/epoll.h>
#include <sys/time.h>
#include <netinet/in.h>

#define DEFAULT_PORT (5000)
#define MAX_EVENTS   (1000)
#define MAX_CONNECT  (1024) 

#define MAX_PACK_SIZE   (4 * 1024)

using namespace std;

struct head_msg
{
    int  size;
    int  type;    
};

#define MSG_HEAD_SZIE   (sizeof(head_msg))

struct conn {
    int cnt;
    int sock;
    char *buf;
    size_t alloced, head, tail;
    bool read_end;
    bool error;
    bool has_head;
    head_msg hm;
    char buf_host[NI_MAXHOST];
    char buf_service[NI_MAXSERV];

    conn(int sock) : sock(sock), cnt(0),buf(0), head(0), tail(0), read_end(false), error(false),has_head(false)
    {
        alloced = MAX_PACK_SIZE;
        buf = (char*)malloc(alloced);
        if (!buf) {
            puts("No memory.\n");
            exit(-1);
        }
    }

    ~conn() { close(sock); }

    void read() {
        for (;;) {
            if (alloced - tail < MAX_PACK_SIZE) {
                if (alloced - (tail - head) < MAX_PACK_SIZE) {
                    alloced *= 2;
                    buf = (char*)realloc(buf, alloced);
                    if (!buf) {
                        puts("No memory(realloc)");
                        exit(-1);
                    }
                } else {
                    memmove(buf, buf+head, tail-head);
                    tail -= head;
                    head = 0;
                }
            }

            if(alloced - tail > 0){
                int n = recv(sock, buf+tail, alloced-tail,0);
                if (n < 0) {
                    if (errno == EAGAIN || errno == EWOULDBLOCK) {
                        if(tail == head){
                            //该连接的数据已经处理完毕
                            break;
                        }
                    }else{
                        perror("read");
                        error = true;
                        return;
                    }
                }else if (n == 0) {
                    read_end = true;
                    cout << buf_host <<":"<<buf_service << " quited" << endl;
                    break;
                }else{
                    tail += n;
                }
            }

            if(false == has_head){
                if(tail - head >= MSG_HEAD_SZIE){
                    hm.size = *(int *)(buf + head);
                    head += sizeof(int);
                    hm.type = *(int *)(buf + head);
                    head += sizeof(int);
                    has_head = true;
                }else{
                    continue;
                }
            }else{
                int t_size = hm.size;
                int t_type = hm.type;
                if(tail - head >= t_size){
                    handle_pack(buf+head,t_size,t_type);
                    head += t_size;
                    has_head = false;
                }
            }
        }
    }

    void handle_pack(char *body,int len,int type)
    {
        stringstream ss;
        info m_info;

        switch(type){
            case CMD_ON:
                cout << "CMD_ON \n";
                break;
            case CMD_INFO:
                cnt = ++cnt % 500000;
                m_info.ParseFromArray(body,len);
                ss << cnt;
                m_info.set_data("\n[msg:" + ss.str() + "]:\n" + m_info.data());
                cout << m_info.data() <<endl;
                write2(sock,m_info);
                break;
            case CMD_OFF:
                cout << "CMD_OFF" <<endl;
                break;
        }
    }

    int write2(int sockfd, ::google::protobuf::Message& msg)
    {
        char buf[MAX_PACK_SIZE] = {0};
        int len = 0;
        len = msg.ByteSizeLong();
        msg.SerializeToArray(buf,len);

        head_msg hm;
        hm.size = len;
        hm.type = CMD_INFO;

        int n1 = send(sockfd,(char *)&hm,MSG_HEAD_SZIE,0);
        int n = send(sockfd,buf,len,0);
        if(n1 < 0 || n < 0){
            if(errno != EAGAIN && errno != EWOULDBLOCK){
                perror("write");
                error = true;
                return -1; 
            }
        }
    }

    void handle() {
        if (error) return;
        read();
        if (error) return;
    }
    int done() const {
        return error || (read_end && (tail == head));
    }
};

static void setnonblocking(int fd)
{
    int flag = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flag | O_NONBLOCK);
}

static int setup_server_socket(int port)
{
    int sock;
    struct sockaddr_in sin;
    int yes=1;

    if ((sock = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket");
        exit(-1);
    }
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes);

    memset(&sin, 0, sizeof sin);

    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = htonl(INADDR_ANY);
    sin.sin_port = htons(port);

    if (bind(sock, (struct sockaddr *) &sin, sizeof sin) < 0) {
        close(sock);
        perror("bind");
        exit(-1);
    }

    if (listen(sock, 1024) < 0) {
        close(sock);
        perror("listen");
        exit(-1);
    }

    return sock;
}

static int sigign() 
{
    struct sigaction sa;
    sa.sa_handler = SIG_IGN;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGPIPE, &sa, 0);
    return 0;
}

int main(int argc, char *argv[])
{

    sigign();

    struct epoll_event ev;
    struct epoll_event events[MAX_EVENTS];
    int listener, epfd;

    int opt, port=DEFAULT_PORT;

    while (-1 != (opt = getopt(argc, argv, "p:"))) {
        switch (opt) {
        case 'p':
            port = atoi(optarg);
            break;
        default:
            fprintf(stderr, "Unknown option: %c\n", opt);
            return 1;
        }
    }

    listener = setup_server_socket(port);

    if ((epfd = epoll_create(128)) < 0) {
        perror("epoll_create");
        exit(-1);
    }

    memset(&ev, 0, sizeof ev);
    ev.events = EPOLLIN;
    ev.data.fd = listener;
    epoll_ctl(epfd, EPOLL_CTL_ADD, listener, &ev);

    printf("Listening port %d\n", port);

    struct timeval tim, tim_prev;
    gettimeofday(&tim_prev, NULL);
    tim_prev = tim;

    for (;;) {
        int i;
        int nfd = epoll_wait(epfd, events, MAX_EVENTS, -1);

        for (i = 0; i < nfd; i++) {
            if (events[i].data.fd == listener) {
                struct sockaddr_in client_addr;
                socklen_t client_addr_len = sizeof client_addr;

                int client = accept(listener, (struct sockaddr *) &client_addr, &client_addr_len);
                if (client < 0) {
                    perror("accept");
                    continue;
                }
                setnonblocking(client);
                memset(&ev, 0, sizeof ev);
                ev.events = EPOLLIN | EPOLLET;
                //ev.events = EPOLLIN;
                conn *pc = new conn(client);
                ev.data.ptr = (void *)pc;
                epoll_ctl(epfd, EPOLL_CTL_ADD, client, &ev);
                getnameinfo((struct sockaddr *)&client_addr,sizeof(client_addr),pc->buf_host,NI_MAXHOST,pc->buf_service,NI_MAXSERV,0);
                cout << pc->buf_host <<":"<<pc->buf_service << " connected" << endl;
            } else {
                conn *pc = (conn*)events[i].data.ptr;
                pc->handle();
                if (pc->done()) {
                    epoll_ctl(epfd, EPOLL_CTL_DEL, pc->sock, &ev);
                    close(pc->sock);
                    delete pc;
                }
            }
        }
    }
    return 0;
}
