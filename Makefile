#Lucas Lopilato
#MPTCP
#

CC	= gcc
LFLAGS = -pthread
CFLAGS = -g

OBJECTS = connection.o utility.o queue.o threads.o libmptcp.a

mptcp : client.o $(OBJECTS)
	gcc $(LFLAGS) client.o $(OBJECTS) -o mptcp -lm

test: test.o $(OBJECTS)
	gcc $(LFLAGS) test.o $(OBJECTS)  -o tester

%.o : %.c
	gcc $(CFLAGS) -c $< -o $@

clean:
	rm -f mptcp tester *.o

run: clean mptcp
	./mptcp -n 1 -h 128.111.68.197 -p 5176 -f test.txt

runm: clean mptcp
	./mptcp -n 5 -h 128.111.68.197 -p 5176 -f test.txt

lossy: clean mptcp
	./mptcp -n 1 -h 128.111.68.197 -p 5177 -f test.txt

lossym: clean mptcp
	./mptcp -v -n 16 -h 128.111.68.197 -p 5177 -f test.txt

tester: clean test
	./tester


