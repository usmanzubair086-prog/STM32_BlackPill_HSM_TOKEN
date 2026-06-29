/*
 * security_vault.h
 * * Target: YubiKey clone V2 (Multi-Slot AES-128-CBC)
 * Role: Cryptographic transformation layer & Entropy mapping
 */

#ifndef SECURITY_VAULT_H_
#define SECURITY_VAULT_H_

#include <stdint.h>
#include <stdbool.h>

/* ---- CONFIGURATION CONSTANTS ---- */
#define AES_BLOCK_SIZE          32   // 32 Bytes allows up to 31 character passwords (2 AES blocks)
#define AES_IV_SIZE             16   // 16 Bytes standard for AES-128 Initialization Vectors

/* --- EXPOSED VAULT API PROTOTYPES --- */

/**
 * @brief  Takes user credentials, encrypts them using a unique random IV via AES-CBC,
 * and commits the resulting cipher blocks directly to a flash slot.
 * @param  slot_id: Target database slot index (0 to MAX_SLOTS - 1).
 * @param  label: Human readable identifier string (e.g., "GitHub").
 * @param  plaintext_pass: The raw password input to protect.
 * @return true if encryption and flash write succeeded, false otherwise.
 */
bool Vault_Encrypt_And_Store(uint8_t slot_id, const char* label, const char* plaintext_pass);

/**
 * @brief  Pulls raw encrypted ciphertext from physical storage, uses the slot's stored IV
 * to run an AES decryption cycle, and outputs the human-readable string.
 * @param  slot_id: Target database slot index to extract from.
 * @param  out_plaintext: Destination char array buffer to write the decrypted string into.
 * @return true if decryption sequence completed successfully.
 */
bool Vault_Retrieve_And_Decrypt(uint8_t slot_id, char* out_plaintext);

#endif /* SECURITY_VAULT_H_ */
