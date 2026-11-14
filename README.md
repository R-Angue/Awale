# Awalé en réseau

Jeu d'awalé multijoueur en C : un serveur multi-threads (sockets TCP, un thread par client) et un client en ligne de commande.

## Compilation

```bash
make            # compile serveur et client
make clean      # supprime les exécutables
```

## Lancement

```bash
# Serveur
./serveur <port>

# Client
./client <adresse_ip> <port>
```

Le client est un simple relais clavier ↔ serveur : tout (connexion, commandes, affichage) est géré par le serveur.

## Connexion

À la connexion, le serveur demande un **pseudo** puis un **mot de passe**.

- **Pseudo inconnu** : le compte est créé avec ce mot de passe.
- **Pseudo existant** : le mot de passe doit correspondre, et le compte ne doit pas déjà être connecté.
- En cas d'erreur, la saisie est redemandée (5 essais maximum, puis la connexion est fermée).
- Le pseudo et le mot de passe d'un nouveau compte ne doivent pas contenir d'espace. Le pseudo fait 31 caractères maximum.
- Un joueur déconnecté peut se reconnecter avec les mêmes identifiants et retrouve sa partie en cours : tapez `join <id_partie>` pour réafficher le plateau.

Les comptes et les parties sont conservés en mémoire : ils sont perdus à l'arrêt du serveur.

## Commandes

### Général

| Commande | Description |
|---|---|
| `help` | Affiche la liste des commandes |
| `list_player` | Liste les joueurs inscrits avec leur biographie |
| `write_bio <texte>` | Définit votre biographie |
| `msg <pseudo> <message>` | Envoie un message privé à un joueur connecté |

### Défis

| Commande | Description |
|---|---|
| `defis <pseudo>` | Envoie un défi au joueur indiqué |
| `list_defis` | Liste les défis que vous avez reçus |
| `Yes <pseudo>` | Accepte le défi de ce joueur : la partie est créée |
| `No <pseudo>` | Refuse le défi de ce joueur |

Un joueur peut recevoir plusieurs défis en même temps (20 maximum) et choisit lequel accepter.

### Parties

| Commande | Description |
|---|---|
| `list_games` | Liste les parties créées |
| `join <id>` | Rejoint (ou reprend) une partie dont vous êtes joueur et réaffiche le plateau |
| `play <0-5>` | Joue la case `0` à `5` de votre côté du plateau |
| `spectate <id>` | Regarde une partie en cours |
| `unspectate <id>` | Arrête de regarder une partie |
| `review <id>` | Affiche l'historique des coups d'une partie |

Un joueur peut participer à plusieurs parties en même temps.

## Règles

- 12 cases, 4 graines dans chacune au départ. Le joueur 1 possède les cases 0 à 5, le joueur 2 les cases 6 à 11.
- Le joueur 1 commence, puis les joueurs jouent à tour de rôle.
- On prend toutes les graines de la case choisie et on les sème une par une dans les cases suivantes (sens croissant), en sautant la case d'origine.
- Si la dernière graine tombe chez l'adversaire dans une case qui contient alors 2 ou 3 graines, on les capture, ainsi que les cases précédentes consécutives qui contiennent aussi 2 ou 3 graines.
- La partie se termine quand un joueur atteint **25 graines**. Le gagnant est annoncé aux deux joueurs et aux spectateurs.

Pour les règles détaillées, regardez cette vidéo :

[![Règles de l'awalé en vidéo](https://img.youtube.com/vi/XPgE4VzLoyE/0.jpg)](https://www.youtube.com/watch?v=XPgE4VzLoyE)

## Affichage du plateau

Chaque joueur voit son propre côté en bas :

```
adversaire (score)
  4   4   4   4   4   4
  4   4   4   4   4   4
moi (score)
```

Les spectateurs voient la partie du point de vue du joueur 1.
Lorsqu'un joueur fait un coup, il doit précisé la case qu'il souhaite jouer. Cette case est de son coté du terrain et est comprise entre 0 et 5 (de la gauche vers la droite).

## Limites

| Élément | Maximum |
|---|---|
| Joueurs inscrits | 100 |
| Parties créées | 50 |
| Défis en attente par joueur | 20 |
| Spectateurs par partie | 100 |
| Coups enregistrés par partie | 100 |

## Messages d'erreur

| Message | Cause |
|---|---|
| `ERROR trop d'essais` | 5 tentatives de connexion échouées |
| `ERROR mot de passe incorrect` | Mauvais mot de passe |
| `ERROR deja connecte` | Ce compte est déjà connecté |
| `ERROR serveur plein` | Limite de 100 joueurs atteinte |
| `ERROR pseudo ou mot de passe invalide` | Espace dans le pseudo ou le mot de passe |
| `ERROR joueur inexistant` | Pseudo inconnu dans `defis` |
| `ERROR vous ne pouvez pas vous defier vous-meme` | `defis` sur son propre pseudo |
| `ERROR defi deja envoye` | Défi déjà en attente vers ce joueur |
| `ERROR ce joueur a trop de defis en attente` | Liste de défis pleine |
| `ERROR aucun defi de ce joueur` | `Yes`/`No` sans défi de ce joueur |
| `ERROR ce joueur est deconnecte` | Le joueur qui a défié n'est plus connecté |
| `ERROR trop de parties` | Limite de 50 parties atteinte |
| `ERROR partie inexistante` | Identifiant de partie invalide |
| `ERROR partie terminée` | La partie est finie |
| `ERROR vous n'êtes pas dans cette partie` | `join` sur une partie qui n'est pas la vôtre |
| `ERROR pas de partie en cours` | `play` sans partie active |
| `ERROR pas votre tour` | Ce n'est pas à vous de jouer |
| `ERROR coup invalide` | Case hors de 0-5 ou vide |
| `ERROR usage: msg <destinataire> <message>` | Commande `msg` incomplète |
| `ERROR destinataire inexistant` | Pseudo inconnu dans `msg` |