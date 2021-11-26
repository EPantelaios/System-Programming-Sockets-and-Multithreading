#include "../include/insert_entry.h"


int free_heap_insert_entry(info_citizen_node **citizen_node, vaccination_date_node **date_node, char **citizen_id_str, 
                           char **virus_name, char **yes_or_no){

    free((*citizen_node)->name);
    free((*citizen_node)->surname);
    free((*citizen_node)->country_list->country);
    free((*citizen_node)->country_list);
    free((*citizen_node));
    free((*citizen_id_str));
    free((*virus_name));
    free((*yes_or_no));
    free((*date_node));

    return 0;
}



int insert_entry(skip_list **skip_lists, char **viruses_array, int cnt_of_viruses, unsigned char **bloom_filter_array, 
                 int bloom_size, country_node **head_country, info_citizen_node **head_citizen, 
                 vaccination_date_node **head_date, int max_layer, char *buffer, char *tmp_buf, int number_of_args){

    //declare variables
    int size=100;
    int citizen_id=0;
    char *token;
    const char word_separation_chars_without_dash[4] = " \n";
    char error_number[3]="-1";
    const float probability_flip_coin=0.5;
    const int minimum_limit_value=0, maximum_limit_value=9999;
    const int minimum_limit_age=1, maximum_limit_age=120;
    int index=-1;

    info_citizen_node *citizen_node=NULL, *tmp_citizen=NULL;
    country_node *tmp_country=NULL;
    vaccination_date_node *date_node=NULL, *tmp_date=NULL;

    citizen_node=(info_citizen_node *)calloc(1, sizeof(info_citizen_node));
    citizen_node->name=(char *)calloc(size, sizeof(char));
    citizen_node->surname=(char *)calloc(size, sizeof(char));
    citizen_node->country_list=(country_node *)calloc(1, sizeof(country_node));
    citizen_node->country_list->country=(char *)calloc(size, sizeof(char));

    date_node=(vaccination_date_node *)calloc(1, sizeof(vaccination_date_node));

    char *citizen_id_str=(char *)calloc(size, sizeof(char));
    char *virus_name=(char *)calloc(size, sizeof(char));
    char *yes_or_no=(char *)calloc(size, sizeof(char));


    //take the first token (citizenID)
    token=strtok(buffer, word_separation_chars_without_dash);

    strcpy(citizen_id_str, token); 
    if(!check_if_contain_only_numbers_and_not_letters(citizen_id_str)){

        print_error_record(tmp_buf);
        free_heap_insert_entry(&citizen_node, &date_node, &citizen_id_str, &virus_name, &yes_or_no);
        return -1;
    }

    citizen_id=atoi(citizen_id_str);
    if(!value_range(citizen_id, minimum_limit_value, maximum_limit_value)){

        print_error_record(tmp_buf);
        free_heap_insert_entry(&citizen_node, &date_node, &citizen_id_str, &virus_name, &yes_or_no);
        return -2;
    }

    //convert integer to string
    sprintf(citizen_id_str, "%d", citizen_id);

    strcpy(citizen_node->name, scan_file_char(buffer));
    if(!strcmp(citizen_node->name, error_number)){

        print_error_record(tmp_buf);
        free_heap_insert_entry(&citizen_node, &date_node, &citizen_id_str, &virus_name, &yes_or_no);
        return -3;
    }

    strcpy(citizen_node->surname, scan_file_char(buffer));
    if(!strcmp(citizen_node->surname, error_number)){

        print_error_record(tmp_buf);
        free_heap_insert_entry(&citizen_node, &date_node, &citizen_id_str, &virus_name, &yes_or_no);
        return -4;
    }

    strcpy(citizen_node->country_list->country, scan_file_char(buffer));
    if(!strcmp(citizen_node->country_list->country, error_number)){

        print_error_record(tmp_buf);
        free_heap_insert_entry(&citizen_node, &date_node, &citizen_id_str, &virus_name, &yes_or_no);
        return -5;
    }

    citizen_node->age=scan_id_and_age(buffer);
    if(citizen_node->age<0){

        print_error_record(tmp_buf);
        free_heap_insert_entry(&citizen_node, &date_node, &citizen_id_str, &virus_name, &yes_or_no);
        return -6;
    }
    if(!value_range(citizen_node->age, minimum_limit_age, maximum_limit_age)){

        print_error_record(tmp_buf);
        free_heap_insert_entry(&citizen_node, &date_node, &citizen_id_str, &virus_name, &yes_or_no);
        return -7;
    }

    strcpy(virus_name, scan_file_char_virus_name(buffer));
    if(!strcmp(virus_name, error_number)){

        print_error_record(tmp_buf);
        free_heap_insert_entry(&citizen_node, &date_node, &citizen_id_str, &virus_name, &yes_or_no);
        return -8;
    }

    strcpy(yes_or_no, scan_file_char(buffer));
    if(!strcmp(yes_or_no, error_number)){

        print_error_record(tmp_buf);
        free_heap_insert_entry(&citizen_node, &date_node, &citizen_id_str, &virus_name, &yes_or_no);
        return -9;
    }

    if(check_yes_or_no(yes_or_no)!=true){

        print_error_record(tmp_buf);
        free_heap_insert_entry(&citizen_node, &date_node, &citizen_id_str, &virus_name, &yes_or_no);
        return -10;
    }

    //in case of YES and not contain date
    if(!strcmp(yes_or_no, "YES") && number_of_args==7){

        print_error_record(tmp_buf);
        free_heap_insert_entry(&citizen_node, &date_node, &citizen_id_str, &virus_name, &yes_or_no);
        return -11;
    }
    //in case of NO and contain date
    if(!strcmp(yes_or_no, "NO") && number_of_args==10){

        print_error_record(tmp_buf);
        free_heap_insert_entry(&citizen_node, &date_node, &citizen_id_str, &virus_name, &yes_or_no);
        return -12;
    }

    if(!strcmp(yes_or_no, "YES")){

        date_node->day=scan_file_date(buffer);
        date_node->month=scan_file_date(buffer);
        date_node->year=scan_file_date(buffer);

        if(!correct_date(date_node->day, date_node->month, date_node->year)){
            
            print_error_record(tmp_buf);
            free_heap_insert_entry(&citizen_node, &date_node, &citizen_id_str, &virus_name, &yes_or_no);
            return -13;
        }
    }

    if(is_new_virus(viruses_array, cnt_of_viruses, virus_name)==true){

        initialize_skip_lists_for_new_virus(skip_lists, max_layer, viruses_array, cnt_of_viruses, virus_name, probability_flip_coin);

        //create bloom filter for new virus
        int pos=find_index_virus_name(viruses_array, cnt_of_viruses, virus_name);
        create_bloom_filter(&bloom_filter_array[pos], bloom_size);
    }

    index=find_index_skip_list(skip_lists, cnt_of_viruses, virus_name);

    //Check if the record is already in the corresponding two skip lists
    if(there_is_that_entry_in_skip_list(skip_lists, index, citizen_id)==true){
        
        //print_error_record(tmp_buf);
        skip_list_node *skip_node = find_entry_skip_list(skip_lists, index, citizen_id);
        printf("ERROR: CITIZEN %d ALREADY VACCINATED ON %d-%d-%d\n", citizen_id, skip_node->date->day, skip_node->date->month, skip_node->date->year);
        free_heap_insert_entry(&citizen_node, &date_node, &citizen_id_str, &virus_name, &yes_or_no);
        return -14;
    }

    if(there_is_that_entry_in_skip_list(skip_lists, index+1, citizen_id)==true){

        printf("ERROR: CITIZEN %d HAS NOT BEEN VACCINATED AGAINST THE VIRUS %s\n", citizen_id, virus_name);
        free_heap_insert_entry(&citizen_node, &date_node, &citizen_id_str, &virus_name, &yes_or_no);
        return -15;
    }


    //Check if the record is already at country list
    if((tmp_country=search_country(*head_country, citizen_node->country_list->country))==NULL){

        tmp_country=insert_country(head_country, citizen_node->country_list->country);
    }

    //Check if the record is already at citizen list
    if((tmp_citizen=find_citizen(*head_citizen, citizen_node->name, citizen_node->surname, citizen_node->country_list->country, citizen_node->age))==NULL){

        tmp_citizen=insert_citizen(head_citizen, citizen_node->name, citizen_node->surname, tmp_country, citizen_node->age);
    }



    if(!strcmp(yes_or_no, "YES")){

        if((tmp_date=search_date(*head_date, date_node->day, date_node->month, date_node->year))==NULL){

            tmp_date=insert_date(head_date, date_node->day, date_node->month, date_node->year);
        }

        insert_skip_list(skip_lists, index, citizen_id, tmp_citizen, tmp_date);


        int pos=find_index_virus_name(viruses_array, cnt_of_viruses, virus_name);
        insert_bloom_filter(bloom_filter_array[pos], bloom_size, citizen_id_str);
    }
    else if(!strcmp(yes_or_no, "NO")){

        tmp_date=NULL;
        insert_skip_list(skip_lists, index+1, citizen_id, tmp_citizen, tmp_date);
    }


    free_heap_insert_entry(&citizen_node, &date_node, &citizen_id_str, &virus_name, &yes_or_no);

    return 0;
}






int add_vaccination_records(country_node **head_country, int stored_cnt_files, int fd_socket, char **cyclic_buffer,
                           int cyclic_buf_size, int socket_buf_size, int *total_files, pthread_mutex_t *mutex_buffer,
                           pthread_cond_t *cond_not_empty, pthread_cond_t *cond_not_full){


    struct dirent **dir_list;
    int cnt_files=0;
    const int size_country=100;
    const int size=1000;
    char *buffer=calloc(size, sizeof(char));
    char *tmp_buf=calloc(size, sizeof(char));
    char *aux_buf=calloc(size, sizeof(char));
    char *path_country=calloc(size_country, sizeof(char));
    char *aux_country=calloc(size_country, sizeof(char));

    country_node *head_country_path=NULL;

    //access the list of countries
    country_node *current = *head_country;

    while(current != NULL){

        clean_str(path_country, size_country);
        sprintf(path_country, "input_dir/%s/", current->country);

        DIR *dir=opendir(path_country);
        if(dir!=NULL){  //directory exists.

            cnt_files=scandir(path_country, &dir_list, NULL, alphasort);

            if(cnt_files!=stored_cnt_files){

                break;
            }
        }
        else if(ENOENT == errno){ //directory does not exist.

            printf("Directory '%s' does not exist. Exit...\n", path_country);
            exit(1);
        }
        else{  //opendir() failed for some other reason.

            printf("Failed to open directory '%s'\n", path_country);
            exit(2);
        }

        current = current->next;
    }


    for(int i=stored_cnt_files;i<cnt_files;i++){

        clean_str(aux_country, size_country);
        strcpy(aux_country, path_country);
        strcat(aux_country, dir_list[i]->d_name);

        insert_country(&head_country_path, aux_country);
    }

    *total_files = 0;

    country_node *tmp_current = head_country_path;

    while(tmp_current!=NULL){

        printf("Country_path = -%s-\n", tmp_current->country);
        *total_files = *total_files + 1;
        tmp_current = tmp_current->next;
    }


    current = head_country_path;

    while(current!=NULL){

        assert(!lock_mutex(mutex_buffer));

        while(buffer_is_full(cyclic_buffer, cyclic_buf_size)==true){

            pthread_cond_wait(cond_not_full, mutex_buffer);
        }

        while(current!=NULL && buffer_is_full(cyclic_buffer, cyclic_buf_size)==false){
            
            add_countries_to_cyclic_buffer(cyclic_buffer, cyclic_buf_size, current->country);

            current = current->next;
        }

        assert(!unlock_mutex(mutex_buffer));

        pthread_cond_broadcast(cond_not_empty);
    }


    if(current==NULL){

        while((*total_files) != -20){

            assert(!lock_mutex(mutex_buffer));

            pthread_cond_broadcast(cond_not_empty);

            pthread_cond_wait(cond_not_full, mutex_buffer);

            assert(!unlock_mutex(mutex_buffer));
        }
    }


    free(buffer);
    free(tmp_buf);
    free(aux_buf);

    //free auto-generated array from function scandir()
    for (int i=0;i<cnt_files;i++){

        free(dir_list[i]);
    }
    free(dir_list);

    return 0;
}