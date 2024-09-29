#pragma once

#define MIN_BURST_PROCESS 10
#define MAX_BURST_PROCESS 100

// Definizione della struttura per rappresentare l'istogramma
typedef struct {
    int burst_time;  // Durata del burst (in millisecondi)
    float probability;   // Probabilità associata a questa durata
} BurstHistogram;

float calculateWeightedMean(const BurstHistogram *cpu_hist, int size);
int generateBurstDuration(BurstHistogram *hist, int size, int num_samples);
void uniformSample(int *sampled_indices, float *probabilities, int num_probabilities, int num_desired_samples);
void normalize(float *probabilities, int size);