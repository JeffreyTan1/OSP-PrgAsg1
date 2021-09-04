.default: all

all: Reader_Writer Sleeping_Barbers

Reader_Writer: Reader_Writer.o
	g++ -Wall -Werror -std=c++11 Reader_Writer.o -O -o Reader_Writer -lpthread


Reader_Writer.o: Reader_Writer.cpp
	g++ -c Reader_Writer.cpp

Sleeping_Barbers: Sleeping_Barbers.o
	g++ -Wall -Werror -std=c++11 Sleeping_Barbers.o -O -o Sleeping_Barbers -lpthread

Sleeping_Barbers.o: Sleeping_Barbers.cpp
	g++ -c Sleeping_Barbers.cpp

clean: 
	rm *.o Reader_Writer Sleeping_Barbers