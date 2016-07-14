log:log.o main.o
	g++  log.o main.o -o log -lpthread
log.o:
	g++ -c log.h log.cpp blockQueue.h
main.o:
	g++ -c main.cpp
clean:
	rm -rf *.o 
