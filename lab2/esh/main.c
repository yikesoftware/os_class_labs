#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include "internal.h"

char** sys_path_list;
char** sys_env;

char *history_file_path;

void show_err(const char* err_info) {
    printf("[*] Command Error: %s.\n", err_info);
}

void die(int errocde, const char* err_info) {
    printf("Error: %s!\n", err_info);
    exit(errocde);
}

int curr_perm() {
    return getuid() == 0 ? 0 : 1;
}

void type_prompt() {
    if (!curr_perm()) {
        printf("\033[1;35mesh:\033[0m\033[1;34m %s\033[0m\033[1;31m#\033[0m ",
               getcwd(NULL, 0));
    } else {
        printf("\033[1;35mesh:\033[0m\033[1;34m %s\033[0m\033[1;32m$\033[0m ",
               getcwd(NULL, 0));
    }
}

void args_destruct(char** args) {
    unsigned int idx = 0;
    while (args[idx]) {
        free(args[idx]);
        idx++;
    }
    free(args);
}


char** cmd_processor(const char* raw_cmd) {
    unsigned int length = strlen(raw_cmd);
    char* buf = (char*)calloc(length + 1, 1);
    strcpy(buf, raw_cmd);

    const char* start = raw_cmd;
    const char* end = raw_cmd + length;

    char** args = (char**)calloc(MAX_ARG_NUM * sizeof(size_t), 1);
    check_alloc(args);
    const char* word_end = NULL;
    unsigned int word_idx = 0;

    while (start < end) {
        if (word_idx > MAX_ARG_NUM) {
            show_err("Too much parameters");
            free(buf);
            args_destruct(args);
            return NULL;
        }
        word_end = strchr(start, CMD_SPLIT_CHAR);
        if (!word_end) {
            word_end = end;
        }

        if (word_end - start > MAX_ARG_LEN) {
            show_err("The parameter is too long");
            free(buf);
            args_destruct(args);
            return NULL;
        }

        args[word_idx] = (char*)calloc(word_end - start + 1, 1);
        check_alloc(args[word_idx]);
        strncpy(args[word_idx], start, word_end - start);
        word_idx++;

        start = word_end + 1;
    }
    free(buf);
    return args;
}

void strip_cmd(char *raw_cmd){
    char* cmd_striped = (char*)calloc(strlen(raw_cmd) + 1, 1);
    check_alloc(cmd_striped);

    const char* start = raw_cmd;
    const char* end = raw_cmd + strlen(raw_cmd) - 1;

    // lstrip
    while (*start == CMD_SPLIT_CHAR && start != end) {
        start++;
    }
    // rstrip
    while (*end == CMD_SPLIT_CHAR && start != end) {
        end--;
    }
    // strip all
    unsigned int wi = 0;
    int split_flag = 0;

    while (start <= end) {
        if (*start != CMD_SPLIT_CHAR) {
            cmd_striped[wi++] = *start;
            split_flag = 0;
        } else {
            if (split_flag == 0) {
                cmd_striped[wi++] = *start;
                split_flag = 1;
            } else {
                continue;
            }
        }
        start++;
    }
    strcpy(raw_cmd, cmd_striped);
    free(cmd_striped);
}

char* read_command(int fd, unsigned int maxlen) {
    char* buf = (char*)calloc(maxlen + 1, 1);
    check_alloc(buf);

    unsigned int wi = 0;
    char ch;
    for (unsigned int i = 0; i < maxlen; i++) {
        if (read(fd, &ch, 1) < 1) {
            break;
        }
        switch (ch) {
            case CMD_NEWLINE_CHAR:
                if (i + 1 < maxlen && read(fd, &ch, 1) < 1) {
                    show_err("Expect 'ENTER' to start a new line");
                    free(buf);
                    return NULL;
                }
                if (ch == CMD_NEWLINE_CHAR) {
                    buf[wi++] = CMD_NEWLINE_CHAR;
                    break;
                }
                if (ch != CMD_ENDLINE_CHAR) {
                    show_err("Expect 'ENTER' to start a new line");
                    free(buf);
                    return NULL;
                } else {
                    continue;
                }

            case CMD_ENDLINE_CHAR:
                buf[wi] = '\0';  // null byte ending
                return buf;

            default:
                buf[wi++] = ch;
                break;
        }
    }
    return buf;
}

char* get_full_binary_path(const char* binary) {
    int idx = 0;
    char* tmp = NULL;
    while (sys_path_list[idx]) {
        tmp = (char*)alloca(strlen(binary) + strlen(sys_path_list[idx]) + 1);
        strcpy(tmp, sys_path_list[idx]);
        strcat(tmp, "/");
        strcat(tmp, binary);
        int ac = access(tmp, X_OK);
        if (!ac) {
            return strdup(tmp);
        }
        idx++;
    }
    return NULL;
}

int run_command(char** args, int wait_flag) {
    if (!args)
        return -1;

    int arg_num = 0;
    while (args[arg_num]) {
        arg_num++;
    }
    if (!arg_num)
        return -1;


    if (check_builtin_cmd(args)) {
        goto fini_exec;
    }

    char* binary = args[0];
    char* filename = get_full_binary_path(binary);
    if (!filename) {
        char msg_buf[64];
        snprintf(msg_buf, 64, "Command '%s' not found", binary);
        show_err(msg_buf);
        return -1;
    }

    int pid = fork();
    if (pid < 0) {
        die(-1, "ERROR IN FORK");
    }

    if (pid == 0) {
        char msg_buf[64];
        int res = execve(filename, args, sys_env);
        if (res == -1) {
            snprintf(msg_buf, 64, "Command '%s' execute error", binary);
            show_err(msg_buf);
            return -1;
        }
    } else {
        if (wait_flag){
            waitpid(pid, NULL, 0);
        }
    }

fini_exec:
    return 0;
}

void set_sys_path_list() {
    char* sys_env_str = getenv("PATH");
    int path_num = 0;
    if (sys_env_str) {
        const char* seek = NULL;
        seek = strchr(sys_env_str, ':');
        while (seek != NULL) {
            path_num++;
            seek = strchr(seek + 1, ':');
        }
        path_num++;
    }

    sys_path_list = (char**)calloc(path_num * sizeof(size_t) + 1, 1);
    check_alloc(sys_path_list);
    int idx = 0;
    const char* start = sys_env_str;
    const char* end = sys_env_str + strlen(sys_env_str);
    const char* split = NULL;
    while (start < end) {
        split = strchr(start, ':');
        if (!split) {
            split = end;
        }
        sys_path_list[idx] = (char*)calloc(split - start + 1, 1);
        check_alloc(sys_path_list[idx]);
        strncpy(sys_path_list[idx], start, split - start);
        // printf("%s\n", sys_path_list[idx]);
        idx++;
        start = split + 1;
    }
}

void set_sys_environ(char** env) {
    set_sys_path_list();
    setenv("SHELL", "/bin/esh", 1);
    if (env == NULL) {
        show_err("Can not set environ");
        return;
    }
    sys_env = env;
    int idx = 0;
    char* key = NULL;
    char* value = NULL;
    char* split = NULL;
    while (env[idx]) {
        split = strchr(env[idx], '=');
        if (!split) {
            continue;
        }
        key = strndup(env[idx], split - env[idx]);
        value = strndup(split + 1, strlen(env[idx]) - strlen(key) - 1);
        setenv(key, value, 0);
        free(key);
        free(value);
        idx++;
    }
}

void add_cmd_to_history(const char *cmd){
    FILE *fp = fopen(history_file_path, "a+");
    if(fp){
        fprintf(fp, "%s\n", cmd);
        fclose(fp);
    }
}

int main(int argc, char* argv[], char** env) {
    setvbuf(stdin, 0, 2, 0);
    setvbuf(stdout, 0, 2, 0);
    setvbuf(stderr, 0, 2, 0);

    char *home_dir = getenv("HOME");
    if(home_dir){
        history_file_path = (char *)malloc(strlen(home_dir)+strlen(HISTORY_CMD_FILE)+0x10);
        strcpy(history_file_path, home_dir);
        strcat(history_file_path, "/");
        strcat(history_file_path, HISTORY_CMD_FILE);
    } else{
        die(0, "Env 'HOME' not found!");
    }

    if (access(history_file_path, F_OK) != 0) {
            creat(history_file_path, S_IRUSR | S_IWUSR);  // RW
    }

    if (access(history_file_path, R_OK | W_OK) != 0) {
        die(0, "Can not access history file");
    }

    set_sys_environ(env);

    while (1) {
        type_prompt();
        char* cmd = read_command(stdin->_fileno, MAX_CMD_LENGTH);
        if (!cmd) {
            show_err("Command reading error");
            continue;
        } else{
            strip_cmd(cmd);
        }

        if(strlen(cmd) == 0){
            free(cmd);
            continue;
        }

        int wait_flag = WAIT_TASK;
        if ((wait_flag = get_wait_flag(cmd)) == NOT_WAIT_TASK)
            *(cmd + strlen(cmd) - 1) = 0;

        char** args = cmd_processor(cmd);
        if (!args) {
            show_err("Command parsing error");
            free(cmd);
            continue;
        }

        run_command(args, wait_flag);

        if(cmd[0] != '!') add_cmd_to_history(cmd);

        free(cmd);
        args_destruct(args);
    }

    return 0;
}
