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