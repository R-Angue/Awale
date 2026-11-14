#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#define GRAS "\033[1m"
#define RESET "\033[0m"

int main(int argc, char **argv)
{
    int sockfd;
    struct sockaddr_in serv_addr;

    if (argc != 3)
    {
        printf("usage : socket_client server port\n");
        exit(1);
    }

    printf("client starting\n");

    /*
     * Initialisation de l'adresse du serveur
     */
    bzero(
        (char *)&serv_addr,
        sizeof(serv_addr));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(argv[1]);
    serv_addr.sin_port = htons(atoi(argv[2]));

    /*
     * Création du socket
     */
    sockfd = socket(
        AF_INET,
        SOCK_STREAM,
        0);

    if (sockfd < 0)
    {
        perror("socket");
        exit(1);
    }

    /*
     * Connexion au serveur
     */
    if (connect(
            sockfd,
            (struct sockaddr *)&serv_addr,
            sizeof(serv_addr)) < 0)
    {
        perror("connect");
        exit(1);
    }

    printf("Connecté au serveur.\n");

    fd_set readfds;

    while (1)
    {

        FD_ZERO(&readfds);

        FD_SET(STDIN_FILENO, &readfds);
        FD_SET(sockfd, &readfds);

        int maxfd = sockfd;

        select(maxfd + 1, &readfds, NULL, NULL, NULL);

        // Quelque chose arrive au clavier
        if (FD_ISSET(STDIN_FILENO, &readfds))
        {
            char commande[256];

            fgets(commande, sizeof(commande), stdin);
            commande[strcspn(commande, "\n")] = '\0';

            write(sockfd, commande, strlen(commande));
        }

        // Quelque chose arrive du serveur
        if (FD_ISSET(sockfd, &readfds))
        {
            char response[1024];
            int n = read(sockfd, response, sizeof(response) - 1);
            if (n <= 0)
            {
                printf("Connexion fermée par le serveur.\n");
                break;
            }
            response[n] = '\0';
            printf(GRAS "%s" RESET, response);
            fflush(stdout);
        }
    }

    close(sockfd);

    return 0;
}
