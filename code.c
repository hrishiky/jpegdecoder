#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/stat.h>
#include <errno.h>

// Funtion to determine if the file is a JPEG or not
int jpeg(const char *filename) {
    // Opens the file and checks if it was done correctly
    FILE *input = fopen(filename, "rb");
    if (!input) {
        perror("fopen error");
        return 0;
    }

    // Creates an array of integers to hold the first 4 bytes of data in the image
    uint8_t magicNumbers[4];
    fread(magicNumbers, 1, 4, input);
    fclose(input);

    /* Debug code to check values taken from the file
    printf("%d", magicNumbers[0]);
    printf("%d", magicNumbers[1]);
    printf("%d", magicNumbers[2]); 
    */

    // Checks if the numbers in the array are the same as the JPEG signature
    if (magicNumbers[0] == 0xFF && magicNumbers[1] == 0xD8 && magicNumbers[2] == 0xFF) {
        return 1;
    } else {
        return 0;
    }
}

// Function to concatenate two integers using bitwise operations
uint16_t concatenate(uint8_t x, uint8_t y) { 
    return (x << 8) | y; 
}

// Creates a struct to return the dimention values from the dimention function
struct Dimentions {
  uint16_t height;
  uint16_t width;
};

// Fuction to determine the dimentions of the JPEG
struct Dimentions dimention(const char *filename) {
    // Opens file and initializes the struct
    FILE *input = fopen(filename, "rb");
    struct Dimentions dimention = {0, 0};

    // Checks if opening the file worked
    if (!input) {
        perror("fopen error");
        return dimention;
    }
    
    // Creates a new struct for reading file length and checks if it is working properly
    struct stat fileStat; 
    if(stat(filename, &fileStat) != 0) {
        perror("stat error");
        fclose(input);
        return dimention;
    }

    // Finds size of file and transfers all of the data to an array
    size_t size = fileStat.st_size;
    uint8_t data[size];
    fread(data, 1, size, input);
    fclose(input);

    // Searches the array for the standard JPEG FFC0 SOF marker, which is 5 bytes away from the height and width bytes
    // Then concatenates the bytes and sets the height and width values to the struct's variables
    size_t counter = 2;
    while (counter < size - 8) {
        if (data[counter] == 0xFF && data[counter + 1] == 0xC0) {
            dimention.height = concatenate(data[counter + 5], data[counter + 6]);
            dimention.width = concatenate(data[counter + 7], data[counter + 8]);
            break;
        }
        counter++;
    }

    return dimention;
}



// Parse Headers
struct Headerinfo {
  uint8_t APP[1];
  uint8_t DQT[1];
  uint8_t DHT[1];
  uint8_t SOF[1];
  uint8_t SOS[1];
};
struct Headerinfo header(const char *filename) {
    FILE *input = fopen(filename, "rb");
    struct Headerinfo header;

    if (!input) {
        perror("fopen error");
        return header;
    }
    
    struct stat fileStat; 
    if(stat(filename, &fileStat) != 0) {
        perror("stat error");
        fclose(input);
        return header;
    }

    size_t size = fileStat.st_size;
    uint8_t data[size];
    fread(data, 1, size, input);
    fclose(input);

    size_t counter = 2;
    while (counter < size - 8) {
        if (data[counter] == 0xFF && data[counter + 1] == 0xC0) {
            header.height = concatenate(data[counter + 5], data[counter + 6]);
            header.width = concatenate(data[counter + 7], data[counter + 8]);
            break;
        }
        counter++;
    }

    return header;
}

// YCbCr to RGB
struct Color {
    int red;
    int green;
    int blue;
};
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



int main() {
    const char *filename = "cat.jpg";
    if (jpeg(filename)) {
        printf("Correct file format\n");
    } else {
        printf("Incorrect file format\n");
        return 0;
    }
    
    struct Dimentions d = dimention(filename);

    printf("Height: %d, Width: %d\n", d.height, d.width);
}

