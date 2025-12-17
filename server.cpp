#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<stdint.h>
#include<errno.h>
#include<math.h>
#include<time.h>
#include<poll.h>

#include<fcntl.h>
#include <unistd.h>
#include<arpa/inet.h>
#include<sys/socket.h>
#include<netinet/ip.h>

#include <string>
#include<vector>

#include "common.h"
#include "hashtable.h"
#include "zset.h"
#include "list.h"
#include "heap.h"
#include "thread_pool.h"

static void msg(const char* msg){
        fprintf(stderr,"%s\n", msg);
}

static void msg_errno(const char* msg){
    fprintf(stderr, "[errno:%d],%s\n",errno, msg);
}

static void die(const char* msg){
    fprintf(stderr, "[%d]%s\n",errno, msg);
    abort();
}

static uint64_t get_monotonic_msec(){
        struct timespec tv = {0,0};
        clock_gettime(CLOCK_MONOTIC, &tv);
        return uint64_t(tv.tv_sec)*1000 + tv.tv_sec/1000/1000;
}

static void fd_set_nb(int fb){
    errno = 0;
    int flags = fcntl(fd, F_GETFL,0);
    if (errno){
        die("fcntl error");
        return ;
    }
    flags |= O_NONBLOCK;
    errno = 0;
    (void)fcntl(fd, SET_FL, flags);
    if(errno){
        die("fcntl error");
    }
}

const size_t k_max_msg = 32<<20;
typedef std::vector<uint8_t> Buffer;

static void buf_append(Buffer &buf, const uint8_t *data, size_t len){
    buf.insert(buf.end(), data, data+len);
}

static void buf_consume(Buffer &buf, size_t n){
    buf.erase(buf.begin(), buf.begin()+n);
}

struct Conn{
    int fd = -1;
    bool want_read = false;
    bool want_write = false;
    bool want_close = false;
    Buffer incoming;
    Buffer outgoing;
    uint64_t last_active_ms = 0;
    DList  idle_node;

};

static struct{
    Hmap db;
    std::vector<Conn*> fd2conn;
    DList idle_list;
    std::vector<HeapItem> heap;
    Thread_Pool thread_pool;

 } g_data;

 static uint32_t handle_accept(int fd){
    struct sockaddr_in client_addr = {};
    socklen_t addrlen = sizeof(client_addr);
    int confid = accept(fd, (struct sockaddr*)&client_addr, &addrlen);
    if(confid<0){
        msg_errno("accept() error");
        return -1;
    }
    uint32_t ip = client_addr.sin_addr.s_addr;
    fprintf(stderr, "new client from %u.%u.%u.%u.%u\n",
        ip & 255, (ip >> 8)&255, (ip>>16) & 255, ip>>24,
        ntohs(client_add.sin_port)
    );
    fd_set_nb(connfd);
    Conn *conn = new conn();
    conn->fd = connfd;
    conn->want_read = true;
    conn->last_active_ms  =   get_monotonic_msec();
    dlist_insert_before(&g_data.idle_list, &conn->idle_node);

    if(g_data.fd2conn.size()<= (size_t)conn->fd){
        g_data.fd2conn.resize(conn->fd+1);
    }
    assert(!g_data.fd2conn[conn->fd]);
    g_data.fd2conn[conn->fd] = conn;
    return 0;
 }

 void conn_destroy(conn *conn){
    (void)close(conn->fd);
    g_data.fd2conn[conn->fd] = NULL;
    dlist_detach(&conn->idle_node);
    delete conn;
 }

 const size_t k_max_args = 200*1000;

 static bool read_u32(const uint8_t *&cur,const uint8_t *&end, const uint32_t &out){
    if(cur+4 >end){
        return false;

    }
    memcpy(&out, cur, 4);
    cur += 4;
    return true;
 }
  
 static bool read_str(const uint8_t *&cur, const uint8_t *&end,size_t n, const uint32_t &out){
    if(cur+n>end){
        return false;
    }
    out.assign(cur, cur+n);
    cur+=n;
    return true;
 }

 static int32_t parse_req(const uint8_t *data, size_t data, std::vector<std::string> &out){
    const uint8_t *end = data + size;
    uint32_t nstr = 0;
    if(!read_u32(data, end, nstr)){
        return -1;
    }
    if(nstr > k_max_args){
        return -1;
    }
    while(out.size()<nstr){
        uint32_t len = 0;
        if(!read_u32(data, end, len)){
            return -1;
        }
        out.push_back(std::string());
        if(!read_str(data, end, len, out.back())){
            return -1;
        }
    }
    if(data!= end){
        return -1;
    }
    return 0;
 }