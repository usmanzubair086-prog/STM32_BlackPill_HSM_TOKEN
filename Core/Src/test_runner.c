/*
 * test_runner.c
 *
 *  Created on: Jun 17, 2026
 *      Author: eccen
 */

#include "security_vault.h"
#include "entropy_gen.h"
#include <string.h>

/* External Variables --------------------------------------------------------*/
extern ADC_HandleTypeDef hadc1;
/* External Functions --------------------------------------------------------*/
extern void USB_Type_String(char *str);
void Run_Security_Sanity_Tests(void)
{
    // Local buffers for testing
    char pass_sample_1[17] = {0};
    char pass_sample_2[17] = {0};
    char flash_read_back[17] = {0};


    // 1. Seed and generate the first password
    Entropy_Seed_Engine(&hadc1);
    Entropy_Generate_Password(pass_sample_1, 16);

    // 2. Wait a brief moment, re-seed, and generate a second password
    HAL_Delay(10);
    Entropy_Seed_Engine(&hadc1);
    Entropy_Generate_Password(pass_sample_2, 16);

    // CRITICAL ASSERTION: If the two passwords are identical, our entropy failed!
    if (strcmp(pass_sample_1, pass_sample_2) == 0)
    {
        // FAIL: Entropy is deterministic (bad seed)
        // Action: Blink an onboard LED rapidly or send an error string over USB
        USB_Type_String("TEST FAIL: Entropy Engine is deterministic!\r\n");
        return;
    }


    // Try committing the unique 'pass_sample_1' straight to Sector 5
    HAL_StatusTypeDef write_status = Vault_Write_Credential(pass_sample_1);

    if (write_status != HAL_OK)
    {
        // FAIL: Hardware Flash controller rejected the write or erase command
        USB_Type_String("TEST FAIL: Flash write sequence failed hardware execution.\r\n");
        return;
    }


    // Pull the data back out of raw memory into our third buffer
    bool read_status = Vault_Read_Credential(flash_read_back, sizeof(flash_read_back));

    if (!read_status)
    {
        // FAIL: Vault read failed initialization checks or buffer limits
        USB_Type_String("TEST FAIL: Vault read rejected (Initialization error).\r\n");
        return;
    }

    // CRITICAL ASSERTION: Does the data read from Flash exactly match what we generated?
    if (strcmp(pass_sample_1, flash_read_back) != 0)
    {
        // FAIL: Data corruption! What went in is not what came out.
        USB_Type_String("TEST FAIL: Data mismatch! Flash corruption or alignment error.\r\n");
        return;
    }

    // ==========================================
    // ALL TESTS PASSED SUCCESSFULLY
    // ==========================================
    USB_Type_String("ALL SYSTEMS GO: Entropy, Flash Write, and Flash Read Verified!\r\n");
}
