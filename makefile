CC      = gcc
CFLAGS  = -Wall -Wextra -g
LDFLAGS = -pthread

SERVER_SRC = awaleV2_serveur.c
CLIENT_SRC = awale_client.c

all: serveur client

serveur: $(SERVER_SRC)
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

client: $(CLIENT_SRC)
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -f serveur client

.PHONY: all clean