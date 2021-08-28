.default: all

all: a

clean:
	rm -rf a *.o *.dSYM

a: ProducerConsumer.o
	g++ -Wall -Werror -std=c++11 -O -o $@ $^ -pthread

%.o: %.cpp
	g++ -Wall -Werror -std=c++11 -O -c $^ -pthread