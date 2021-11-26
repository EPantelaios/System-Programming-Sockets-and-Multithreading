#include "../include/threads.h"



int buffer_is_empty(char **cyclic_buffer, int cyclic_buf_size){

    for(int i=0;i<cyclic_buf_size;i++){

        if(strcmp(cyclic_buffer[i], "")){

            return false;
        }
    }

    return true;
}


int get_country_from_cyclic_buffer(char **cyclic_buffer, int cyclic_buf_size, char *buf){

    for(int i=0;i<cyclic_buf_size;i++){

        if(strcmp(cyclic_buffer[i], "")){

            strncpy(buf, cyclic_buffer[i], strlen(cyclic_buffer[i]));
            memset(buf + strlen(cyclic_buffer[i]), '\0', 1);

            clean_str(cyclic_buffer[i], 100);

            return 0;
        }
    }

    return 1;
}


int buffer_is_full(char **cyclic_buffer, int cyclic_buf_size){

    for(int i=0;i<cyclic_buf_size;i++){

        if(!strcmp(cyclic_buffer[i], "")){

            return false;
        }
    }

    return true;
}


int add_countries_to_cyclic_buffer(char **cyclic_buffer, int cyclic_buf_size, char *buf){

    for(int i=0;i<cyclic_buf_size;i++){

        if(!strcmp(cyclic_buffer[i], "")){

            clean_str(cyclic_buffer[i], 100);
            strncpy(cyclic_buffer[i], buf, strlen(buf));
            memset(cyclic_buffer[i] + strlen(buf), '\0', 1);

            return 0;
        }
    }

    return 1;
}


int lock_mutex(pthread_mutex_t *mutex_buffer){

    int error_code=0;

    if((error_code = pthread_mutex_lock(mutex_buffer))){

        strerror(error_code);
        return 1;
    }

    return 0;
}


int unlock_mutex(pthread_mutex_t *mutex_buffer){

    int error_code=0;

    if((error_code = pthread_mutex_unlock(mutex_buffer))){

        strerror(error_code);
        return 1;
    }

    return 0;
}


void *start_routine_func(void *data){

    thread_args *data_ptr = (thread_args *)data;

    insert_entry_thread(data_ptr->skip_lists, data_ptr->viruses_array, data_ptr->cnt_of_viruses, data_ptr->bloom_filter_array,
                        data_ptr->bloom_size, &data_ptr->head_country, &data_ptr->head_citizen, &data_ptr->head_date,
                        data_ptr->max_layer, data_ptr->cyclic_buffer, data_ptr->cyclic_buf_size, data_ptr->total_files,
                        data_ptr->mutex_buffer, data_ptr->cond_not_empty, data_ptr->cond_not_full);


    return 0;
}


int insert_entry_thread(skip_list **skip_lists, char **viruses_array, int cnt_of_viruses, unsigned char **bloom_filter_array,
                        int bloom_size, country_node **head_country, info_citizen_node **head_citizen,
                        vaccination_date_node **head_date, int max_layer, char **cyclic_buffer, int cyclic_buf_size,
                        int *total_files, pthread_mutex_t *mutex_buffer, pthread_cond_t *cond_not_empty, pthread_cond_t *cond_not_full){


    FILE *fp=NULL;
    int number_of_args=0;
    int lines=0;
    const int size=1000;
    const int size_virus=100;
    char *buffer, *tmp_buf, *aux_buf, *citizen_id_str, *country, *virus_name, *yes_or_no;

    assert(!initialize_aux_vars(&buffer, &tmp_buf, &aux_buf, &citizen_id_str, &country, &virus_name, 
                    &yes_or_no, size, size_virus));


    char *buf = (char *)calloc(size, sizeof(char));


    while(1){

        assert(!lock_mutex(mutex_buffer));

        while(buffer_is_empty(cyclic_buffer, cyclic_buf_size)==true){

            if((*total_files) == -20){

                pthread_cond_signal(cond_not_full);
            }

            pthread_cond_wait(cond_not_empty, mutex_buffer);
        }

        clean_str(buf, size);
        get_country_from_cyclic_buffer(cyclic_buffer, cyclic_buf_size, buf);

        assert(!unlock_mutex(mutex_buffer));

        lines = count_lines(buf);

        if((fp=fopen(buf, "r"))!=NULL){

            for(int j=0;j<lines;j++){

                if(fgets(buffer, size, fp)!=NULL){

                    assert(!lock_mutex(mutex_buffer));

                    strcpy(tmp_buf ,buffer);
                    strcpy(aux_buf, buffer);

                    if(buffer==NULL || buffer[0]=='\n'){

                        continue;
                    }

                    //check the format and the number of tokens of the record
                    number_of_args=correct_format_record(aux_buf);
                    if(number_of_args==false){

                        print_error_record(aux_buf);
                        continue;
                    }


                    insert_entry(skip_lists, viruses_array, cnt_of_viruses, bloom_filter_array, bloom_size, head_country, head_citizen,
                                            head_date, max_layer, buffer, tmp_buf, number_of_args);

                    
                    assert(!unlock_mutex(mutex_buffer));
                }
            }
        }

        fclose(fp);


        assert(!lock_mutex(mutex_buffer));


        *total_files = *total_files - 1;

        //if all files have been read
        if((*total_files) == 0){

            if(buffer_is_empty(cyclic_buffer, cyclic_buf_size)==true){

                *total_files = -20;
            }
        }

        assert(!unlock_mutex(mutex_buffer));

        pthread_cond_signal(cond_not_full);
    }


    free(buf);
    free(buffer);
    free(aux_buf);
    free(tmp_buf);
    free(citizen_id_str);
    free(country);
    free(virus_name);
    free(yes_or_no);

    pthread_exit(0);
}