#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include "./simpleSocketAPI.h"

#define SERVADDR "127.0.0.1"  // Définition de l'adresse IP d'écoute
#define SERVPORT "0"          // Définition du port d'écoute, si 0 port choisi dynamiquement
#define LISTENLEN 5           // Taille de la file des demandes de connexion
#define MAXBUFFERLEN 1024     // Taille du tampon pour les échanges de données
#define MAXBUFFERDATALEN 8192 // Taille du tampon pour les échanges des connections data
#define MAXHOSTLEN 64         // Taille d'un nom de machine
#define MAXPORTLEN 64         // Taille d'un numéro de port
int ecode = 0;                // Code retour des fonctions
char bufferData[MAXBUFFERDATALEN]; // Tampon de communication entre le client et le serveur

void envoyerMessageClient(int sock, char *buffer);
void envoyerMessageServeur(int sock, char *buffer);
void lireMessageClient(int sock, char *buffer);
void lireMessageServeur(int sock, char *buffer);
void lireMessageData(int sock, char *buffer, char *bufferData);

int main()
{
    char serverAddr[MAXHOSTLEN];       // Adresse du serveur
    char serverPort[MAXPORTLEN];       // Port du server
    int descSockRDV;                   // Descripteur de socket de rendez-vous
    int descSockCOM;                   // Descripteur de socket de communication
    int descSockDATACOM;               // Descripteur de socket de communication data
    struct addrinfo hints;             // Contrôle la fonction getaddrinfo
    struct addrinfo *res;              // Contient le résultat de la fonction getaddrinfo
    struct sockaddr_storage myinfo;    // Informations sur la connexion de RDV
    struct sockaddr_storage from;      // Informations sur le client connecté
    socklen_t len;                     // Variable utilisée pour stocker les
                                       // longueurs des structures de socket
    char buffer[MAXBUFFERLEN];         // Tampon de communication entre le client et le serveur
    char bufferCut[5];                 // Tampon prenant les 4 premiers caractères du buffer (les commandes ftp)

    // Information serveur distant
    int sockRemoteServer;
    int sockRemoteDataServer;
    char login[30];
    char password[30];
    char remoteServerName[MAXHOSTLEN];
    char remoteServerPort[MAXPORTLEN] = "21";
    bool remoteConnectionOk = true;
    bool sendData = false;

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

    while (1)
    {
        // Attente connexion du client
        // Lorsque demande de connexion, creation d'une socket de communication avec le client
        descSockCOM = accept(descSockRDV, (struct sockaddr *)&from, &len);
        if (descSockCOM == -1)
        {
            perror("[LOG] Erreur accept\n");
            exit(6);
        }

        // à chaque connexion d'un client, on fork() pour créer un processus fils
        // qui va gérer la communication avec le client
        pid_t child_pid = fork(); // Création d'un processus enfant
        if (child_pid == -1)
        {
            perror("fork");
            exit(EXIT_FAILURE);
        }

        if (child_pid == 0)
        {
            // Echange de données avec le client connecté
            strcpy(buffer, "220 Bienvenue sur le proxy FTP !\n");
            envoyerMessageClient(descSockCOM, buffer);

            // Tant que l'utilisateur est connecté au serveur distant
            while (remoteConnectionOk)
            {
                lireMessageClient(descSockCOM, buffer);

                // On sépare le message du client en plusieurs parties avec le caractère @
                sscanf(buffer, "%[^@]@%s", login, remoteServerName);
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

                //331 Demande de mot de passe
                lireMessageServeur(sockRemoteServer, buffer);

                // On boucle tant qu'il y a communication entre le client et le ftp distant
                bool userConnectedToRemote = false;
                while (remoteConnectionOk)
                {
                    // On vérifie si l'utilisateur est connecté au serveur distant
                    // Sinon on le connecte avec le login donné juste avant*
                    if (!userConnectedToRemote)
                    {
                        printf("[LOG] Connexion de l'utilisateur au serveur distant FTP.\n");
                        // On se connecte avec le login précédement donné
                        // On transmet le message du client au serveur distant ftp
                        strcat(login, "\r\n");
                        write(sockRemoteServer, login, strlen(login));
                        printf("-<>- taille : %i : %s", strlen(login), login);

                        // On lit le message du serveur distant ftp
                        //230 Utilisateur connecté
                        lireMessageServeur(sockRemoteServer, buffer);

                        // On transmet le message du serveur distant ftp au client
                        envoyerMessageClient(descSockCOM, buffer);

                        userConnectedToRemote = true;
                    }
                    //"En attente" d'une commande du client
                    lireMessageClient(descSockCOM, buffer);

                    // On vérifie si l'utilisateur souhaite arreter la connection
                    // On découpe le buffer pour récuperer la commande et faire un traitement différent
                    strncpy(bufferCut, buffer, 4);
                    if (strcmp(bufferCut, "QUIT") == 0)
                    {
                        printf("[LOG] Fermeture de la connexion avec le serveur distant FTP.\n");
                        remoteConnectionOk = false;
                        memset(buffer, 0, MAXBUFFERLEN);
                        strcpy(buffer, "221 Connexion au proxy fermée.\r\n");
                        envoyerMessageClient(descSockCOM, buffer);
                    }
                    //Si la commande est ls
                    else if (strcmp(bufferCut, "PORT") == 0)
                    {
                        // On récupère l'ip et le port coté client
                        printf("[LOG] Récupération de l'ip et du port client.\n");
                        char ip1[4], ip2[4], ip3[4], ip4[4], firstPort[4], secondPort[4];
                        sscanf(buffer, "PORT %[^,],%[^,],%[^,],%[^,],%[^,],%s", ip1, ip2, ip3, ip4, firstPort, secondPort);
                        char ipClientData[16];
                        sprintf(ipClientData, "%s.%s.%s.%s", ip1, ip2, ip3, ip4);
                        int port = atol(firstPort) * 256 + atol(secondPort);
                        char portClientString[6];
                        sprintf(portClientString, "%d", port);
                        printf("[LOG] ipClientData : %s\n", ipClientData);
                        printf("[LOG] portClientData : %s\n", portClientString);

                        // On confirme que la commande port a bien été prise en compte
                        memset(buffer, 0, MAXBUFFERLEN);
                        strcpy(buffer, "200 PORT command successful.\r\n");
                        envoyerMessageClient(descSockCOM, buffer);

                        // On se met en passif coté proxy - serveur distant FTP
                        printf("[LOG] Passage en mode passif coté proxy - serveur distant FTP.\n");
                        memset(buffer, 0, MAXBUFFERLEN);
                        strcpy(buffer, "PASV\r\n");
                        envoyerMessageServeur(sockRemoteServer, buffer);

                        // On lit le message du serveur distant ftp
                        lireMessageServeur(sockRemoteServer, buffer);

                        // On récupère l'ip et le port coté serveur distant ftp
                        printf("[LOG] Récupération de l'ip et du port serveur distant FTP.\n");
                        sscanf(buffer, "227 Entering Passive Mode (%[^,],%[^,],%[^,],%[^,],%[^,],%[^)]).", ip1, ip2, ip3, ip4, firstPort, secondPort);
                        char ipFtpDistantData[16];
                        sprintf(ipFtpDistantData, "%s.%s.%s.%s", ip1, ip2, ip3, ip4);
                        port = atol(firstPort) * 256 + atol(secondPort);
                        char portServeurDataString[6];
                        sprintf(portServeurDataString, "%d", port);

                        // On se connecte au serveur distant FTP avec la connection de données
                        ecode = connect2Server(ipFtpDistantData, portServeurDataString, &sockRemoteDataServer);
                        if (ecode == -1)
                        {
                            perror("[LOG] Erreur connexion serveur distant FTP");
                            exit(8);
                        }
                        printf("[LOG] Connexion Proxy ---> FTP distant en PASSIF OK.\n");

                        // On lit la réponse du client à la commande "200 PORT command successful." (renvoi LIST)
                        lireMessageClient(descSockCOM, buffer);

                        // On envoi la commande LIST au serveur distant FTP
                        envoyerMessageServeur(sockRemoteServer, buffer);

                        // On lit le message du serveur distant ftp (150)
                        lireMessageServeur(sockRemoteServer, buffer);

                        // On envoi le message au client (150 Opening ASCII mode data connection for file list.)
                        envoyerMessageClient(descSockCOM, buffer);

                        // On lit le message du serveur distant ftp sur la connexion de données (le LS)
                        lireMessageData(sockRemoteDataServer, buffer, bufferData);
                        close(sockRemoteDataServer);

                        // On se connecte en mode actif du client au proxy
                        ecode = connect2Server(ipClientData, portClientString, &descSockDATACOM);
                        if (ecode == -1)
                        {
                            perror("[LOG] Erreur connexion client");
                            exit(8);
                        }

                        // On envoi le contenu de bufferData au client
                        envoyerMessageClient(descSockDATACOM, bufferData);
                        close(descSockDATACOM);
                        memset(bufferData, 0, MAXBUFFERDATALEN);

                        // On lit le message du serveur distant ftp (226 Transfer complete)
                        lireMessageServeur(sockRemoteServer, buffer);

                        // On envoi le message au client (226)
                        envoyerMessageClient(descSockCOM, buffer);
                    }
                    else
                    {
                        //Pour tout les autres messages
                        // On transmet le message du client au serveur distant ftp
                        envoyerMessageServeur(sockRemoteServer, buffer);

                        // On lit le message du serveur distant ftp
                        lireMessageServeur(sockRemoteServer, buffer);

                        // On transmet le message du serveur distant ftp au client
                        envoyerMessageClient(descSockCOM, buffer);
                    }
                }
            }

            // Fermeture de la connexion
            close(sockRemoteServer);
            close(descSockCOM);
            close(descSockRDV);
            exit(0);
        }
    }
}






// Fonctions
void envoyerMessageClient(int sock, char *buffer)
{
    ecode = write(sock, buffer, strlen(buffer));
    if (ecode == -1)
    {
        perror("[LOG] Erreur envoi message client");
        exit(5);
    }
    printf("<--- [SEND][CLIENT] : taille : %i : %s", strlen(buffer), buffer);
}

void envoyerMessageServeur(int sock, char *buffer)
{
    ecode = write(sock, buffer, strlen(buffer));
    if (ecode == -1)
    {
        perror("[LOG] Erreur envoi message serveur");
        exit(5);
    }
    printf("---> [SEND][SERVER] : taille : %i : %s", strlen(buffer), buffer);
}

void lireMessageClient(int sock, char *buffer)
{
    memset(buffer, 0, MAXBUFFERLEN);
    ecode = read(sock, buffer, MAXBUFFERLEN - 1);
    buffer[ecode] = '\0';
    if (ecode == -1)
    {
        perror("[LOG] Erreur lecture socket client");
        exit(7);
    }
}

void lireMessageServeur(int sock, char *buffer)
{
    memset(buffer, 0, MAXBUFFERLEN);
    ecode = read(sock, buffer, MAXBUFFERLEN - 1);
    buffer[ecode] = '\0';
    if (ecode == -1)
    {
        perror("[LOG] Erreur lecture socket serveur");
        exit(7);
    }
}

void lireMessageData(int sock, char *buffer, char *bufferData)
{
    memset(buffer, 0, MAXBUFFERLEN);
    memset(bufferData, 0, MAXBUFFERDATALEN);
    ecode = read(sock, buffer, MAXBUFFERLEN - 1);
    strcat(bufferData, buffer);
    memset(buffer, 0, MAXBUFFERLEN);
    while (ecode != 0)
    {
        ecode = read(sock, buffer, MAXBUFFERLEN - 1);
        strcat(bufferData, buffer);
        memset(buffer, 0, MAXBUFFERLEN);
    }
    if (ecode == -1)
    {
        perror("[LOG][DATA] Erreur lecture socket data");
        exit(7);
    }
}
