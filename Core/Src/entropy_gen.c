#include "entropy_gen.h"

/* Private variables ---------------------------------------------------------*/
// Persistent reference pointer linking to the configured hardware ADC instance
static ADC_HandleTypeDef *local_hadc = NULL;

/* Exported functions --------------------------------------------------------*/

/**
  * @brief  Binds the local tracking pointer to the active system ADC instance.
  * @param  hadc: Pointer to the ADC handle (e.g., &hadc1 seeded from main)
  * @retval None
  */
void Entropy_Seed_Engine(ADC_HandleTypeDef *hadc)
{
    local_hadc = hadc;
}

/**
  * @brief  Gathers raw electrical noise bit-by-bit to build unconstrained binary byte blocks.
  * Essential for generating high-entropy cryptographic IVs and secret keys.
  * @param  out_bytes: Pointer to target destination array container
  * @param  len: Total number of bytes requested (e.g., 16 for an AES block)
  * @retval None
  */
void Entropy_Generate_Bytes(uint8_t *out_bytes, uint32_t len)
{
    if (local_hadc == NULL || out_bytes == NULL) return;

    for (uint32_t i = 0; i < len; i++)
    {
        uint8_t accumulated_byte = 0;

        // Sequence through 8 conversion steps to extract a complete 8-bit byte
        for (int bit = 0; bit < 8; bit++)
        {
            HAL_ADC_Start(local_hadc);

            if (HAL_ADC_PollForConversion(local_hadc, 10) == HAL_OK)
            {
                // Isolate the highly volatile Least Significant Bit (LSB) from the sample
                uint32_t raw_analog_value = HAL_ADC_GetValue(local_hadc);
                accumulated_byte |= ((raw_analog_value & 0x01) << bit);
            }

            HAL_ADC_Stop(local_hadc);

            // Short delay to let the analog internal sampling capacitor settle
            HAL_Delay(1);
        }

        out_bytes[i] = accumulated_byte;
    }
}

/**
  * @brief  Generates a high-entropy string restricted strictly to printable ASCII characters.
  * Designed for user-facing credentials typed out via USB HID emulation.
  * @param  out_pass: Pointer to the target string buffer
  * @param  len: Desired character length of the generated password
  * @retval None
  */
void Entropy_Generate_Password(char *out_pass, uint32_t len)
{
    if (local_hadc == NULL || out_pass == NULL || len == 0) return;

    // Fixed pool of secure, web-safe alphanumeric and symbol targets
    static const char character_set[] =
        "abcdefghijklmnopqrstuvwxyz"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "0123456789"
        "!@#$%^&*()-_=+";

    uint32_t pool_size = sizeof(character_set) - 1;

    for (uint32_t i = 0; i < len; i++)
    {
        uint32_t gathered_entropy_bits = 0;

        // Sample 12 consecutive times to extract a 12-bit integer for mapping variance
        for (int bit = 0; bit < 12; bit++)
        {
            HAL_ADC_Start(local_hadc);

            if (HAL_ADC_PollForConversion(local_hadc, 10) == HAL_OK)
            {
                gathered_entropy_bits |= ((HAL_ADC_GetValue(local_hadc) & 0x01) << bit);
            }

            HAL_ADC_Stop(local_hadc);
            HAL_Delay(1);
        }

        // Modulo distribution across the safe character matrix
        out_pass[i] = character_set[gathered_entropy_bits % pool_size];
    }

    // Explicitly enforce terminal safe string null encapsulation
    out_pass[len] = '\0';
}
