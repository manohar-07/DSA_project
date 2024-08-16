
# Huffman Coding for File Compression and Decompression

This repository contains a C implementation of the Huffman Coding algorithm for file compression and decompression. The program can encode a text file into a compressed binary format and decode it back to the original text.

## Features
- **Encode**: Compresses a text file using Huffman Coding.
- **Decode**: Decompresses a previously encoded file back to the original text.

## How to Compile
To compile the program, use the following command:
```bash
gcc huffman.c -o huffman
``` 
## How to Execute
Encoding:
To compress a text file:
```bash
./huffman encode sample.txt encoded.dat
```
- sample.txt: The input text file you want to compress.
- encoded.dat: The output file where the compressed data will be stored.

Decoding: 
To decompress a previously encoded file:
```bash
./huffman decode encoded.dat decoded.txt
```
- encoded.dat: The compressed input file.
- decoded.txt: The output file where the decompressed text will be saved.

## Example Usage
Encoding Example
```bash
./huffman encode sample.txt encoded.dat
```
Decoding Example
```bash
./huffman decode encoded.dat decoded.txt
```
- sample.txt (42.3kb) got compressed into encoded.dat (23.5kb)
- compression ratio: 55.6%


