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

typedef struct User {
	int socketClient;
	char login[50];
	}user;

int main()
{
	int socketEcoute;
	struct sockaddr_in pointDeRencontreLocal;
	socklen_t longueurAdresse;
	struct sockaddr_in pointDeRencontreDistant;
	char messageEnvoi[LG_MESSAGE]; /* le message de la couche Application ! */  
	char messageRecu[LG_MESSAGE]; /* le message de la couche Application ! */ 
	char messageProtocole[LG_MESSAGE]; 
	char list[LG_MESSAGE];
	int ecrits;
	int lus; /* nb d'octets ecrits et lus */
	char *mp;
	//int retour;
	user users[MAX_USERS];
	struct pollfd pollfds[MAX_USERS + 1];

	memset(users, '\0', MAX_USERS*sizeof(user));

	// Crée un socket de communication 
	socketEcoute = socket(PF_INET, SOCK_STREAM, 0); /* 0 indique que l'on utilisera le protocole par défaut associé à SOCK_STREAM soit TCP */

	// Teste la valeur renvoyée par l'appel système socket()
	if(socketEcoute < 0) /* échec ? */
	{
		perror("socket"); // Affiche le message d'erreur
		exit(-1); // On sort en indiquant un code erreur
	} 

	printf("Socket créée avec succès ! (%d)\n", socketEcoute);

	// On prépare l'adresse d'attachement locale
	longueurAdresse = sizeof(struct sockaddr_in);
	memset(&pointDeRencontreLocal, 0x00, longueurAdresse);
	pointDeRencontreLocal.sin_family        = PF_INET;
	pointDeRencontreLocal.sin_addr.s_addr   = htonl(INADDR_ANY); // toutes les interfaces locales disponibles
	pointDeRencontreLocal.sin_port          = htons(PORT);   

	// On demande l'attachement local de la socket
	if((bind(socketEcoute, (struct sockaddr *)&pointDeRencontreLocal, longueurAdresse)) < 0)
	{
		perror("bind");
		exit(-2);
	}

	printf("Socket attachée avec succès !\n");

	// On fixe la taille de la file d'attente à 5 (pour les demande de connexion non encore traitées)
	if(listen(socketEcoute, 5) < 0)
	{
		perror("listen");
		exit(-3);
	}

	// Fin de l'étape n°6 !
	printf("Socket placée en écoute passive ...\n");

	// boucle d'attente de connexion : en théorie, un serveur attend indéfiniment !
	while(1)
	{
		int nevents;
		int nfds = 0;
		
		memset(messageRecu, 0x00, LG_MESSAGE*sizeof(char));
		memset(messageEnvoi, 0x00, LG_MESSAGE*sizeof(char));
		memset(messageProtocole, 0x00, LG_MESSAGE*sizeof(char));
		// Liste des sockets à écouter
		// socketEcoute + users[].socket => pollfds[]
		pollfds[nfds].fd = socketEcoute;
		pollfds[nfds].events = POLLIN;
		pollfds[nfds].revents = 0;
		nfds++;
		
		for(int i = 0 ; i<MAX_USERS;i++){
			if(users[i].socketClient > 0){
				pollfds[nfds].fd = users[i].socketClient;
				pollfds[nfds].events = POLLIN;
				pollfds[nfds].revents = 0;
				nfds++;
				}
			}
		nevents = poll(pollfds, nfds, -1);
		if (nevents > 0) 
		{
			// parcours de pollfds[] à la recherche des revents != 0
			for(int j =0;j< nfds;j++)
			{
				if(pollfds[j].revents != 0)
				{
					if(j == 0)
					{	
						int i;
						for(i=0;i<MAX_USERS;i++)
						{
							if(users[i].socketClient == 0)
							{
								users[i].socketClient = accept(socketEcoute,(struct sockaddr *)&pointDeRencontreDistant, & longueurAdresse);
								
								snprintf(users[i].login,50, "user%d",i+1) ;
								printf("%s s'est connecté\n\n",users[i].login) ;
								for(int k=0; k<MAX_USERS;k++)
								{
									if(users[k].socketClient != 0)
									{	
										snprintf(messageProtocole, 256, "greetings %s\n",users[i].login);
										ecrits = write(users[k].socketClient, messageProtocole, strlen(messageProtocole));
										if(ecrits == -1)
										{
											perror("write");
											close(users[k].socketClient);
											exit(-6);
										}
									}
								}
								if(users[i].socketClient < 0)
								{
									perror("accept");
									close(users[i].socketClient);
									close(socketEcoute);
									exit(-4);						
								}
								break;
							}	
						}
						if(i == MAX_USERS)
						{
							close(users[i].socketClient);
							printf("Max d'utilisateur atteint\n");
							break;
						}
					}
					else
					{
						for(int i=0; i<MAX_USERS;i++)
						{
							if(pollfds[j].fd == users[i].socketClient)
							{
								lus = read(users[i].socketClient,messageRecu,LG_MESSAGE*sizeof(char));
								if(strstr(messageRecu,"<version>") != NULL)
								{	
									strcpy(messageEnvoi,"Version 1.0\n");
									write(users[i].socketClient, messageEnvoi, strlen(messageEnvoi));
								}
								else if(strstr(messageRecu,"<list>") != NULL)
								{
									for(int k=0; k<MAX_USERS;k++)
									{
										if(users[k].socketClient != 0)
										{		
											snprintf(list, 256, "%s|",users[k].login);
											strcat(messageEnvoi,list);
										}
									}
									strcat(messageEnvoi,"\n");
									write(users[i].socketClient, messageEnvoi, strlen(messageEnvoi));
								}
								else if(strstr(messageRecu,"<message>") != NULL)
								{
									for(int k=0;k<MAX_USERS;k++)
									{
										
										strcpy(messageProtocole,messageRecu);
										strtok(messageProtocole," ");
										mp = strtok(NULL," ");
										if(strstr(mp,users[k].login) != NULL)
										{
											if(users[k].socketClient != 0)
											{
												snprintf(messageEnvoi, 256, "<message> %s:",users[i].login);
												mp = strtok(NULL," ");
												while(mp != NULL)
												{
													strcat(messageEnvoi," ");
													strcat(messageEnvoi,mp);
													mp = strtok(NULL," ");
												}
												write(users[k].socketClient, messageEnvoi, strlen(messageEnvoi));
											}
											/*else
											{
												strcpy(messageEnvoi,"<error>\n");
												write(users[i].socketClient, messageEnvoi, strlen(messageEnvoi));
											}*/
										}
									}
								}
								else
								{
									snprintf(messageEnvoi, 256, "%s: ",users[i].login);
									strcat(messageEnvoi,messageRecu);
									for(int k=0; k<MAX_USERS;k++)
									{
										if(users[k].socketClient != 0)
										{		
											if(k != i)
											{	
													write(users[k].socketClient, messageEnvoi, strlen(messageEnvoi));
											}
										}
									}	
								}
								switch(lus)
								{
									case -1:
										perror("read");
										close(users[i].socketClient);
										exit(-5);
									case 0 :
										fprintf(stderr,"%s s'est déconecté\n\n",users[i].login);
										close(users[i].socketClient);
										users[i].socketClient = 0;
										memset(users[i].login, 0x00, 50*sizeof(char));
										for(int k=0; k<MAX_USERS;k++)
										{
											if(users[k].socketClient != 0)
											{	
												strcpy(messageEnvoi,"s'est déconecté\n"); 
												ecrits = write(users[k].socketClient, messageEnvoi, strlen(messageEnvoi));
												if(ecrits == -1)
												{
													perror("write");
													close(users[k].socketClient);
													exit(-6);
												}
											}
										}
									default:
										printf("Message reçu de %s: %s (%d octets)\n\n",users[i].login,messageRecu,lus);
								}
								
							}
						}	
					}
				}
			}
		}
		

			// si c'est la socket socketEcoute => accept() + création d'une nouvelle entrée dans la table users[]
			//
			// sinon c'est une socket client => read() et gestion des erreurs pour le cas de la déconnexion
		else {
			printf("poll() returned %d\n", nevents);
		}
	}

	// On ferme la ressource avant de quitter   
	close(socketEcoute);

	return 0;
}
