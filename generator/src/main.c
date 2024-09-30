#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <dirent.h>
#include <time.h>
#include <sys/stat.h>

#include "../include/trace_generator.h"

char usage_buffer[1024] = "Usage: %s <num_process> <num_burst_per_process> <histogram_folder> <dest_folder> \n\
\n\
<num_process>: Number of processes to create and simulate. \n\
<burst_per_process>: Number of burst per process. \n\
<histogram_folder>: The path to the folder containing the histogram data. \n\
<dest_folder>: The path to the folder where the traces will be saved. NOTE! all the files in the folder will be deleted. if the folder does not exist it will be created. \n\
\n";


int main(int argc, char **argv)
{
    TraceGen        tg;
    DIR             *dir;
    struct dirent   *ent;


    if (argc != 5)
    {
        printf(usage_buffer, argv[0]);
        return 1;
    }

    srand(time(NULL));

    int num_processes = atoi(argv[1]);
    int burst_per_process = atoi(argv[2]);
    const char *histogram_folder = argv[3];
    const char *dest_folder = argv[4];

    if (num_processes < 1 || burst_per_process < 1)
    {
        printf(usage_buffer, argv[0]);
        return 1;
    }

    // Create the destination folder if it does not exist
    struct stat st = {0};
    if (stat(dest_folder, &st) == -1)
    {
        mkdir(dest_folder, 0700);
    } 
    // Delete all the files in the folder
    else
    {
        char cmd[256];
        sprintf(cmd, "rm -f %s/*", dest_folder);
        system(cmd);
    }

    TraceGen_init(&tg, num_processes, burst_per_process, histogram_folder);

    // Open the histogram folder and load the histograms into the burst_profiles list
    if ((dir = opendir(histogram_folder)) == NULL)
        assert(0 && "Could not open histogram folder");

    while ((ent = readdir(dir)) != NULL)
    {
        if (ent->d_type == DT_REG)
        {
            char filename[256];
            sprintf(filename, "%s/%s", histogram_folder, ent->d_name);
            BurstProfile *bf = BurstProfile_loadHistogram(filename);
            List_pushBack(&tg.burst_profiles, &bf->list);
        }
    }
    closedir(dir);

    // Create the processes events based on the loaded histograms 
    // and save them in a file
    for (int i = 0; i < num_processes; i++)
    {
        int profile_index = rand() % tg.burst_profiles.size;
        BurstProfile *bf = (BurstProfile *)List_getAt(&tg.burst_profiles, profile_index);
        createEventProc(bf, i, burst_per_process, dest_folder);
    }

    // Free the memory allocated for the histograms
    ListItem *aux;
    while ((aux = List_popFront(&tg.burst_profiles)) != NULL)
    {
        BurstProfile *bf = (BurstProfile *)aux;
        free(bf->cpu_hist);
        free(bf->io_hist);
        free(bf->source_type);
        free(bf);
    }

    return 0;
}