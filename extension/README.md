# HBLK Blockchain Explorer

A web-based explorer for the blockchain built in this repo. It reads the binary
file written by `blockchain_serialize` (HBLK format, v0.1 to v0.3) directly in
the browser and shows:

- every block as a linked card: index, data, timestamp, difficulty, nonce,
  previous hash and hash
- each block's transactions, with the outputs they spend and who receives coins
- wallet balances, computed from the list of unspent transaction outputs
- an integrity check for every block: recomputes the SHA-256 hash, checks it
  meets the difficulty, and checks the index and link to the previous block

## How to use

1. Serialize a blockchain with `blockchain_serialize(blockchain, "chain.hblk")`
2. Open `index.html` in a browser
3. Drop `chain.hblk` on the page, or click "Load demo" to see the genesis block

No install or build step needed. Everything runs locally in the browser.
