## lsn 项目学习



组织框架

1. read line : 方式1. linux的readline函数，方式二，通过字符读取的方式getchar   拿到char* line
2. split_line: 将line解析成命令和参数  拿到char** args: 通过strtok函数
3. lsh_execute:   能够解析的命令 使用自己的函数来run, 还可以系统调用的方式来run(使用fork和execvp)



笔记点：

realloc函数

fprintf函数  stderr

strtok函数

fork和execvp  子进程的调用和回收

