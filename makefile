CC=g++
CFLAGS=-Wall -g -I./udt4/src/ -L./udt4/src/ -ludt -lstdc++ -std=c++11
PTHREADS=-lpthread

all: build-server build-client

build-server:
	$(CC) -o chatserver server/chatserver.cpp $(CFLAGS) $(PTHREADS)

build-client:
	$(CC) -o chatclient client/chatclient.cpp $(CFLAGS) $(PTHREADS)
clean:
	rm -rf chatserver chatclient Submission.zip

submit:
	zip Submission.zip client/* server/*