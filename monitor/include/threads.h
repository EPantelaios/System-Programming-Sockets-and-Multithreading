#ifndef THREADS_H
#define THREADS_H


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <dirent.h>
#include <stdbool.h>
#include <pthread.h>
#include <errno.h>

#include "check_input.h"
#include "skip_list.h"
#include "bloom_filter.h" 
#include "info_citizen_list.h"
#include "country_list.h"
#include "date_list.h"
#include "initialize.h"
#include "insert_entry.h"




typedef struct thread_args{

    skip_list **skip_lists;
    char **viruses_array;
    int cnt_of_viruses;
    unsigned char **bloom_filter_array;
    int bloom_size;
    country_node *head_country;
    info_citizen_node *head_citizen;
    vaccination_date_node *head_date;
    int max_layer;

    char **cyclic_buffer;
    int cyclic_buf_size;

    int *total_files;

    pthread_mutex_t *mutex_buffer;

    pthread_cond_t *cond_not_empty;
    pthread_cond_t *cond_not_full;

}thread_args;



void *start_routine_func(void *data);


int insert_entry_thread(skip_list **skip_lists, char **viruses_array, int cnt_of_viruses, unsigned char **bloom_filter_array, 
                        int bloom_size, country_node **head_country, info_citizen_node **head_citizen, 
                        vaccination_date_node **head_date, int max_layer, char **cyclic_buffer, int cyclic_buf_size,
                        int *total_files, pthread_mutex_t *mutex_buffer, pthread_cond_t *cond_not_empty, pthread_cond_t *cond_not_full);



int buffer_is_empty(char **cyclic_buffer, int cyclic_buf_size);


int get_country_from_cyclic_buffer(char **cyclic_buffer, int cyclic_buf_size, char *buf);


int buffer_is_full(char **cyclic_buffer, int cyclic_buf_size);


int add_countries_to_cyclic_buffer(char **cyclic_buffer, int cyclic_buf_size, char *buf);


int lock_mutex(pthread_mutex_t *mutex_buffer);


int unlock_mutex(pthread_mutex_t *mutex_buffer);


#endif