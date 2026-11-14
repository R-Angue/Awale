#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>

typedef struct
{
    int socket;
    pthread_t thread;
    char pseudo[256];

    int defi_en_attente;
    char defi_de[256];
} player;

typedef struct
{
    player players[2];
    int tour;
    int score[2];
    int plateau[12];
} game;

int afficher_plateau(game *g)
{
    printf("Tour du joueur : %d\n", (g->tour + 1) % 2);
    printf("Score : Joueur 1 = %d, Joueur 2 = %d\n", g->score[0], g->score[1]);
    printf("Plateau : \n");

    if (g->tour % 2 == 0)
    {
        printf("Joueur 1 (haut)\n");
        for (int i = 11; i >= 6; i--)
        {
            printf(" %2d ", g->plateau[i]);
        }
        printf("\n");
        for (int i = 0; i < 6; i++)
        {
            printf(" %2d ", g->plateau[i]);
        }
        printf("\nJoueur 2 (bas)");
    }
    else
    {
        printf("Joueur 2 (haut)\n");
        for (int i = 5; i >= 0; i--)
        {
            printf(" %2d ", g->plateau[i]);
        }
        printf("\n");
        for (int i = 6; i < 12; i++)
        {
            printf(" %2d ", g->plateau[i]);
        }
        printf("\nJoueur 1 (bas)");
    }

    printf("\n");
    return 0;
}

int init_game(game *g)
{
    g->tour = 0;
    g->score[0] = 0;
    g->score[1] = 0;

    for (int i = 0; i < 12; i++)
    {
        g->plateau[i] = 4; // Chaque case commence avec 4 graines
    }

    return 0;
}

int loop_game(game *g)
{
    while (1)
    {

        afficher_plateau(g);
        // choose a move
        int move;
        scanf("%d", &move);
        // Verify if the move is valid
        if (move < 0 || move > 5)
        {
            printf("Mouvement invalide. Veuillez choisir un mouvement entre 0 et 5.\n");
            continue;
        }
        if (g->plateau[move + g->tour % 2 * 6] == 0)
        {
            printf("Mouvement invalide. La case choisie est vide.\n");
            continue;
        }
        // Update game state
        int graines_a_deplacer = g->plateau[move + g->tour % 2 * 6];
        g->plateau[move + g->tour % 2 * 6] = 0;
        int current_position = move + g->tour % 2 * 6;
        while (graines_a_deplacer > 0)
        {
            current_position = (current_position + 1) % 12;
            if (current_position == move + g->tour % 2 * 6)
            {
                printf("Sauter la case d'origine.\n");
                continue; // Skip the original pit
            }
            printf("Déplacement vers la case %d\n", current_position);
            g->plateau[current_position] += 1;
            graines_a_deplacer--;
        }

        // Update points
        if ((move + g->tour % 2 * 6 < 6 && current_position >= 6) || (move + g->tour % 2 * 6 >= 6 && current_position < 6))
        {
            while (g->plateau[current_position] == 2 || g->plateau[current_position] == 3)
            {
                g->score[g->tour % 2] += g->plateau[current_position];
                g->plateau[current_position] = 0;
                current_position = (current_position - 1 + 12) % 12;
            }
        }

        // Check if the game is over
        if (g->score[0] >= 25 || g->score[1] >= 25)
        {
            break;
        }
        g->tour++;
    }
    return 0;
}

int main(int argc, char **argv)
{
    game g;
    init_game(&g);
    loop_game(&g);

    return 0;
}
