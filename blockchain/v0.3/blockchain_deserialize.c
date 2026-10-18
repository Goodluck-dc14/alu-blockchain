#include "blockchain.h"

#define READ(fd, buf, n) (read((fd), (buf), (n)) != (ssize_t)(n))
#define CHECK_ENDIAN(x) (endianness ? SWAPENDIAN(x) : (void)0)

/**
 * read_out - deserializes a transaction output (101 bytes)
 * @fd: file descriptor
 * @out: output to fill
 * @endianness: 1 if bytes need swapping
 * Return: 0 on success, -1 on failure
 */
static int read_out(int fd, tx_out_t *out, uint8_t endianness)
{
if (READ(fd, &out->amount, 4) || READ(fd, out->pub, EC_PUB_LEN) ||
	READ(fd, out->hash, SHA256_DIGEST_LENGTH))
return (-1);
CHECK_ENDIAN(out->amount);
return (0);
}

/**
 * read_tx - deserializes one transaction into a list
 * @fd: file descriptor
 * @list: list to add the transaction to
 * @endianness: 1 if bytes need swapping
 * Return: 0 on success, -1 on failure
 */
static int read_tx(int fd, llist_t *list, uint8_t endianness)
{
transaction_t *tx = calloc(1, sizeof(*tx));
int32_t nb_in, nb_out, i;
tx_in_t *in;
tx_out_t *out;

if (!tx || llist_add_node(list, tx, ADD_NODE_REAR))
return (free(tx), -1);
tx->inputs = llist_create(MT_SUPPORT_FALSE);
tx->outputs = llist_create(MT_SUPPORT_FALSE);
if (!tx->inputs || !tx->outputs ||
	READ(fd, tx->id, SHA256_DIGEST_LENGTH) ||
	READ(fd, &nb_in, 4) || READ(fd, &nb_out, 4))
return (-1);
CHECK_ENDIAN(nb_in);
CHECK_ENDIAN(nb_out);
for (i = 0; i < nb_in; i++)
{
in = calloc(1, sizeof(*in));
if (!in || llist_add_node(tx->inputs, in, ADD_NODE_REAR))
return (free(in), -1);
if (READ(fd, in->block_hash, SHA256_DIGEST_LENGTH) ||
	READ(fd, in->tx_id, SHA256_DIGEST_LENGTH) ||
	READ(fd, in->tx_out_hash, SHA256_DIGEST_LENGTH) ||
	READ(fd, in->sig.sig, SIG_MAX_LEN) || READ(fd, &in->sig.len, 1))
return (-1);
}
for (i = 0; i < nb_out; i++)
{
out = calloc(1, sizeof(*out));
if (!out || llist_add_node(tx->outputs, out, ADD_NODE_REAR))
return (free(out), -1);
if (read_out(fd, out, endianness))
return (-1);
}
return (0);
}

/**
 * read_block - deserializes one block and its transactions
 * @fd: file descriptor
 * @block: block to fill
 * @endianness: 1 if bytes need swapping
 * Return: 0 on success, -1 on failure
 */
static int read_block(int fd, block_t *block, uint8_t endianness)
{
int32_t nb_tx, i;

if (READ(fd, &block->info, sizeof(block->info)) ||
	READ(fd, &block->data.len, 4))
return (-1);
CHECK_ENDIAN(block->info.index);
CHECK_ENDIAN(block->info.difficulty);
CHECK_ENDIAN(block->info.timestamp);
CHECK_ENDIAN(block->info.nonce);
CHECK_ENDIAN(block->data.len);
if (block->data.len > BLOCKCHAIN_DATA_MAX ||
	READ(fd, block->data.buffer, block->data.len) ||
	READ(fd, block->hash, SHA256_DIGEST_LENGTH) ||
	READ(fd, &nb_tx, 4))
return (-1);
CHECK_ENDIAN(nb_tx);
if (nb_tx == -1)
return (0);
block->transactions = llist_create(MT_SUPPORT_FALSE);
if (!block->transactions)
return (-1);
for (i = 0; i < nb_tx; i++)
if (read_tx(fd, block->transactions, endianness))
return (-1);
return (0);
}

/**
 * deserialize_blocks - deserializes all the blocks in the file
 * @fd: open fd to save file
 * @size: number of blocks in the file
 * @endianness: 1 if bytes need swapping
 * Return: pointer to list of blocks or NULL
 */
llist_t *deserialize_blocks(int fd, uint32_t size, uint8_t endianness)
{
llist_t *list = llist_create(MT_SUPPORT_TRUE);
block_t *block;
uint32_t i;

if (!list)
return (NULL);
for (i = 0; i < size; i++)
{
block = calloc(1, sizeof(*block));
if (!block || llist_add_node(list, block, ADD_NODE_REAR))
{
free(block);
break;
}
if (read_block(fd, block, endianness))
break;
}
if (i == size)
return (list);
llist_destroy(list, 1, (node_dtor_t)block_destroy);
return (NULL);
}

/**
 * blockchain_deserialize - deserializes blockchain from file
 * @path: path to serialized blockchain file
 * Return: pointer to deserialized blockchain or NULL
 */
blockchain_t *blockchain_deserialize(char const *path)
{
int fd;
blockchain_t *chain;
uint8_t endianness;
char buf[4] = {0};
uint32_t nb_blocks, nb_unspent, i;
unspent_tx_out_t *u;

if (!path)
return (NULL);
fd = open(path, O_RDONLY);
if (fd == -1)
return (NULL);
if (READ(fd, buf, 4) || memcmp(buf, HBLK_MAGIC, 4) ||
	READ(fd, buf, 3) || memcmp(buf, HBLK_VERSION, 3) ||
	READ(fd, &endianness, 1) || READ(fd, &nb_blocks, 4) ||
	READ(fd, &nb_unspent, 4))
return (close(fd), NULL);
endianness = endianness != _get_endianness();
CHECK_ENDIAN(nb_blocks);
CHECK_ENDIAN(nb_unspent);
chain = calloc(1, sizeof(*chain));
if (!chain)
return (close(fd), NULL);
chain->chain = deserialize_blocks(fd, nb_blocks, endianness);
if (!chain->chain)
return (free(chain), close(fd), NULL);
chain->unspent = llist_create(MT_SUPPORT_TRUE);
if (!chain->unspent)
return (blockchain_destroy(chain), close(fd), NULL);
for (i = 0; i < nb_unspent; i++)
{
u = calloc(1, sizeof(*u));
if (!u || llist_add_node(chain->unspent, u, ADD_NODE_REAR))
return (free(u), blockchain_destroy(chain), close(fd), NULL);
if (READ(fd, u->block_hash, SHA256_DIGEST_LENGTH) ||
	READ(fd, u->tx_id, SHA256_DIGEST_LENGTH) ||
	read_out(fd, &u->out, endianness))
return (blockchain_destroy(chain), close(fd), NULL);
}
return (close(fd), chain);
}
