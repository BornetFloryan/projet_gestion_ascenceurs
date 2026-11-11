CC = gcc
CPPFLAGS = -Iinclude
CFLAGS = -std=c11 -O2 -Wall -Wextra -Wpedantic -pthread
LDFLAGS = -pthread -lrt

SOURCES = src/main.c src/communication.c src/synchronisation.c \
          src/immeuble.c src/ordonnancement.c src/utilisateur.c \
          src/ascenseur.c src/gestionnaire.c src/alertes.c \
          src/scenario.c src/statistiques.c
BUILD_DIR = build
OBJECTS = $(SOURCES:src/%.c=$(BUILD_DIR)/%.o)
EXECUTABLE = gestion-ascenseurs

all: $(EXECUTABLE)


$(EXECUTABLE): $(OBJECTS)
	$(CC) $(OBJECTS) -o $@ $(LDFLAGS)


$(BUILD_DIR)/%.o: src/%.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

run: $(EXECUTABLE)
	./$(EXECUTABLE)

help: $(EXECUTABLE)
	./$(EXECUTABLE) -h

clean:
	rm -rf $(BUILD_DIR) $(EXECUTABLE)
	rm -f src/*.o

.PHONY: all clean run help
