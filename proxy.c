#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include "./simpleSocketAPI.h"

#define SERVADDR "127.0.0.1" // Définition de l'adresse IP d'écoute
#define SERVPORT "0"         // Définition du port d'écoute, si 0 port choisi dynamiquement
#define LISTENLEN 1          // Taille de la file des demandes de connexion
#define MAXBUFFERLEN 1024    // Taille du tampon pour les échanges de données
#define MAXHOSTLEN 64        // Taille d'un nom de machine
#define MAXPORTLEN 64        // Taille d'un numéro de port

int main()
{
    int ecode;                      // Code retour des fonctions
    char serverAddr[MAXHOSTLEN];    // Adresse du serveur
    char serverPort[MAXPORTLEN];    // Port du server
    int descSockRDV;                // Descripteur de socket de rendez-vous
    int descSockCOM;                // Descripteur de socket de communication
    struct addrinfo hints;          // Contrôle la fonction getaddrinfo
    struct addrinfo *res;           // Contient le résultat de la fonction getaddrinfo
    struct sockaddr_storage myinfo; // Informations sur la connexion de RDV
    struct sockaddr_storage from;   // Informations sur le client connecté
    socklen_t len;                  // Variable utilisée pour stocker les
                                    // longueurs des structures de socket
    char buffer[MAXBUFFERLEN];      // Tampon de communication entre le client et le serveur

    // Information serveur distant
    int sockRemoteServer;
    char login[30];
    char password[30];
    char remoteServerName[MAXHOSTLEN];
    char remoteServerPort[MAXPORTLEN] = "21";
    bool remoteConnectionOk = true;

    // Initialisation de la socket de RDV IPv4/TCP
    descSockRDV = socket(AF_INET, SOCK_STREAM, 0);
    if (descSockRDV == -1)
    {
        perror("Erreur création socket RDV\n");
        exit(2);
    }
    // Publication de la socket au niveau du système
    // Assignation d'une adresse IP et un numéro de port
    // Mise à zéro de hints
    memset(&hints, 0, sizeof(hints));
    // Initialisation de hints
    hints.ai_flags = AI_PASSIVE;     // mode serveur, nous allons utiliser la fonction bind
    hints.ai_socktype = SOCK_STREAM; // TCP
    hints.ai_family = AF_INET;       // seules les adresses IPv4 seront présentées par
                                     // la fonction getaddrinfo

    // Récupération des informations du serveur
    ecode = getaddrinfo(SERVADDR, SERVPORT, &hints, &res);
    if (ecode)
    {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(ecode));
        exit(1);
    }
    // Publication de la socket
    ecode = bind(descSockRDV, res->ai_addr, res->ai_addrlen);
    if (ecode == -1)
    {
        perror("[LOG] Erreur liaison de la socket de RDV");
        exit(3);
    }
    // Nous n'avons plus besoin de cette liste chainée addrinfo
    freeaddrinfo(res);

    // Récuppération du nom de la machine et du numéro de port pour affichage à l'écran
    len = sizeof(struct sockaddr_storage);
    ecode = getsockname(descSockRDV, (struct sockaddr *)&myinfo, &len);
    if (ecode == -1)
    {
        perror("[LOG] SERVEUR: getsockname");
        exit(4);
    }
    ecode = getnameinfo((struct sockaddr *)&myinfo, sizeof(myinfo), serverAddr, MAXHOSTLEN,
                        serverPort, MAXPORTLEN, NI_NUMERICHOST | NI_NUMERICSERV);
    if (ecode != 0)
    {
        fprintf(stderr, "[LOG] error in getnameinfo: %s\n", gai_strerror(ecode));
        exit(4);
    }
    printf("[LOG] L'adresse d'ecoute est: %s\n", serverAddr);
    printf("[LOG] Le port d'ecoute est: %s\n", serverPort);

    // Definition de la taille du tampon contenant les demandes de connexion
    ecode = listen(descSockRDV, LISTENLEN);
    if (ecode == -1)
    {
        perror("[LOG] Erreur initialisation buffer d'écoute");
        exit(5);
    }

    len = sizeof(struct sockaddr_storage);
    // Attente connexion du client
    // Lorsque demande de connexion, creation d'une socket de communication avec le client
    descSockCOM = accept(descSockRDV, (struct sockaddr *)&from, &len);
    if (descSockCOM == -1)
    {
        perror("[LOG] Erreur accept\n");
        exit(6);
    }
    // // Echange de données avec le client connecté

    strcpy(buffer, "221 Bienvenue sur le proxy FTP !\n");
    write(descSockCOM, buffer, strlen(buffer));

    /*******
     *
     * A vous de continuer !
     *
     * *****/

    // Tant que l'utilisateur n'écrit pas "exit" dans le terminal
    while (remoteConnectionOk)
    {
        memset(buffer, 0, MAXBUFFERLEN); // On vide le tampon de communication
        // On lit le message du client
        ecode = read(descSockCOM, buffer, MAXBUFFERLEN - 1);
        buffer[ecode] = '\0'; // On ajoute le caractère de fin de chaîne
        if (ecode == -1)
        {
            perror("[LOG] Erreur lecture socket");
            exit(7);
        }
        buffer[ecode] = '\0'; // On ajoute le caractère de fin de chaîne

        // On affiche le message du client
        printf("---> : %s", buffer);
        // On sépare le message du client en plusieurs parties avec le caractère @
        sscanf(buffer, "%[^@]@%s", login, remoteServerName);
        // On affiche les deux parties
        printf("[LOG] loginUtilisateur : %s\n", login);
        printf("[LOG] ipServeurDistant : %s\n", remoteServerName);

        // On se connecte au serveur FTP distant
        ecode = connect2Server(remoteServerName, remoteServerPort, &sockRemoteServer);

        if (ecode == -1)
        {
            perror("[LOG] Erreur connexion serveur distant FTP");
            exit(8);
        }
        printf("[LOG] Connexion serveur distant FTP OK\n");

        // Lecture du message du serveur distant FTP
        memset(buffer, 0, MAXBUFFERLEN);
        ecode = read(sockRemoteServer, buffer, MAXBUFFERLEN - 1);
        buffer[ecode] = '\0'; // On ajoute le caractère de fin de chaîne
        if (ecode == -1)
        {
            perror("[LOG] Erreur lecture socket");
            exit(7);
        }
        printf("<--- : %s", buffer);

        // On transmet le message du serveur distant ftp au client
        write(descSockCOM, buffer, strlen(buffer));






        // On boucle tant qu'il y a communication entre le client et le ftp distant
        bool userConnectedToRemote = false;
        while (remoteConnectionOk)
        {
            // On vérifie si l'utilisateur est connecté au serveur distant
            // Sinon on le connecte avec le login donné juste avant
            if (!userConnectedToRemote)
            {

                // Login
                memset(buffer, 0, MAXBUFFERLEN);
                printf("[LOG] Envoi du login au serveur distant FTP.\n");

                // On se connecte avec le login précédement donné
                // On transmet le message du client au serveur distant ftp
                strcat(login, "\r\n");
                write(sockRemoteServer, login, strlen(login));

                // On lit le message du serveur distant ftp
                ecode = read(sockRemoteServer, buffer, MAXBUFFERLEN - 1);
                buffer[ecode] = '\0'; // On ajoute le caractère de fin de chaîne
                if (ecode == -1)
                {
                    perror("[LOG] Erreur lecture socket");
                    exit(7);
                }
                // On affiche le message du serveur distant ftp
                printf("<--- : %s", buffer);

                // On transmet le message du serveur distant ftp au client
                write(descSockCOM, buffer, strlen(buffer));
            }






            memset(buffer, 0, MAXBUFFERLEN);

            // On lit le message du client
            ecode = read(descSockCOM, buffer, MAXBUFFERLEN - 1);
            buffer[ecode] = '\0'; // On ajoute le caractère de fin de chaîne
            if (ecode == -1)
            {
                perror("[LOG] Erreur lecture socket");
                exit(7);
            }
            // On affiche le message du client
            printf("---> : %s", buffer);

            // On vérifie si l'utilisateur souhaite arreter la connection
            char *result = strstr(buffer, "QUIT");
            if (result != NULL)
            {
                remoteConnectionOk = false;
                printf("[LOG] Connexion serveur distant FTP fermée.\n");
            }

            // On transmet le message du client au serveur distant ftp
            write(sockRemoteServer, buffer, strlen(buffer));

            // On lit le message du serveur distant ftp
            memset(buffer, 0, MAXBUFFERLEN);
            ecode = read(sockRemoteServer, buffer, MAXBUFFERLEN - 1);
            buffer[ecode] = '\0'; // On ajoute le caractère de fin de chaîne
            if (ecode == -1)
            {
                perror("[LOG] Erreur lecture socket");
                exit(7);
            }
            // On affiche le message du serveur distant ftp
            printf("<--- : %s", buffer);

            // On transmet le message du serveur distant ftp au client
            write(descSockCOM, buffer, strlen(buffer));
        }
    }

    // Fermeture de la connexion
    close(sockRemoteServer);
    close(descSockCOM);
    close(descSockRDV);
}
