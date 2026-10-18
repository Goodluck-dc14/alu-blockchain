#include "blockchain.h"

/**
 * write_all - writes exactly n bytes
 * @fd: file descriptor
 * @buf: buffer to write
 * @n: number of bytes
 * Return: 0 on success, -1 on failure
 */
static int write_all(int fd, void const *buf, size_t n)
{
return (write(fd, buf, n) == (ssize_t)n ? 0 : -1);
}

/**
 * write_out - serializes a transaction output (101 bytes)
 * @fd: file descriptor
 * @out: transaction output
 * Return: 0 on success, -1 on failure
 */
static int write_out(int fd, tx_out_t const *out)
{
if (!out || write_all(fd, &out->amount, 4) ||
    write_all(fd, out->pub, EC_PUB_LEN) ||
    write_all(fd, out->hash, SHA256_DIGEST_LENGTH))
return (-1);
return (0);
}

/**
 * write_tx - serializes a transaction
 * @fd: file descriptor
 * @tx: transaction
 * Return: 0 on success, -1 on failure
 */
static int write_tx(int fd, transaction_t const *tx)
{
int32_t nb_in, nb_out, i;
tx_in_t *in;

if (!tx)
return (-1);
nb_in = llist_size(tx->inputs);
nb_out = llist_size(tx->outputs);
if (write_all(fd, tx->id, SHA256_DIGEST_LENGTH) ||
    write_all(fd, &nb_in, 4) || write_all(fd, &nb_out, 4))
return (-1);
for (i = 0; i < nb_in; i++)
{
in = llist_get_node_at(tx->inputs, i);
if (!in || write_all(fd, in->block_hash, SHA256_DIGEST_LENGTH) ||
    write_all(fd, in->tx_id, SHA256_DIGEST_LENGTH) ||
    write_all(fd, in->tx_out_hash, SHA256_DIGEST_LENGTH) ||
    write_all(fd, in->sig.sig, SIG_MAX_LEN) ||
    write_all(fd, &in->sig.len, 1))
return (-1);
}
for (i = 0; i < nb_out; i++)
if (write_out(fd, llist_get_node_at(tx->outputs, i)))
return (-1);
return (0);
}

/**
 * write_block - serializes a block and its transactions
 * @fd: file descriptor
 * @block: block
 * Return: 0 on success, -1 on failure
 */
static int write_block(int fd, block_t const *block)
{
int32_t nb_tx, i;

if (!block)
return (-1);
nb_tx = block->transactions ? llist_size(block->transactions) : -1;
if (write_all(fd, &block->info, sizeof(block->info)) ||
    write_all(fd, &block->data.len, 4) ||
    write_all(fd, block->data.buffer, block->data.len) ||
    write_all(fd, block->hash, SHA256_DIGEST_LENGTH) ||
    write_all(fd, &nb_tx, 4))
return (-1);
for (i = 0; i < nb_tx; i++)
if (write_tx(fd, llist_get_node_at(block->transactions, i)))
return (-1);
return (0);
}

/**
 * blockchain_serialize - serializes blockchain to file
 * @blockchain: pointer to blockchain to serialize
 * @path: path to save file
 * Return: 0 on success else -1 on failure.
 */
int blockchain_serialize(blockchain_t const *blockchain, char const *path)
{
int fd, i;
int32_t nb_blocks, nb_unspent;
uint8_t endian = _get_endianness();
unspent_tx_out_t *u;

if (!blockchain || !blockchain->chain || !path)
return (-1);
nb_blocks = llist_size(blockchain->chain);
nb_unspent = blockchain->unspent ? llist_size(blockchain->unspent) : 0;
fd = open(path, O_CREAT | O_TRUNC | O_WRONLY, S_IRUSR | S_IWUSR);
if (fd == -1)
return (-1);
if (write_all(fd, HBLK_MAGIC, 4) || write_all(fd, HBLK_VERSION, 3) ||
    write_all(fd, &endian, 1) || write_all(fd, &nb_blocks, 4) ||
    write_all(fd, &nb_unspent, 4))
return (close(fd), -1);
for (i = 0; i < nb_blocks; i++)
if (write_block(fd, llist_get_node_at(blockchain->chain, i)))
return (close(fd), -1);
for (i = 0; i < nb_unspent; i++)
{
u = llist_get_node_at(blockchain->unspent, i);
if (!u || write_all(fd, u->block_hash, SHA256_DIGEST_LENGTH) ||
    write_all(fd, u->tx_id, SHA256_DIGEST_LENGTH) ||
    write_out(fd, &u->out))
return (close(fd), -1);
}
return (close(fd), 0);
}
