all: run input_ready

obj = main.o clock-dwf.o my_mq.o list.o

obj2 = input_ready.o

run: $(obj)
	gcc -o run $(obj) 
main.o: main.c main.h clock-dwf.h my_mq.h common.h
	gcc -c main.c 

clock-dwf.o: clock-dwf.c clock-dwf.h common.h
	gcc -c clock-dwf.c

my_mq.o: my_mq.c my_mq.h common.h
	gcc -c my_mq.c

input_ready: $(obj2)
	g++ -o input_ready $(obj2)

input_ready.o: input_ready.cpp 
	g++ -c input_ready.cpp

clean:
	rm $(obj)
	rm $(obj2)
