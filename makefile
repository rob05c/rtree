CC=g++
OPTIONS=-std=c++11 -g -Wfatal-errors
LINK_OPTIONS=-ltbb
all: rtree
rtree: main.o rtree.o
	$(CC) $(OPTIONS) main.o rtree.o -o rtree $(LINK_OPTIONS)
main.o: 
	$(CC) $(OPTIONS) -c main.cpp -o main.o
rtree.o: 
	$(CC) $(OPTIONS) -c rtree.cpp -o rtree.o
clean:
	rm -f *o rtree
