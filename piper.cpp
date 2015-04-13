//A simple piping implementation for testing
#include <iostream>
#include <iomanip>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <unistd.h>
using namespace std;

int main()
{
	char *cmd1[] = {"/bin/ls", "-l", "/", 0}; // list the number of lines there are files
	//char *cmd2[] = {"/usr/bin/wc", "-w", "/", 0}; //count the number of words
	int fd[2], rs; //can have only 1 command here
    rs = pipe(fd);

    if(rs < 0) //check if broken
    {
        perror("broken pipe");
        exit(EXIT_FAILURE);
    }

    rs = fork(); //fork it here

    if(rs < 0) //check if broken after fork
    {
        perror("broken fork");
        exit(EXIT_FAILURE);
    }

    if(rs == 0) //child process
    {
        dup2(fd[0], STDIN_FILENO);
        close(fd[0]);
        rs = execvp(cmd1[0], cmd1); //run here
        if(rs == 0)
        {
            perror("broken execution");
            exit(EXIT_FAILURE);
        }
    }

    if(rs > 0)
    {
    	while(wait(NULL) != rs);
        dup2(fd[1], STDOUT_FILENO);
        close(fd[1]);
        rs = execvp(cmd1[0], cmd1);
        if(rs < 0) 
        {
            perror("broken execution");
            exit(EXIT_FAILURE);
        }
    }
    return 0;
} //int main()