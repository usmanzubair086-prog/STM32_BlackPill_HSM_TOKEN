/*
 * security_vault.c
 *
 *  Created on: Jun 17, 2026
 *      Author: eccen
 */

#include "security_vault.h"
#include <string.h>

/**
 * @brief Checks Sector 5 to see if the magic signature exists.
 */
bool Vault_Is_Initialized(void)
{
    // Direct pointer cast to volatile hardware address space
    volatile const Vault_Storage_TypeDef *flash_vault = (volatile const Vault_Storage_TypeDef *)VAULT_FLASH_ADDRESS;

    // Check if the flash memory matches our precise magic signature
    return (flash_vault->magic_signature == VAULT_MAGIC_NUMBER);
}

/**
 * @brief Encrypts a plaintext string and commits it to Sector 5 Flash.
 */
HAL_StatusTypeDef Vault_Write_Credential(const char* plaintext_str)
{
    // Defensive Check: Ensure the input pointer is valid
    if (plaintext_str == NULL)
    {
        return HAL_ERROR;
    }

    // Defensive Check: Bound check string length to fit our physical container safely
    size_t str_len = strlen(plaintext_str);
    if (str_len >= MAX_PASSWORD_LEN)
    {
        return HAL_ERROR; // String is too large for the allocated flash allocation
    }

    // Stack Isolation: Clear the structure entirely to prevent stack memory leakage to Flash
    Vault_Storage_TypeDef local_vault;
    memset(&local_vault, 0, sizeof(Vault_Storage_TypeDef));

    // Populate data payload
    local_vault.magic_signature = VAULT_MAGIC_NUMBER;
    local_vault.password_length = (uint32_t)str_len;

    // Safe bounded memory copy
    memcpy(local_vault.encrypted_payload, plaintext_str, str_len);
    // (Placeholder for AES Encryption step: encrypt local_vault.encrypted_payload here)

    // Hardware Step 1: Unlock Flash Interface Configuration Registers
    HAL_StatusTypeDef status = HAL_FLASH_Unlock();
    if (status != HAL_OK)
    {
        return status;
    }

    // Hardware Step 2: Configure Sector Erase Parameters
    FLASH_EraseInitTypeDef erase_config;
    uint32_t sector_erase_error = 0;

    erase_config.TypeErase    = FLASH_TYPEERASE_SECTORS;
    erase_config.Sector       = VAULT_FLASH_SECTOR;
    erase_config.NbSectors    = 1;
    erase_config.VoltageRange = FLASH_VOLTAGE_RANGE_3; // Optimized for 2.7V - 3.6V standard VCC

    // Clear Flash flags before operating
    __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_EOP | FLASH_FLAG_OPERR | FLASH_FLAG_WRPERR |
                           FLASH_FLAG_PGAERR | FLASH_FLAG_PGPERR | FLASH_FLAG_PGSERR);

    // Execute Sector Wipe Pump
    status = HAL_FLASHEx_Erase(&erase_config, &sector_erase_error);
    if (status != HAL_OK)
    {
        HAL_FLASH_Lock(); // Secure hardware before bailing
        return status;
    }

    // Hardware Step 3: Aligned Word-by-Word Write Loop
    // Calculate iterations based on exact 32-bit hardware bus width (4 bytes)
    uint32_t total_words = sizeof(Vault_Storage_TypeDef) / 4;
    uint32_t *source_ptr = (uint32_t *)&local_vault;
    uint32_t target_address = VAULT_FLASH_ADDRESS;

    for (uint32_t i = 0; i < total_words; i++)
    {
        status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, target_address, source_ptr[i]);
        if (status != HAL_OK)
        {
            break; // Catch hardware failure mid-stream
        }
        target_address += 4; // Shift memory offset forward by exactly 1 word
    }

    // Hardware Step 4: Re-lock configuration registers to protect silicon integrity
    HAL_FLASH_Lock();

    return status;
}

/**
 * @brief Reads the raw flash data, decrypts it, and extracts the plain text.
 */
bool Vault_Read_Credential(char* output_buffer, uint32_t max_buffer_size)
{
    // Defensive Check: Validate input parameters to prevent null reference or invalid space
    if (output_buffer == NULL || max_buffer_size == 0)
    {
        return false;
    }

    // Check if configuration exists
    if (!Vault_Is_Initialized())
    {
        return false;
    }

    // Map struct memory directly to the flash memory region
    volatile const Vault_Storage_TypeDef *flash_vault = (volatile const Vault_Storage_TypeDef *)VAULT_FLASH_ADDRESS;

    // Defensive Check: Protect against memory corruptions reading an out-of-bounds length value
    uint32_t stored_len = flash_vault->password_length;
    if (stored_len >= MAX_PASSWORD_LEN || (stored_len + 1) > max_buffer_size)
    {
        return false; // Target buffer is too small or flash data is corrupted
    }

    // Safe read execution
    memcpy(output_buffer, (const void *)flash_vault->encrypted_payload, stored_len);
    // (Placeholder for AES Decryption step: decrypt output_buffer here)

    // Strict Memory Rule: Explicitly cap off the C string manually
    output_buffer[stored_len] = '\0';

    return true;
}
