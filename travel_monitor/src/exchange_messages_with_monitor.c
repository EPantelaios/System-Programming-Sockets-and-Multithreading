#include "../include/exchange_messages_with_monitor.h"


int communicate_with_monitor_process(child_info *child_array, int num_monitors, int socket_buf_size, int cyclic_buf_size,
                                     int bloom_size, char *input_dir, int num_threads){

    int port_num=0, connect_status=0, ret_val=0;
    const int size=1000;
    const int size_virus=100;
    char IP[100];
    struct dirent **dir_list;
    int cnt_dirs=0;
    char *buf = (char *)calloc(size, sizeof(char));

    DIR *dir=opendir(input_dir);
    if(dir!=NULL){  //directory exists.

        cnt_dirs=scandir(input_dir, &dir_list, NULL, alphasort);

        //dont count "." and ".." directories
        cnt_dirs = cnt_dirs - 2;

        closedir(dir);
    }
    else if(ENOENT == errno){ //directory does not exist.

       printf("Directory '%s' does not exist. Exit...\n", input_dir);
       return -1;
    }
    else{  //opendir() failed for some other reason.

        printf("Failed to open directory '%s'\n", input_dir);
        return -2;
    }

    for(int i=0;i<cnt_dirs;i++){

        insert_country(&child_array[(i%num_monitors)].head_country, dir_list[i+2]->d_name);
    }


    port_num = rand()%16383 + 49152;

    for(int i=0;i<num_monitors;i++){

        child_array[i].port = port_num + i;

        char *exec_args[1000] = {"monitorServer", "-p", "", "-t", "", "-b", "", "-c", "", "-s", ""};

        clean_str(buf, size);
        sprintf (buf, "%d", child_array[i].port);
        exec_args[2] = strdup(buf);
        
        clean_str(buf, size);
        sprintf (buf, "%d", num_threads);
        exec_args[4] = strdup(buf);

        clean_str(buf, size);
        sprintf (buf, "%d", socket_buf_size);
        exec_args[6] = strdup(buf);

        clean_str(buf, size);
        sprintf (buf, "%d", cyclic_buf_size);
        exec_args[8] = strdup(buf);

        clean_str(buf, size);
        sprintf (buf, "%d", bloom_size);
        exec_args[10] = strdup(buf);

        country_node *current = child_array[i].head_country;

        int cur_index=11;

        while(current != NULL){
            
            exec_args[cur_index] = strdup(current->country);
            
            cur_index++;
            
            current = current->next;
        }

        exec_args[cur_index] = NULL;

        child_array[i].child_pid=fork();
        if(child_array[i].child_pid<0){

            perror("fork error");
            exit(1);
        }
        
        //for the child process
        if(child_array[i].child_pid==0){

            execv("./monitorServer", exec_args);
            exit(0);
        }

        free(exec_args[2]);
        free(exec_args[4]);
        free(exec_args[6]);
        free(exec_args[8]);
        free(exec_args[10]);

        for(int i=11;i<cur_index;i++){

            free(exec_args[i]);
        }
    }



    //find external IP address
    char hostname[100];
    hostname[99]='\0';
    struct hostent *host_struct;

    gethostname(hostname, 100);
    host_struct = gethostbyname(hostname);
    if(host_struct == NULL){

        printf("gethostname() error: %s\n", hostname);
        exit(1);
    }

    struct in_addr **addr;
    addr = (struct in_addr **)host_struct->h_addr_list;
    strcpy(IP, inet_ntoa(*addr[0]));

    for(int i=0;i<num_monitors;i++){

        struct sockaddr_in client;
        memset(&client, '\0', sizeof(client));

        client.sin_family = AF_INET;

        inet_aton(IP, (struct in_addr *)&client.sin_addr.s_addr);

        client.sin_port = htons(child_array[i].port);

        if((child_array[i].fd_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0){

            perror("socket() Error");
            exit(1);
        }

        do{

            connect_status = connect(child_array[i].fd_socket, (struct sockaddr *) &client, sizeof(client));
            //wait a little in ordert not to happen busy waiting
            usleep(10000);

        }while (connect_status < 0);
    }

    for(int i=0;i<num_monitors;i++){

        do{

            clean_str(buf, size);
            ret_val = read_from_socket(child_array[i].fd_socket, buf, socket_buf_size);
            //wait a little in ordert not to happen busy waiting
            usleep(10000);
        }while(ret_val == -1);

        child_array[i].cnt_of_viruses=atoi(buf);
        assert(!initialize_viruses_array(&child_array[i].viruses_array, child_array[i].cnt_of_viruses, size_virus));
    }


    for(int i=0;i<num_monitors;i++){

        for(int j=0;j<child_array[i].cnt_of_viruses;j++){

            clean_str(buf, size);
            read_from_socket(child_array[i].fd_socket, buf, socket_buf_size);
            strcpy(child_array[i].viruses_array[j], buf);
        }
    }


    for(int i=0;i<num_monitors;i++){

        assert(!initialize_bloom_filter(&child_array[i].bloom_filter_array, child_array[i].cnt_of_viruses));
        
        for(int j=0;j<child_array[i].cnt_of_viruses;j++){

            create_bloom_filter(&child_array[i].bloom_filter_array[j], bloom_size);
            read_from_socket(child_array[i].fd_socket, (char *)child_array[i].bloom_filter_array[j], socket_buf_size);
        }
    }


    for(int i=0;i<num_monitors;i++){

        clean_str(buf, size);
        read_from_socket(child_array[i].fd_socket, buf, socket_buf_size);

        if(!strcmp(buf, "*I am ready to serve requests*")){

            printf("\nMonitor Process with pid: %d is ready to serve requests!\n", child_array[i].child_pid);
        }
    }


    for (int i=0;i<cnt_dirs+2;i++){

        free(dir_list[i]);
    }
    free(dir_list);

    free(buf);

    return 0;
}




void print_monitors_info(child_info *child_array, int num_monitors){

    FILE *fp=NULL;

    remove("Monitor_info");

    if((fp=fopen("Monitor_info", "w"))!=NULL){

        for(int i=0;i<num_monitors;i++){

            fprintf(fp, "%d. Monitor PID: %d\n", i+1, child_array[i].child_pid);

            //print all countries for current monitor process
            country_node *current = child_array[i].head_country;

            if(current==NULL){

                fprintf(fp, "List is empty\n");
            }
            
            while(current != NULL){

                fprintf(fp, "%s\n", current->country);
                current = current->next;
            }

            fprintf(fp, "\n");
        }

        fclose(fp);
    }
}