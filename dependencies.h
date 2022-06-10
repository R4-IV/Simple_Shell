#include <iostream>
#include <vector>
#include <string>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <filesystem>
#include <stdio.h>
#include <sys/stat.h>
#include <fstream>
#include <fcntl.h>
#include <cstring>
#include <algorithm>
//All the header files required for the shell to run its commnads

//function definitions that are implemented in functions.cpp and required in main.cpp
char* getShellPath();

void closeFileDescriptors(std::vector<int> &redirectSymbols);

std::vector<int> findRedirectionSymbols(std::vector<std::string> &commandIn);

void ioRedirection(std::vector<int> &redirectSymbols , std::vector<std::string> &commandIn);

bool hasAmpersand(std::vector<std::string> &tokenString);

bool spacesAdjacent(char left, char right);

bool isFile(std::string file);

int validCommand(std::string userCommand);

void executePrograms(int result , std::vector<std::string> &tokens);

void tokenizeInputs(int mode = 0 , std::string line = "0" );

