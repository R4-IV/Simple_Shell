#include "dependencies.h"
//included dependencies.h which hold all function definitions which are needed to compile main
//definitins in functions.ccp are provided when the object files are linked during compilation

//main takes multiple arguments this is used to be able to accept input from bash scripts.
int main(int argc , char *argv[]){
    //Linux platform specific command that sets a new environ variable to be = to path to the shell executable by setting a char* memory to be equal to val /proc/self/exe.
    setenv("shell" , getShellPath() , true);
    //shell loop provides the user with the command prompt.
    while(true){
        //checks if the shell is called with an arguments or not.
        if(argc == 2){
            //checks if the supplied argument is a file.
            if(isFile(argv[1])){
                //opens ifstream for the file so it can be read.
            std::ifstream file(argv[1]);
                //checks if file is open
            if (file.is_open()) {
                //string to store line input from file
                std::string line;
                //parses the lines with my tokenise function end sends the inputs for execution.
                while (std::getline(file, line)) {
                    //condition to skip parsing header of the bash.
                    if(line.compare("!#bin/bash") != 0){
                        tokenizeInputs(1 , line);
                    }
                }
                //closes file.
                file.close();
            }
                //exits shell once all bash commands were executed.
            exit(0);
        }
            else{
                //if a non file argument is used prints error and exits.
                std::cout << "Not a file " << std::endl;
                exit(0);
            }
            
        }
        //if there is no input called with the shell then the shell proceeds to
        //Tokenise input function where user input from the command line is handled
        else{
            tokenizeInputs();
        }
    }
    return 0;
}
