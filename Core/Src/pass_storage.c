#include "pass_storage.h"
#include <string.h>

// Flash Mapping Configuration (Using Sector 5 on standard STM32F401/411 lines)
#define FLASH_STORAGE_ADDR   ((uint32_t)0x08020000)
#define FLASH_STORAGE_SECTOR FLASH_SECTOR_5

// Local RAM mirrored cache of the password database
static PasswordDatabase ram_db;

/**
  * @brief  Hydrates the local RAM array from physical flash cells on system boot.
  */
bool PassStorage_Init(void)
{
    // Deep memory clone directly from physical silicon space into RAM cache
    memcpy(&ram_db, (void*)FLASH_STORAGE_ADDR, sizeof(PasswordDatabase));

    // Check if the flash region contains our structural identity mark
    if (ram_db.db_initialized != STORAGE_MAGIC_NUM)
    {
        // Flash is dirty or entirely blank (0xFF). Securely purge RAM mirror.
        memset(&ram_db, 0, sizeof(PasswordDatabase));
        ram_db.db_initialized = STORAGE_MAGIC_NUM;

        // Zero out arrays completely
        uint8_t dummy_iv[AES_IV_SIZE] = {0};
        uint8_t dummy_cipher[AES_BLOCK_SIZE] = {0};

        // Commit clean master database formatting template down to the sector
        return PassStorage_WriteSlot(0, "SystemInit", dummy_iv, dummy_cipher);
    }

    return true;
}

/**
  * @brief  Internal helper to safely burn raw RAM state into underlying silicon cells.
  */
static bool Flash_CommitDatabase(void)
{
    bool write_success = true;

    // 1. Critical Section Lockout: Freeze all background system interrupts
    __disable_irq();
    HAL_FLASH_Unlock();

    // 2. Structural Configuration for the Erase Block Routine
    FLASH_EraseInitTypeDef erase_config;
    erase_config.TypeErase    = FLASH_TYPEERASE_SECTORS;
    erase_config.VoltageRange = FLASH_VOLTAGE_RANGE_3; // Optimal performance for 3.3V rails
    erase_config.Sector       = FLASH_STORAGE_SECTOR;
    erase_config.NbSectors    = 1;

    uint32_t sector_error = 0;
    if (HAL_FLASHEx_Erase(&erase_config, &sector_error) != HAL_OK)
    {
        write_success = false;
        goto flash_fault_cleanup;
    }

    // 3. Process data streams in exact 32-bit word alignment chunks
    uint32_t *data_stream = (uint32_t *)&ram_db;
    uint32_t current_address = FLASH_STORAGE_ADDR;
    uint32_t execution_words = sizeof(PasswordDatabase) / 4;

    // Padding catch to guarantee loop accuracy if the structure sizes ever vary
    if (sizeof(PasswordDatabase) % 4 != 0) execution_words++;

    for (uint32_t i = 0; i < execution_words; i++)
    {
        if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, current_address, data_stream[i]) != HAL_OK)
        {
            write_success = false;
            goto flash_fault_cleanup;
        }
        current_address += 4;
    }

flash_fault_cleanup:
    HAL_FLASH_Lock();
    __enable_irq(); // Relinquish critical control lock and restore execution timelines

    // 4. Hardware Verification Verification: Direct hardware comparison
    if (write_success)
    {
        if (memcmp(&ram_db, (void*)FLASH_STORAGE_ADDR, sizeof(PasswordDatabase)) != 0)
        {
            write_success = false; // The storage cell was rejected due to damage or write faults
        }
    }

    return write_success;
}

/**
  * @brief  Safely extracts binary data chunks from an isolated database slot.
  */
bool PassStorage_ReadSlot(uint8_t slot_id, char *out_label, uint8_t *out_iv, uint8_t *out_ciphertext)
{
    // Defensive pointer check and array bounds screening
    if (slot_id >= MAX_SLOTS || out_label == NULL || out_iv == NULL || out_ciphertext == NULL)
    {
        return false;
    }

    // Check if the requested index houses live cryptographic properties
    if (ram_db.slots[slot_id].is_valid != STORAGE_MAGIC_NUM)
    {
        return false;
    }

    // String data extraction with hard constraints
    strncpy(out_label, ram_db.slots[slot_id].label, MAX_LABEL_LEN);
    out_label[MAX_LABEL_LEN - 1] = '\0'; // Guarantee physical cancellation barrier

    // Binary data copy bypasses string parsers entirely, avoiding null truncation traps
    memcpy(out_iv, ram_db.slots[slot_id].iv, AES_IV_SIZE);
    memcpy(out_ciphertext, ram_db.slots[slot_id].ciphertext, AES_BLOCK_SIZE);

    return true;
}

/**
  * @brief  Pushes raw binary cryptography arrays into specified flash indices.
  */
bool PassStorage_WriteSlot(uint8_t slot_id, const char *label, const uint8_t *iv, const uint8_t *ciphertext)
{
    if (slot_id >= MAX_SLOTS || label == NULL || iv == NULL || ciphertext == NULL)
    {
        return false;
    }

    // Secure clear phase: Wipe the targeted slot completely in RAM before overwriting
    memset(&ram_db.slots[slot_id], 0, sizeof(PasswordSlot));

    // Mount markers and string tags safely
    ram_db.slots[slot_id].is_valid = STORAGE_MAGIC_NUM;
    strncpy(ram_db.slots[slot_id].label, label, MAX_LABEL_LEN - 1);
    ram_db.slots[slot_id].label[MAX_LABEL_LEN - 1] = '\0';

    // Direct block replication prevents early termination bugs from random zero blocks
    memcpy(ram_db.slots[slot_id].iv, iv, AES_IV_SIZE);
    memcpy(ram_db.slots[slot_id].ciphertext, ciphertext, AES_BLOCK_SIZE);

    // Fire structural burn loop
    return Flash_CommitDatabase();
}

/**
  * @brief  Securely purges all information trace vectors in a target slot index.
  */
bool PassStorage_ClearSlot(uint8_t slot_id)
{
    if (slot_id >= MAX_SLOTS) return false;

    // Secure-zero overwrite cycle inside the cache matrix
    memset(&ram_db.slots[slot_id], 0, sizeof(PasswordSlot));

    return Flash_CommitDatabase();
}
