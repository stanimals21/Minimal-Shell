#include<iostream>
#include <vector>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>

using namespace std;

string trim (string input){
    int i=0;
    while (i < input.size() && input [i] == ' ')
        i++;
    if (i < input.size())
        input = input.substr (i);
    else{
        return "";
    }
    
    i = input.size() - 1;
    while (i>=0 && input[i] == ' ')
        i--;
    if (i >= 0)
        input = input.substr (0, i+1);
    else
        return "";
    
    return input;
    

}

vector<string> split (string line, string separator=" "){
    vector<string> result;
    while (line.size()){
        size_t found = line.find(separator);
        if (found == string::npos){
            string lastpart = trim (line);
            if (lastpart.size()>0){
                result.push_back(lastpart);
            }
            break;
        }
        string segment = trim (line.substr(0, found));
        //cout << "line: " << line << "found: " << found << endl;
        line = line.substr (found+1);

        //cout << "[" << segment << "]"<< endl;
        if (segment.size() != 0) 
            result.push_back (segment);

        //cout << line << endl;
    }
    return result;
}

char** vec_to_char_array (vector<string> parts){
    char ** result = new char * [parts.size() + 1]; // add 1 for the NULL at the end
    for (int i=0; i<parts.size(); i++){
        // allocate a big enough string
        result [i] = new char [parts [i].size() + 1]; // add 1 for the NULL byte
        strcpy (result [i], parts[i].c_str());
    }
    result [parts.size()] = NULL;
    return result;
}

void execute (string command){
    // get rid of spaces and quote chars


    vector<string> argstrings = split (command, " "); // split the command into space-separated parts
    char** args = vec_to_char_array (argstrings);// convert vec<string> into an array of char*

    // for printing current directory to console
    char cwd[1024];

    // for cd implementation
    if (argstrings[0] == "cd")
    {
        if(argstrings[1] == "-")
        {
            chdir("..");
            cout << getcwd(cwd, sizeof(cwd)) << endl;
        }
        else{
            chdir(args[1]);
            //cout << getcwd(cwd, sizeof(cwd)) << endl;
        }
    }
    else{
        execvp (args[0], args);
    }
}

string encode(string newString)
{
    int quoteCount = 0;
    int singleQuoteCount = 0;

    for (int i = 0; i < newString.length(); i++) {
        if(newString[i] == '"')
        {
            quoteCount++;
        }
        else if(newString[i] == '\'')
        {
            singleQuoteCount++;
        }

        if(quoteCount % 2 != 0 || singleQuoteCount %2 != 0) // if we haven't encountered an ending quotation mark
        {
            if (newString[i] == '|')
            {
                newString[i] = '^';
            }
            else if(newString[i] == '>')
            {
                newString[i] = '!';
            }
            else if(newString[i] == '<')
            {
                newString[i] = '@';
            }
        }
    }
    return newString;
}

vector<string> decode(vector<string> tparts)
{
    for (int i = 0; i < tparts.size(); i++) 
    {
        string currentString = tparts[i];
        for (int j = 0; j < currentString.length(); j++) 
        {
            if(currentString[j] == '^')
            {
                currentString[j] = '|';
            }
            else if(currentString[j] == '!')
            {
                currentString[j] = '>';
            }
            else if(currentString[j] == '@')
            {
                currentString[j] = '<';
            }
        }
        tparts[i] = currentString;
    }
    return tparts;
}

int main (){
    while (true){ // repeat this loop until the user presses Ctrl + C
        cout << "$ ";
        string commandline = "";/*get from STDIN, e.g., "ls  -la |   grep Jul  | grep . | grep .cpp" */
        getline(cin,commandline);
        commandline = encode(commandline);

        // split the command by the "|", which tells you the pipe levels
        vector<string> tparts = split (commandline, "|");
        tparts = decode(tparts);

        int originalFd = dup(0); // to redirect stdin back to console at end of parent process 

        // for each pipe, do the following:
        for (int i=0; i<tparts.size(); i++){
            // make pipe
            int fd[2];
            pipe(fd);
			if (!fork()){
                // redirect output to the next level
                // unless this is the last level
                if (i < tparts.size() - 1){
                    dup2 (fd[1], 1);
                    // redirect STDOUT to fd[1], so that it can write to the other side
                    close (fd[1]);   // STDOUT already points fd[1], which can be closed
                }
                //execute function that can split the command by spaces to 
                // find out all the arguments, see the definition
                execute (tparts [i]); // this is where you execute
            }else{
                wait(0);            // wait for the child process
                dup2 (fd [0], 0);
                close (fd [1]);
				// then do other redirects
            }
        }
        dup2(originalFd,0);
    }
}