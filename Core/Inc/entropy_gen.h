/*
 * entropy_gen.h
 *
 *  Created on: Jun 16, 2026
 *      Author: eccen
 */


#ifndef ENTROPY_GEN_H_
#define ENTROPY_GEN_H_

#include "main.h"   // Needed for HAL handle types (ADC_HandleTypeDef)
#include <stdint.h>

/* --- EXPOSED FUNCTION PROTOTYPES --- */

/**
 * @brief Gathers real-world electromagnetic and timing noise to seed srand().
 * @param hadc Pointer to the initialized floating ADC channel handle (e.g., &hadc1)
 */
void Entropy_Seed_Engine(ADC_HandleTypeDef *hadc);

/**
 * @brief Generates a cryptographically randomized high-entropy password.
 * @param output_buffer Pointer to the array where the generated string will live.
 * @param length Desired number of characters for the target password.
 */
void Entropy_Generate_Password(char* output_buffer, uint32_t length);

#endif /* ENTROPY_GEN_H_ */
