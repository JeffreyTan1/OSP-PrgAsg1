.default: all

all: Reader_Writer Sleeping_Barbers Menu

Menu: Menu.o
	g++ -Wall -Werror -std=c++11 Menu.o -O -o ./simulation -lpthread

Menu.o: Menu.cpp
	g++ -c Menu.cpp

Reader_Writer: Reader_Writer.o
	g++ -Wall -Werror -std=c++11 Reader_Writer.o -O -o Reader_Writer -lpthread

Reader_Writer.o: Reader_Writer.cpp
	g++ -c Reader_Writer.cpp

Sleeping_Barbers: Sleeping_Barbers.o
	g++ -Wall -Werror -std=c++11 Sleeping_Barbers.o -O -o Sleeping_Barbers -lpthread

Sleeping_Barbers.o: Sleeping_Barbers.cpp
	g++ -c Sleeping_Barbers.cpp

clean: 
	rm *.o Reader_Writer Sleeping_Barbers Menu