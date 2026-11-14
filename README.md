# Awalé en réseau

Jeu d'awalé client/serveur en C (sockets TCP + threads).

## Lancement

```bash
# Serveur
./server <port>

# Client
./client <adresse_ip> <port>
```

## Connexion

À la connexion, le serveur demande un **pseudo** puis un **mot de passe**.

- Pseudo inconnu : le compte est créé avec ce mot de passe.
- Pseudo existant : le mot de passe doit correspondre, et le compte ne doit pas déjà être connecté.
- Un joueur qui se reconnecte retrouve sa partie en cours (`join <id>` réaffiche le plateau).

## Commandes

| Commande | Description |
|---|---|
| `list_player` | Liste les joueurs inscrits |
| `list_games` | Liste les parties créées |
| `defis <pseudo>` | Envoie un défi au joueur indiqué |
| `Yes` | Accepte le défi reçu (crée la partie) |
| `No` | Refuse le défi reçu |
| `join <id>` | Rejoint (ou reprend) une partie dont on est joueur |
| `play <0-5>` | Joue la case `0` à `5` de son côté du plateau |
| `spectate <id>` | Regarde une partie en cours |
| `unspectate <id>` | Arrête de regarder une partie |
| `review <id>` | Affiche l'historique des coups d'une partie |
| `write_bio` | Renseigne une biographie

## Règles

- 12 cases, 4 graines au départ dans chacune.
- On égrène les graines de la case choisie dans le sens des cases croissantes, en sautant la case d'origine.
- Si la dernière graine tombe chez l'adversaire dans une case qui contient alors 2 ou 3 graines, on les capture, ainsi que les cases précédentes consécutives qui contiennent aussi 2 ou 3 graines.
- La partie se termine quand un joueur atteint 25 graines.

## Messages d'erreur

| Message | Cause |
|---|---|
| `ERROR mot de passe incorrect` | Mauvais mot de passe |
| `ERROR deja connecte` | Ce compte est déjà connecté |
| `ERROR serveur plein` | Limite de 100 joueurs atteinte |
| `ERROR joueur inexistant` | Pseudo inconnu dans `defis` |
| `ERROR aucun defi` | `Yes`/`No` sans défi en attente |
| `ERROR partie inexistante` | Identifiant de partie invalide |
| `ERROR partie terminée` | La partie est finie |
| `ERROR pas de partie en cours` | `play` sans partie active |
| `ERROR pas votre tour` | Ce n'est pas à vous de jouer |
| `ERROR coup invalide` | Case hors de 0-5 ou vide |
