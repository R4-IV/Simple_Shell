//includes dependencies header for function header files
#include "dependencies.h"

//file descriptor ints used for io redirection
int fileDescIn;
int fileDescOut;
int fileDescOutOvr;
int devNull;

//tracks whether the input contained "&"
bool Ampersand = false;
//Buffer used to store path to shell exe
char fileExeBuffer[1024];

//function that retrieves link to proc/self/exe and stores it in a char array buffer this contains path to shell exe
//and is a linux specific implementation it returns a char* (string).
char* getShellPath() {
    memset(fileExeBuffer, 0, sizeof(fileExeBuffer));
    if (readlink("/proc/self/exe", fileExeBuffer, sizeof(fileExeBuffer)-1) < 0){
        perror("readlink error");
    }
    char* ptr = fileExeBuffer;
    return ptr;
}

//function that checks whether file descriptors were opened based on < , > , >> in the token vector command
//and closes the ones that are opened. takes an integer vector which contains the positions of redirect symbols
//inside the token vector.
void closeFileDescriptors(std::vector<int> &redirectSymbols){
    if(redirectSymbols[0] != -1){
        close(fileDescIn);
    }
    if(redirectSymbols[1] != -1){
        close(fileDescOut);
    }
    if(redirectSymbols[2] != -1){
        close(fileDescOutOvr);
    }
}

//takes string vector as input which is the tokenised user input and tries to find any redirection symbols in the
//input once it finds them it outputs their individual location aswell as the position of the earliest occurence of an
//redirect symbol.
std::vector<int> findRedirectionSymbols(std::vector<std::string> &commandIn){
    int in , out , outovr;
    int minimumPosition = 1000;
    in = -1;
    out = -1;
    outovr = -1;
    std::vector<int> outvec;
    for(int i = 0; i < commandIn.size(); i++){
        if(commandIn[i].compare("<") == 0){
            in = i;
        }
        if(commandIn[i].compare(">") == 0){
            out = i;
        }
        if(commandIn[i].compare(">>") == 0){
            outovr = i;
        }
    }
    outvec.push_back(in);
    outvec.push_back(out);
    outvec.push_back(outovr);
    for(int i = 0; i < outvec.size(); i++){
        if(outvec[i] != -1 && outvec[i] < minimumPosition){
            minimumPosition = outvec[i];
        }
    }
    outvec.push_back(minimumPosition);
    return outvec;
}

//uses the vector from the previous function to open filedescriptors ascociated with the redirect symbol
//if the file provided is invalid the function prints an error and returns.
//Once the descriptors are open dup2() is used to overwrite default streams with them.
//This function is ran in the grandchild process therefore the parent does not need to restore streams after each command.
void ioRedirection(std::vector<int> &redirectSymbols , std::vector<std::string> &commandIn){
    //STDIN redirection <
    if(redirectSymbols[0] != -1){
        if((fileDescIn = open(commandIn[redirectSymbols[0] + 1].c_str() , O_RDONLY)) == -1){
            perror("Cannot open input file\n");
            return;
        }
        else{
            dup2(fileDescIn , STDIN_FILENO);
        }
    }
    //STDOUT redirection with file overwriting >
    if(redirectSymbols[1] != -1){
        if((fileDescOut = open(commandIn[redirectSymbols[1] + 1].c_str() , O_WRONLY | O_CREAT | O_TRUNC , S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)) == -1){
            perror("Cannot open output file\n");
            return;
        }
        else{
            dup2(fileDescOut , STDOUT_FILENO);
        }
    }
    //STDOUT redirection with file appending >>
    if(redirectSymbols[2] != -1){
        if((fileDescOutOvr = open(commandIn[redirectSymbols[2] + 1].c_str() , O_WRONLY | O_CREAT | O_APPEND , S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)) == -1){
            perror("Cannot open output file\n");
            return;
        }
        else{
            dup2(fileDescOutOvr , STDOUT_FILENO);
        }
    }
}


//function checks the input vector and returns true when "&" is present used to chec if the program needs to run in the background
bool hasAmpersand(std::vector<std::string> &tokenString){
    bool ampersand = false;
    
    for(int i = 0; i < tokenString.size(); i++){
        if(tokenString[i].compare("&") == 0){
            ampersand = true;
        }
    }
    return ampersand;
}

//Checks if the next character is a white space character and returns true if it is
//function is partially used to remove multiple leading , trailing or spaces in between commands
bool spacesAdjacent(char left, char right) {
    return (left == right) && (isspace(left));
}

//function that checks whether the string input is a file, used to see if the shell arg is a bash file
//and to check whether the copy command is copying a dir or a file as there different behaviours for both
bool isFile(std::string file){
    struct stat FileAttributes;
    const char* Fileinput = file.c_str();
    if(stat(Fileinput , &FileAttributes) < 0){
        printf("File error message = %s\n" , strerror(errno));
        return false;
    }
    if(S_ISDIR(FileAttributes.st_mode)){
        return false;
    }
    else{
        return true;
    }
}
 
//function checks if the user input vector  contains any of the internal commands that need to be implemented and returns the command position in the arr
int validCommand(std::string userCommand){
    //returns -1 if the command isnt an internal command
    int returnCommand = -1;
    const int arrSize  = 8;
    static const std::string arr[arrSize] = {"cd" , "cls" , "dir" , "copy" , "print" , "md" , "rd" , "quit"};
    
    for(int i = 0; i < arrSize; i++){
        int comparator = arr[i].compare(userCommand);
        if(comparator == 0){
            returnCommand = i;
            break;
        }
    }
    return returnCommand;
}

//function that executes internal and external commands in the grandchild process
//uses command ar positions to execute internal commands
void executePrograms(int result , std::vector<std::string> &tokens){
    switch(result){
            //clear function uses escape command esc + c to reset the console
        case 1: {
            if(tokens.size() > 1){
                std::cout << "cls: too many arguments" << std::endl;
                return;
            }
            printf("\033c");
            
            break;
        }
            //dir command prints the contents of the current dir similarly to ls -1
        case 2: {
            if(tokens.size() > 1){
                std::cout << "dir: too many arguments" << std::endl;
                return;
            }
            //gets the full path of all dir entries and uses ::vector::erase to remove the path from the begining
            std::string currentDirPath = getenv("PWD");
            currentDirPath.append("/");
            int pathLength = currentDirPath.length();
            for(const auto & dirEntry : std::filesystem::directory_iterator(currentDirPath)){
                std::string temp = dirEntry.path();
                std::cout << temp.erase(0 , pathLength) << std::endl;
            }
            break;
        }
            //copy file internal command checks if the argument is a file or directory
        case 3: {
            if(tokens.size() != 3){
                std::cout << "copy: invalid argument amount (copy <source> <target>)" << std::endl;
                return;
            }
            struct stat FileAttributes;
            //Source of the copy
            const char* Filein = tokens[1].c_str();
            if(stat(Filein , &FileAttributes) < 0){
                printf("File error message = %s\n" , strerror(errno));
                break;
            }
            if(S_ISDIR(FileAttributes.st_mode)){
                std::size_t foundSlash = tokens[1].find_last_of("/\\");
                //if the directory provided is not a path but on the same level as the current pwd
                if(foundSlash == -1){
                    tokens[2].append("/");
                    tokens[2].append(tokens[1]);
                }
                //if the target is a path directory a slash is appended aswell as the target name this makes a new path
                // /target/sourceName
                else{
                    tokens[2].append("/");
                    tokens[2].append(tokens[1].substr(foundSlash + 1 , tokens[1].length()));
                    
                }
                
            }
            try
               {
                   //copy function if used on directory will first create the target dir and then copy contents into it
                   std::filesystem::copy(tokens[1], tokens[2], std::filesystem::copy_options::overwrite_existing | std::filesystem::copy_options::recursive);
               }
            //prints out the error ascociated with copying
               catch (std::exception& e)
               {
                   std::cout << e.what();
               }
            break;
        }
            //print function outputs to the console or any stout redirected file the arguments that the user provides
            //multiple spaces/tabs are reduced to single space/tab;
        case 4: {
            for(int i = 1; i < tokens.size(); i++){
                if(i == tokens.size() - 1){
                    //if last token end line
                    std::cout << tokens[i] << std::endl;
                }
                else{
                    //else print tokens seperated by white space
                    std::cout << tokens[i] << " ";
                }
            }
            break;
        }
            //make directory command creates a directory with 777 permissions read/write/execute
        case 5: {
            if(tokens.size() != 2){
                std::cout << "md: invalid argument amount (md <target>)" << std::endl;
                return;
            }
            if(mkdir(tokens[1].c_str() , 0777) != 0){
                perror(tokens[1].c_str());
            }
            break;
        }
            //remove directory command
        case 6: {
            if(tokens.size() != 2){
                std::cout << "rd: invalid argument amount (rd <target>)" << std::endl;
                return;
            }
            try {
                //removes first argument in vector
                std::filesystem::remove(tokens[1]);
            }
            //fails if directory is not empty
            catch(std::exception& e)
            {
                std::cout << e.what();
            }
            break;
        }
            //Handles external command input such as man , ps , ls , pwd , env
        case -1:{
            //command to be executed
            const char* cmd = tokens[0].c_str();
            //vector with the rest of the arguments
            std::vector<const char*> arguments;
            for(int i = 0; i < tokens.size(); i++){
                arguments.push_back(tokens[i].c_str());
            }
            //adds a null terminator to the argument list
            arguments.push_back((char*)NULL);
            
            //using execvp because it can take arguments as an array and uses PATH to find the command on its own so i dont
            //have to provide absolute paths for them
            execvp(cmd , const_cast <char*const*>(arguments.data()));
        }
    }
}

//tokenises user input and forks children so that programs can be executed takes mode and string as optional paramaters definined in .h file in order to differenciate between user input and bash file input
void tokenizeInputs(int mode, std::string line){
    //int variables used to store position of start and end characters between spaces
    int tokenStart = 0;
    int tokenEnd = 0;
    //stores user input std::cin
    std::string userCommand;
    
    //mode == 1 then pass a line from a bash file as the usercommand
    if(mode == 1){
        userCommand = line;
    }
    //else present user with prompt with full path to cwd
    else{
        std::cout << std::getenv("PWD") << "/myshell > ";
        std::getline(std::cin , userCommand);
    }
    //creates a string vector to hold all user input tokens
    std::vector < std::string > tokenizedCommand;
    //deletes duplicate spaces
    std::string::iterator newEnd = std::unique(userCommand.begin(), userCommand.end(), spacesAdjacent);
    userCommand.erase(newEnd, userCommand.end());
    //checks if input is empty
    if(userCommand.length() == 0){
        return;
    }
    //deletes first leading space if the command is not length 0
    if(isspace(userCommand.at(0))){
        userCommand.erase(0 , 1);
    }
    //checks if the input is empty after deleting first space
    if(userCommand.length() == 0){
        return;
    }
    //deletes last trailing space if it exists
    if(isspace(userCommand.at(userCommand.length() - 1))){
        userCommand.erase(userCommand.length() - 1 , 1);
    }
    //loops through all the characters of the user in string and finds a space and creates a substring of all words between spaces and adds them to the vector
    while(tokenEnd != userCommand.length()){
        if(isspace(userCommand.at(tokenEnd))){
            tokenizedCommand.push_back(userCommand.substr(tokenStart , (tokenEnd - tokenStart)));
            tokenEnd++;
            tokenStart = tokenEnd;
        }
        if(tokenEnd + 1 == userCommand.length()){
            tokenizedCommand.push_back(userCommand.substr(tokenStart , (tokenEnd + 1 - tokenStart)));
        }
        tokenEnd++;
    }
    //if the vector is empty return as there is no command input
    if(tokenizedCommand.empty()){
        return;
    }
    //gets a result of valid command by passing the first element of the vector into the function
    int result = validCommand(tokenizedCommand[0]);
        
    //quit the shell must run in the parent shell otherwise it would not quit the shell
    if(result == 7){
        if(tokenizedCommand.size() > 1){
            std::cout << "quit: too many argumnets" << std::endl;
            return;
        }
        exit(0);
    }
    //runs cd program in parent
    if(result == 0){
        //if vector size one print pwd
        if(tokenizedCommand.size() == 1){
            std::cout << getenv("PWD") << std::endl;
        }
        //if size 2 use chdir to move to the next dir and update environ pwd
        else if(tokenizedCommand.size() == 2){
            std::string path = tokenizedCommand[1];
            const char* strpath = path.c_str();
            if(chdir(strpath) == -1){
                perror(strpath);
            }
            char cwd[256];
            getcwd(cwd , sizeof(cwd));
            setenv("PWD" , cwd , true);
        }
        return;
    }
    //forks child process
    pid_t pid = fork();
    //checks if child process experienced an error
    if(pid < 0){
        perror("Fork Failed");
    }
    //vector to store redirect symbol positions if none exist vector[3] = 1000
    std::vector<int> redirInfo = findRedirectionSymbols(tokenizedCommand);
    // if pid == 0 only executes for the child process the parent doesn't touch this code
    if(pid == 0){
        //if vector has ampersand update value this is done as the ampersand is removed from the vector before it needs to be checked again.
        if(hasAmpersand(tokenizedCommand)){
            Ampersand = true;
        }
        else{
            Ampersand = false;
        }
        //create a grandchild process this is done for background execution,
        // if the user specifies the bbackground execution the child will return immiediatly therefore leaving grandchild to be orphaned and taken over by init.
        pid_t grandChildPid = fork();
        //grandchild fork error handling
        if(grandChildPid < 0){
            perror("Fork Failed");
        }
        //code only executed by the grandchild
        if(grandChildPid == 0){
            //if background execution stdout is set to dev null so that it doesnt interfere with console
            //this can be overwritten with redirection symbols
            if(hasAmpersand(tokenizedCommand)){
                if((devNull = open("/dev/null" , O_WRONLY)) == -1){
                    perror("Cannot open output file\n");
                }
                else{
                    dup2(devNull , STDOUT_FILENO);
                }
                //closes file descriptor dev null
                close(devNull);
                //removes the "&" at the end of the command
                tokenizedCommand.pop_back();
            }
            //checks if a redirection symbol exists and if it does sets streams to the correct one
            if(redirInfo[3] != 1000){
                ioRedirection(redirInfo ,tokenizedCommand);
                closeFileDescriptors(redirInfo);
                tokenizedCommand.erase(tokenizedCommand.begin() + redirInfo[3] , tokenizedCommand.end());
            }
            //calls execute command function with the specified case (result) and the arguments (tokenised Command)
            executePrograms(result , tokenizedCommand);
            //grandchild exists after execution
            exit(0);
        }
        if(Ampersand){
            //if background execution exit immiediatly
            exit(0);
        }
        else{
            //wait for grandchild to return an exit code
            wait(NULL);
            //child exits
            exit(0);
        }
    }
    //parent waits for child to return an exit code
    wait(NULL);
}

