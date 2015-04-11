/* A simple shell for ECS 150
 * Filename: ashell.cpp
 * Authors: Felix Ng, John Garcia
 *
 * In this version:
 *      input parsing - done!
 *      history - done!
 *      cd - done!
 *      pwd - done!
 *      ls - done!
 *      exit - done!
 *      app opening - done!
 *      piping - not started
 *      online source for file permissions implementation: 
 *      http://stackoverflow.com/questions/10323060/printing-file-permissions-like-ls-l-using-stat2-in-c
*/

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <ctype.h>
#include <string>
#include <queue>
#include <iostream> //for testing only******
using namespace std;
 
void ResetCanonicalMode(int fd, struct termios *savedattributes)
{
    tcsetattr(fd, TCSANOW, savedattributes);
}
 
void SetNonCanonicalMode(int fd, struct termios *savedattributes)
{
    struct termios TermAttributes;
    
    //Make sure stdin is a terminal
    if(!isatty(fd)){
        fprintf (stderr, "Not a terminal.\n");
        exit(0);
    }
   
    //Save the terminal attributes so we can restore them later
    tcgetattr(fd, savedattributes);
   
    //Set the funny terminal modes
    tcgetattr (fd, &TermAttributes);
    TermAttributes.c_lflag &= ~(ICANON | ECHO); // Clear ICANON and ECHO.
    TermAttributes.c_cc[VMIN] = 1;
    TermAttributes.c_cc[VTIME] = 0;
    tcsetattr(fd, TCSAFLUSH, &TermAttributes);
} //void ResetCanonicalMode()

string history(queue<string> *myHistory, string input, int printCnt); //prototype
 
string getInput(queue<string> *myHistory) 
{
    struct termios SavedTermAttributes;
    char rawIn;
    int histCount = 0; //history counter for up and down keys
    string formattedIn;
    SetNonCanonicalMode(STDIN_FILENO, &SavedTermAttributes); 
    write(STDOUT_FILENO, ">", 1);
       
    while(1)
    {
        read(STDIN_FILENO, &rawIn, 1); //always read for input
        if(0x04 == rawIn) break; //Ctrl-d
        if(rawIn == '\n') break; //Enter key

        if(rawIn == 0x1B) //up and down arrow check
        {
            read(STDIN_FILENO, &rawIn, 1);
            if(rawIn == 0x5B)
            {
                read(STDIN_FILENO, &rawIn, 1); //keep reading input

                if (rawIn == 0x41) //up arrow
                {
                    if(histCount < myHistory->size())
                        histCount++;
                    else
                        write(1, "\a", 1); //bell

                    if(myHistory->size() > 0) 
                        formattedIn = history(myHistory, "`u", histCount);

                    continue;
                }

                if(rawIn == 0x42) //down arrow
                {
                    histCount--;
                    if(histCount <= 0)
                    {   
                        for(int i = 0; i < 100 ; i++ )
                            write (1, "\b \b", 3); //lazy backspaces
                        write(1, "\a>", 2); //also a bell
                        histCount = 0;
                        formattedIn.clear();
                        continue;
                    }

                    formattedIn = history(myHistory, "`d", histCount);
                    continue;
                    //return "`d";
                }

                if(rawIn == 0x43 || rawIn == 0x44)
                    continue; //left and right keys dont matter so continue
            }
        }

        if (rawIn == 0x7F || rawIn == 0x08) //mac "Delete"  or "backspace"
        {   
            if (formattedIn.empty()) //if nothing to delete play bell
            {             
                write(STDOUT_FILENO, "\a", 1);
            } 

            else 
            {
                write(STDOUT_FILENO, "\b \b", 3);
                formattedIn.resize(formattedIn.length() - 1);
            }
        } 

        else
        {
            if(isprint(rawIn))
            {
                write(STDOUT_FILENO, &rawIn, 1); //printf("RX: '%c' 0x%02X\n",RXChar, RXChar);
                formattedIn += rawIn; //stores input into a string
            }
            
            else
            {
                write(STDOUT_FILENO, "X", 1); //printf("RX: ' ' 0x%02X\n",RXChar);
                formattedIn += '\a';
            }
        }
    } //while input

    write(STDOUT_FILENO, "\n", 1);
   
    ResetCanonicalMode(STDIN_FILENO, &SavedTermAttributes);
    return formattedIn;
} //string getInput()
 
string history(queue<string> *myHistory, string input, int printCnt)
{
    queue<string> printQ; //queue only for a history of 10 elements

    if(input == "`u" || input == "`d") //do not record up and down arrow keys into history
    {
        queue<string> printQ = *myHistory;
        int printQsize = printQ.size();

        for(int i = 0; i < 100 ; i++ )
            write (1, "\b \b", 3); //lazy backspaces

        for(int i = 0; i < printQsize; i++)
        {
            if(i == printQsize - printCnt)
            {
                write(1, ">", 1);
                write(1, printQ.front().data(), printQ.front().length());
                return printQ.front();
            }
            printQ.pop();
        }
    }

    else
        myHistory->push(input); //all other input

    if(myHistory->size() > 10) //get rid of the element that is not the recent 10
        myHistory->pop();
 
    return myHistory->front();
} //int history()

void printHistory(queue<string> printQ)
{
    int n = printQ.size();

    for(int i = 0; i < n ; i++)
    {
        char c = i + 48;
        write(STDOUT_FILENO, &c, 1);
        write(STDOUT_FILENO, " ", 1);
        write(1, printQ.front().data(), printQ.front().length());
        write(1, "\n", 1);
        printQ.pop();
    }
} //void printHistory()

string pwd()
{
    string pwd;
    char cwd[50];
    getcwd(cwd, 50);
    pwd += cwd;
    return pwd;
} //string pwd()
 
int main(int argc, char *argv[])
{
    queue<string> myHistory; //queue for all history
 
    while(1)
    {
        string input = getInput(&myHistory); //always waiting for input
               
        string command = input.substr(0, input.find_first_of(' ', 0)); //puts executable in its own string
        string arguments = "";

        if(input.find_first_of(' ', 0) != string::npos) //if there is a space
            arguments = input.substr(input.find_first_of(' ' , 0) + 1);

        history(&myHistory, input, 0); //history function call

        if(command == "cd")
        {
            string dir;

            if(arguments.empty()) //just cd command
                dir = getenv("HOME");

            else if(arguments == ".") //do nothing
                dir = pwd();

            else if(arguments.compare(0 , 2 , "..") == 0) //go up one directory
                dir = pwd().substr(0, pwd().find_last_of('/')) + arguments.substr(2, string::npos);

            else
                dir = pwd() + '/' + arguments; 
                       
            if(chdir(dir.data()) == -1) //changes dir, checks error
            { 
                write(1, "cd: ", 4);
                write(1, arguments.data(), arguments.length());        
                write(1, ": No such file or directory\n", 28);
            }
        } //cd command

        else if(command == "ls")
        {
            string dir, specdir;

            if(arguments.empty()) //just ls command
            {
                dir = pwd(); //current directory
                DIR *mydir;
                mydir = opendir(dir.data()); //open the directory and look
                struct dirent *myfolder = NULL; //file pointer for dirent
                struct stat fileStat; //var for file permissions

                while((myfolder = readdir(mydir)) != NULL) //reads file contents and prints file names
                {
                    stat(myfolder->d_name, &fileStat); //send files to stat struct

                    if(S_ISDIR(fileStat.st_mode)) //file permissions here
                        write(1, "d", 1);
                    else
                        write(1, "-", 1);
                    
                    if(fileStat.st_mode & S_IRUSR)
                        write(1, "r", 1);
                    else
                        write(1, "-", 1);
                    
                    if(fileStat.st_mode & S_IWUSR)
                        write(1, "w", 1);
                    else
                        write(1, "-", 1);
                    
                    if(fileStat.st_mode & S_IXUSR)
                        write(1, "x", 1);
                    else
                        write(1, "-", 1);
                    
                    if(fileStat.st_mode & S_IRGRP)
                        write(1, "r", 1);
                    else
                        write(1, "-", 1);
                    
                    if(fileStat.st_mode & S_IWGRP)
                        write(1, "w", 1);
                    else
                        write(1, "-", 1);
                    
                    if(fileStat.st_mode & S_IXGRP)
                        write(1, "x", 1);
                    else
                        write(1, "-", 1);
                    
                    if(fileStat.st_mode & S_IROTH)
                        write(1, "r", 1);
                    else
                        write(1, "-", 1);
                    
                    if(fileStat.st_mode & S_IWOTH)
                        write(1, "w", 1);
                    else
                        write(1, "-", 1);
                    
                    if(fileStat.st_mode & S_IXOTH)
                        write(1, "x", 1);
                    else
                        write(1, "-", 1); 

                    write(1, " ", 1); //space between
                    write(STDOUT_FILENO, myfolder->d_name, strlen(myfolder->d_name)); //file prints
                    write(1, "\n", 1);

                }
                closedir(mydir);
            }

            else //ls command with a specified directory
            {
                dir = pwd(); //current directory
                specdir = pwd() + '/' + arguments; //specified directory
                DIR *mydir;
                mydir = opendir(specdir.data());
                struct dirent *myfolder = NULL;
                struct stat fileStat;

                if(chdir(specdir.data()) == -1) //if file or dir doesnt exist
                {
                    write(1, "ls: ", 4);
                    write(1, arguments.data(), arguments.length());        
                    write(1, ": No such file or directory\n", 28);   
                }

                else //the file or dir exists and go to it
                {
                    while((myfolder = readdir(mydir)) != NULL) //read and print out the files
                    {
                        stat(myfolder->d_name, &fileStat); //send files to stat struct

                        if(S_ISDIR(fileStat.st_mode)) //file permissions here
                            write(1, "d", 1);
                        else
                            write(1, "-", 1);
                    
                        if(fileStat.st_mode & S_IRUSR)
                            write(1, "r", 1);
                        else
                            write(1, "-", 1);
                    
                        if(fileStat.st_mode & S_IWUSR)
                            write(1, "w", 1);
                        else
                            write(1, "-", 1);
                    
                        if(fileStat.st_mode & S_IXUSR)
                            write(1, "x", 1);
                        else
                            write(1, "-", 1);
                    
                        if(fileStat.st_mode & S_IRGRP)
                            write(1, "r", 1);
                        else
                            write(1, "-", 1);
                    
                        if(fileStat.st_mode & S_IWGRP)
                            write(1, "w", 1);
                        else
                            write(1, "-", 1);
                    
                        if(fileStat.st_mode & S_IXGRP)
                            write(1, "x", 1);
                        else
                            write(1, "-", 1);
                    
                        if(fileStat.st_mode & S_IROTH)
                            write(1, "r", 1);
                        else
                            write(1, "-", 1);
                    
                        if(fileStat.st_mode & S_IWOTH)
                            write(1, "w", 1);
                        else
                            write(1, "-", 1);
                    
                        if(fileStat.st_mode & S_IXOTH)
                            write(1, "x", 1);
                        else
                            write(1, "-", 1); 

                        write(1, " ", 1); //space between
                        write(STDOUT_FILENO, myfolder->d_name, strlen(myfolder->d_name));
                        write(1, "\n", 1);
                    }
                    closedir(mydir); //now return to your original directory
                }
            }
        } //ls command

        else if(command == "history")
        {
            printHistory(myHistory);
        } //history command

        else if(command == "pwd")
        {
            write(1, pwd().data(), pwd().length()); //system call to pwd
            write(1, "\n", 1); 
        } //pwd command

        else if(command == "exit")
        {
            _exit(0); //brings you back to command line
        } //exit command

        else
        {
            int pid = fork(); //var for child process

            if(pid != 0) //child checking first
                while(wait(NULL) != pid); //wait until notified

            else //children are okay
            {
                char *argv[32]= {NULL}; //pointer to size 32 array
                argv[0] = strdup(command.c_str()); //copy command into first argv list
                int i = 1;
 
                while(!arguments.empty()) //parse arguments further into argv
                {                            
                    if(arguments.find_first_of(' ') != string::npos) //if arguments has a space character
                    {
                        argv[i] = strdup(arguments.substr(0, arguments.find_first_of(' ')).data());
                        arguments.erase(0, arguments.find_first_of(' ') + 1);
                    } 

                    else //if no space character
                    { 
                        argv[i] = strdup(arguments.substr(0, arguments.find_first_of(' ')).data());
                        arguments.erase(0, arguments.find_first_of(' '));  
                    }

                    i++; //incrementor
                }
               
                argv[i] = '\0';

                if(execvp(command.data(), argv) < 0) //command does not match any records here
                {
                    write(1, command.data(), command.length());
                    write(1, ": command not found\n" , 20);
                }

                /*else //execute commands with arguments
                {
                    execv(command.data(), argv);
                }*/

                //write(1, input.data(), input.length());
                //write(1, ": command not found\n", 20) ;
            }
        } //all other command options
    } //while loop
    return 0;
} //int main()