#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>

#define MAX_DEFIS 20

typedef struct game game;
typedef struct player player;

enum GameState
{
    IN_PROGRESS,
    FINISHED
};

struct player
{
    int id;
    int socket;
    pthread_t thread;
    char pseudo[32];
    char password[256];
    int connected;

    player *defis_recus[MAX_DEFIS];
    int nb_defis;
    game *partie_active;

    char bio[256];
};

struct game
{
    int id;
    player *players[2];
    int tour;
    int score[2];
    int plateau[12];
    int winner; // 0 pour aucun, 1 pour joueur 1, 2 pour joueur 2
    enum GameState state;
    player *spectateur[100];
    int move_history[100];
    int move_count;
};

typedef struct
{
    player *players[100];
    game *games[50];
    int count_players;
    int count_games;
    pthread_mutex_t mutex;
} server_state;

typedef struct

{
    int socket;
    server_state *state;
} client_data;

static void envoyer(player *p, const char *msg)
{
    if (p != NULL && p->connected)
        write(p->socket, msg, strlen(msg));
}

static player *trouver_joueur(server_state *state, const char *pseudo)
{
    for (int i = 0; i < state->count_players; i++)
        if (strcmp(state->players[i]->pseudo, pseudo) == 0)
            return state->players[i];
    return NULL;
}

/* 0 = ok, -1 = liste pleine, -2 = défi déjà envoyé */
static int ajouter_defi(player *cible, player *de)
{
    for (int i = 0; i < cible->nb_defis; i++)
        if (cible->defis_recus[i] == de)
            return -2;
    if (cible->nb_defis >= MAX_DEFIS)
        return -1;
    cible->defis_recus[cible->nb_defis++] = de;
    return 0;
}

/* 0 = retiré, -1 = ce défi n'existait pas */
static int retirer_defi(player *cible, player *de)
{
    for (int i = 0; i < cible->nb_defis; i++)
    {
        if (cible->defis_recus[i] == de)
        {
            for (int j = i; j < cible->nb_defis - 1; j++)
                cible->defis_recus[j] = cible->defis_recus[j + 1];
            cible->nb_defis--;
            return 0;
        }
    }
    return -1;
}

static int demander(int sock, const char *prompt, char *out, size_t taille)
{
    write(sock, prompt, strlen(prompt));
    int n = read(sock, out, taille - 1);
    if (n <= 0)
        return -1;
    out[n] = '\0';
    out[strcspn(out, "\r\n")] = '\0';
    return 0;
}

void init_game(game *g, player *player1, player *player2, server_state *state)
{
    printf("creation d'une partie\n");
    g->id = state->count_games;
    g->tour = 0;
    g->score[0] = 0;
    g->score[1] = 0;
    g->players[0] = player1;
    g->players[1] = player2;
    for (int i = 0; i < 100; i++)
    {
        g->spectateur[i] = NULL;
    }

    state->games[state->count_games] = g;
    state->count_games++;

    for (int i = 0; i < 12; i++)
    {
        g->plateau[i] = 4; // Chaque case commence avec 4 graines
    }

    g->winner = 0;
    g->state = IN_PROGRESS;
    printf("Partie Créée : %d\n", g->id);
}

int afficher_plateau(game *g, int spectate_only)
{
    printf("Tour du joueur : %d\n", (g->tour + 1) % 2);
    printf("Score : Joueur 1 = %d, Joueur 2 = %d\n", g->score[0], g->score[1]);
    printf("Plateau : \n");
    char buffer_player_1[1024];
    char buffer_player_2[1024];

    /* Affichage du plateau pour le joueur 1 */

    snprintf(buffer_player_1, sizeof(buffer_player_1), "%s (%d)\n", g->players[0]->pseudo, g->score[0]);
    for (int i = 5; i >= 0; i--)
    {
        snprintf(buffer_player_1 + strlen(buffer_player_1), sizeof(buffer_player_1) - strlen(buffer_player_1), " %2d ", g->plateau[i]);
    }
    snprintf(buffer_player_1 + strlen(buffer_player_1), sizeof(buffer_player_1) - strlen(buffer_player_1), "\n");
    for (int i = 6; i < 12; i++)
    {
        snprintf(buffer_player_1 + strlen(buffer_player_1), sizeof(buffer_player_1) - strlen(buffer_player_1), " %2d ", g->plateau[i]);
    }
    snprintf(buffer_player_1 + strlen(buffer_player_1), sizeof(buffer_player_1) - strlen(buffer_player_1), "\n%s (%d)\n", g->players[1]->pseudo, g->score[1]);

    /* Affichage du plateau pour le joueur 2 */
    snprintf(buffer_player_2, sizeof(buffer_player_2), "%s (%d)\n", g->players[1]->pseudo, g->score[1]);
    for (int i = 11; i >= 6; i--)
    {
        snprintf(buffer_player_2 + strlen(buffer_player_2), sizeof(buffer_player_2) - strlen(buffer_player_2), " %2d ", g->plateau[i]);
    }
    snprintf(buffer_player_2 + strlen(buffer_player_2), sizeof(buffer_player_2) - strlen(buffer_player_2), "\n");
    for (int i = 0; i < 6; i++)
    {
        snprintf(buffer_player_2 + strlen(buffer_player_2), sizeof(buffer_player_2) - strlen(buffer_player_2), " %2d ", g->plateau[i]);
    }
    snprintf(buffer_player_2 + strlen(buffer_player_2), sizeof(buffer_player_2) - strlen(buffer_player_2), "\n%s (%d)\n", g->players[0]->pseudo, g->score[0]);

    /* Envoie du plateau aux deux joueurs */
    if (spectate_only == 0)
    {
        envoyer(g->players[0], buffer_player_2);
        envoyer(g->players[1], buffer_player_1);
    }
    for (int i = 0; i < 100; i++)
        envoyer(g->spectateur[i], buffer_player_1);

    return 0;
}

void *gestion_client(void *arg)
{
    client_data *data = (client_data *)arg;

    int newsockfd = data->socket;
    server_state *state = data->state;

    free(data);

    char pseudo[32], mot_de_passe[256];
    int id = -1;
    int essais = 0;

    while (id == -1)
    {
        if (essais >= 5)
        {
            write(newsockfd, "ERROR trop d'essais\n", strlen("ERROR trop d'essais\n"));
            close(newsockfd);
            return NULL;
        }
        essais++;

        /* Lectures AVANT de prendre le mutex */
        if (demander(newsockfd, "Entrez votre pseudo : ", pseudo, sizeof(pseudo)) < 0 ||
            demander(newsockfd, "Entrez votre mot de passe : ", mot_de_passe, sizeof(mot_de_passe)) < 0)
        {
            close(newsockfd); // le client est parti : on arrête vraiment
            return NULL;
        }

        pthread_mutex_lock(&state->mutex);

        int trouve = -1;
        for (int i = 0; i < state->count_players; i++)
        {
            if (strcmp(state->players[i]->pseudo, pseudo) == 0)
            {
                trouve = i;
                break;
            }
        }

        if (trouve != -1)
        {
            if (strcmp(state->players[trouve]->password, mot_de_passe) != 0)
            {
                pthread_mutex_unlock(&state->mutex);
                write(newsockfd, "ERROR mot de passe incorrect\n", strlen("ERROR mot de passe incorrect\n"));
                continue; // on redemande
            }
            if (state->players[trouve]->connected)
            {
                pthread_mutex_unlock(&state->mutex);
                write(newsockfd, "ERROR deja connecte\n", strlen("ERROR deja connecte\n"));
                continue;
            }
            state->players[trouve]->socket = newsockfd;
            state->players[trouve]->thread = pthread_self();
            state->players[trouve]->connected = 1;
            id = trouve;
        }
        else
        {
            if (state->count_players >= 100)
            {
                pthread_mutex_unlock(&state->mutex);
                write(newsockfd, "ERROR serveur plein\n", strlen("ERROR serveur plein\n"));
                close(newsockfd); // inutile de reboucler : c'est plein
                return NULL;
            }
            /* Ne dois pas contenir d'espace*/
            if (strchr(pseudo, ' ') != NULL || strchr(mot_de_passe, ' ') != NULL)
            {
                pthread_mutex_unlock(&state->mutex);
                write(newsockfd, "ERROR pseudo ou mot de passe invalide\n", strlen("ERROR pseudo ou mot de passe invalide\n"));
                continue;
            }

            player *p = malloc(sizeof(player));
            if (p == NULL)
            {
                pthread_mutex_unlock(&state->mutex);
                close(newsockfd);
                return NULL;
            }
            memset(p, 0, sizeof(player));
            snprintf(p->pseudo, sizeof(p->pseudo), "%s", pseudo);
            snprintf(p->password, sizeof(p->password), "%s", mot_de_passe);
            p->id = state->count_players;
            p->socket = newsockfd;
            p->thread = pthread_self();
            p->connected = 1;
            state->players[p->id] = p;
            state->count_players++;
            id = p->id;
        }
        pthread_mutex_unlock(&state->mutex);
    }
    write(newsockfd, "OK\n", strlen("OK\n"));

    /* Boucle de gestion des commandes. */
    char buffer[256];

    while (1)
    {

        int n = read(newsockfd, buffer, sizeof(buffer) - 1);
        if (n <= 0)
        {
            break;
        }

        buffer[n] = '\0';
        buffer[strcspn(buffer, "\r\n")] = '\0';

        /*HELP POUR LES COMMANDES*/
        if (strncmp(buffer, "help", 4) == 0)
        {
            char help_message[] = "Commandes disponibles :\n"
                      "list_player - Liste des joueurs\n"
                      "list_games - Liste des parties\n"
                      "list_defis - Liste des défis reçus\n"
                      "write_bio <bio> - Écrire votre biographie\n"
                      "defis <pseudo> - Défier un joueur\n"
                      "Yes <pseudo> - Accepter le défi de ce joueur\n"
                      "No <pseudo> - Refuser le défi de ce joueur\n"
                      "join <id_partie> - Rejoindre / reprendre votre partie\n"
                      "play <0-5> - Jouer un coup\n"
                      "spectate <id_partie> - Regarder une partie\n"
                      "unspectate <id_partie> - Arrêter de regarder\n"
                      "review <id_partie> - Historique des coups\n"
                      "msg <pseudo> <message> - Envoyer un message\n";
            write(newsockfd, help_message, strlen(help_message));
            continue;
        }

        /*LISTE PLAYERS*/
        else if (strncmp(buffer, "list_player", 12) == 0)
        {

            pthread_mutex_lock(&state->mutex);
            char liste[4096] = "";
            for (int i = 0; i < state->count_players; i++)
            {
                strcat(liste, state->players[i]->pseudo);
                strcat(liste, " - ");
                strcat(liste, state->players[i]->bio);
                strcat(liste, "\n");
            }

            pthread_mutex_unlock(&state->mutex);
            write(newsockfd, liste, strlen(liste));
        }

        /* LISTE PARTIES */
        else if (strncmp(buffer, "list_games", 10) == 0)
        {
            pthread_mutex_lock(&state->mutex);
            char liste[1024] = "";
            for (int i = 0; i < state->count_games; i++)
            {
                snprintf(liste + strlen(liste), sizeof(liste) - strlen(liste), "Partie %d\n", state->games[i]->id);
            }
            pthread_mutex_unlock(&state->mutex);
            write(newsockfd, liste, strlen(liste));
        }
        /* ECRIRE UNE BIOGRAPHIE */
        else if (strncmp(buffer, "write_bio ", 10) == 0)
        {
            char bio[256];
            strcpy(bio, buffer + 10);

            pthread_mutex_lock(&state->mutex);
            strcpy(state->players[id]->bio, bio);
            pthread_mutex_unlock(&state->mutex);
        }

        /*SPECTATE*/
        else if (strncmp(buffer, "spectate ", 9) == 0)
        {
            char game_id[256];
            strcpy(game_id, buffer + 9);

            pthread_mutex_lock(&state->mutex);

            int id_partie = atoi(game_id);
            if (id_partie < 0 || id_partie >= state->count_games)
            {
                pthread_mutex_unlock(&state->mutex);
                write(newsockfd, "ERROR partie inexistante\n", strlen("ERROR partie inexistante\n"));
                continue;
            }
            game *g = state->games[id_partie];

            if (g->state == FINISHED)
            {
                pthread_mutex_unlock(&state->mutex);
                write(newsockfd, "ERROR partie terminée\n", strlen("ERROR partie terminée\n"));
                continue;
            }

            /* Ajouter le joueur à la liste des spectateurs */
            for (int i = 0; i < 100; i++)
            {
                if (g->spectateur[i] == NULL)
                {
                    g->spectateur[i] = state->players[id];
                    break;
                }
            }

            afficher_plateau(g, 1);
            pthread_mutex_unlock(&state->mutex);
        }

        /* UNSPECTATE*/
        else if (strncmp(buffer, "unspectate ", 11) == 0)
        {
            char game_id[256];
            strcpy(game_id, buffer + 11);

            pthread_mutex_lock(&state->mutex);

            int id_partie = atoi(game_id);
            if (id_partie < 0 || id_partie >= state->count_games)
            {
                pthread_mutex_unlock(&state->mutex);
                write(newsockfd, "ERROR partie inexistante\n", strlen("ERROR partie inexistante\n"));
                continue;
            }
            game *g = state->games[id_partie];

            /* Retirer le joueur de la liste des spectateurs */
            for (int i = 0; i < 100; i++)
            {
                if (g->spectateur[i] != NULL && strcmp(g->spectateur[i]->pseudo, pseudo) == 0)
                {
                    g->spectateur[i] = NULL;
                    break;
                }
            }

            pthread_mutex_unlock(&state->mutex);
        }

        /* REVIEW */
        else if (strncmp(buffer, "review ", 7) == 0)
        {
            char game_id[256];
            strcpy(game_id, buffer + 7);

            pthread_mutex_lock(&state->mutex);

            int id_partie = atoi(game_id);
            if (id_partie < 0 || id_partie >= state->count_games)
            {
                pthread_mutex_unlock(&state->mutex);
                write(newsockfd, "ERROR partie inexistante\n", strlen("ERROR partie inexistante\n"));
                continue;
            }
            game *g = state->games[id_partie];

            /* Afficher l'historique des coups */
            for (int i = 0; i < g->move_count; i++)
            {
                char message[512];
                snprintf(message, sizeof(message), "Coup %d: %d\n", i + 1, g->move_history[i]);
                write(newsockfd, message, strlen(message));
            }

            pthread_mutex_unlock(&state->mutex);
        }

        /* DEFI */
        else if (strncmp(buffer, "defis ", 6) == 0)
        {
            pthread_mutex_lock(&state->mutex);
            player *moi = state->players[id];
            player *cible = trouver_joueur(state, buffer + 6);
            const char *erreur = NULL;

            if (cible == NULL)
                erreur = "ERROR joueur inexistant\n";
            else if (cible == moi)
                erreur = "ERROR vous ne pouvez pas vous defier vous-meme\n";
            else
            {
                int r = ajouter_defi(cible, moi);
                if (r == -2)
                    erreur = "ERROR defi deja envoye\n";
                else if (r == -1)
                    erreur = "ERROR ce joueur a trop de defis en attente\n";
                else
                {
                    char message[512];
                    snprintf(message, sizeof(message), "Vous avez reçu un défi de %s\n", moi->pseudo);
                    envoyer(cible, message);
                }
            }
            pthread_mutex_unlock(&state->mutex);
            if (erreur)
                write(newsockfd, erreur, strlen(erreur));
        }

        /* LISTE DES DEFIS RECUS */
        else if (strncmp(buffer, "list_defis", 10) == 0)
        {
            pthread_mutex_lock(&state->mutex);
            player *moi = state->players[id];
            char liste[2048] = "";
            if (moi->nb_defis == 0)
                snprintf(liste, sizeof(liste), "Aucun défi en attente\n");
            for (int i = 0; i < moi->nb_defis; i++)
                snprintf(liste + strlen(liste), sizeof(liste) - strlen(liste),
                         "Défi de %s\n", moi->defis_recus[i]->pseudo);
            pthread_mutex_unlock(&state->mutex);
            write(newsockfd, liste, strlen(liste));
        }

        /* ACCEPTER : Yes <pseudo> */
        else if (strncmp(buffer, "Yes ", 4) == 0)
        {
            pthread_mutex_lock(&state->mutex);
            player *moi = state->players[id];
            player *defieur = trouver_joueur(state, buffer + 4);
            const char *erreur = NULL;

            if (defieur == NULL || retirer_defi(moi, defieur) != 0)
                erreur = "ERROR aucun defi de ce joueur\n";
            else if (!defieur->connected)
                erreur = "ERROR ce joueur est deconnecte\n";
            else if (state->count_games >= 50)
                erreur = "ERROR trop de parties\n";
            else
            {
                game *new_game = malloc(sizeof(game));
                if (new_game == NULL)
                    erreur = "ERROR memoire\n";
                else
                {
                    memset(new_game, 0, sizeof(game));
                    init_game(new_game, moi, defieur, state);
                    moi->partie_active = new_game;
                    defieur->partie_active = new_game;

                    char message[512];
                    snprintf(message, sizeof(message), "%s a accepté votre défi\n", moi->pseudo);
                    envoyer(defieur, message);
                    afficher_plateau(new_game, 0);
                }
            }
            pthread_mutex_unlock(&state->mutex);
            if (erreur)
                write(newsockfd, erreur, strlen(erreur));
        }

        /* REFUSER : No <pseudo> */
        else if (strncmp(buffer, "No ", 3) == 0)
        {
            pthread_mutex_lock(&state->mutex);
            player *moi = state->players[id];
            player *defieur = trouver_joueur(state, buffer + 3);
            const char *erreur = NULL;

            if (defieur == NULL || retirer_defi(moi, defieur) != 0)
                erreur = "ERROR aucun defi de ce joueur\n";
            else
            {
                char message[512];
                snprintf(message, sizeof(message), "%s a refusé votre défi\n", moi->pseudo);
                envoyer(defieur, message);
            }
            pthread_mutex_unlock(&state->mutex);
            if (erreur)
                write(newsockfd, erreur, strlen(erreur));
        }
        /* Rejoindre une partie */
        else if (strncmp(buffer, "join ", 5) == 0)
        {
            char game_id[256];
            strcpy(game_id, buffer + 5);

            pthread_mutex_lock(&state->mutex);

            int id_partie = atoi(game_id);
            if (id_partie < 0 || id_partie >= state->count_games)
            {
                pthread_mutex_unlock(&state->mutex);
                write(newsockfd, "ERROR partie inexistante\n", strlen("ERROR partie inexistante\n"));
                continue;
            }
            game *g = state->games[id_partie];

            if (g->state == FINISHED)
            {
                pthread_mutex_unlock(&state->mutex);
                write(newsockfd, "ERROR partie terminée\n", strlen("ERROR partie terminée\n"));
                continue;
            }

            if (g->players[0]->socket != newsockfd && g->players[1]->socket != newsockfd)
            {
                pthread_mutex_unlock(&state->mutex);
                write(newsockfd, "ERROR vous n'êtes pas dans cette partie\n", strlen("ERROR vous n'êtes pas dans cette partie\n"));
                continue;
            }

            /* CORRECTION : Retrouver l'index du joueur actuel dans state->players */
            int joueur_index = -1;
            for (int i = 0; i < state->count_players; i++)
            {
                if (strcmp(state->players[i]->pseudo, pseudo) == 0)
                {
                    joueur_index = i;
                    break;
                }
            }

            if (joueur_index != -1)
            {
                state->players[joueur_index]->partie_active = g;
            }

            afficher_plateau(g, 0);
            pthread_mutex_unlock(&state->mutex);
        }

        /* Jouer un Coup */
        else if (strncmp(buffer, "play ", 5) == 0)
        {
            int move = atoi(buffer + 5);

            pthread_mutex_lock(&state->mutex);

            /* Chercher le joueur actuel */
            int jouer_index = -1;
            for (int i = 0; i < state->count_players; i++)
            {
                if (strcmp(state->players[i]->pseudo, pseudo) == 0)
                {
                    jouer_index = i;
                    break;
                }
            }
            if (jouer_index == -1)
            {
                pthread_mutex_unlock(&state->mutex);
                continue;
            }
            game *g = state->players[jouer_index]->partie_active;

            if (g == NULL)
            {
                pthread_mutex_unlock(&state->mutex);
                write(newsockfd, "ERROR pas de partie en cours\n", strlen("ERROR pas de partie en cours\n"));
                continue;
            }

            /* Vérifier si c'est le tour du joueur */
            if ((g->players[g->tour % 2] != state->players[jouer_index]))
            {
                pthread_mutex_unlock(&state->mutex);
                write(newsockfd, "ERROR pas votre tour\n", strlen("ERROR pas votre tour\n"));
                continue;
            }

            /* Vérifier si le coup est valide */
            if (move < 0 || move > 5 || g->plateau[move + g->tour % 2 * 6] == 0)
            {
                pthread_mutex_unlock(&state->mutex);
                write(newsockfd, "ERROR coup invalide\n", strlen("ERROR coup invalide\n"));
                continue;
            }
            /* Mettre à jour le plateau de jeu */
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

            g->move_history[g->move_count++] = move;

            /* Mettre à jour le score */
            if ((move + g->tour % 2 * 6 < 6 && current_position >= 6) || (move + g->tour % 2 * 6 >= 6 && current_position < 6))
            {
                while (g->plateau[current_position] == 2 || g->plateau[current_position] == 3)
                {
                    g->score[g->tour % 2] += g->plateau[current_position];
                    g->plateau[current_position] = 0;
                    current_position = (current_position - 1 + 12) % 12;
                }
            }
            /* Verifier fin de partie */
            if (g->score[0] >= 25 || g->score[1] >= 25)
            {
                g->state = FINISHED;
                g->winner = (g->score[0] >= 25) ? 1 : 2;
                afficher_plateau(g, 0);

                char fin[512];
                snprintf(fin, sizeof(fin), "Partie terminée : %s a gagné !\n",
                         g->players[g->winner - 1]->pseudo);
                envoyer(g->players[0], fin);
                envoyer(g->players[1], fin);
                for (int i = 0; i < 100; i++)
                    envoyer(g->spectateur[i], fin);

                g->players[0]->partie_active = NULL;
                g->players[1]->partie_active = NULL;
                pthread_mutex_unlock(&state->mutex);
                continue;
            }
            g->tour++;
            afficher_plateau(g, 0);
            pthread_mutex_unlock(&state->mutex);
        }
        /* Envoyer un message à un joueur */
        /* Format : msg <destinataire> <message> */
        else if (strncmp(buffer, "msg ", 4) == 0)
        {
            char *destinataire = strtok(buffer + 4, " ");
            char *message = strtok(NULL, "\n");

            if (destinataire == NULL || message == NULL)
            {
                write(newsockfd, "ERROR usage: msg <destinataire> <message>\n",
                      strlen("ERROR usage: msg <destinataire> <message>\n"));
                continue;
            }

            pthread_mutex_lock(&state->mutex);

            int destinataire_index = -1;
            for (int i = 0; i < state->count_players; i++)
            {
                if (strcmp(state->players[i]->pseudo, destinataire) == 0)
                {
                    destinataire_index = i;
                    break;
                }
            }

            if (destinataire_index == -1)
            {
                pthread_mutex_unlock(&state->mutex);
                write(newsockfd, "ERROR destinataire inexistant\n", strlen("ERROR destinataire inexistant\n"));
                continue;
            }

            /* Envoyer le message */
            char full_message[512];
            snprintf(full_message, sizeof(full_message), "Message de %s: %s\n", pseudo, message);
            envoyer(state->players[destinataire_index], full_message);
            pthread_mutex_unlock(&state->mutex);
        }
    }

    /* Le client s'est déconnecté.  il faut mettre la variable a nulle*/
    pthread_mutex_lock(&state->mutex);

    player *p = state->players[id];
    if (p->socket == newsockfd) {
        p->socket = -1;
        p->connected = 0;
    }

    /* Le retirer des parties qu'il regardait */
    for (int g = 0; g < state->count_games; g++)
    {
        for (int s = 0; s < 100; s++)
        {
            if (state->games[g]->spectateur[s] == p)
                state->games[g]->spectateur[s] = NULL;
        }
    }

    pthread_mutex_unlock(&state->mutex);

    close(newsockfd);

    return NULL;
}

int main(int argc, char **argv)
{
    int sockfd;
    int newsockfd;

    socklen_t clilen;

    struct sockaddr_in cli_addr;
    struct sockaddr_in serv_addr;

    if (argc != 2)
    {
        printf("usage: socket_server port\n");
        exit(1);
    }

    printf("server starting...\n");

    /* État partagé entre les threads. */
    server_state state;

    state.count_players = 0;
    state.count_games = 0;

    /*
     * Initialisation du mutex.
     */
    if (pthread_mutex_init(&state.mutex, NULL) != 0)
    {
        perror("pthread_mutex_init");
        exit(1);
    }

    /*
     * Ouverture du socket.
     */
    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if (sockfd < 0)
    {
        perror("socket");
        exit(1);
    }

    /*
     * Initialisation de l'adresse du serveur.
     */
    bzero(
        (char *)&serv_addr,
        sizeof(serv_addr));

    serv_addr.sin_family = AF_INET;

    serv_addr.sin_addr.s_addr =
        htonl(INADDR_ANY);

    serv_addr.sin_port =
        htons(atoi(argv[1]));

    /*
     * bind
     */
    if (bind(
            sockfd,
            (struct sockaddr *)&serv_addr,
            sizeof(serv_addr)) < 0)
    {
        perror("bind");
        exit(1);
    }

    /*
     * Le serveur peut accepter plusieurs connexions.
     */
    listen(sockfd, 10);

    printf(
        "Serveur en attente de connexions...\n");

    while (1)
    {
        clilen = sizeof(cli_addr);
        newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);
        if (newsockfd < 0)
        {
            perror("accept");
            continue;
        }
        printf("Nouvelle connexion !\n");

        /*
         * Données transmises au thread.
         */
        client_data *data = malloc(sizeof(client_data));
        data->socket = newsockfd;
        data->state = &state;

        /*
         * Création du thread.
         */
        pthread_t thread;

        if (pthread_create(&thread, NULL, gestion_client, data) != 0)
        {
            perror("pthread_create");
            free(data);
            close(newsockfd);
            continue;
        }

        pthread_detach(thread);
    }
    close(sockfd);
    pthread_mutex_destroy(&state.mutex);

    return 0;
}