#include "security_vault.h"
#include "pass_storage.h"
#include "entropy_gen.h" // Assumes your hardware ADC noise functions are here
#include "aes.h"        // Tiny-AES-c library
#include <string.h>

// Master Key for AES-128. In production, this should be derived from a user pin code.
static const uint8_t MASTER_KEY[16] = {
    0x1A, 0x2B, 0x3C, 0x4D, 0x5E, 0x6F, 0x7A, 0x8B,
    0x9C, 0x0D, 0x1E, 0x2F, 0x3A, 0x4B, 0x5C, 0x6D
};

bool Vault_Encrypt_And_Store(uint8_t slot_id, const char* label, const char* plaintext_pass)
{
    struct AES_ctx ctx;
    uint8_t random_iv[AES_IV_SIZE] = {0};
    uint8_t crypto_buffer[AES_BLOCK_SIZE] = {0};

    if (plaintext_pass == NULL || label == NULL) return false;

    // 1. Prepare raw plaintext into our 32-byte chunk (leaving trailing spaces zero-padded)
    strncpy((char*)crypto_buffer, plaintext_pass, AES_BLOCK_SIZE - 1);

    // 2. Pull a completely unique, unpredictable 16-byte vector from your ADC chaos generator
    // Even if the password is the same, this guarantees the ciphertext changes completely!
    Entropy_Generate_Bytes(random_iv, AES_IV_SIZE);

    // 3. Mount the Tiny-AES context configuration using your key and the fresh IV
    AES_init_ctx_iv(&ctx, MASTER_KEY, random_iv);

    // 4. Transform the data into pure scrambled ciphertext blocks in place
    AES_CBC_encrypt_buffer(&ctx, crypto_buffer, AES_BLOCK_SIZE);

    // 5. Hand the secure packages down to the warehouse module for silicon writing
    // pass_storage should accept matching uint8_t pointers for the IV and Ciphertext
    return PassStorage_WriteSlot(slot_id, label, random_iv, crypto_buffer);
}

bool Vault_Retrieve_And_Decrypt(uint8_t slot_id, char* out_plaintext)
{
    struct AES_ctx ctx;
    char retrieved_label[16] = {0};
    uint8_t stored_iv[AES_IV_SIZE] = {0};
    uint8_t cipher_buffer[AES_BLOCK_SIZE] = {0};

    if (out_plaintext == NULL) return false;

    // 1. Fetch raw data assets back from physical flash memory cache
    if (!PassStorage_ReadSlot(slot_id, retrieved_label, stored_iv, cipher_buffer))
    {
        return false; // Storage read failure or slot uninitialized
    }

    // 2. Build the exact matching execution context using the original IV saved with this slot
    AES_init_ctx_iv(&ctx, MASTER_KEY, stored_iv);

    // 3. Run the inverse mathematical matrix to reverse the scrambling in place
    AES_CBC_decrypt_buffer(&ctx, cipher_buffer, AES_BLOCK_SIZE);

    // 4. Safely offload the recovered plain text back into your main runtime ecosystem
    memcpy(out_plaintext, cipher_buffer, AES_BLOCK_SIZE);
    out_plaintext[AES_BLOCK_SIZE - 1] = '\0'; // Force string safety constraint

    return true;
}
