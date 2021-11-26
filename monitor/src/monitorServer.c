#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include "../include/skip_list.h"
#include "../include/bloom_filter.h"
#include "../include/info_citizen_list.h"
#include "../include/country_list.h"
#include "../include/date_list.h"
#include "../include/check_input.h"
#include "../include/insert_entry.h"
#include "../include/initialize.h"
#include "../include/read_write_socket.h"
#include "../include/threads.h"



int main(int argc, char *argv[]){

    int cnt_of_viruses=0, stored_cnt_files=0;
    const int size=1000, size_virus=100, size_country=100;
    int num_l=25, special_code=0, addr_len=0, cnt_files=0, citizen_id=0;
    char date[10];
    char hostname[100];
    hostname[99]='\0';
    char IP[100];
    struct dirent **dir_list;
    unsigned long long int current_population_earth=7000000000;
    const int max_layer=(int)log2(current_population_earth);
    int server_fd=0, new_server_fd=0;
    char *buffer, *tmp_buf, *aux_buf, *citizen_id_str, *country, *virus_name, *yes_or_no;
    int port=0, num_threads=0, socket_buf_size=0, cyclic_buf_size=0, bloom_size=0;

    char *buf = calloc(size, sizeof(char));
    char *path_country=calloc(size_country, sizeof(char));
    char *aux_country=calloc(size_country, sizeof(char));

    //to generate pseudo-random numbers
    srand(time(NULL));

    travel_requests_monitor travel_requests_stats;
    travel_requests_stats.accepted=0;
    travel_requests_stats.rejected=0;

    //head pointers for citizen, country and date list
    info_citizen_node *head_citizen=NULL;
    country_node *head_country=NULL;
    country_node *head_country_path=NULL;
    vaccination_date_node *head_date=NULL;

    assert(!check_arguments_monitor(argc, argv, &port, &num_threads, &socket_buf_size, &cyclic_buf_size, &bloom_size, &head_country));

    assert(!initialize_aux_vars(&buffer, &tmp_buf, &aux_buf, &citizen_id_str, &country, &virus_name, 
                    &yes_or_no, size, size_virus));

    struct hostent *host_struct;

    gethostname(hostname, 100);
    host_struct = gethostbyname(hostname);
    if(host_struct == NULL){

        printf("gethostname() error: %s\n", hostname);
        exit(1);
    }

    //find external IP address
    struct in_addr **addr;
    addr = (struct in_addr **)host_struct->h_addr_list;
    strcpy(IP, inet_ntoa(*addr[0]));

    struct sockaddr_in server, client;
    memset(&server, '\0', sizeof(server));
    memset(&client, '\0', sizeof(client));

    server.sin_family = AF_INET;
    inet_aton(IP, (struct in_addr *)&server.sin_addr.s_addr);
    server.sin_port = htons(port);

    if((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0){

        perror("socket() Error");
        exit(1);
    }

    if(bind(server_fd, (struct sockaddr *) &server, sizeof(server)) < 0){

        perror("bind() Error");
        exit(1);
    }

    if (listen(server_fd, 10) < 0){

        perror("listen() Error");
        exit(1);
    }

    addr_len = sizeof(client);

    if((new_server_fd = accept(server_fd, (struct sockaddr *) &client, (socklen_t *) &addr_len)) < 0){

        perror("accept() Error");
        exit (EXIT_FAILURE);
    }



    cnt_of_viruses=count_viruses(num_l);
    int tmp_viruses=cnt_of_viruses*2;

    char **viruses_array;
    assert(!initialize_viruses_array(&viruses_array, cnt_of_viruses, size_virus));

    unsigned char **bloom_filter_array;
    assert(!initialize_bloom_filter(&bloom_filter_array, cnt_of_viruses));

    skip_list **skip_lists;
    assert(!initialize_skip_list(&skip_lists, tmp_viruses));

    char **cyclic_buffer = calloc(cyclic_buf_size, sizeof(sizeof(char *)));
    for(int i=0;i<cyclic_buf_size;i++){

        cyclic_buffer[i] = calloc(size_country, sizeof(char));
    }


    //access the list of countries
    country_node *current = head_country;

    while(current != NULL){

        clean_str(path_country, size_country);
        sprintf(path_country, "input_dir/%s/", current->country);

        DIR *dir=opendir(path_country);
        if(dir!=NULL){  //directory exists.

            cnt_files=scandir(path_country, &dir_list, NULL, alphasort);

            //store the number of files in case they are added new files
            stored_cnt_files=cnt_files;
        }
        else if(ENOENT == errno){ //directory does not exist.

            printf("Directory '%s' does not exist. Exit...\n", path_country);
            exit(1);
        }
        else{  //opendir() failed for some other reason.

            printf("Failed to open directory '%s'\n", path_country);
            exit(2);
        }


        for(int i=2;i<cnt_files;i++){

            clean_str(aux_country, size_country);
            strcpy(aux_country, path_country);
            strcat(aux_country, dir_list[i]->d_name);

            insert_country(&head_country_path, aux_country);
        }


        //free auto-generated array from function scandir()
        for (int i=0;i<cnt_files;i++){

            free(dir_list[i]);
        }
        free(dir_list);

        current = current->next;
    }


    //count how many different files there are
    int *total_files=calloc(1, sizeof(int));

    country_node *tmp_current = head_country_path;

    while(tmp_current!=NULL){

        *total_files = *total_files + 1;
        tmp_current = tmp_current->next;
    }


    pthread_mutex_t mutex_buffer;
    assert(!pthread_mutex_init(&mutex_buffer, NULL));

    pthread_cond_t cond_not_full, cond_not_empty;
    assert(!pthread_cond_init(&cond_not_full, NULL));
    assert(!pthread_cond_init(&cond_not_empty, NULL));


    thread_args *data_ptr = calloc(1, sizeof(thread_args));

    data_ptr->skip_lists = skip_lists;
    data_ptr->viruses_array = viruses_array;
    data_ptr->cnt_of_viruses = cnt_of_viruses;
    data_ptr->bloom_filter_array = bloom_filter_array;
    data_ptr->bloom_size = bloom_size;
    data_ptr->head_country = head_country;
    data_ptr->head_citizen = head_citizen;
    data_ptr->head_date = head_date;
    data_ptr->max_layer = max_layer;
    data_ptr->cyclic_buffer = cyclic_buffer;
    data_ptr->cyclic_buf_size = cyclic_buf_size;
    data_ptr->total_files = total_files;
    data_ptr->mutex_buffer = &mutex_buffer;
    data_ptr->cond_not_empty = &cond_not_empty;
    data_ptr->cond_not_full = &cond_not_full;


    pthread_t my_thread[num_threads];

    for(int i=0;i<num_threads;i++){

        assert(!pthread_create(&my_thread[i], NULL, start_routine_func, data_ptr));
    }


    current = head_country_path;

    while(current!=NULL){

        assert(!lock_mutex(&mutex_buffer));

        while(buffer_is_full(cyclic_buffer, cyclic_buf_size)==true){

            pthread_cond_wait(&cond_not_full, &mutex_buffer);
        }

        //placed the files in the circle cyclic buffer
        while(current!=NULL && buffer_is_full(cyclic_buffer, cyclic_buf_size)==false){
            
            add_countries_to_cyclic_buffer(cyclic_buffer, cyclic_buf_size, current->country);

            current = current->next;
        }

        assert(!unlock_mutex(&mutex_buffer));

        pthread_cond_broadcast(&cond_not_empty);
    }


    //wait for the last thread to read the file and store the data
    if(current==NULL){

        while((*total_files) != -20){

            assert(!lock_mutex(&mutex_buffer));

            pthread_cond_broadcast(&cond_not_empty);

            //wait until all files have been read and the structures have been updated
            pthread_cond_wait(&cond_not_full, &mutex_buffer);

            assert(!unlock_mutex(&mutex_buffer));
        }
    }


    //send the viruses and bloom_filters to travelMonitorClient
    int index_virus_array = max_index_virus_array(viruses_array, cnt_of_viruses);
    clean_str(buf, size);
    sprintf(buf, "%d", index_virus_array);
    write_to_socket(new_server_fd, buf, (int)strlen(buf), socket_buf_size, 2);

    for(int i=0;i<index_virus_array;i++){

        write_to_socket(new_server_fd, viruses_array[i], (int)strlen(viruses_array[i]), socket_buf_size, 2);
    }

    for(int i=0;i<index_virus_array;i++){
       
        write_to_socket(new_server_fd, (char *)bloom_filter_array[i], bloom_size, socket_buf_size, 2);
    }

    clean_str(buf, size);
    strcpy(buf, "*I am ready to serve requests*");
    write_to_socket(new_server_fd, buf, (int)strlen(buf), socket_buf_size, 2);



    //Child process wait for commands from parent.
    while(1){

        clean_str(buf, size);
        special_code=read_from_socket(new_server_fd, buf, socket_buf_size);

        if(special_code==3){ //travelRequest

            citizen_id=atoi(buf);

            clean_str(buf, size);
            special_code=read_from_socket(new_server_fd, buf, socket_buf_size);
            strcpy(virus_name, buf);

            clean_str(buf, size);
            special_code=read_from_socket(new_server_fd, buf, socket_buf_size);
            strcpy(country, buf);


            int index=find_index_skip_list(skip_lists, cnt_of_viruses, virus_name);

            skip_list_node *node=find_entry_skip_list(skip_lists, index, citizen_id);
            if(node!=NULL && !strcmp(node->info_citizen->country_list->country, country)){

                clean_str(buf, size);
                sprintf(date, "%d-%d-%d", node->date->day, node->date->month, node->date->year);
                strcpy(buf, date);

                write_to_socket(new_server_fd, buf, (int)strlen(buf), socket_buf_size, 4);
                travel_requests_stats.accepted++;
            }
            else{
                
                clean_str(buf, size);
                write_to_socket(new_server_fd, buf, 0, socket_buf_size, 5);
                travel_requests_stats.rejected++;
            }

        }
        else if(special_code==6){ //searchVaccinationStatus

            int cnt_nodes=0;
            char skip_list_buf[size_virus];

            citizen_id=atoi(buf);
            
            int max_index = max_index_skip_lists(viruses_array, cnt_of_viruses); 

            skip_list_node *node;

            for(int i=0;i<max_index+1;i++){

                if((node=find_entry_skip_list(skip_lists, i, citizen_id))!=NULL){
                    
                    //send at parent process the personal data of the citizen only the first time
                    if(cnt_nodes==0){
                        
                        clean_str(skip_list_buf, size_virus);
                        sprintf(skip_list_buf, "%d %s %s %s", node->citizenID, node->info_citizen->name, node->info_citizen->surname,
                                node->info_citizen->country_list->country);

                        write_to_socket(new_server_fd, skip_list_buf, (int)strlen(skip_list_buf), socket_buf_size, 7);

                        clean_str(skip_list_buf, size_virus);
                        sprintf(skip_list_buf, "AGE %d", node->info_citizen->age); 
                        write_to_socket(new_server_fd, skip_list_buf, (int)strlen(skip_list_buf), socket_buf_size, 7);

                        cnt_nodes++;
                    }

                    clean_str(skip_list_buf, size_virus);
                    if(i%2==0){   //For vaccinated skip_list
                        
                        clean_str(skip_list_buf, size_virus);
                        sprintf(skip_list_buf, "%s VACCINATED ON %d-%d-%d", skip_lists[i]->virus_name, node->date->day, node->date->month, node->date->year);
                        write_to_socket(new_server_fd, skip_list_buf, (int)strlen(skip_list_buf), socket_buf_size, 8);
                    }
                    else{   //For non-vaccinated skip list

                        clean_str(skip_list_buf, size_virus);
                        sprintf(skip_list_buf, "%s NOT YET VACCINATED", skip_lists[i]->virus_name);
                        write_to_socket(new_server_fd, skip_list_buf, (int)strlen(skip_list_buf), socket_buf_size, 8);
                    }
                }
            }

            if(cnt_nodes==1){

                //send terminated messages with special code: 9
                clean_str(skip_list_buf, size_virus);
                write_to_socket(new_server_fd, skip_list_buf, 0, socket_buf_size, 9);
            }

        }

        else if(special_code==10){ //addVaccinationRecords

            assert(!add_vaccination_records(&head_country, stored_cnt_files, new_server_fd, cyclic_buffer, cyclic_buf_size,
                                            socket_buf_size, total_files, &mutex_buffer, &cond_not_empty, &cond_not_full));


            index_virus_array = max_index_virus_array(viruses_array, cnt_of_viruses);

            clean_str(buf, size);
            sprintf(buf, "%d", index_virus_array);
            write_to_socket(new_server_fd, buf, (int)strlen(buf), socket_buf_size, 11);

            for(int i=0;i<index_virus_array;i++){

                write_to_socket(new_server_fd, viruses_array[i], (int)strlen(viruses_array[i]), socket_buf_size, 12);
            }

            for(int i=0;i<index_virus_array;i++){
            
                write_to_socket(new_server_fd, (char *)bloom_filter_array[i], bloom_size, socket_buf_size, 13);
            }


        }

        else if(special_code==14){ //exit

            if(!strcmp(buf, "*Terminate Message*")){

                FILE *fp=NULL;
                clean_str(buf, size);
                sprintf(buf, "log_file.%d", getpid());

                if((fp=fopen(buf, "w"))!=NULL){

                    //print all countries for current monitor process
                    country_node *current = head_country;

                    if(current==NULL){

                        fprintf(fp, "List is empty\n");
                    }
                    
                    while(current != NULL){

                        fprintf(fp, "%s\n", current->country);
                        current = current->next;
                    }

                    fprintf(fp, "\n");

                    fprintf(fp, "TOTAL TRAVEL REQUESTS %d\n", travel_requests_stats.accepted + travel_requests_stats.rejected);
                    fprintf(fp, "ACCEPTED %d\n", travel_requests_stats.accepted);
                    fprintf(fp, "REJECTED %d\n", travel_requests_stats.rejected);

                    fclose(fp);
                }

                delete_list_date(&head_date);
                delete_list_citizen(&head_citizen);
                delete_list_country(&head_country);
                delete_list_country(&head_country_path);

                delete_skip_list_structure(skip_lists, cnt_of_viruses*2);

                delete_bloom_filter(bloom_filter_array, cnt_of_viruses);

                for(int i=0;i<cnt_of_viruses;i++){

                    free(viruses_array[i]);
                }
                free(viruses_array);

                free(aux_country);
                free(path_country);
                free(buf);
                free(buffer);
                free(aux_buf);
                free(tmp_buf);
                free(citizen_id_str);
                free(country);
                free(virus_name);
                free(yes_or_no);
                free(data_ptr);
                free(total_files);

                pthread_mutex_destroy(&mutex_buffer);

                close(new_server_fd);
                close(server_fd);

                exit(0);
            }
        }

        else if(special_code==-1){

            printf("Parent pid = %d\n", getppid());
            printf("Parent process has terminated\n");
            close(new_server_fd);
            close(server_fd);

            exit(1);
        }

    }
}