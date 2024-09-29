#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "../include/fake_os.h"

/**
 * @brief 
 * 
 * @param cpu_hist The array of CPU burst histograms.
 * @param size The size of the array.
 * @return The weighted mean of CPU burst times.
 */
float calculateWeightedMean(const BurstHistogram *cpu_hist, int size)
{
	static float weighted_sum = 0.0;
	float total_probability = 0.0;

	if (weighted_sum)
		return weighted_sum;

	for (int i = 0; i < size; i++) {
		weighted_sum += cpu_hist[i].burst_time * cpu_hist[i].probability;
		total_probability += cpu_hist[i].probability;
	}

	float weighted_mean = weighted_sum / total_probability;
	return weighted_mean;
}
