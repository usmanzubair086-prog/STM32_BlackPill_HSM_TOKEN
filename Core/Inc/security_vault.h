/*
 * security_vault.h
 *
 *  Created on: Jun 16, 2026
 *      Author: eccen
 */

#ifndef SECURITY_VAULT_H_
#define SECURITY_VAULT_H_

#include "main.h" // Give access to main config files
#include <stdint.h>
#include <stdbool.h>

/* ----FLASH MEMORY MAPPING----- */
// Base address of Sector 5 on STM32F401CC (Capacity: 128KB in Sector 5)
#define VAULT_FLASH_ADDRESS		0x8020000
#define VAULT_FLASH_SECTOR		FLASH_SECTOR_5

/* ---- Configuration Constants ----  */
#define MAX_PASSWORD_LEN		32
#define VAULT_MAGIC_NUMBER		0x53454355 // Hex Signature for "SECU"

/* ---- CRYPTOGRAPHIC DATA STRUCTURE ---- */
/**
	* @brief Stripped down, word-aligned data structure for storage.
	* Multiple of 4 bytes to match 32-bit flash programming!
	*/

typedef struct {
    uint32_t magic_signature;               // 4 Bytes: Verifies if vault is initialized
    uint32_t password_length;               // 4 Bytes: Stores the active length of the string
    uint8_t  encrypted_payload[MAX_PASSWORD_LEN]; // 32 Bytes: Ciphertext container
} Vault_Storage_TypeDef;

/* --- EXPOSED FUNCTION PROTOTYPES --- */

/**
 * @brief Checks Sector 5 to see if the magic signature exists.
 * @return true if valid credentials exist, false if empty/erased (0xFFFFFFFF)
 */
bool Vault_Is_Initialized(void);

/**
 * @brief Encrypts a plaintext string and commits it to Sector 5 Flash.
 * @param plaintext_str Pointer to the source string to be secured.
 * @return HAL_StatusTypeDef returns HAL_OK on successful write sequence.
 */
HAL_StatusTypeDef Vault_Write_Credential(const char* plaintext_str);

/**
 * @brief Reads the raw flash data, decrypts it, and extracts the plain text.
 * @param output_buffer Destination char array to drop the decrypted string into.
 * @param max_buffer_size Prevents memory overflow inside the destination buffer.
 * @return true if decryption and read succeeded, false otherwise.
 */
bool Vault_Read_Credential(char* output_buffer, uint32_t max_buffer_size);

#endif /* SECURITY_VAULT_H_ */
