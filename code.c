//

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <errno.h>
#include <math.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

// Variables


// Functions
uint16_t concatenate(uint8_t x, uint8_t y) { 
    return (x << 8) | y; 
}
int Max(int a, int b) { 
    return (a > b) ? a : b; 
} 
int Min(int a, int b) { 
    return (a < b) ? a : b; 
}


// Structs
struct Dimensions {
  uint16_t height;
  uint16_t width;
};
struct Headerinfo {
  int length;
  uint8_t *data;
};
struct HeaderWrapper {
    struct Headerinfo *dqt;
    int dqtcount;
    struct Headerinfo *sof;
    int sofcount;
    struct Headerinfo *dht;
    int dhtcount;
    struct Headerinfo *sos;
    int soscount;
    uint8_t *data;
};
struct Color {
    int red;
    int green;
    int blue;
};
struct DHTData {
    int Tc;
    int Th;
    uint8_t numcodes[16];
    int symlength;
    uint8_t *symbols;
};
struct DHTWrapper {
    struct DHTData *dhtdata;
    int dhtdatacount;
};
struct HuffmanNode {
    uint8_t symbol;
    bool leaf;
    int frequency;
    struct HuffmanNode *left;
    struct HuffmanNode *right;
};
struct HuffmanTable {
    uint8_t numcodes[4];
    uint8_t *symbols;
    struct HuffmanNode *root;
};


// Check if JPEG
int jpeg(const char *filename) {
    FILE *input = fopen(filename, "rb");
    if (!input) {
        perror("fopen error");
        return 0;
    }

    uint8_t magicNumbers[4];
    fread(magicNumbers, 1, 4, input);
    fclose(input);

    if (magicNumbers[0] == 0xFF && magicNumbers[1] == 0xD8 && magicNumbers[2] == 0xFF) {
        return 1;
    } else {
        return 0;
    }
}

struct Dimensions dimension(const char *filename) {
    FILE *input = fopen(filename, "rb");
    struct Dimensions dimension = {0, 0};

    if (!input) {
        perror("fopen error");
        return dimension;
    }
    
    struct stat fileStat; 
    if(stat(filename, &fileStat) != 0) {
        perror("stat error");
        fclose(input);
        return dimension;
    }

    size_t size = fileStat.st_size;
    uint8_t data[size];
    fread(data, 1, size, input);
    fclose(input);

    size_t counter = 2;
    while (counter < size - 8) {
        if (data[counter] == 0xFF && data[counter + 1] == 0xC0) {
            dimension.height = concatenate(data[counter + 5], data[counter + 6]);
            dimension.width = concatenate(data[counter + 7], data[counter + 8]);
            break;
        }
        counter++;
    }

    return dimension;
}



// Parse Headers
struct HeaderWrapper parseheader(const char *filename) {
    FILE *input = fopen(filename, "rb");
    struct HeaderWrapper wrapper = {0};

    if (!input) {
        perror("fopen error");
        return wrapper;
    }
    
    struct stat fileStat; 
    if(stat(filename, &fileStat) != 0) {
        perror("stat error");
        fclose(input);
        return wrapper;
    }

    size_t size = fileStat.st_size;
    wrapper.data = malloc(size);
    fread(wrapper.data, 1, size, input);
    fclose(input);

    size_t counter = 0;
    while (counter < size - 2) {
        if (wrapper.data[counter] == 0xFF) {
            uint8_t marker = wrapper.data[counter + 1];
            uint16_t length = concatenate(wrapper.data[counter + 2], wrapper.data[counter + 3]);

            struct Headerinfo header = {length + 2, malloc(length + 2)};
            if (header.data == NULL) { 
                perror("malloc error"); 
                free(wrapper.data); 
                return wrapper;
            }
            for (int i = 0; i < length + 2; i++) {
                header.data[i] = wrapper.data[counter + i];
            }

            if (marker == 0xDB) { // DQT 
                wrapper.dqt = realloc(wrapper.dqt, (wrapper.dqtcount + 1) * sizeof(struct Headerinfo));
                if (wrapper.dqt == NULL) { 
                    perror("realloc error"); 
                    free(wrapper.data); 
                    return wrapper;
                }
                wrapper.dqt[wrapper.dqtcount++] = header;
                counter += length + 2;
            } else if (marker == 0xC0) { // SOF
                wrapper.sof = realloc(wrapper.sof, (wrapper.sofcount + 1) * sizeof(struct Headerinfo));
                if (wrapper.sof == NULL) { 
                    perror("realloc error"); 
                    free(wrapper.data); 
                    return wrapper;
                }
                wrapper.sof[wrapper.sofcount++] = header;
                counter += length + 2;
            } else if (marker == 0xC4) { // DHT
                wrapper.dht = realloc(wrapper.dht, (wrapper.dhtcount + 1) * sizeof(struct Headerinfo));
                if (wrapper.dht == NULL) { 
                    perror("realloc error"); 
                    free(wrapper.data); 
                    return wrapper;
                }
                wrapper.dht[wrapper.dhtcount++] = header;
                counter += length + 2;
            } else if (marker == 0xDA) { // SOS
                size_t start = counter - 1;
                while (counter < size) {
                    if (wrapper.data[counter] == 0xFF && wrapper.data[counter + 1] == 0xD9) {
                        printf("EOI Header found\n");
                        break;
                    }
                    counter++;
                }
                size_t datalength = (counter - start) + 2;
                struct Headerinfo sosheader = {datalength, malloc(datalength)};
                if (sosheader.data == NULL) { 
                    perror("malloc error"); 
                    free(wrapper.data); 
                    return wrapper;
                }
                for (int i = 0; i < datalength; i++) {
                    sosheader.data[i] = wrapper.data[start + i];
                }
                wrapper.sos = realloc(wrapper.sos, (wrapper.soscount + 1) * sizeof(struct Headerinfo));
                if (wrapper.sos == NULL) { 
                    perror("realloc error"); 
                    free(wrapper.data); 
                    return wrapper;
                }
                wrapper.sos[wrapper.soscount++] = sosheader;
                break;
            } else {
                free(header.data);
                counter += 2;
            }
        } else {
            counter++;
        }
    }
    return wrapper;
}

// Parse DHT
struct DHTWrapper parseDHT(struct HeaderWrapper wrapper) {
    struct DHTWrapper dhtwrapper;
    dhtwrapper.dhtdata = (struct DHTData *)malloc(wrapper.dhtcount * sizeof(struct DHTData));
    dhtwrapper.dhtdatacount = wrapper.dhtcount;
    int counter;
    for (int i = 0; i < wrapper.dhtcount; i++) {
        int tcth = wrapper.dht[i].data[4];
        dhtwrapper.dhtdata[i].Tc = (tcth & 0xF0) >> 4;
        dhtwrapper.dhtdata[i].Th = (tcth & 0x0F);
        
        int symlength = 0;
        for (int j = 0; j < 16; j++) {
            dhtwrapper.dhtdata[i].numcodes[j] = wrapper.dht[i].data[j + 5];
            symlength += wrapper.dht[i].data[j + 5];
        }

        dhtwrapper.dhtdata[i].symlength = symlength;
        if (dhtwrapper.dhtdata[i].symbols == NULL) {
            perror("malloc error"); 
            return dhtwrapper; 
        }
        
        dhtwrapper.dhtdata[i].symbols = (uint8_t *)malloc(symlength);
        for (int k = 0; k < symlength; k++) {
            dhtwrapper.dhtdata[i].symbols[k] = wrapper.dht[i].data[k + 21];
        }
    }
    return dhtwrapper;
}


// Create Huffman Tables
struct HuffmanNode* createNode(bool leaf, uint8_t symbol, int frequency) {
    struct HuffmanNode *node = (struct HuffmanNode *)malloc(sizeof(struct HuffmanNode));
    node->leaf = leaf;
    node->symbol = symbol;
    node->frequency = frequency;
    node->left = NULL;
    node->right = NULL;
    return node;
}


// YCbCr to RGB
struct Color rgb(int y, int cb, int cr) {
    struct Color color = {0, 0, 0};
    int r = (int) (y + 1.40200 * (cr - 0x80));
    int g = (int) (y - 0.34414 * (cb - 0x80) - 0.71414 * (cr - 0x80));
    int b = (int) (y + 1.77200 * (cb - 0x80));

    color.red = Max(0, Min(255, r));
    color.green = Max(0, Min(255, g));
    color.blue = Max(0, Min(255, b));

    return color;
}



int main(int argc, char *argv[]) {

    if (argc > 2 || argc == 1) {
        printf("Usage: ./code (filename)");
        return 0;
    }
    const char *filename = argv[1];
    
    if (jpeg(filename)) {
        printf("Correct file format\n");
    } else {
        printf("Incorrect file format\n");
        return 0;
    }
    
    struct Dimensions d = dimension(filename);
    printf("Height: %d, Width: %d\n", d.height, d.width);

    struct HeaderWrapper hw = parseheader(filename);
    printf("DQT Header Count: %d\n", hw.dqtcount);
    printf("SOF Header Count: %d\n", hw.sofcount);
    printf("DHT Header Count: %d\n", hw.dhtcount);
    printf("SOS Header Count: %d\n", hw.soscount);
    printf("\n");

    for (int i = 0; i < hw.dqtcount; i++) {
        printf("DQT Header Length (%d): %d\n", i+1, hw.dqt[i].length);
        printf("DQT Header Info (%d): ", i+1);
        for (int j = 0; j < hw.dqt[i].length; j++) {
            printf("%d ", hw.dqt[i].data[j]);
        }
        printf("\n");
    }
    printf("\n");

    for (int i = 0; i < hw.sofcount; i++) {
        printf("SOF Header Length (%d): %d\n", i+1, hw.sof[i].length);
        printf("SOF Header Info (%d): ", i+1);
        for (int j = 0; j < hw.sof[i].length; j++) {
            printf("%d ", hw.sof[i].data[j]);
        }
        printf("\n");
    }
    printf("\n");

    for (int i = 0; i < hw.dhtcount; i++) {
        printf("DHT Header Length (%d): %d\n", i+1, hw.dht[i].length);
        printf("DHT Header Info (%d): ", i+1);
        for (int j = 0; j < hw.dht[i].length; j++) {
            printf("%d ", hw.dht[i].data[j]);
        }
        printf("\n");
    }
    printf("\n");

    for (int i = 0; i < hw.soscount; i++) {
        printf("SOS Header Length (%d): %d\n", i+1, hw.sos[i].length);
        printf("SOS Header Info (%d): ", i+1);
        printf("%d ", hw.sos[i].data[1]);
        printf("%d ", hw.sos[i].data[2]);
        printf("\n");
    }
    printf("\n");

    struct DHTWrapper dhtw = parseDHT(hw);
    for (int i = 0; i < dhtw.dhtdatacount; i++) {
        printf("DHT Tc: %i\n", dhtw.dhtdata[i].Tc);
        printf("DHT Th: %i\n", dhtw.dhtdata[i].Th);

        printf("DHT NumCodes: ");
        for (int j = 0; j < 16; j++) {
            printf("%d ", dhtw.dhtdata[i].numcodes[j]);
        }
        printf("\n");

        printf("DHT Symbols: ");
        for (int k = 0; k < dhtw.dhtdata[i].symlength; k++) {
            printf("%d ", dhtw.dhtdata[i].symbols[k]);
        }
        printf("\n");
    }
    
}