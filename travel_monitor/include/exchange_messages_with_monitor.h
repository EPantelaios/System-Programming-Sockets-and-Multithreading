#ifndef EXCHANGE_MESSAGES_WITH_MONITOR_H
#define EXCHANGE_MESSAGES_WITH_MONITOR_H

#include <dirent.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <inttypes.h>
#include <pthread.h>

#include "bloom_filter.h"
#include "read_write_socket.h"
#include "country_list.h"


typedef struct child_info{

    pid_t child_pid;
    int fd_socket;
    int port;
    country_node *head_country;
    unsigned char **bloom_filter_array;
    int cnt_of_viruses;
    char **viruses_array;

}child_info;


int communicate_with_monitor_process(child_info *child_array, int num_monitors, int socket_buf_size, int cyclic_buf_size,
                                     int bloom_size, char *input_dir, int num_threads);


void print_monitors_info(child_info *child_array, int num_monitors);

#endif