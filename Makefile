# Definizione dei percorsi
SRC_DIR := src
BUILD_DIR := build
TEST_DIR := testing

# Nome dell'eseguibile
TARGET := disastros

# Compilatore e opzioni
CC := gcc
CFLAGS := --std=gnu99 -Wall -D_LIST_DEBUG_ -g 
AR=ar

# Trova tutti i file sorgente nella cartella src/ e testing/
SOURCES := $(wildcard $(SRC_DIR)/*.c) $(wildcard $(TEST_DIR)/*.c)

# Genera i nomi dei file oggetto nella cartella build/
OBJECTS := $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(filter %,$(SOURCES)))

# Target predefinito
all: $(TARGET)

# Compila il target
$(TARGET): $(OBJECTS)
	$(CC) $^ -o $@

# Compila i file oggetto
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Compila i file oggetto per i file sorgente nella cartella testing/
$(BUILD_DIR)/%.o: $(TEST_DIR)/%.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Pulizia dei file generati
clean:
	rm -rf $(BUILD_DIR) $(TARGET)

re: clean all

# Fai in modo che il makefile non cerchi file con gli stessi nomi dei target
.PHONY: all clean

