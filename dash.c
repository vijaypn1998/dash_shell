#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <stdbool.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>

extern int errno;
int num_paths = 1; //Initial path is /bin.
int status; // used for waiting for all child processes to complete.
int std_in_num = 0;
int std_out_num = 1;
int std_error_num = 2;

//0 is treated as success of a function execution and true for flags used.
//-1 is treated as failure of a function execution and false for flags used.

char *get_input(FILE **filename); //returns one line of input from filename, strips the newline character, returns NULL for no input.
int get_commands(char *each_line, char ***commands); //takes one line of input and stores all the commands in commands variable and returns number of commands.
char *prepend_path(char *command, char* path); // takes a command and a path and returns the concatinated output.
int exec_built_in(char *command, char ***paths); //takes a commannd(built-in) and updates the paths variable with all the paths provided in path command, returns 0/-1 after execution.
int exec_non_built_in(char *command, char **paths); // takes a command(non-built in) and paths to check, returns 0/-1 after execution.
int exec_command(char *command, char ***paths); //parent function to execute both built-in and non-built in commands, takes command and paths to read/update as innput.


char *get_input(FILE **filename){

    char *each_line;
	size_t buffer_size = 200; //initial set to 200, getline will relloc if > 200
	size_t characters;

    // use getline() to get the input string
    each_line = (char *)malloc(buffer_size * sizeof(char));
    if(each_line == NULL){
        perror("Error:");
        exit(0);
    }
    characters = getline(&each_line, &buffer_size, *filename);

    //for no input
    if(characters == -1){
        free(each_line);
        return NULL;
    }

    //Required to remove newline character at the end.
    each_line = each_line + characters - 1;
    if(*each_line == '\n') *each_line = '\0';
    each_line = each_line - characters + 1;

    return each_line;

}

int get_commands(char *each_line, char ***commands){

	size_t characters;
	char *outer_token;
	char *outer_ptr = NULL;
	const char multi_comm_delim[] = "&";
	int count_outer = 0;
    int command_length = 0;
    int temp;
    char space_c = ' ';
    char tab_c = '\t';
    int i = 0;

    characters = strlen(each_line) + 20; //incase empty input is fed , strlen(eachline) will be 0, adding 20 as a buffer length
    *commands = (char **)malloc(sizeof(char *) * characters);

    //*commands stores all the commands after splitting by "&", removes any leading/trailing white spaces.
    count_outer = 0;
    outer_token = strtok_r(each_line, multi_comm_delim, &outer_ptr);
    **commands = outer_token;
    while(outer_token != NULL){
        count_outer = count_outer + 1;
        outer_token = strtok_r(NULL, multi_comm_delim, &outer_ptr);
        *commands = *commands + 1;
        **commands = outer_token;
    }
    *commands = *commands - count_outer;

    while(**commands != NULL){
        // Traversing to the end of the command to make it easier to remove trailing spaces and get the length as well.
        command_length = 0;
        while(***commands != '\0'){
            **commands = **commands + 1;
            command_length++;
        }
        **commands = **commands - 1; //To make it point to the last character of the string
        
        // to remove trailing white spaces 
        i = command_length - 1;
        while(i > -1){
            if((***commands == space_c) || (***commands == tab_c)){
                command_length = command_length - 1;
                ***commands = '\0';
            }
            else break;
            **commands = **commands - 1 ;
            i--;
        }
        **commands = **commands - i; // to restore the pointer to the start of the string

        // to remove leading white spaces
        temp = command_length - 1;
        i = 0;
        while(i <= temp){
            if((***commands == space_c) || (***commands == tab_c)){
                command_length = command_length - 1;
                **commands = **commands + 1 ;
            }
            else break;
            i++;
        }
        *commands = *commands + 1;
    }
    *commands = *commands - count_outer;
    return count_outer;
}

char *prepend_path(char *command, char* path){

    int path_length = strlen(path);
    int com_length = 0;
    char *command_copy = NULL;
    char *outer_token;
    char *outer_ptr = NULL;
    const char space_delim[] = " ";
    int i = 0;
    char *p_command = NULL ;

    command_copy = (char *)malloc((strlen(command) + 1)* sizeof(char));
    strcpy(command_copy, command);
    outer_token = strtok_r(command_copy, space_delim, &outer_ptr);
    com_length = strlen(outer_token);
    p_command = (char *)malloc((path_length + com_length + 2) * sizeof(char));

    //concatenate path and command seperated by "/" and terminate by "\0"
    i = 0;
    while(i < path_length){
        *p_command = *path;
        path = path + 1;
        p_command = p_command + 1;
        i++;
    }
    path = path - path_length;
    *p_command = '/';
    p_command = p_command + 1;

    i = 0;
    while(i < com_length){
        *p_command = *outer_token;
        outer_token = outer_token + 1;
        p_command = p_command + 1;
        i++;
    }

    *p_command = '\0';
    p_command = p_command - com_length - path_length - 1;
    free(command_copy);
    return p_command;
}

int exec_built_in(char *command, char ***paths){

    char *command_copy = NULL;
    char *outer_token;
    char *inner_token;
    char *outer_ptr = NULL;
    char *inner_ptr = NULL;
    char *paths_check = NULL;
    const char space_delim[] = " ";
    int error_value = 0;
    struct stat sb;

    command_copy = (char *)malloc((strlen(command) + 1)* sizeof(char));
    strcpy(command_copy, command);
    outer_token = strtok_r(command_copy, space_delim, &outer_ptr);

    if(strcmp(outer_token, "exit") == 0){
        if(*outer_ptr == '\0') exit(0); //no extra string other than exit should be present.
        else error_value = -1;
    }

    else if(strcmp(outer_token, "cd") == 0){
        outer_token = strtok_r(NULL, space_delim, &outer_ptr);
        if(*outer_ptr == '\0'){ //Only 1 path is valid for cd.
            error_value = chdir(outer_token);
            free(command_copy);
            return error_value;
        }
        else{
            error_value = -1;
            free(command_copy);
            return error_value;
        }
    }

    else if(strcmp(outer_token, "path") == 0){
        paths_check = (char *)malloc((strlen(command) + 1) * sizeof(char));
        strcpy(paths_check, outer_ptr);
        inner_token = strtok_r(paths_check, space_delim, &inner_ptr);
        while(inner_token != NULL){
            if (stat(inner_token, &sb) != 0){ //Check if the path given in the command is valid.
                error_value = -1;
                free(command_copy);
                return error_value;
            }
            inner_token = strtok_r(NULL, space_delim, &inner_ptr);
        }
        free(paths_check);

        *paths = (char **)malloc(strlen(command) * sizeof(char *));
        outer_token = strtok_r(NULL, space_delim, &outer_ptr);

        //Update the paths given to the paths variable and also the number of paths.
        num_paths = 0;
        while(outer_token != NULL){
            **paths = outer_token;
            *paths = *paths + 1;
            num_paths++;
            outer_token = strtok_r(NULL, space_delim, &outer_ptr);
        }
        *paths = *paths - num_paths; //point it back to the 1st path.
        return error_value;

    }
    free(command_copy);
    return error_value;

}

int exec_non_built_in(char *command, char **paths){

    char *command_copy = NULL;
    char *command_copy2 = NULL;
    char *command_only = NULL;
    char *outer_ptr = NULL;
    char *inner_ptr = NULL;
    const char space_delim[] = " ";
    const char redirect_delim[] = ">";
    char *outer_token = NULL;
    char *inner_token = NULL;
    int error_value = 0;
    int path_flag = -1;
    int redirection_flag = -1;
    int redirection_count = 0;
    char *p_command = NULL;
    char *path_to_use = NULL;
    char **execv_args = NULL;
    char *file_name = NULL;
    int child_pid ;
    int i = 0;
    int loop_var = 0;
    int file_desc = -1;

    command_copy = (char *)malloc((strlen(command) + 1)* sizeof(char));
    command_copy2 = (char *)malloc((strlen(command) + 1)* sizeof(char));
    

    strcpy(command_copy, command);
    strcpy(command_copy2, command);
    command_only = strtok_r(command_copy, redirect_delim, &inner_ptr); //to get command without any redirection string if present.

    if(num_paths == 0){
        error_value = -1;
        return error_value;
    }

    //to check if the executable(command) is present in one of the paths.
    while(i < num_paths){
        path_to_use = (char *)malloc((strlen(*paths) + 1) * sizeof(char));
        strcpy(path_to_use, *paths);
        p_command = (char *)malloc((strlen(*paths) + strlen(command) + 2) * sizeof(char));
        p_command = prepend_path(command_only, path_to_use);
        if(access(p_command, X_OK) == 0){
            path_flag = 0;
            break;
        }
        else free(p_command);
        free(path_to_use);
        paths = paths + 1;
        i++;
    }

    if(path_flag == -1){
        error_value = -1;
        free(command_copy);
        free(command_copy2);
        return error_value;
    }

    //to check if redirection is present and has valid synxtax.
    if(path_flag == 0){
        for(i = 0 ; i < strlen(command_copy2) ; i++){
            if(command_copy2[i] == '>') redirection_count++;
            if(redirection_count > 1){
                free(p_command);
                free(path_to_use);
                free(command_copy);
                free(command_copy2);
                error_value = -1;
                return error_value;
            }
        }
        if(redirection_count == 1){ //To ensure only one ">" is present
            strtok_r(command_copy2, redirect_delim, &inner_ptr);
            inner_token = strtok_r(NULL, space_delim, &inner_ptr);
            if(*inner_ptr == '\0' && inner_token != NULL) {// Condition to ensure only 1 file is present for output redirection
                file_name = inner_token;
                redirection_flag = 0;
            }
            else{
                free(p_command);
                free(path_to_use);
                free(command_copy);
                free(command_copy2);
                error_value = -1;
                return error_value;
            }
        }
    }

    if(path_flag == 0 && redirection_flag == -1){ //For commands without output redirection
        loop_var = 0;
        execv_args = (char **)malloc((strlen(command_only) + strlen(p_command)) * sizeof(char *));
        outer_token = strtok_r(command_only, space_delim, &outer_ptr);
        execv_args[loop_var] = outer_token; // command

        // command arguments/parameters.
        outer_token = strtok_r(NULL, space_delim, &outer_ptr);
        while(outer_token != NULL){
            execv_args[++loop_var] = outer_token;
            outer_token = strtok_r(NULL, space_delim, &outer_ptr);
        }
        execv_args[++loop_var] = NULL; // NULL terminated array needed for execv.

        child_pid = fork();
        if(child_pid == -1){
            perror("Error:");
            exit(0);
        }
        else if(child_pid == 0){
            error_value = execv(p_command, execv_args);
        }
        free(execv_args);
        free(p_command);
        free(path_to_use);
        free(command_copy);
        free(command_copy2);
    }
    else if(path_flag == 0 && redirection_flag == 0){ //For commands with output redirection
        loop_var = 0;
        execv_args = (char **)malloc((strlen(command_only) + strlen(p_command)) * sizeof(char *));
        outer_token = strtok_r(command_only, space_delim, &outer_ptr);
        execv_args[loop_var] = outer_token; // command

        // command arguments/parameters.
        outer_token = strtok_r(NULL, space_delim, &outer_ptr);
        while(outer_token != NULL){
            execv_args[++loop_var] = outer_token;
            outer_token = strtok_r(NULL, space_delim, &outer_ptr);
        }
        execv_args[++loop_var] = NULL; // NULL terminated array needed for execv.

        child_pid = fork();
        if(child_pid == -1){
            perror("Error:");
            exit(0);
        }
        else if(child_pid == 0){
            file_desc = open(file_name, O_RDWR | O_CREAT | O_TRUNC, S_IRWXU); //opens file with owner read,write and exec permissions, creates if not present, truncate the file if present.
            if(file_desc == -1){
                perror("Error:");
                exit(0);
            }
            dup2(file_desc, std_out_num); //redirect stdout and stderror to file_desc
            dup2(file_desc, std_error_num);
            close(file_desc); // close the original file_desc as stdout and stderror are redirected
            error_value = execv(p_command, execv_args);
        }
        free(execv_args);
        free(p_command);
        free(path_to_use);
        free(command_copy);
        free(command_copy2);
    }
    return error_value;
}

int exec_command(char *command, char ***paths){
    
    char *command_copy = NULL;
    char *outer_token;
	char *outer_ptr = NULL;
	const char space_delim[] = " ";
    int error_value = -1;
    
    command_copy = (char *)malloc((strlen(command) + 1)* sizeof(char));
    strcpy(command_copy, command);
    outer_token = strtok_r(command_copy, space_delim, &outer_ptr);
    if(strcmp(outer_token, "path") == 0 || strcmp(outer_token, "cd") == 0 || strcmp(outer_token, "exit") == 0){ //for built-in commannds
        error_value = exec_built_in(command, paths);
    }
    else if(num_paths != 0){ // for non-built-in commannds , if there are non-zero number of paths.
        error_value = exec_non_built_in(command, *paths);
    }
    else{
        error_value = -1;
    }
    free(command_copy);
    return error_value;

}

int main(int argc, char **argv) {

    int num_commands = 0;
    int temp = 0;
    int error_value = 0;
    char *i_path = "/bin";
    char **u_path = &i_path;
    char error_message[30] = "An error has occurred \n";
    char *each_command = NULL;
    char **all_commands = NULL;
    char *input_file;
    char *input_line;
    int pid_wait;
    FILE *file_pointer;

    //if more than 1 command line argument is passed.
    if(argc > 2){
        exit(0);
    }

    //Interactive mode or input redirection from file.
    if(argc == 1){
        while(1){
            if(isatty(std_in_num)) // print dash only for interactive mode, not needed for input redirection from file.
                printf("dash> ");
            input_line = get_input(&stdin);
            if(input_line == NULL){ //for empty file(incase of input redirection).
                write(std_error_num, error_message, strlen(error_message));
                return 1;
            }
            num_commands = get_commands(input_line, &all_commands);
            if(*input_line == '\0'){ // for user pressing enter without typing anything.
                free(all_commands);
                continue;
            }
            else if(num_commands == 0)// for 0 commands(example user types "&").
                write(std_error_num, error_message, strlen(error_message));
            else{
                for(temp = 0; temp < num_commands ; temp++){
                    each_command = all_commands[temp];
                    error_value = exec_command(each_command, &u_path);
                    if(error_value != 0)
                        write(std_error_num, error_message, strlen(error_message));
                }
            }
            pid_wait = wait(&status); 
            while(pid_wait > 0){ //wait for all child processes to complete. 
                pid_wait = wait(&status);
            }
            free(input_line);
            free(all_commands);
        }
    }
    //Batch mode.
    else {
        input_file = argv[1];
        file_pointer = fopen(input_file, "r");
        if(file_pointer == NULL){
            perror("Unable to open file!");
            exit(1);
        }

        input_line = get_input(&file_pointer);
        if(input_line == NULL){ //for empty file
            write(std_error_num, error_message, strlen(error_message));
            return 1;
        }
        while(input_line != NULL){
            num_commands = get_commands(input_line, &all_commands);
            if(*input_line == '\0'){ // for empty line : continue without printing any error.
                free(all_commands);
                input_line = get_input(&file_pointer);
                continue;
            }
            else if(num_commands == 0)// for 0 commands.
                write(std_error_num, error_message, strlen(error_message));
            else{
                for(temp = 0; temp < num_commands ; temp++){
                    each_command = all_commands[temp];
                    error_value = exec_command(each_command, &u_path);
                    if(error_value != 0)
                        write(std_error_num, error_message, strlen(error_message));
                }
            }
            pid_wait = wait(&status);
            while(pid_wait > 0){ //wait for all child processes to complete. 
                pid_wait = wait(&status);
            }
            free(all_commands);
            free(input_line);
            input_line = get_input(&file_pointer);
        }
        fclose(file_pointer);
    }

    return 1;
}