all: serveur3-GALSEN serveur2-GALSEN serveur1-GALSEN 

serveur1-GALSEN: serveur1-GALSEN.c
	gcc  -pthread serveur1-GALSEN.c -lpthread -o serveur1-GALSEN


serveur2-GALSEN: serveur2-GALSEN.c
	gcc  -pthread serveur2-GALSEN.c -lpthread -o serveur2-GALSEN


serveur3-GALSEN: serveur3-GALSEN.c
	gcc  -pthread serveur3-GALSEN.c -lpthread -o serveur3-GALSEN

clean:
	rm -f serveur2-GALSEN *.o
	rm -f serveur1-GALSEN *.0
	rm -f serveur3-GALSEN *.0
