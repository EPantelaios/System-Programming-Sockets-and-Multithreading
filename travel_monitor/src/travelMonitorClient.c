#include "../include/check_input.h"
#include "../include/travel_stats.h"
#include "../include/bloom_filter.h"
#include "../include/read_write_socket.h"
#include "../include/country_list.h"
#include "../include/exchange_messages_with_monitor.h"


/*
    Special Codes for the communication between parent and child process through the sockets

    1: Parent process sends basic info like socket_buffer_size, bloom_size and the paths of directories (countries) it will manage each child process.
    2: Child processs sends the viruses and bloom filters which it manages to parent process.
    3: For /travelRequest parent process send 'citizenID', 'virus_name' and 'country' to each child process 
       to give vaccination info for 'citizenID' in case it manages 'citizenID' itself.
    4: For /travelRequest child process sends 'YES' for the current citizenID.
    5: For /travelRequestTo child process sends 'NO' for the current citizenID.
    6: For /searchVaccinationStatus parent process send requests to all monitor processes to serve the request.
    7: For /searchVaccinationStatus monitor process send the info for the citizen.
    8: For /searchVaccinationStatus monitor process send the requests (vaccinations info) for the citizenID.
    9: For /searchVaccinationStatus monitor process send a terminated message when all vaccinations info have been sent.
    10: For /addVaccinationRecords parent process notifies the appropriate child process that new files have been added.
    11: For /addVaccinationRecords child process sends the (new) number of viruses to parent process.
    12: For /addVaccinationRecords child process sends the (new) viruses to parent process.
    13: For /addVaccinationRecords child process sends the updated bloom filters to parent process.
    13: For /exit parent process send message to terminate each process
*/



int main(int argc, char *argv[]){

    int num_monitors=0, socket_buf_size=0, cyclic_buf_size=0, bloom_size=0, num_threads=0;
    char *input_dir;
    const int size=1000;
    const int size_virus=100;
    const int buf_size=30;

    travel_requests travel_requests_stats;
    travel_requests_stats.accepted=0;
    travel_requests_stats.rejected=0;

    travel_stats *head_stats=NULL;

    //to generate pseudo-random numbers
    srand(time(NULL));

    int special_code=0, status=0;

    assert(!check_arguments(argc, argv,  &num_monitors, &socket_buf_size, &cyclic_buf_size, &bloom_size, &input_dir, &num_threads));

    child_info *child_array;
    child_array = (child_info *)calloc(num_monitors, sizeof(child_info));
    for(int i=0;i<num_monitors;i++){

        child_array[i].head_country=NULL;
    }

    //check if number of directories is less than the number of monitors.
    //if yes, assign number of directories to number of monitors
    assert(!check_number_of_counties(&num_monitors, input_dir));

    assert(!communicate_with_monitor_process(child_array, num_monitors, socket_buf_size, cyclic_buf_size, bloom_size, input_dir, num_threads));

    //parent process
    char *buf = (char *)calloc(size, sizeof(char));
    size_t buffersize=1000;

    char *token, *buffer, *tmp_buf, *aux_buf, *citizen_id_str, *country, *virus_name, *yes_or_no;
    assert(!initialize_aux_vars(&buffer, &tmp_buf, &aux_buf, &citizen_id_str, &country, &virus_name, 
                    &yes_or_no, buffersize, size_virus));

    const char word_separation_chars_without_dash[4] = " \n";
    char error_number[3]="-1";
    int citizen_id=0;
    int cnt_of_args=0;
    char *buf_ret_val=NULL;


    while(true){

        print_commands();

        //print monitors's PID and the countries that each of them manages
        print_monitors_info(child_array, num_monitors);

        buf_ret_val = fgets(buffer, buffersize, stdin);

        if(buf_ret_val==NULL){

            printf("False input. Try again...\n");
            continue;
        }
        
        if(!strcmp("\n", buffer)){

            continue;
        }
       
        strcpy(tmp_buf ,buffer);
        strcpy(aux_buf, buffer);

        tmp_buf=strtok(tmp_buf, "\n");
        token=strtok(buffer, word_separation_chars_without_dash);

        //Command: /travelRequest citizenID date countryFrom countryTo virusName
        if(!strcmp("/travelRequest", token)){

            date_node travel_date;
            char country_from[buf_size];
            char country_to[buf_size];
            cnt_of_args=count_arguments_without_dash_symbol(aux_buf);

            //correct number of args
            if(cnt_of_args!=6){

                printf("False input. Try again...\n");
                continue;
            }

            strcpy(aux_buf, tmp_buf);
            strtok(aux_buf, word_separation_chars_without_dash);

            token=strtok(NULL, word_separation_chars_without_dash);

            strcpy(citizen_id_str, token); 
            if(!check_if_contain_only_numbers_and_not_letters(citizen_id_str)){

                print_error_input(tmp_buf);
                return -1;
            }

            citizen_id=atoi(citizen_id_str);
            if(citizen_id<0){

                print_error_input(tmp_buf);
                continue;
            }

            //convert integer to string
            sprintf(citizen_id_str, "%d", citizen_id);


            travel_date.day=scan_file_date(aux_buf);
            travel_date.month=scan_file_date(aux_buf);
            travel_date.year=scan_file_date(aux_buf);
            if(!correct_date(travel_date.day, travel_date.month, travel_date.year)){

                print_error_input(tmp_buf);
                continue;
            }

            strcpy(country_from, scan_file_char(aux_buf));
            if(!strcmp(country_from, error_number)){

                print_error_input(tmp_buf);
                continue;
            }

            strcpy(country_to, scan_file_char(aux_buf));
            if(!strcmp(country_to, error_number)){

                print_error_input(tmp_buf);
                continue;
            }

            strcpy(virus_name, scan_file_char_virus_name(aux_buf));
            if(!strcmp(virus_name, error_number)){

                print_error_input(tmp_buf);
                continue;
            }

            int cnt_country_exist=-1;

            for(int i=0;i<num_monitors;i++){

                if(there_is_this_country(child_array[i].head_country, country_from)==true){

                    cnt_country_exist=i;
                    break;
                }
            }

            if(cnt_country_exist<0){

                printf("ERROR INPUT. There are not records for country %s\n", country_from);
                continue;
            }

            if(is_new_virus(child_array[cnt_country_exist].viruses_array, child_array[cnt_country_exist].cnt_of_viruses, virus_name)==true){

                printf("ERROR INPUT. There is not virus with name: '%s'\n", virus_name);
                continue;
            }

            int pos=find_index_virus_name(child_array[cnt_country_exist].viruses_array, child_array[cnt_country_exist].cnt_of_viruses, virus_name);

            travel_stats tmp_node_stats;
            tmp_node_stats.country_to=(char *)calloc(size_virus, sizeof(char));
            tmp_node_stats.virus_name=(char *)calloc(size_virus, sizeof(char));
            
            strcpy(tmp_node_stats.country_to, country_to);
            strcpy(tmp_node_stats.virus_name, virus_name);
        
            tmp_node_stats.date.day=travel_date.day;
            tmp_node_stats.date.month=travel_date.month;
            tmp_node_stats.date.year=travel_date.year;


            if(search_bloom_filter(child_array[cnt_country_exist].bloom_filter_array[pos], bloom_size, citizen_id_str)==false){

                printf("\n1: REQUEST REJECTED - YOU ARE NOT VACCINATED\n");
                travel_requests_stats.rejected++;
                
                tmp_node_stats.approved_request=0;
                insert_travel_stats(&head_stats, tmp_node_stats);
                
                free(tmp_node_stats.country_to);
                free(tmp_node_stats.virus_name);
            }
            else{
                
                clean_str(buf, size);
                sprintf(buf, "%d", citizen_id);
                write_to_socket(child_array[cnt_country_exist].fd_socket, buf, (int)strlen(buf), socket_buf_size, 3);

                write_to_socket(child_array[cnt_country_exist].fd_socket, virus_name, (int)strlen(virus_name), socket_buf_size, 3);

                write_to_socket(child_array[cnt_country_exist].fd_socket, country_from, (int)strlen(country_from), socket_buf_size, 3);


                clean_str(buf, size);
                special_code=read_from_socket(child_array[cnt_country_exist].fd_socket, buf, socket_buf_size);

                if(special_code==4){

                    date_node vaccination_date;

                    token=strtok(buf, "-");
                    vaccination_date.day=atoi(token);
                    token=strtok(NULL, "-");
                    vaccination_date.month=atoi(token);
                    token=strtok(NULL, "");
                    vaccination_date.year=atoi(token);

                    if(compare_date_six_months(travel_date, vaccination_date)==true){

                        printf("\nREQUEST ACCEPTED - HAPPY TRAVELS\n");
                        travel_requests_stats.accepted++;

                        tmp_node_stats.approved_request=1;
                        insert_travel_stats(&head_stats, tmp_node_stats);

                        free(tmp_node_stats.country_to);
                        free(tmp_node_stats.virus_name);
                    }
                    else{
                  
                        printf("\nREQUEST REJECTED - YOU WILL NEED ANOTHER VACCINATION BEFORE TRAVEL DATE\n");
                        travel_requests_stats.rejected++;

                        tmp_node_stats.approved_request=0;
                        insert_travel_stats(&head_stats, tmp_node_stats);

                        free(tmp_node_stats.country_to);
                        free(tmp_node_stats.virus_name);
                    }

                }
                else if(special_code==5){

                    printf("\n2: REQUEST REJECTED - YOY ARE NOT VACCINATED\n");
                    travel_requests_stats.rejected++;

                    tmp_node_stats.approved_request=0;
                    insert_travel_stats(&head_stats, tmp_node_stats);

                    free(tmp_node_stats.country_to);
                    free(tmp_node_stats.virus_name);
                }
            }
        }


        //Command: /travelStats virusName date1 date2 [country]
        else if(!strcmp("/travelStats", token)){

            date_node date1;
            date_node date2;
            char country_to[50];
            travel_requests ret_stats;
            ret_stats.accepted=0;
            ret_stats.rejected=0;

            cnt_of_args=count_arguments_without_dash_symbol(aux_buf);

            strcpy(aux_buf, tmp_buf);
            token=strtok(aux_buf, word_separation_chars_without_dash);

            //travelStats virusName date1 date2
            if(cnt_of_args==4){

                strcpy(virus_name, scan_file_char_virus_name(aux_buf));
                if(!strcmp(virus_name, error_number)){

                    print_error_input(tmp_buf);
                    continue;
                }

                date1.day=scan_file_date(aux_buf);
                date1.month=scan_file_date(aux_buf);
                date1.year=scan_file_date(aux_buf);
                if(!correct_date(date1.day, date1.month, date1.year)){
                    
                    print_error_input(tmp_buf);
                    continue;
                }

                date2.day=scan_file_date(aux_buf);
                date2.month=scan_file_date(aux_buf);
                date2.year=scan_file_date(aux_buf);
                if(!correct_date(date2.day, date2.month, date2.year)){
                    
                    print_error_input(tmp_buf);
                    continue;
                }

                ret_stats=traverse_travel_stats_list_without_country(head_stats, virus_name, date1, date2);

                printf("\nTOTAL REQUESTS %d\n", ret_stats.accepted + ret_stats.rejected);
                printf("ACCEPTED %d\n", ret_stats.accepted);
                printf("REJECTED %d\n", ret_stats.rejected);

            }
            //travelStats virusName date1 date2 country
            else if(cnt_of_args==5){

                strcpy(virus_name, scan_file_char_virus_name(aux_buf));
                if(!strcmp(virus_name, error_number)){

                    print_error_input(tmp_buf);
                    continue;
                }

                date1.day=scan_file_date(aux_buf);
                date1.month=scan_file_date(aux_buf);
                date1.year=scan_file_date(aux_buf);
                if(!correct_date(date1.day, date1.month, date1.year)){
                    
                    print_error_input(tmp_buf);
                    continue;
                }

                date2.day=scan_file_date(aux_buf);
                date2.month=scan_file_date(aux_buf);
                date2.year=scan_file_date(aux_buf);
                if(!correct_date(date2.day, date2.month, date2.year)){
                    
                    print_error_input(tmp_buf);
                    continue;
                }

                strcpy(country_to, scan_file_char(aux_buf));
                if(!strcmp(country_to, error_number)){

                    print_error_input(tmp_buf);
                    continue;
                }

                ret_stats=traverse_travel_stats_list(head_stats, country_to, virus_name, date1, date2);

                printf("\nCountry: %s\n", country_to);
                printf("TOTAL REQUESTS %d\n", ret_stats.accepted + ret_stats.rejected);
                printf("ACCEPTED %d\n", ret_stats.accepted);
                printf("REJECTED %d\n", ret_stats.rejected);

            }
            else{

                printf("False input. Try again...\n");
                continue;
            }
        }


        //Command: /addVaccinationRecords country
        else if(!strcmp("/addVaccinationRecords", token)){

            cnt_of_args=count_arguments_without_dash_symbol(aux_buf);

            //correct number of args
            if(cnt_of_args!=2){

                printf("False input. Try again...\n");
                continue;
            }
            
            strcpy(aux_buf, tmp_buf);

            token=strtok(aux_buf, word_separation_chars_without_dash);

            strcpy(country, scan_file_char(aux_buf));

            if(!strcmp(country, error_number)){

                print_error_input(tmp_buf);
                continue;
            }

            int i=0;
            for(i=0;i<num_monitors;i++){

                if(there_is_this_country(child_array[i].head_country, country)==true){

                    break;
                }
            }

            if(i == num_monitors){

                printf("Error. There is not country '%s' in the dataset\n", country);
                continue;
            }

            int index=i;

            for(int j=0;j<child_array[index].cnt_of_viruses;j++){

                free(child_array[index].viruses_array[j]);
            }
            free(child_array[index].viruses_array);

            delete_bloom_filter(child_array[index].bloom_filter_array, child_array[index].cnt_of_viruses);



            write_to_socket(child_array[index].fd_socket, country, (int)strlen(country), socket_buf_size, 10);


            clean_str(buf, size);
            special_code=read_from_socket(child_array[index].fd_socket, buf, socket_buf_size);

            if(special_code==11){
                
                child_array[index].cnt_of_viruses=atoi(buf);
                assert(!initialize_viruses_array(&child_array[index].viruses_array, child_array[index].cnt_of_viruses, size_virus));
            }
            else{

                assert(1==0);
            }

            for(int j=0;j<child_array[index].cnt_of_viruses;j++){

                clean_str(buf, size);
                special_code=read_from_socket(child_array[index].fd_socket, buf, socket_buf_size);
                if(special_code==12){

                     strcpy(child_array[index].viruses_array[j], buf);
                }
                else{

                    assert(1==0);
                }
            }

            assert(!initialize_bloom_filter(&child_array[index].bloom_filter_array, child_array[index].cnt_of_viruses));

            for(int j=0;j<child_array[index].cnt_of_viruses;j++){

                create_bloom_filter(&child_array[index].bloom_filter_array[j], bloom_size);            
                special_code=read_from_socket(child_array[index].fd_socket, (char *)child_array[index].bloom_filter_array[j], socket_buf_size);
                if(special_code!=13){

                    assert(1==0);
                }
            }

        }


        //Command: /searchVaccinationStatus citizenID
        else if(!strcmp("/searchVaccinationStatus", token)){

            cnt_of_args=count_arguments_without_dash_symbol(aux_buf);

            //correct number of args
            if(cnt_of_args!=2){

                printf("False input. Try again...\n");
                continue;
            }

            strcpy(aux_buf, tmp_buf);
            strtok(aux_buf, word_separation_chars_without_dash);

            token=strtok(NULL, word_separation_chars_without_dash);

            strcpy(citizen_id_str, token); 
            if(!check_if_contain_only_numbers_and_not_letters(citizen_id_str)){

                print_error_input(tmp_buf);
                return -1;
            }

            citizen_id=atoi(citizen_id_str);
            if(citizen_id<0){

                print_error_input(tmp_buf);
                continue;
            }

            //convert integer to string
            sprintf(citizen_id_str, "%d", citizen_id);

            for(int i=0;i<num_monitors;i++){
                
                write_to_socket(child_array[i].fd_socket, citizen_id_str, (int)strlen(citizen_id_str), socket_buf_size, 6);
            }

            int cnt_fds=0;
            fd_set current_fds, ready_fds;

            FD_ZERO(&current_fds);
            for(int i=0;i<num_monitors;i++){

                FD_SET(child_array[i].fd_socket, &current_fds);
            }

            struct timeval time_out;
            time_out.tv_sec = 0;
            time_out.tv_usec = 100000; // 0.1 second

            ready_fds = current_fds;

            int array_fds[num_monitors];

            for(int i=0;i<num_monitors;i++){

                array_fds[i]=child_array[i].fd_socket;
            }
            
            int max_fd=find_max_integer_from_array(array_fds, num_monitors);

            cnt_fds=select(max_fd+1, &ready_fds, NULL, NULL, &time_out);

            if(cnt_fds < 0){

                perror("Select() error");
                exit(-1);
            }
            else if(cnt_fds > 0){

                for(int i=3;i<max_fd+1;i++){

                    if(FD_ISSET(i, &ready_fds)){

                        pid_t wait_ret = waitpid(-1, &status, WNOHANG);
                        if(wait_ret!=0){

                            for(int j=0;j<num_monitors;j++){

                                if(wait_ret==child_array[j].child_pid){

                                    printf("\nChild with pid = -%d- is terminated\n", wait_ret);
                                    close(child_array[j].fd_socket);
                                    close(child_array[j].fd_socket);
                                    FD_CLR(i, &current_fds);
                                }
                            }

                            break;
                        }

                        printf("\n");
                        clean_str(buf, size);

                        while((special_code=read_from_socket(i, buf, socket_buf_size))!=9){

                            if(special_code==7 || special_code==8){
                                
                               printf("%s\n", buf);
                            }

                            clean_str(buf, size);
                        }

                    }
                }
            }
            else{

                printf("There are not vaccination info for this citizenID: '%d'\n", citizen_id);
            }
        }


        //Command: exit
        else if(!strcmp("/exit", token)){

            printf("\nExit...\n");


            //send special message to processes to terminated.
            clean_str(buf, size);
            strcpy(buf, "*Terminate Message*");

            for(int i=0;i<num_monitors;i++){

                write_to_socket(child_array[i].fd_socket, buf, (int)strlen(buf), socket_buf_size, 14);
            }

            pid_t wait_pid;
            while ((wait_pid = wait(&status)) > 0);

            //Create "log_file" with countries and requests
            FILE *fp=NULL;
            sprintf(buf, "log_file.%d", getpid());

            if((fp=fopen(buf, "w"))!=NULL){

                for(int i=0;i<num_monitors;i++){

                    //print all countries for current monitor process
                    country_node *current = child_array[i].head_country;

                    if(current==NULL){

                        fprintf(fp, "List is empty\n");
                    }
                    
                    while(current != NULL){

                        fprintf(fp, "%s\n", current->country);
                        current = current->next;
                    }
                }
                fprintf(fp, "\n");

                fprintf(fp, "TOTAL TRAVEL REQUESTS %d\n", travel_requests_stats.accepted + travel_requests_stats.rejected);
                fprintf(fp, "ACCEPTED %d\n", travel_requests_stats.accepted);
                fprintf(fp, "REJECTED %d\n", travel_requests_stats.rejected);

                fclose(fp);
            }


            for(int i=0;i<num_monitors;i++){
                
                delete_bloom_filter(child_array[i].bloom_filter_array, child_array[i].cnt_of_viruses);
                delete_list_country(&child_array[i].head_country);
            }

            for(int i=0;i<num_monitors;i++){

                for(int j=0;j<child_array[i].cnt_of_viruses;j++){

                    free(child_array[i].viruses_array[j]);
                }

                free(child_array[i].viruses_array);
                close(child_array[i].fd_socket);
            }
            free(child_array);

            delete_list_travel_stats(&head_stats);

            free(buffer);
            free(aux_buf);
            free(tmp_buf);
            free(buf);
            free(citizen_id_str);
            free(country);
            free(virus_name);
            free(yes_or_no);

            remove("Monitor_info");

            return 0;
        }

        else{

            printf("Invalid input. Try again.\n");
        }
    }
}