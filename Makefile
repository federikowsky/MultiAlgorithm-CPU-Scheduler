# Compilatore e opzioni
CC := gcc
CFLAGS := --std=gnu99 -Wall -D_LIST_DEBUG_ -O2

# Definizione dei percorsi
SRC_DIR := scheduler/src
BUILD_DIR := scheduler/build
TEST_DIR := scheduler/test
TEST_BUILD_DIR := $(TEST_DIR)/build

# Nome dell'eseguibile
TARGET := disastros

# Trova tutti i file sorgente nella cartella src/
SOURCES := $(wildcard $(SRC_DIR)/*.c)

# Genera i nomi dei file oggetto nella cartella build/ per il codice sorgente principale
OBJECTS := $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(SOURCES))

# Trova tutti i file sorgente nella cartella test/
TEST_SOURCES := $(wildcard $(TEST_DIR)/*.c)

# Genera i nomi dei file oggetto nella cartella test/build/ per i file di test
TEST_OBJECTS := $(patsubst $(TEST_DIR)/%.c,$(TEST_BUILD_DIR)/%.o,$(TEST_SOURCES))

# Genera i nomi degli eseguibili nella cartella test/
TEST_TARGETS := $(patsubst $(TEST_DIR)/%.c,$(TEST_DIR)/%,$(TEST_SOURCES))

# Escludi main.o dai file oggetto per i test
TEST_BUILD_OBJECTS := $(filter-out $(BUILD_DIR)/main.o, $(OBJECTS))

# Target predefinito
all: $(TARGET)

# Compila l'eseguibile principale
$(TARGET): $(OBJECTS)
	$(CC) $^ -o $@

# Compila i file oggetto dai sorgenti nella cartella src/
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Compilazione dei test: compila gli eseguibili nella directory test/
test: $(TEST_TARGETS)

# Crea gli eseguibili di test nella cartella test/ usando i file oggetto in test/build/ e quelli sorgente di src/ (escluso main.o)
$(TEST_DIR)/%: $(TEST_BUILD_DIR)/%.o $(TEST_BUILD_OBJECTS)
	@mkdir -p $(TEST_BUILD_DIR)
	$(CC) $(CFLAGS) $^ -o $@

# Compila i file oggetto per i test e li salva in test/build/
$(TEST_BUILD_DIR)/%.o: $(TEST_DIR)/%.c
	@mkdir -p $(TEST_BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Pulizia dei file generati
clean:
	rm -rf $(BUILD_DIR) $(TARGET)

test_clean:
	rm -rf $(TEST_BUILD_DIR) $(TEST_TARGETS)

re: clean all

# Fai in modo che il makefile non cerchi file con gli stessi nomi dei target
.PHONY: all clean test re
