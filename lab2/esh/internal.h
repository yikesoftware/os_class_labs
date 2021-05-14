#define MAX_CMD_LENGTH 1024
#define CMD_ENDLINE_CHAR '\n'
#define CMD_NEWLINE_CHAR '\\'
#define CMD_SPLIT_CHAR ' '
#define MAX_ARG_NUM 64
#define MAX_ARG_LEN 128
#define HISTORY_CMD_FILE ".esh_history"

#define WAIT_TASK 1
#define NOT_WAIT_TASK 0

#define check_alloc(ptr) {                              \
    if(!(ptr)){die(-1, "MEMORY ALLOC ERROR!");}         \
}

#define get_wait_flag(cmd) *((cmd)+strlen(cmd)-1) == '&'? NOT_WAIT_TASK:WAIT_TASK

int get_args_len(char **args);

void show_err(const char *err_info);
void die(int errocde, const char *err_info);
int curr_perm();
void type_prompt();
char *read_command(int fd, unsigned int maxlen);
char **cmd_processor(const char *raw_cmd);
int run_command(char **args, int wait_flag);
void set_sys_path_list();

int check_builtin_cmd(char **args);

// builtin functions
void builtin_chdir(char **args, int arg_num);
void builtin_exit(char **args, int arg_num);
void builtin_export(char **args, int arg_num);
void builtin_history(char **args, int arg_num);
void builtin_history_no(char** args, int arg_num, int no);
