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
#define LG_LOGIN    20

typedef struct User {
	int socketClient;
	char login[LG_LOGIN];
	}user;

int main()
{
	int socketEcoute;
	int socketClient;
	struct sockaddr_in pointDeRencontreLocal;
	socklen_t longueurAdresse;
	struct sockaddr_in pointDeRencontreDistant;
	char messageEnvoi[LG_MESSAGE]; /* le message de la couche Application ! */  
	char messageRecu[LG_MESSAGE]; /* le message de la couche Application ! */ 
	char messageProtocole[LG_MESSAGE]; 
	char list[LG_MESSAGE];
	int ecrits, lus; /* nb d'octets ecrits et lus */
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
			for(int j = 0; j < nfds; j++)
			{
				if(pollfds[j].revents != 0)
				{
					if(j == 0)
					{	
						int i;
						socketClient = accept(socketEcoute,(struct sockaddr *)&pointDeRencontreDistant, & longueurAdresse);
						for(i = 0; i < MAX_USERS; i++)
						{
							if(users[i].socketClient == 0)
							{
								users[i].socketClient = socketClient;
								
								snprintf(users[i].login, LG_LOGIN, "user%d",i+1) ;
								printf("%s s'est connecté\n\n",users[i].login);
								write(users[i].socketClient, "<version> 1.0\n\n", strlen("<version> 1.0\n\n"));
								for(int k = 0; k < MAX_USERS; k++)
								{
									if(users[k].socketClient != 0)
									{	
										snprintf(messageProtocole, 256, "<greetings> %s\n",users[i].login);
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
							strcpy(messageEnvoi,"<erreur> Max d'utilisateur atteint\n");
							write(socketClient, messageEnvoi, strlen(messageEnvoi));
							close(socketClient);
							printf("Max d'utilisateur atteint\n\n");
							break;
						}
					}
					else
					{
						for(int i = 0; i < MAX_USERS; i++)
						{
							if(pollfds[j].fd == users[i].socketClient)
							{
								lus = read(users[i].socketClient,messageRecu,LG_MESSAGE*sizeof(char));
								if(strncmp(messageRecu,"<version>",9) == 0)
								{	
									strcpy(messageEnvoi,"<version> 1.0\n");
									write(users[i].socketClient, messageEnvoi, strlen(messageEnvoi));
								}
								else if(strncmp(messageRecu,"<list>",6) == 0)
								{
									strcat(messageEnvoi,"<list> |");
									for(int k = 0; k < MAX_USERS; k++)
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
								else if(strncmp(messageRecu,"<login>",7) == 0)
								{
									messageRecu[lus-1] = ' ';
									strcpy(messageProtocole,messageRecu);
									strtok(messageProtocole," ");
									mp = strtok(NULL," ");
									if(mp != NULL)
									{
										memset(users[i].login, 0x00, LG_LOGIN*sizeof(char));
										strcpy(users[i].login,mp);
									}
								}
								else if(strncmp(messageRecu,"<message>",9) == 0)
								{
									for(int k=0;k<MAX_USERS;k++)
									{
										if(users[k].socketClient != 0)
										{
											if(i != k)
											{
												strcpy(messageProtocole,messageRecu);
												strtok(messageProtocole," ");
												mp = strtok(NULL," ");
												if(strstr(mp,users[k].login) != NULL)
												{
													snprintf(messageEnvoi, 256, "<message> %s",users[i].login);
													mp = strtok(NULL," ");
													while(mp != NULL)
													{
														strcat(messageEnvoi," ");
														strcat(messageEnvoi,mp);
														mp = strtok(NULL," ");
													}
													write(users[k].socketClient, messageEnvoi, strlen(messageEnvoi));
													break;
												}
												else if(strchr(mp,'*') != NULL)
												{
													snprintf(messageEnvoi, 256, "<message> %s",users[i].login);
													mp = strtok(NULL," ");
													while(mp != NULL)
													{
														strcat(messageEnvoi," ");
														strcat(messageEnvoi,mp);
														mp = strtok(NULL," ");
													}	
													write(users[k].socketClient, messageEnvoi, strlen(messageEnvoi));
												}
											}
										}
									}
									if(strcmp(messageEnvoi,"") == 0)
									{
										strcpy(messageEnvoi,"<error> Destinataire non disponible ou n'existe pas\n");
										write(users[i].socketClient, messageEnvoi, strlen(messageEnvoi));
									}
								}
								/*else if(strncmp(messageRecu,"<quitter>",9) != 0)
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
								}*/
								switch(lus)
								{
									case -1:
										perror("read");
										close(users[i].socketClient);
										exit(-5);
									case 0 :
										fprintf(stderr,"%s s'est déconecté\n\n",users[i].login);
										for(int k=0; k<MAX_USERS;k++)
										{
											if(users[k].socketClient != 0)
											{	
												sprintf(messageEnvoi,"%s s'est déconecté\n",users[i].login); 
												ecrits = write(users[k].socketClient, messageEnvoi, strlen(messageEnvoi));
												if(ecrits == -1)
												{
													perror("write");
													close(users[k].socketClient);
													exit(-6);
												}
											}
										}
										close(users[i].socketClient);
										users[i].socketClient = 0;
										memset(users[i].login, 0x00, LG_LOGIN *sizeof(char));
										break;
									default:
										printf("Message reçu de %s: %s (%d octets)\n\n",users[i].login,messageRecu,lus);
								}
								
							}
						}	
					}
				}
			}
		}
		else {
			printf("poll() returned %d\n", nevents);
		}
	}

	// On ferme la ressource avant de quitter   
	close(socketEcoute);

	return 0;
}
