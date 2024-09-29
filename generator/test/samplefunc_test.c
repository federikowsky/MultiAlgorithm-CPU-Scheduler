#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>
#include <string.h>
#include <dirent.h>

#include "../include/trace_generator.h"

void histogram(const char *filename, BurstDist *cpu_hist, int *cpu_size, BurstDist *io_hist, int *io_size)
{
    char line[256];
    int is_cpu = 0, is_io = 0;
    int cpu_count = 0, io_count = 0;
    
    FILE *file = fopen(filename, "r");
	assert(file && "file not found");

    // Temporary arrays to store burst histograms
    static BurstDist temp_cpu[100];
    static BurstDist temp_io[100];

    while (fgets(line, sizeof(line), file))
	{
		if (line[0] == '#' || line[0] == '\n')
            continue;

        if (strncmp(line, "CPU_BURST", 9) == 0)
		{
            is_cpu = 1;
            is_io = 0;
            continue;
        }
        if (strncmp(line, "IO_BURST", 8) == 0)
		{
            is_io = 1;
            is_cpu = 0;
            continue;
        }

        // Find the position of comment if present
        char *comment_pos = strchr(line, '#');
        if (comment_pos) {
            *comment_pos = '\0'; // Truncate the line at the comment
        }

        int burst_time;
        double probability;
        // Parse burst time and probability
        if (sscanf(line, "%d %lf", &burst_time, &probability) == 2 || sscanf(line, "%d,%lf", &burst_time, &probability) == 2)
		{
            if (is_cpu)
			{
                temp_cpu[cpu_count].burst_time = burst_time;
                temp_cpu[cpu_count].probability = probability;
                cpu_count++;
            } 
			else if (is_io)
			{
                temp_io[io_count].burst_time = burst_time;
                temp_io[io_count].probability = probability;
                io_count++;
            }
        }
    }

    fclose(file);

    // Copy data from temp arrays
    memcpy(cpu_hist, temp_cpu, cpu_count * sizeof(BurstDist));
    memcpy(io_hist, temp_io, io_count * sizeof(BurstDist));

	*cpu_size = cpu_count;
	*io_size = io_count;
}


// Funzione di test per verificare la corretta generazione dei burst
void test_generateBurstDuration(const char *filename) {
    BurstDist cpu_burst_hist[100], io_burst_hist[100];
    int cpu_histogram_size, io_histogram_size;

    histogram(filename, cpu_burst_hist, &cpu_histogram_size, io_burst_hist, &io_histogram_size);

    if (cpu_histogram_size <= 0 || io_histogram_size <= 0) {
        printf("Failed to load histograms\n");
        return;
    }

    int num_samples = 10000;
    int *cpu_counts = (int *)calloc(cpu_histogram_size, sizeof(int));
    int *io_counts = (int *)calloc(io_histogram_size, sizeof(int));

    for (int i = 0; i < num_samples; i++) {
        int duration;
        double random_val = (double)rand() / RAND_MAX;

        if (random_val < 0.5) {
            duration = generateBurstDuration(cpu_burst_hist, cpu_histogram_size, 1);
            for (int j = 0; j < cpu_histogram_size; j++) {
                if (duration == cpu_burst_hist[j].burst_time) {
                    cpu_counts[j]++;
                    break;
                }
            }
        } else {
            duration = generateBurstDuration(io_burst_hist, io_histogram_size, 1);
            for (int j = 0; j < io_histogram_size; j++) {
                if (duration == io_burst_hist[j].burst_time) {
                    io_counts[j]++;
                    break;
                }
            }
        }
    }

    num_samples = num_samples / 2; // mezzi perche alterniamo CPU e IO burst 
    printf("CPU Burst Histogram:\n");
    for (int i = 0; i < cpu_histogram_size; i++) {
        printf("Duration %d: %d (Expected ~%d)\n", cpu_burst_hist[i].burst_time, cpu_counts[i], (int)(num_samples * cpu_burst_hist[i].probability));
    }

    printf("IO Burst Histogram:\n");
    for (int i = 0; i < io_histogram_size; i++) {
        printf("Duration %d: %d (Expected ~%d)\n", io_burst_hist[i].burst_time, io_counts[i], (int)(num_samples * io_burst_hist[i].probability));
    }

    for (int i = 0; i < cpu_histogram_size; i++) {
        int prob = (int)(num_samples * cpu_burst_hist[i].probability);
        int x = cpu_counts[i];

        int expected = (int)(num_samples * cpu_burst_hist[i].probability);
        int tolerance = expected * 0.15; // Tolleranza del 15%
        assert(cpu_counts[i] > expected - tolerance && cpu_counts[i] < expected + tolerance);
    }

    for (int i = 0; i < io_histogram_size; i++) {
        int prob = (int)(num_samples * cpu_burst_hist[i].probability);

        int expected = (int)(num_samples * io_burst_hist[i].probability);
        int tolerance = expected * 0.15; // Tolleranza del 15%
        assert(io_counts[i] > expected - tolerance && io_counts[i] < expected + tolerance);
    }
}


int main(int argc, char **argv) {
    srand(time(NULL));

    if (argc < 2) {
        printf("Usage: %s <path_of_histogram_file>\n", argv[0]);
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