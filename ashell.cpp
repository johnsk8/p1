/* A simple shell for ECS 150
 * Filename: ashell.cpp
 * Authors: Felix Ng, John Garcia
 *
 * In this version:
 *  input parsing   -   done! 
 *  history         -   done!
 *  cd              -   done!
 *  pwd             -   done!
 *  ls              -   started
 *  exit            -   done!
 *  app opening     -   not started
*/
#include <unistd.h>
#include <sys/types.h>      // for ls
#include <sys/stat.h>       // for ls
#include <sys/wait.h>       // for waitpid()
#include <dirent.h>         // for ls
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <ctype.h>          // has macros and typedefs
#include <string>           // easier than c string
#include <queue>            // for history
#include <cstring>
#include <iostream> //for testing only

using namespace std;

void ResetCanonicalMode(int fd, struct termios *savedattributes) {
    tcsetattr(fd, TCSANOW, savedattributes);
}

void SetNonCanonicalMode(int fd, struct termios *savedattributes) {
    struct termios TermAttributes;
    //char *name;

    // Make sure stdin is a terminal.
    if(!isatty(fd)){
        fprintf (stderr, "Not a terminal.\n");
        exit(0);
    }

    // Save the terminal attributes so we can restore them later.
    tcgetattr(fd, savedattributes);

    // Set the funny terminal modes.
    tcgetattr (fd, &TermAttributes);
    TermAttributes.c_lflag &= ~(ICANON | ECHO); // Clear ICANON and ECHO.
    TermAttributes.c_cc[VMIN] = 1;
    TermAttributes.c_cc[VTIME] = 0;
    tcsetattr(fd, TCSAFLUSH, &TermAttributes);
}



string history(queue<string> *myHistory, string input, int printCnt) {
    queue<string> printQ; //queue only for a history of 10 elements
 
    if(input == "`u" || input == "`d") { //do not record up and down arrow keys into history
        queue<string> printQ = *myHistory;
        int printQsize = printQ.size();
 
        for(int i = 0; i < 100 ; i++ )
            write (1, "\b \b", 3); //lazy backspaces
 
        for(int i = 0; i < printQsize; i++) {
            if(i == printQsize - printCnt) {
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


string getInput(queue<string> *myHistory) {
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
 
        if(rawIn == 0x1B) {//up and down arrow check
            read(STDIN_FILENO, &rawIn, 1);
            if(rawIn == 0x5B) {
                read(STDIN_FILENO, &rawIn, 1);  //keep reading input
 
                if (rawIn == 0x41) {                //up arrow
                    if(histCount < (int)myHistory->size()) {
                        histCount++;
                   } else {
                       write(1, "\a", 1);
                   }
 
                    if(myHistory->size() > 0)
                        formattedIn = history(myHistory, "`u", histCount);
 
                    continue;
                }
 
                if(rawIn == 0x42) {                                     // down arrow
                    histCount--;
                    if(histCount <= 0) {  
                        for(int i = 0; i < 100 ; i++ )
                            write (1, "\b \b", 3);                      //lazy backspaces
                        write(1, ">", 1);
                        histCount = 0;
                        formattedIn.clear();
                        continue;
                    }
 
                    formattedIn = history(myHistory, "`d", histCount);
                    continue;
                }
                
                if(rawIn == 0x43 || rawIn == 0x44) continue;       //left and right keys dont matter so continue
            }
        }
 
        if (rawIn == 0x7F || rawIn == 0x08) {               //mac "Delete"  or "backspace"
            if (formattedIn.empty()) {                      //if nothing to delete play bell
                write(STDOUT_FILENO, "\a", 1);
            } else {
                write(STDOUT_FILENO, "\b \b", 3);
                formattedIn.resize(formattedIn.length() - 1);
            }
        } else {
            if(isprint(rawIn)) {
                write(STDOUT_FILENO, &rawIn, 1);        //printf("RX: '%c' 0x%02X\n",RXChar, RXChar);
                formattedIn += rawIn;                   //stores input into a string
            } else {
                write(STDOUT_FILENO, "X", 1); //printf("RX: ' ' 0x%02X\n",RXChar);
                formattedIn += '\a';
            }
        }
    } //while input
 
    write(STDOUT_FILENO, "\n", 1);
   
    ResetCanonicalMode(STDIN_FILENO, &SavedTermAttributes);
    return formattedIn;
} //string getInput()

void printHistory(queue<string> printQ) {
    int n = printQ.size();
    for(int i = 0; i < n ; i++) {
        char c = i + 48;
        write(STDOUT_FILENO, &c, 1);
        write(STDOUT_FILENO, " ", 1);
        write(1, printQ.front().data(), printQ.front().length());   //loop should output strings instead of chars when combined with felix
        write(1, "\n", 1);
        printQ.pop();
    }
}

string pwd() {
    string pwd;
    char cwd[50];
    getcwd(cwd, 50);

    pwd += cwd;
    return pwd;
}

string setDir(string arguments) {
    string dir;
        if (arguments.empty()) {
            dir = getenv("HOME");                                           // cout << "Change directory to HOME" << endl;
        } else if (arguments == ".") {          // do nothing
            dir = pwd();
        } else if (arguments.compare(0 , 2 , "..") == 0) {          // go up one directory
            dir = pwd().substr(0, pwd().find_last_of('/')) + arguments.substr(2, string::npos);
        } else {
            dir = pwd() + '/' + arguments;
        }
                
    return dir;
}

void printLS(string dir, string arguments) {
    DIR *mydir;
    if((mydir = opendir(dir.data())) == NULL) {        //open the directory and look
        write(1, "ls: ", 4);
        write(1, arguments.data(), arguments.length());
        write(1, ": No such directory\n", 20);
        return;
    }
    struct dirent *myfolder = NULL; // file pointer for dirent
    struct stat fileStat; // var for file permissions
    
    while((myfolder = readdir(mydir)) != NULL) { // reads file contents and prints file names
        stat(myfolder->d_name, &fileStat); // send files to stat struct

        if(S_ISDIR(fileStat.st_mode)) // file permissions here
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

int main(int argc, char *argv[]) {
    queue<string> myHistory;                //queue for all history

    while (1) {
        string input = getInput(&myHistory); //always waiting for input
        string command = input.substr(0, input.find_first_of(' '));          // cout << "command: " << command << endl;      //  puts executable in its own string
        string arguments;

        if(input.find_first_of(' ') != string::npos) {       // if space exists after command
            arguments = input.substr(input.find_first_of(' ') + 1);     // puts everything after the space into arguments
        }
        
        history(&myHistory, input, 0); //history function call
        
        if (command == "cd") {                                                   // cout << "change directory ";
            if(chdir(setDir(arguments).data()) == -1) {           // changes dir, checks error
                    write(1, "cd: ", 4);                                        // cout << arguments << ": No such file or directory" << endl;
                    write(1, arguments.data(), arguments.length());
                    write(1, ": No such file or directory\n", 28);
            }

        } else if (command == "ls") {                                                   // cout << "list directory ";
                if(arguments.empty()) arguments = ".";          // empty arguments should return current directory, not home
                string dir = setDir(arguments);
                printLS(dir, arguments);
 
        } else if(command == "history") {
            printHistory(myHistory);
            
        } else if (command == "pwd") {      // print working directory
            write(1, pwd().data(), pwd().length());
            write(1, "\n", 1);
            
        } else if (command == "exit") {     // breaks and exits shell
            _exit(0);
        } else {                            // attempts to run process
           
            // fork() returns 0 to child process
            // wait() waits for child
            // execv(path , arguments)
            int pid;
            if ((pid = fork()) != 0){
                while (wait(NULL) != pid);          // cout << "child terminated"<< endl;
            
            } else {
                char *argv[32]= {NULL};  // pointer to size 32 array
                
                argv[0] = strdup(command.c_str());
                
                int i = 1;
 
                while (!arguments.empty()) {                            // parse arguments further into argv
                    if(arguments.find_first_of(' ') != string::npos) {          // if arguments has a space character
                        argv[i] = strdup(arguments.substr(0, arguments.find_first_of(' ')).data());
                        arguments.erase(0, arguments.find_first_of(' ') + 1);
                    } else {                                                    // if no space character
                        argv[i] = strdup(arguments.substr(0, arguments.find_first_of(' ')).data());
                        arguments.erase(0, arguments.find_first_of(' '));
                    }
                    i++;
                }
                
                argv[i] = '\0';

                if (execvp(argv[0], argv) < 0){
                    write(1, command.data(), command.length());
                    write(1, ": command not found\n" , 20);
                }
            }
           
        }

    }   // main while loop
    return 0;
}
