#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include "internal.h"

extern char** sys_env;
extern char* history_file_path;

char* builtin_command_list[] = {"cd", "exit", "export", "!!", NULL};

void (*builtin_functions_map[])(char**, int) = {
    builtin_chdir, builtin_exit, builtin_export, builtin_history, NULL};

int get_args_len(char** args) {
    if (!args) {
        return -1;
    }
    int idx = 0;
    int len = 0;
    while (args[idx++]) {
        len++;
    }
    return len;
}

int check_builtin_cmd(char** args) {
    int idx = 0;
    int arg_num = get_args_len(args);
    void (*func)(char**, int) = NULL;

    if (args[0] && strchr(args[0], '!') == args[0] && strlen(args[0]) > 1 &&
        *(args[0] + 1) != '!') {
        builtin_history_no(args, arg_num, atoi(args[0] + 1));
        return 1;
    }

    while (builtin_command_list[idx]) {
        if (!strcmp(builtin_command_list[idx], args[0])) {
            func = builtin_functions_map[idx];
            func(args, arg_num);
            return 1;
        }
        idx++;
    }
    return 0;
}

void builtin_chdir(char** args, int arg_num) {
    if (arg_num != 2) {
        show_err("Wrong number of parameters");
        return;
    }
    char* target = args[1];
    char* msg_buf = (char*)malloc(4200);
    struct stat st;
    stat(target, &st);
    if (S_ISDIR(st.st_mode)) {
        if (chdir(args[1])) {
            snprintf(msg_buf, 4200, "Can not change current dir to '%s'",
                     target);
            show_err(msg_buf);
        }
    } else {
        snprintf(msg_buf, 4200, "'%s' is not a valid dir", target);
        show_err(msg_buf);
    }
    free(msg_buf);
    return;
}

void builtin_exit(char** args, int arg_num) {
    if (arg_num == 1) {
        exit(-1);
    } else {
        show_err("Wrong number of parameters");
    }
}

void builtin_export(char** args, int arg_num) {
    if (arg_num == 1) {
        int idx = 0;
        while (sys_env[idx]) {
            printf("%s\n", sys_env[idx++]);
        }
        return;
    }
    if (arg_num == 3) {
        setenv(args[1], args[2], 1);
        return;
    } else {
        show_err("Wrong number of parameters");
        return;
    }
}

int get_file_rows(const char *filename){
    int rows = 0;
    FILE* fp = fopen(filename, "r");
    while (!feof(fp)) {
        if (fgetc(fp) == '\n')
            rows++;
    }
    fclose(fp);
    return rows;
}

// top 10
void builtin_history(char** args, int arg_num) {
    int skip_rows = 0;
    int cmd_no = 0;
    char* buf = (char*)calloc(1024, 1);
    int rows = get_file_rows(history_file_path);

    FILE* fp = fopen(history_file_path, "r");

    skip_rows = rows > 10 ? rows - 10 : 0;
    cmd_no = rows > 10 ? 10 : rows;
    //printf("skip_rows: %d cmd_no: %d\n", skip_rows, cmd_no);

    for(int i = 0; i < skip_rows; i++){
        fscanf(fp,"%*[^\n]%*c");
    }

    while(fgets(buf, 1024, fp)) {
        printf("%d: %s", cmd_no--, buf);
        memset(buf, 0, 1024);
    }
    free(buf);
    fclose(fp);
}

// top 1-10
void builtin_history_no(char** args, int arg_num, int no) {
    int rows = get_file_rows(history_file_path);
    char* buf = (char*)calloc(1024, 1);
    FILE* fp = fopen(history_file_path, "r");
    if (no >= 1 && no <= 10) {
        if( no > rows){
            goto NO_HIS_CMD;
        }
        int skip_rows = rows - (no-1) - 1;

        for(int i = 0; i < skip_rows; i++){
            fscanf(fp,"%*[^\n]%*c");
        }
        if(fgets(buf, 1024, fp)){
            printf("%d: %s", no, buf);
        }
    } else {
        NO_HIS_CMD:
            show_err("No such command in history");
    }
    free(buf);
    fclose(fp);
}