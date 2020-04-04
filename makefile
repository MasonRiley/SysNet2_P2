all:
	$(CC) -Wall chat_server.c -O2 -std=c11 -lpthread -o chat_server
	$(CC) -Wall chat_client.c -O2 -std=c11 -lpthread -o chat_client

debug:
	$(CC) -Wall -g chat_server.c -O0 -std=c11 -lpthread -o chat_server_dbg

clean:
	$(RM) -rf chat_server chat_server_dbg
