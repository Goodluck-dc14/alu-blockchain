#include "blockchain.h"

/**
 * block_mine - mines a block so its hash matches its difficulty
 * @block: pointer to the block to mine
 */
void block_mine(block_t *block)
{
uint64_t nonce = 0;

if (!block)
return;
do {
block->info.nonce = nonce++;
block_hash(block, block->hash);
} while (!hash_matches_difficulty(block->hash, block->info.difficulty));
}
