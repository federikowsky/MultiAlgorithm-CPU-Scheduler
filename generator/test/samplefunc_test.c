#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>
#include <string.h>
#include <dirent.h>

#include "../include/trace_generator.h"


// Funzione di test per verificare la corretta generazione dei burst
void test_generateBurstDuration(const char *filename) {
    
    BurstProfile *bf = BurstProfile_loadHistogram(filename);

    if (bf->cpu_size <= 0 || bf->io_size <= 0) {
        printf("Failed to load histograms\n");
        return;
    }

    int num_samples = 10000;
    int *cpu_counts = (int *)calloc(bf->cpu_size, sizeof(int));
    int *io_counts = (int *)calloc(bf->io_size, sizeof(int));

    for (int i = 0; i < num_samples; i++) {
        int duration;
        double random_val = (double)rand() / RAND_MAX;

        if (random_val < 0.5) {
            duration = getBurstDuration(bf->cpu_hist, bf->cpu_size, 1);
            for (int j = 0; j < bf->cpu_size; j++) {
                if (duration == bf->cpu_hist[j].burst_time) {
                    cpu_counts[j]++;
                    break;
                }
            }
        } else {
            duration = getBurstDuration(bf->io_hist, bf->io_size, 1);
            for (int j = 0; j < bf->io_size; j++) {
                if (duration == bf->io_hist[j].burst_time) {
                    io_counts[j]++;
                    break;
                }
            }
        }
    }

    num_samples = num_samples / 2; // mezzi perche alterniamo CPU e IO burst 
    printf("CPU Burst Histogram:\n");
    for (int i = 0; i < bf->cpu_size; i++) {
        printf("Duration %d: %d (Expected ~%d)\n", bf->cpu_hist[i].burst_time, cpu_counts[i], (int)(num_samples * bf->cpu_hist[i].probability));
    }

    printf("IO Burst Histogram:\n");
    for (int i = 0; i < bf->io_size; i++) {
        printf("Duration %d: %d (Expected ~%d)\n", bf->io_hist[i].burst_time, io_counts[i], (int)(num_samples * bf->io_hist[i].probability));
    }

    for (int i = 0; i < bf->cpu_size; i++) {
        int expected = (int)(num_samples * bf->cpu_hist[i].probability);
        int tolerance = expected * 0.15; // Tolleranza del 15%
        assert(cpu_counts[i] > expected - tolerance && cpu_counts[i] < expected + tolerance);
    }

    for (int i = 0; i < bf->io_size; i++) {
        int expected = (int)(num_samples * bf->io_hist[i].probability);
        int tolerance = expected * 0.15; // Tolleranza del 15%
        assert(io_counts[i] > expected - tolerance && io_counts[i] < expected + tolerance);
    }
}


int main(int argc, char **argv) {
    srand(time(NULL));

    if (argc < 2) {
        printf("Usage: %s <path_of_histogram_folder>\n", argv[0]);
        return 1;
    }

    // Path della cartella contenente gli istogrammi
    const char *histogram_folder = argv[1];

    // Apri la cartella
    DIR *dir = opendir(histogram_folder);
    if (!dir) {
        printf("Failed to open histogram folder\n");
        return 1;
    }

    // Leggi tutti i file nella cartella
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        // Ignora le voci speciali "." e ".."
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        // Costruisci il percorso completo del file
        char file_path[256];
        snprintf(file_path, sizeof(file_path), "%s/%s", histogram_folder, entry->d_name);

        // Test che verifica la corretta generazione dei burst in base all'istogramma
        printf("\nTesting histogram file: %s\n\n", file_path);
        test_generateBurstDuration(file_path);
    }

    // Chiudi la cartella
    closedir(dir);
    return 0;
}