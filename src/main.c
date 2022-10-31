/***************************************************************************//**

  @file         main.c

  @author       Stephen Brennan

  @date         Thursday,  8 January 2015

  @brief        LSH (Libstephen SHell)

*******************************************************************************/

#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/*
  Function Declarations for builtin shell commands:
 */
int lsh_cd(char **args);
int lsh_help(char **args);
int lsh_exit(char **args);

/*
  List of builtin commands, followed by their corresponding functions.
 */
char *builtin_str[] = {
  "cd",
  "help",
  "exit"
};

int (*builtin_func[]) (char **) = {
  &lsh_cd,
  &lsh_help,
  &lsh_exit
};

int lsh_num_builtins() {
  return sizeof(builtin_str) / sizeof(char *);
}

/*
  Builtin function implementations.
*/

/**
   @brief Builtin command: change directory.
   @param args List of args.  args[0] is "cd".  args[1] is the directory.
   @return Always returns 1, to continue executing.
 */
int lsh_cd(char **args)
{
  //linux下，一切皆是文件。
  //而函数perror就是向stderr中输出，类似fprintf(stderr,"错误信息")。
  /*在所有的文件描述符中，有三个是已经被固定占用了，
  分别是stdin(文件描述符为0)、stdout(文件描述符为1)、stderr(文件描述符为2)。
  stdin是标准输入，默认是从键盘输入。
  stdout是标准输出，stderr是标准错误，它们两个的默认输出都是终端。
  而函数perror就是向stderr中输出，类似fprintf(stderr,"错误信息")。

另外stderr和stdout还有一个区别就是，stdout是带有行缓冲的，
所以一行的数据不会直接输出，而是遇见换行符才会输出这一行，
但是stderr是没有缓冲区的，它会直接输出。*/
  if (args[1] == NULL) {
    fprintf(stderr, "lsh: expected argument to \"cd\"\n");
  } else {
    //chdir C语言中的系统调用函数（同cd)
    if (chdir(args[1]) != 0) {
      perror("lsh");
    }
  }
  return 1;
}

/**
   @brief Builtin command: print help.
   @param args List of args.  Not examined.
   @return Always returns 1, to continue executing.
 */
int lsh_help(char **args)
{
  int i;
  printf("Stephen Brennan's LSH\n");
  printf("Type program names and arguments, and hit enter.\n");
  printf("The following are built in:\n");

  for (i = 0; i < lsh_num_builtins(); i++) {
    printf("  %s\n", builtin_str[i]);
  }

  printf("Use the man command for information on other programs.\n");
  return 1;
}

/**
   @brief Builtin command: exit.
   @param args List of args.  Not examined.
   @return Always returns 0, to terminate execution.
 */
int lsh_exit(char **args)
{
  return 0;
}

/**
  @brief Launch a program and wait for it to terminate.
  @param args Null terminated list of arguments (including program).
  @return Always returns 1, to continue execution.
 */
int lsh_launch(char **args)
{
  pid_t pid;
  int status;
  /*一、fork()函数

在Linux下有两个基本的系统调用可以用于创建子进程：fork()和vfork()，
当一个进程正在运行的时候，使用了fork()函数之后就会创建另一个进程。
与一般函数不同的是，fork()函数会有两次返回值，
一次返回给父进程（该返回值是子进程的PID（Process ID）），
第二次返回是给子进程，其返回值为0.
所以在调用该函数以后，我们需要通过返回值来判断当前的代码时父进程还是子进程在运行：

    返回值大于0 -> 父进程在运行
    返回值等于0 -> 子进程在运行
    返回值小于0 -> 函数系统调用出错

通常系统调用出错的原因有两个：①已存在的系统进程已经太多；
②该实际用户ID的进程总数已经超过了限制。
*/
  pid = fork(); //fork 和 execvp联合使用。 不先fork的话，execvp接管该进程执行命令，程序后面的就无法执行了
  if (pid == 0) {
    // Child process
    // 在UNIX中，如果要使用我们的C程序运行另一个程序，execvp（）函数将非常有用。
    // 该函数将要运行的UNIX命令的名称作为第一个参数。 
    // 第二个参数（ argv ）表示command的参数列表。 这是一个char*字符串数组。 
    // 在这里， argv包含完整的命令及其参数。 
    if (execvp(args[0], args) == -1) {
      perror("lsh");
    }
    exit(EXIT_FAILURE);
  } else if (pid < 0) {
    // Error forking
    perror("lsh");
  } else {
    // Parent process
    do {
      /*1. 判定等待集合的成员
        等待集合的成员是由参数 pid 来确定的：
（1）如果Pid>0，那么等待集合就是一个单独的子进程 ,它的进程 ID 等于 pid。
（2）如果Pid=-1，那么等待集合就是由父进程所有的子进程组成的。
默认情况下 (当 options=0 时 )，waitpid挂起调用进程的执行，
直到它的等待集合 (wait set) 中的一个子进程终止。
如果等待集合中的一个进程在刚调用的时刻就已经 终止了，
那么 waitpid 就立即返回 。
在这两种情况中，waitpid返回导致 waitpid 返回的已终止子进程的PID此时，
已终止的子进程已经被回收，内核会从系统中删除掉它的所有痕迹。
*/
      waitpid(pid, &status, WUNTRACED);
    } while (!WIFEXITED(status) && !WIFSIGNALED(status));
  }

  return 1;
}

/**
   @brief Execute shell built-in or launch program.
   @param args Null terminated list of arguments.
   @return 1 if the shell should continue running, 0 if it should terminate
 */
int lsh_execute(char **args)
{
  int i;

  if (args[0] == NULL) {
    // An empty command was entered.
    return 1;
  }

  for (i = 0; i < lsh_num_builtins(); i++) {
    if (strcmp(args[0], builtin_str[i]) == 0) {
      return (*builtin_func[i])(args);
    }
  }

  return lsh_launch(args);
}

/**
   @brief Read a line of input from stdin.
   @return The line from stdin.
 */
char *lsh_read_line(void)
{
#ifdef LSH_USE_STD_GETLINE
  char *line = NULL;
  ssize_t bufsize = 0; // have getline allocate a buffer for us
  if (getline(&line, &bufsize, stdin) == -1) {
    if (feof(stdin)) {
      exit(EXIT_SUCCESS);  // We received an EOF
    } else  {
      perror("lsh: getline\n");
      exit(EXIT_FAILURE);
    }
  }
  return line;
#else
#define LSH_RL_BUFSIZE 1024
  int bufsize = LSH_RL_BUFSIZE;
  int position = 0;
  char *buffer = malloc(sizeof(char) * bufsize);
  int c;

  if (!buffer) {
    fprintf(stderr, "lsh: allocation error\n");
    exit(EXIT_FAILURE);
  }

  //字符串转换成字符的处理
  while (1) {
    // Read a character
    c = getchar();

    if (c == EOF) {
      exit(EXIT_SUCCESS);
    } else if (c == '\n') {
      buffer[position] = '\0';
      return buffer;
    } else {
      buffer[position] = c;
    }
    position++;

    // If we have exceeded the buffer, reallocate.
    if (position >= bufsize) {
      bufsize += LSH_RL_BUFSIZE;
      buffer = realloc(buffer, bufsize);
      if (!buffer) {
        fprintf(stderr, "lsh: allocation error\n");
        exit(EXIT_FAILURE);
      }
    }
  }
#endif
}

#define LSH_TOK_BUFSIZE 64
#define LSH_TOK_DELIM " \t\r\n\a"
/**
   @brief Split a line into tokens (very naively).
   @param line The line.
   @return Null-terminated array of tokens.
 */
char **lsh_split_line(char *line)
{
  int bufsize = LSH_TOK_BUFSIZE, position = 0;
  char **tokens = malloc(bufsize * sizeof(char*));
  char *token, **tokens_backup;

  if (!tokens) {
    fprintf(stderr, "lsh: allocation error\n");
    exit(EXIT_FAILURE);
  }

  // 函数原型 char *strtok(char s[], const char *delim);
  // 根据delim分割字符分割字符，首次调用时指向要分解的字符串，之后再次调用时要把s设置为NULL
  // 发现line中字符串包含delim中包含的分割字符时，会将该字符改为‘\0’字符。
  // 使用该函数进行字符串分割时，会破坏源字符串，剩余字符串存在静态变量中，多线程访问该静态变量，会出现错误
  /*5、strtok_s函数
strtok_s是windows下的一个分割字符串安全函数，其函数原型如下：
char *strtok_s( char *strToken, const char *strDelimit, char **buf);
这个函数将剩余的字符串存储在buf变量中，而不是静态变量中，从而保证了安全性。
6、strtok_r函数
strtok_s函数是linux下分割字符串的安全函数，函数声明如下：
char *strtok_r(char *str, const char *delim, char **saveptr);
该函数也会破坏带分解字符串的完整性，但是其将剩余的字符串保存在saveptr变量中，保证了安全性。
*/
  token = strtok(line, LSH_TOK_DELIM);
  while (token != NULL) {
    tokens[position] = token;
    position++;

    if (position >= bufsize) {
      bufsize += LSH_TOK_BUFSIZE;
      tokens_backup = tokens;
      tokens = realloc(tokens, bufsize * sizeof(char*));
      if (!tokens) {
		    free(tokens_backup);
        fprintf(stderr, "lsh: allocation error\n");
        exit(EXIT_FAILURE);
      }
    }

    token = strtok(NULL, LSH_TOK_DELIM);
  }
  tokens[position] = NULL;
  return tokens;
}

/**
   @brief Loop getting input and executing it.
 */
void lsh_loop(void)
{
  char *line;
  char **args;
  int status;

  do {
    printf("> ");
    line = lsh_read_line();
    args = lsh_split_line(line);
    status = lsh_execute(args);

    free(line);
    free(args);
  } while (status);
}

/**
   @brief Main entry point.
   @param argc Argument count.
   @param argv Argument vector.
   @return status code
 */
int main(int argc, char **argv)
{
  // Load config files, if any.

  // Run command loop.
  lsh_loop();

  // Perform any shutdown/cleanup.

  return EXIT_SUCCESS;
}

