#client.c
#server_thread.c
all: server_thread client
server_thread:server_thread.c
	gcc -o $@ $^ -lpthread
client:client.c
	gcc -o $@ $^

clean:
	rm -rf client server_thread
