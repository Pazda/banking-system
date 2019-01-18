default: bankingServer bankingClient

bankingServer: bankingServer.c
	gcc bankingServer.c -o bankingServer -fsanitize=address -pthread
	
bankingClient: bankingClient.c
	gcc bankingClient.c -o bankingClient -fsanitize=address -pthread
	
clean:
	rm bankingServer
	rm bankingClient
