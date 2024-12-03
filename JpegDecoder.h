#pragma once
#include <bits/stdc++.h>
using namespace std;

class JpegDecoder
{
public:
    JpegDecoder();
    ~JpegDecoder();
    void clean();

    bool read(const char* filename);

    bool decode(const char* filename);

private:
    int w; // 宽度
    int h; // 高度

    unsigned char* rgbbuffer;

    unsigned char Ytable[64];
    unsigned char CbCrtable[64];

    struct HuffmanTable
    {
        unsigned char bits[16];   
        unsigned char huffval[256]; 
        int huffsize[257];  
        unsigned int huffcode[257];
        int maxcode[18]; 
        int mincode[17];  
        int valptr[17];
    };

    struct Component {
        unsigned char id; // 组件标识符（例如，Y=1, Cb=2, Cr=3）
        unsigned char h;  // 水平采样因子
        unsigned char v;  // 垂直采样因子
        unsigned char quantTable;  // 使用的量化表编号
    };

    HuffmanTable Y_DC_Huffman_Table;
    HuffmanTable Y_AC_Huffman_Table;
    HuffmanTable CbCr_DC_Huffman_Table;
    HuffmanTable CbCr_AC_Huffman_Table;

    // 3 个组件（Y, Cb, Cr）
    Component components[3];
    int numComponents;

private:

    bool parseJPEGFile(FILE* file);

    bool decodeImageData(FILE* file);

    void initializeHuffmanTables();

    void buildHuffmanTable(const unsigned char* bits, const unsigned char* huffval, HuffmanTable& table);

    int decodeHuffmanSymbol(FILE* file, HuffmanTable& htable, int& bitBuffer, int& bitCount);

    int huffmanExtend(int v, int t);

    int readBits(FILE* file, int nbits, int& bitBuffer, int& bitCount);

    bool decodeBlock(FILE* file, HuffmanTable& htdc, HuffmanTable& htac, int& prevDC, unsigned char* qtable, short* block, int& bitBuffer, int& bitCount);

    void reorderZigZag(short* block);

    void dequantize(short* block, unsigned char* qtable);

    void inverseDCT(short* block, double* fblock);

    void upsampleChroma(double* chroma, unsigned char* upsampledChroma, int origWidth, int origHeight, int targetWidth, int targetHeight);
};
