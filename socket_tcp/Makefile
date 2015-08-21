SERVER := service.o socket.o
CLIENT := client.o socket.o


ALL:
	make client
	make service
client:$(CLIENT)
	$(CC) -o $@ $^
service:$(SERVER)
	$(CC) -o $@ $^
clean:
	rm -rf *.o client service