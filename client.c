#include <stdio.h>
#include <stdlib.h> /* pour exit */
#include <unistd.h> /* pour close */
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h> /* pour memset */
#include <netinet/in.h> /* pour struct sockaddr_in */
#include <arpa/inet.h> /* pour htons et inet_aton */
#include <unistd.h> /* pour sleep */
#include <poll.h>

#define PORT IPPORT_USERRESERVED // = 5000
#define MAX_USERS 3
#define LG_MESSAGE   256
int connect(int sockfd, const struct sockaddr *serv_addr,
socklen_t addrlen);
 
int main()
{
	struct sockaddr_in pointDeRencontreDistant;
	socklen_t longueurAdresse;
	char messageEnvoi[LG_MESSAGE]; /* le message de la couche Application ! */
	char messageRecu[LG_MESSAGE]; /* le message de la couche Application ! */
	int ecrits, lus; /* nb d’octets ecrits et lus */
	int socketClient;
	char *a;
	struct pollfd pollfds[2];
	
	// Crée un socket de communication
	socketClient = socket(PF_INET, SOCK_STREAM, 0); /* 0 indique que l’on utilisera le
	protocole par défaut associé à SOCK_STREAM soit TCP */
	// Teste la valeur renvoyée par l’appel système socket()
	if(socketClient < 0) /* échec ? */
	{
		perror("socket"); // Affiche le message d’erreur
		exit(-1); // On sort en indiquant un code erreur
	}
	printf("Socket créée avec succès ! (%d)\n", socketClient);
	// Obtient la longueur en octets de la structure sockaddr_in
	longueurAdresse = sizeof(pointDeRencontreDistant);
	// Initialise à 0 la structure sockaddr_in
	memset(&pointDeRencontreDistant, 0x00, longueurAdresse);
	// Renseigne la structure sockaddr_in avec les informations
	pointDeRencontreDistant.sin_family = PF_INET;
	// On choisit le numéro de port d’écoute du serveur
	pointDeRencontreDistant.sin_port = htons(IPPORT_USERRESERVED); // = 5000
	// On choisit l’adresse IPv4 du serveur
	inet_aton("127.0.0.1", &pointDeRencontreDistant.sin_addr);
	
	// Débute la connexion vers le processus serveur distant
	if((connect(socketClient, (struct sockaddr *)&pointDeRencontreDistant,
	longueurAdresse)) == -1)
	{
		perror("connect"); // Affiche le message d’erreur
		close(socketClient); // On ferme la ressource avant de quitter
		exit(-2); // On sort en indiquant un code erreur
	}
	printf("Connexion au serveur réussie avec succès !\n");
	//--- Début de l’étape n°4 :
	// Initialise à 0 les messages
	// Envoie un message au serveur
	
	while(strncmp(messageEnvoi,"<quitter>",9))
	{
		int nevents;
		
		memset(messageEnvoi, 0x00, LG_MESSAGE*sizeof(char));
		memset(messageRecu, 0x00, LG_MESSAGE*sizeof(char));
		
		pollfds[0].fd = socketClient;
		pollfds[0].events = POLLIN;
		pollfds[0].revents = 0;
		pollfds[1].fd = STDIN_FILENO;
		pollfds[1].events = POLLIN;
		pollfds[1].revents = 0;
		 
		nevents = poll(pollfds, 2, -1);
		if (nevents > 0) 
		{
			if (pollfds[1].revents != 0) 
			{	
				fgets(messageEnvoi, sizeof(messageEnvoi), stdin);
				printf("\n");	
		
				ecrits = write(socketClient, messageEnvoi, strlen(messageEnvoi)); 
				switch(ecrits)
				{
					case -1 : 
						perror("write");
						close(socketClient);
						exit(-3);
					return 0;
				}
			}
			if (pollfds[0].revents != 0) 
			{
				lus = read(socketClient, messageRecu, LG_MESSAGE*sizeof(char));
				switch(lus)
				{
					case -1 : 
						perror("read");
						close(socketClient);
						exit(-4);
					case 0 : 
						fprintf(stderr, "La socket a été fermée par le serveur !\n\n");
						close(socketClient);
						return 0;
					default:
						if(strncmp(messageRecu,"<message>",9) == 0)
						{
							strtok(messageRecu," ");
							a = strtok(NULL," ");
							snprintf(messageRecu, 256, "%s:\n",a);
							a = strtok(NULL," ");
							while(a != NULL)
							{
								strcat(messageRecu,a);
								strcat(messageRecu," ");
								a = strtok(NULL," ");
							}
						}
						printf("%s \n", messageRecu);
				}	
			}
		}
	}
	//--- Fin de l’étape n°4 !
	// On ferme la ressource avant de quitter
	close(socketClient);
	return 0;
}
