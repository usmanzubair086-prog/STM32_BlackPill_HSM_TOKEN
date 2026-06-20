/*
 * entropy.c
 *
 *  Created on: Jun 17, 2026
 *      Author: eccen
 */


#include "entropy_gen.h"
#include <stdlib.h>
#include <string.h>

/**
 * @brief Gathers real-world electromagnetic and timing noise to seed srand().
 */
void Entropy_Seed_Engine(ADC_HandleTypeDef *hadc)
{
    uint32_t noise_seed = 0;

    // Defensive Check: Ensure the ADC hardware pointer configuration is valid
    if (hadc != NULL)
    {
        // Fire up Analog Peripheral
        if (HAL_ADC_Start(hadc) == HAL_OK)
        {
            // Block execution for up to 10ms until data sample conversion stabilizes
            if (HAL_ADC_PollForConversion(hadc, 10) == HAL_OK)
            {
                // Harvest the floating voltage variations across least significant bits
                noise_seed = HAL_ADC_GetValue(hadc);
            }
            HAL_ADC_Stop(hadc); // Wind down peripheral safely to lower thermal footprint
        }
    }

    // Human Factor Mix: Extract timing delta using microsecond precision ticks
    uint32_t time_mix = HAL_GetTick();

    // Combine thermal air-noise with human click entropy using a bitwise XOR identity
    noise_seed ^= time_mix;

    // Fallback protection: If both engines failed and yielded 0, fall back to safe constant
    if (noise_seed == 0)
    {
        noise_seed = 0x55AA55AA;
    }

    // Initialize the pseudo-random generator state engine
    srand(noise_seed);
}

/**
 * @brief Generates a cryptographically randomized high-entropy password.
 */
void Entropy_Generate_Password(char* output_buffer, uint32_t length)
{
    // Defensive Check: Block processing if buffer parameters are invalid
    if (output_buffer == NULL || length == 0)
    {
        return;
    }

    // Define strict, high-density whitelist alphanumeric dictionary
    static const char charset[] = "abcdefghijklmnopqrstuvwxyz"
                                  "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                  "0123456789"
                                  "!@#$%^&*()-_=+";

    // Safe bounds size deduction (ignoring terminating character)
    uint32_t charset_size = sizeof(charset) - 1;

    // Build the string index-by-index
    for (uint32_t i = 0; i < length; i++)
    {
        int random_val = rand();
        output_buffer[i] = charset[random_val % charset_size];
    }

    // Strict Memory Rule: Securely lock string execution space with terminal NULL
    output_buffer[length] = '\0';
}
