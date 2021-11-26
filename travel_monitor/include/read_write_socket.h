#ifndef READ_WRITE_SOCKET_TRAVEL_H
#define READ_WRITE_SOCKET_TRAVEL_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include <unistd.h>

#include "initialize_travelMonitor.h"


int read_from_socket(int fd, char *buf, unsigned int buffer_size);

int write_to_socket(int fd, char *buf, int bytes_to_write, unsigned int buffer_size, int special_code);


#endif