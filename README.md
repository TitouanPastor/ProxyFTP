# ProxyFTP Project

The ProxyFTP project is a software solution that acts as a proxy or intermediary between an FTP client and an FTP server. By being located at the border between the local domain and the rest of the world, the proxy allows users to access FTP servers on the Internet.



The identification of the user and the server site must be provided to the proxy in the following form: login@servername. It is important to note that several machines can connect at the same time to the proxy, which will create a thread for each connected client.



The project was initiated by the teachers and the students were responsible for programming the connection between the client and the server using the proxy. The ProxyFTP project provides an effective solution for securing FTP connections.



## Instructions

To launch the proxy, simply go to the ProxyFTP folder and run the "make" command to execute the makefile. Then you can launch the proxy with the command "./proxy". It is important to follow the instructions to open a second window and connect with "ftp -d [address] [port]".



## Authors



- [Titouan Pastor](https://github.com/TitouanPastor)

- [Baptiste Bayche](https://github.com/BaptisteBayche)
