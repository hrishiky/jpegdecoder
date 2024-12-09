# JPEG Decoder
*This is a work in progress*

**A program that can decode the JPEG binary:**

ㅤㅤ•ㅤwritten in plain C with no image libraries (only standard libraries like <stdio.h>)

ㅤㅤ•ㅤdecodes pixel data straight from the JPEG binary data

ㅤㅤㅤㅤ•ㅤparses headers and saves their internal data

ㅤㅤㅤㅤ•ㅤcreates Huffman tables to sort and decode data into

ㅤㅤㅤㅤ•ㅤuses Inverse Discrete Cosine Tranform to decode compressed image data

ㅤㅤㅤㅤ•ㅤconverts JPEG's YCbCr to RGB pixel data

ㅤㅤ•ㅤcan convert the decoded JPEG data to ASCII art

ㅤㅤ•ㅤoptions to export image pixel and header data and ASCII art as .json or .txt


