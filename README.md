# Projet de ProxyFTP
Le projet de ProxyFTP est une solution logicielle qui agit comme un mandataire ou intermédiaire entre un client FTP et un serveur FTP. En étant situé à la frontière entre le domaine local et le reste du monde, le proxy permet aux utilisateurs d'accéder à des serveurs FTP sur Internet.

L'identification de l'utilisateur et du site serveur doit être fournie au proxy sous la forme suivante: nom_login@nom_serveur. Il est important de noter que plusieurs machines peuvent se connecter en même temps au proxy, qui va créer un fils pour chaque client connecté.

Le projet a été initié par les professeurs et les étudiants ont été chargés de programmer la connexion entre le client et le serveur à l'aide du proxy. Le projet ProxyFTP offre une solution efficace pour sécuriser les connexions FTP.

## Instructions
Pour lancer le proxy, il suffit de se placer dans le dossier ProxyFTP et d'exécuter la commande "make" pour exécuter le makefile. Ensuite, on peut lancer le proxy avec la commande "./proxy". Il est important de suivre les instructions pour ouvrir une seconde fenêtre et se connecter avec "ftp -d [adresse] [port]".

## Auteurs

- [Titouan Pastor](https://github.com/TitouanPastor)
- [Baptiste Bayche](https://github.com/BaptisteBayche)
