#pragma once
#include <bits/stdc++.h>
class JpegEncoder
{
public:
	JpegEncoder();
	~JpegEncoder();
	void clean(void);
	bool read(const char* fileName);
	bool encode(const char* fileName, int qualityFactor);

private:
	int w;
	int h;
	unsigned char* rgbbuffer;
	unsigned char Ytable[64];
	unsigned char CbCrtable[64];
	
	struct BitString
	{
		int length;
		int value;
	};

	BitString Y_DC_Huffman_Table[12];
	BitString Y_AC_Huffman_Table[256];

	BitString CbCr_DC_Huffman_Table[12];
	BitString CbCr_AC_Huffman_Table[256];

private:
	void init_HuffmanTable();
	void initQualityTable(int qualityFactor);
	void build_HuffmanTable(const char* nr_nodes, const unsigned char* std_table, BitString* huffman_table);

	BitString getBitCode(int value);
	void ConvertColor(int xpos, int ypos, char* Ydata, char* Cbdata, char* Crdata);

	void DCT(const char* channel_data, short* fdc_data);
	void huffmanCoding(const short* DU, short& prevDC, const BitString* HTDC, const BitString* HTAC,BitString* outputBitString, int& bitStringCounts);


private:
	void write_header(FILE* file);
	void write_byte(unsigned char value, FILE* file);
	void write_word(unsigned short value, FILE* file);
	void write_bitString(const BitString* bs, int counts, int& newByte, int& newBytePos, FILE* file);
	void flush_bits(int& newByte, int& newBytePos, FILE* file);
	void write(const void* p, int byteSize, FILE* file);

};

