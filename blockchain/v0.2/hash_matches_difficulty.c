#include "blockchain.h"

/**
 * hash_matches_difficulty - checks if a hash matches a difficulty
 * @hash: hash to check
 * @difficulty: number of leading zero bits required
 * Return: 1 if the hash matches the difficulty, 0 otherwise
 */
int hash_matches_difficulty(uint8_t const hash[SHA256_DIGEST_LENGTH],
uint32_t difficulty)
{
uint32_t i, full = difficulty / 8, rem = difficulty % 8;

if (!hash || difficulty > SHA256_DIGEST_LENGTH * 8)
return (0);
for (i = 0; i < full; i++)
if (hash[i])
return (0);
if (rem && (hash[full] >> (8 - rem)))
return (0);
return (1);
}
