#include "proto/x.pb.h"

#include <google/protobuf/message_lite.h>

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

#define MAX_PACK_SIZE    (4 * 1024)

using namespace std;

struct msg_head
{
    uint32_t size;
    uint32_t type;
};

struct conn {
    int cnt;
    int sock;
    char *buf;
    size_t alloced, head, tail;
    bool read_end;
    bool error;

    conn(int sock) : sock(sock), cnt(0),buf(0), head(0), tail(0), read_end(false), error(false)
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
                    if (errno != EAGAIN && errno != EWOULDBLOCK) {
                        break;
                    }
                    perror("read");
                    error = true;
                    return;
                }
                if (n == 0) {
                    read_end = true;
                    return;
                }
                tail += n;
            }

            int head_size = sizeof(msg_head);
            msg_head p_mh;
            if(tail - head >= head_size){
                p_mh = *(msg_head *)(buf+head);
                head += head_size;
            }
            if(tail - head >= p_mh.size){
                handle_pack(buf+head,p_mh.size,p_mh.type);
                head += p_mh.size;
            }
        }
    }

    void handle_pack(char *body,int len,int type)
    {
        stringstream ss;
        info m_info;

        switch(type){
            case CMD_ON:
                //
                break;
            case CMD_INFO:
                cnt = ++cnt % 500000;
                m_info.ParseFromArray(body,len);
                ss << cnt;
                m_info.set_data("\n[msg : " + ss.str() + "]:\n" + m_info.data());
                write2(sock,m_info);
                break;
            case CMD_OFF:
                //
                break;

        }
    }

    int write2(int sockfd, ::google::protobuf::Message& msg)
    {
        char buf[MAX_PACK_SIZE] = {0};
        int len = 0;
        msg.SerializeToArray(buf,len);
        msg_head mh;
        mh.size = len;
        mh.type = CMD_INFO;
        int n1 = send(sockfd,(void *)&mh,sizeof(mh),0);
        int n = send(sockfd,buf,len,0);
        if(n1 < 0 || n < 0){
            if(errno != EAGAIN){
                perror("write");
                error = true;
                return -1; 
            }
        }
    }

    int write() {
        while (head < tail) {
            int n = ::write(sock, buf+head, tail-head);
            if (n < 0) {
                if (errno == EAGAIN) break;
                perror("write");
                error = true;
                return -1;
            }
            // n >= 0
            head += n;
        }
        return tail-head;
    }

    void handle() {
        if (error) return;
        read();
        if (error) return;
        write();
    }
    int done() const {
        return error || (read_end && (tail == head));
    }
};

struct connect_manager
{
private:
    int  on_lines;
    int  sockets[MAX_CONNECT];
public:

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

int main(int argc, char *argv[])
{
    struct epoll_event ev;
    struct epoll_event events[MAX_EVENTS];
    int listener, epfd;
    int procs=1;

    int opt, port=DEFAULT_PORT;

    while (-1 != (opt = getopt(argc, argv, "p:f:"))) {
        switch (opt) {
        case 'p':
            port = atoi(optarg);
            break;
        case 'f':
            procs = atoi(optarg);
            break;
        default:
            fprintf(stderr, "Unknown option: %c\n", opt);
            return 1;
        }
    }

    listener = setup_server_socket(port);

    for (int i=1; i<procs; ++i)
        fork();

    if ((epfd = epoll_create(128)) < 0) {
        perror("epoll_create");
        exit(-1);
    }

    memset(&ev, 0, sizeof ev);
    ev.events = EPOLLIN;
    ev.data.fd = listener;
    epoll_ctl(epfd, EPOLL_CTL_ADD, listener, &ev);

    printf("Listening port %d\n", port);

    unsigned long proc = 0;
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
                ev.data.ptr = (void*)new conn(client);
                epoll_ctl(epfd, EPOLL_CTL_ADD, client, &ev);
            } else {
                conn *pc = (conn*)events[i].data.ptr;
                pc->handle();
                proc++;
                if (pc->done()) {
                    epoll_ctl(epfd, EPOLL_CTL_DEL, pc->sock, &ev);
                    delete pc;
                }
            }
        }

        if (proc > 100000) {
            proc = 0;
            gettimeofday(&tim, NULL);
            timersub(&tim, &tim_prev, &tim_prev);
            long long d = tim_prev.tv_sec;
            d *= 1000;
            d += tim_prev.tv_usec / 1000;
            printf("%lld msec per 100000 req\n", d);
            printf("%lld reqs per sec\n", 100000LL*1000/d);
            tim_prev = tim;
        }
    }
    return 0;
}
