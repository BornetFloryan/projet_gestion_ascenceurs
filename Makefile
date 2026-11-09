CC = gcc
CFLAGS = -Wall -Wextra -Iinclude -pthread -std=c11
SRC = src/main.c src/communication.c src/ascenseur.c src/utilisateur.c src/gestionnaire.c
OBJ = $(SRC:.c=.o)
EXEC = gestion-ascenseurs

all: $(EXEC)

$(EXEC): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $(OBJ)

src/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

run: $(EXEC)
	./$(EXEC)

clean:
	rm -f $(OBJ) $(EXEC)

.PHONY: all run clean
