# Robert Pepek, M00738240 - Operating Systems CW01# CompLab1/01 tutor: Ahmed Eissa myshell:

myshell: functions.o main.o dependencies.h
	g++ -std=c++17 -Wall  functions.o main.o -o myshell

main.o: main.cpp
	g++ -std=c++17 -Wall  -c main.cpp

functions.o: functions.cpp
	g++ -std=c++17 -Wall  -c functions.cpp

clean:
	rm *.o myshell
	
	