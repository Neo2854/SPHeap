synthetic_req.out: synthetic_req.cpp spheap.o mem_dll.o
	g++ -g synthetic_req.cpp spheap.o mem_dll.o -o synthetic_req.out -lm

spheap.o: spheap.cpp spheap.h
	g++ -g -c spheap.cpp

onebin.o: onebin.c onebin.h
	g++ -g -c onebin.cpp

mem_dll.o: mem_dll.cpp mem_dll.h
	g++ -g -c mem_dll.cpp