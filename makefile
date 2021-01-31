CC = gcc
CFLAGS = -Wall -pedantic
LIBS = -lpthread
TARGET = server client
.PHONY: clean test cleandata



%:%.c
	@$(CC) $(CFLAGS) $< -o $@ $(LIBS)

%.o:%.c
	@$(CC) $(CFLAGS) $< -c -o $@

all: $(TARGET)

client: client.o libClientLib.a
	@$(CC) $(CFLAGS) $^ -o $@ -L. -lClientLib

libClientLib.a: client_lib.o client_lib.h
	@ar rvs $@ $^

server: server.o libServerLib.a 
	@$(CC) $(CFLAGS) $^ -o $@ -L. -lServerLib $(LIBS)

libServerLib.a: server_lib.o server_lib.h hash.o hash.h
	@ar rvs $@ $^



test: cleandata
	@chmod +x *.sh
	@./server &
	@./start_test.sh

clean: cleandata
	@rm -r *.o *.a client server 

cleandata:
	@-rm -r data *.log



