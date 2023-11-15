#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_BUFFER_SIZE 256
#define READ_ERROR -1
#define WRITE_ERROR -1
#define OPERATION_FAILURE 1
#define OPERATION_SUCCESS 0
#define FILE_OPEN_OPERATION_FAILURE -1
#define END_OF_FILE_MARKER -1
#define MEM_ALLOC_FAILURE -1

int numChars = 256;
int nActive = 0;
int *counts = NULL;
unsigned int original_size = 0;

typedef struct
{
    int index;
    unsigned int count;
} NODE;

NODE *nodes = NULL;
int num_nodes = 0;
int *leaf_indx = NULL;
int *parent_indx = NULL;
int free_indx = 1;

int *stack;
int stack_top;

unsigned char buffer[MAX_BUFFER_SIZE];
int bits_in_buffer = 0;
int current_bit = 0;
int eof_input = 0;

int readHeader(FILE *f);
int writeHeader(FILE *f);
int readBit(FILE *f);
int writeBit(FILE *f, int bit);
int flushBuffer(FILE *f);
void decodeBitStream(FILE *fin, FILE *fout);
int decode(const char *ifile, const char *ofile);
void encodeAlphabet(FILE *fout, int character);
int encode(const char *ifile, const char *ofile);
void buildTree();
void addLeaves();
int addNode(int index, int count);
void initialize();
void determine_counts(FILE *f)
{
    int c;
    while ((c = fgetc(f)) != EOF)
    {
        ++counts[c];
        ++original_size;
    }
    for (c = 0; c < numChars; ++c)
        if (counts[c] > 0)
            ++nActive;
}

void initialize()
{
    counts = (int *)
        calloc(2 * numChars, sizeof(int));
    leaf_indx = counts + numChars - 1;
}

void allocateTree()
{
    nodes = (NODE *)
        calloc(2 * nActive, sizeof(NODE));
    parent_indx = (int *)
        calloc(nActive, sizeof(int));
}

int addNode(int index, int count)
{ // sorting characters based on their counts
    int i = num_nodes++;
    while (i > 0 && nodes[i].count > count)
    {
        memcpy(&nodes[i + 1], &nodes[i], sizeof(NODE));
        if (nodes[i].index < 0)
            ++leaf_indx[-nodes[i].index];
        else
            ++parent_indx[nodes[i].index];
        --i;
    }

    ++i;
    nodes[i].index = index;
    nodes[i].count = count;
    if (index < 0)
        leaf_indx[-index] = i;
    else
        parent_indx[index] = i;

    return i;
}

void buildTree()
{
    int a, b, index;
    while (free_indx < num_nodes)
    {
        a = free_indx++;
        b = free_indx++;
        index = addNode(b / 2,
                         nodes[a].count + nodes[b].count);
        parent_indx[b / 2] = index;
    }
}
void addLeaves()
{
    int i, freq;
    for (i = 0; i < numChars; ++i)
    {
        freq = counts[i];
        if (freq > 0)
            addNode(-(i + 1), freq);
    }
}



int encode(const char *ifile, const char *ofile)
{
    FILE *fin, *fout;
    if ((fin = fopen(ifile, "rb")) == NULL)
    {
        perror("Failed to open input file");
        return FILE_OPEN_OPERATION_FAILURE;
    }
    if ((fout = fopen(ofile, "wb")) == NULL)
    {
        perror("Failed to open output file");
        fclose(fin);
        return FILE_OPEN_OPERATION_FAILURE;
    }

    determine_counts(fin);
    stack = (int *)calloc(nActive - 1, sizeof(int));
    allocateTree();

    addLeaves();
    writeHeader(fout);
    buildTree();
    fseek(fin, 0, SEEK_SET); // sets file pointer to the beginning of the file
    int c;
    while ((c = fgetc(fin)) != EOF)
        encodeAlphabet(fout, c);
    flushBuffer(fout);
    free(stack);
    fclose(fin);
    fclose(fout);

    return 0;
}

void encodeAlphabet(FILE *fout, int character)
{
    int node_index;
    stack_top = 0; // The stack is used to store the bits of the Huffman code temporarily.
    node_index = leaf_indx[character + 1];
    while (node_index < num_nodes)
    {
        stack[stack_top++] = node_index % 2;
        node_index = parent_indx[(node_index + 1) / 2];   // This process continues until the root of the Huffman tree is reached (when node_index >= num_nodes)
    }
    while (--stack_top > -1)
        writeBit(fout, stack[stack_top]);
}

int decode(const char *ifile, const char *ofile)
{
    FILE *fin, *fout;
    if ((fin = fopen(ifile, "rb")) == NULL)
    {
        perror("Failed to open input file");
        return FILE_OPEN_OPERATION_FAILURE;
    }
    if ((fout = fopen(ofile, "wb")) == NULL)
    {
        perror("Failed to open output file");
        fclose(fin);
        return FILE_OPEN_OPERATION_FAILURE;
    }

    if (readHeader(fin) == 0)
    {
        buildTree();
        decodeBitStream(fin, fout);
    }
    fclose(fin);
    fclose(fout);

    return 0;
}

void decodeBitStream(FILE *fin, FILE *fout)
{
    int i = 0, bit, node_index = nodes[num_nodes].index;
    while (1)
    {
        bit = readBit(fin);
        if (bit == -1)
            break;
        node_index = nodes[node_index * 2 - bit].index;
        if (node_index < 0)
        {
            char c = -node_index - 1;
            fwrite(&c, 1, 1, fout);
            if (++i == original_size)
                break;
            node_index = nodes[num_nodes].index;
        }
    }
}

int writeBit(FILE *f, int bit)      //writing a single bit to the output file
{
    if (bits_in_buffer == MAX_BUFFER_SIZE << 3)
    {
        size_t bytes_written =
            fwrite(buffer, 1, MAX_BUFFER_SIZE, f);
        if (bytes_written < MAX_BUFFER_SIZE && ferror(f))
            return WRITE_ERROR;
        bits_in_buffer = 0;
        memset(buffer, 0, MAX_BUFFER_SIZE);
    }
    if (bit)
        buffer[bits_in_buffer >> 3] |=         //Converts the bit index to a byte index
            (0x1 << (7 - bits_in_buffer % 8));
    ++bits_in_buffer;
    return OPERATION_SUCCESS;
}

int flushBuffer(FILE *f)
{
    if (bits_in_buffer)
    {
        size_t bytes_written =
            fwrite(buffer, 1,
                   (bits_in_buffer + 7) >> 3, f);
        if (bytes_written < MAX_BUFFER_SIZE && ferror(f))
            return -1;
        bits_in_buffer = 0;
    }
    return 0;
}

int readBit(FILE *f)
{
    if (current_bit == bits_in_buffer)
    {
        if (eof_input)
            return END_OF_FILE_MARKER;
        else
        {
            size_t bytes_read =
                fread(buffer, 1, MAX_BUFFER_SIZE, f);
            if (bytes_read < MAX_BUFFER_SIZE)
            {
                if (feof(f))
                    eof_input = 1;
            }
            bits_in_buffer = bytes_read << 3;
            current_bit = 0;
        }
    }

    if (bits_in_buffer == 0)
        return END_OF_FILE_MARKER;
    int bit = (buffer[current_bit >> 3] >>
               (7 - current_bit % 8)) &
              0x1;
    ++current_bit;
    return bit;
}

int writeHeader(FILE *f) //for writing the header information of the Huffman tree to the output file during the encoding process
{
    int i, j, byte = 0,
              size = sizeof(unsigned int) + 1 + nActive * (1 + sizeof(int));
    unsigned int count;
    char *buffer = (char *)calloc(size, 1);
    if (buffer == NULL)
        return MEM_ALLOC_FAILURE;

    j = sizeof(int);
    while (j--)
        buffer[byte++] =(original_size >> (j << 3)) & 0xff;  //Writes the original size of the input data to the header. It converts the original size to bytes and stores them   in the buffer.

    buffer[byte++] = (char)nActive;
    for (i = 1; i <= nActive; ++i)
    {
        count = nodes[i].count;
        buffer[byte++] =
            (char)(-nodes[i].index - 1);
        j = sizeof(int);
        while (j--)
            buffer[byte++] = (count >> (j << 3)) & 0xff;
    }
    fwrite(buffer, 1, size, f);
    free(buffer);
    return 0;
}

int readHeader(FILE *f)       //reads the header info where size and num of nodes of tree are mentioned and builds the tree again
{
    int i, j, byte = 0, size;
    size_t bytes_read;
    unsigned char buff[4];

    bytes_read = fread(&buff, 1, sizeof(int), f);
    if (bytes_read < 1)
        return END_OF_FILE_MARKER;
    byte = 0;
    original_size = buff[byte++];
    while (byte < sizeof(int))
        original_size =
            (original_size << (1 << 3)) | buff[byte++];

    bytes_read = fread(&nActive, 1, 1, f);
    if (bytes_read < 1)
        return END_OF_FILE_MARKER;

    allocateTree();

    size = nActive * (1 + sizeof(int));
    unsigned int count;
    char *buffer = (char *)calloc(size, 1);
    if (buffer == NULL)
        return MEM_ALLOC_FAILURE;
    fread(buffer, 1, size, f);
    byte = 0;
    for (i = 1; i <= nActive; ++i)
    {
        nodes[i].index = -(buffer[byte++] + 1);
        j = 0;
        count = (unsigned char)buffer[byte++];
        while (++j < sizeof(int))
        {
            count = (count << (1 << 3)) |
                     (unsigned char)buffer[byte++];
        }
        nodes[i].count = count;
    }
    num_nodes = (int)nActive;
    free(buffer);
    return 0;
}

void print_help()
{
    fprintf(stderr,
            "USAGE: ./huffman [encode | decode] "
            "<input out> <output file>\n");
}

int main(int argc, char **argv)
{
    if (argc != 4)
    {
        print_help();
        return OPERATION_FAILURE;
    }
    initialize();
    if (strcmp(argv[1], "encode") == 0)
        encode(argv[2], argv[3]);
    else if (strcmp(argv[1], "decode") == 0)
        decode(argv[2], argv[3]);
    else
        print_help();
    
    free(parent_indx);
    free(counts);
    free(nodes);


    return OPERATION_SUCCESS;
}
