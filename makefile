all:
	$(CC) -Wall chat_server.c -O2 -std=c11 -lpthread -o chat_server
	$(CC) -Wall chat_client.c -O2 -std=c11 -lpthread -o chat_client

clean:
	$(RM) -rf chat_server chat_client 
