#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>

typedef struct {
    int duration;
    float probability;
} BurstHistogram;

int generateBurstDuration(BurstHistogram *hist, int size, int num_samples) {
    float rand_val = (float)rand() / RAND_MAX;
    float cumulative_prob = 0.0;
    for (int i = 0; i < size; i++) {
        cumulative_prob += hist[i].probability;
        if (rand_val < cumulative_prob) {
            return hist[i].duration;
        }
    }
    return hist[size - 1].duration; // Fallback in case of rounding errors
}

void test_generateBurstDuration() {
    BurstHistogram cpu_burst_hist[] = {
        {5, 0.15},
        {10, 0.30},
        {20, 0.50},
        {40, 0.05}
    };
    int cpu_histogram_size = sizeof(cpu_burst_hist) / sizeof(BurstHistogram);

    int num_samples = 10000;
    int counts[4] = {0};

    for (int i = 0; i < num_samples; i++) {
        int duration = generateBurstDuration(cpu_burst_hist, cpu_histogram_size, 1);
        switch (duration) {
            case 5: counts[0]++; break;
            case 10: counts[1]++; break;
            case 20: counts[2]++; break;
            case 40: counts[3]++; break;
            default: assert(0 && "Unexpected duration");
        }
    }

    printf("Duration 5: %d (Expected ~%d)\n", counts[0], (int)(num_samples * 0.15));
    printf("Duration 10: %d (Expected ~%d)\n", counts[1], (int)(num_samples * 0.30));
    printf("Duration 20: %d (Expected ~%d)\n", counts[2], (int)(num_samples * 0.50));
    printf("Duration 40: %d (Expected ~%d)\n", counts[3], (int)(num_samples * 0.05));

    assert(counts[0] > 1300 && counts[0] < 1700);
    assert(counts[1] > 2800 && counts[1] < 3200);
    assert(counts[2] > 4800 && counts[2] < 5200);
    assert(counts[3] > 300 && counts[3] < 700);
}

int main() {
    srand(time(NULL));
    test_generateBurstDuration();
    return 0;
}