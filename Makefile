make : Client/client Serveur/server

Client/client : Client/client2.c Client/client2.h
	gcc -g Client/client2.c -o Client/client

Serveur/server : Serveur/server2.c Serveur/server2.h Serveur/client2.h 
	gcc -g Serveur/server2.c -o Serveur/server

clean :
	rm -f *.o Serveur/server Client/client Data/Discussion/.txt

exec_server :
	Serveur/server

exec_server_valgrind :
	valgrind Serveur/server