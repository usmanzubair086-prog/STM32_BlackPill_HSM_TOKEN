#ifndef PASS_STORAGE_H
#define PASS_STORAGE_H

#include "stm32f4xx_hal.h"
#include <stdbool.h>
#include <stdint.h>

#define MAX_SLOTS          10    // Total credential slots
#define MAX_LABEL_LEN      16    // Max characters for account tags (e.g., "GitHub")
#define AES_BLOCK_SIZE     32    // Fixed size of encrypted data (2 AES blocks)
#define AES_IV_SIZE        16    // Fixed size of Initialization Vector
#define STORAGE_MAGIC_NUM  0xA5  // Validation token for storage initialization

// Cryptographically hardened structure for a binary database slot
typedef struct {
    uint8_t  is_valid;                     // Status flag (STORAGE_MAGIC_NUM if populated)
    char     label[MAX_LABEL_LEN];         // Human-readable plaintext label
    uint8_t  iv[AES_IV_SIZE];              // Pure binary IV array (No null terminators!)
    uint8_t  ciphertext[AES_BLOCK_SIZE];   // Pure binary encrypted payload array
} PasswordSlot;

// Complete word-aligned database layout mapped directly into flash memory
typedef struct {
    uint8_t      db_initialized;           // Master configuration flag
    PasswordSlot slots[MAX_SLOTS];         // Array of structurally secure data slots
} PasswordDatabase;

/* --- EXPOSED FLASH STORAGE API --- */

bool PassStorage_Init(void);

bool PassStorage_ReadSlot(uint8_t slot_id,
                          char *out_label,
                          uint8_t *out_iv,
                          uint8_t *out_ciphertext);

bool PassStorage_WriteSlot(uint8_t slot_id,
                           const char *label,
                           const uint8_t *iv,
                           const uint8_t *ciphertext);

bool PassStorage_ClearSlot(uint8_t slot_id);

#endif /* PASS_STORAGE_H */
