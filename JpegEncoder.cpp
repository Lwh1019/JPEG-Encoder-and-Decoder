#define _CRT_SECURE_NO_WARNINGS
#include "JpegEncoder.h"
#include <bits/stdc++.h>
using namespace std;

namespace {
	const unsigned char Luminance_Quantization_Table[64] =
	{
		16,  11,  10,  16,  24,  40,  51,  61,
		12,  12,  14,  19,  26,  58,  60,  55,
		14,  13,  16,  24,  40,  57,  69,  56,
		14,  17,  22,  29,  51,  87,  80,  62,
		18,  22,  37,  56,  68, 109, 103,  77,
		24,  35,  55,  64,  81, 104, 113,  92,
		49,  64,  78,  87, 103, 121, 120, 101,
		72,  92,  95,  98, 112, 100, 103,  99
	};
	const unsigned char Chrominance_Quantization_Table[64] =
	{
		17,  18,  24,  47,  99,  99,  99,  99,
		18,  21,  26,  66,  99,  99,  99,  99,
		24,  26,  56,  99,  99,  99,  99,  99,
		47,  66,  99,  99,  99,  99,  99,  99,
		99,  99,  99,  99,  99,  99,  99,  99,
		99,  99,  99,  99,  99,  99,  99,  99,
		99,  99,  99,  99,  99,  99,  99,  99,
		99,  99,  99,  99,  99,  99,  99,  99
	};
	const char ZigZag[64] =
	{
		0, 1, 5, 6,14,15,27,28,
		2, 4, 7,13,16,26,29,42,
		3, 8,12,17,25,30,41,43,
		9,11,18,24,31,40,44,53,
		10,19,23,32,39,45,52,54,
		20,22,33,38,46,51,55,60,
		21,34,37,47,50,56,59,61,
		35,36,48,49,57,58,62,63
	};
	const char Standard_DC_Luminance_NRCodes[] = { 0, 0, 7, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0 };
	const unsigned char Standard_DC_Luminance_Values[] = { 4, 5, 3, 2, 6, 1, 0, 7, 8, 9, 10, 11 };

	const char Standard_DC_Chrominance_NRCodes[] = { 0, 3, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0 };
	const unsigned char Standard_DC_Chrominance_Values[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 };


	const char Standard_AC_Luminance_NRCodes[] = { 0, 2, 1, 3, 3, 2, 4, 3, 5, 5, 4, 4, 0, 0, 1, 0x7d };
	const unsigned char Standard_AC_Luminance_Values[] =
	{
		0x01, 0x02, 0x03, 0x00, 0x04, 0x11, 0x05, 0x12,
		0x21, 0x31, 0x41, 0x06, 0x13, 0x51, 0x61, 0x07,
		0x22, 0x71, 0x14, 0x32, 0x81, 0x91, 0xa1, 0x08,
		0x23, 0x42, 0xb1, 0xc1, 0x15, 0x52, 0xd1, 0xf0,
		0x24, 0x33, 0x62, 0x72, 0x82, 0x09, 0x0a, 0x16,
		0x17, 0x18, 0x19, 0x1a, 0x25, 0x26, 0x27, 0x28,
		0x29, 0x2a, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39,
		0x3a, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49,
		0x4a, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59,
		0x5a, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69,
		0x6a, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79,
		0x7a, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89,
		0x8a, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98,
		0x99, 0x9a, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7,
		0xa8, 0xa9, 0xaa, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6,
		0xb7, 0xb8, 0xb9, 0xba, 0xc2, 0xc3, 0xc4, 0xc5,
		0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xd2, 0xd3, 0xd4,
		0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xda, 0xe1, 0xe2,
		0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9, 0xea,
		0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8,
		0xf9, 0xfa
	};

	const char Standard_AC_Chrominance_NRCodes[] = { 0, 2, 1, 2, 4, 4, 3, 4, 7, 5, 4, 4, 0, 1, 2, 0x77 };
	const unsigned char Standard_AC_Chrominance_Values[] =
	{
		0x00, 0x01, 0x02, 0x03, 0x11, 0x04, 0x05, 0x21,
		0x31, 0x06, 0x12, 0x41, 0x51, 0x07, 0x61, 0x71,
		0x13, 0x22, 0x32, 0x81, 0x08, 0x14, 0x42, 0x91,
		0xa1, 0xb1, 0xc1, 0x09, 0x23, 0x33, 0x52, 0xf0,
		0x15, 0x62, 0x72, 0xd1, 0x0a, 0x16, 0x24, 0x34,
		0xe1, 0x25, 0xf1, 0x17, 0x18, 0x19, 0x1a, 0x26,
		0x27, 0x28, 0x29, 0x2a, 0x35, 0x36, 0x37, 0x38,
		0x39, 0x3a, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48,
		0x49, 0x4a, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58,
		0x59, 0x5a, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68,
		0x69, 0x6a, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78,
		0x79, 0x7a, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87,
		0x88, 0x89, 0x8a, 0x92, 0x93, 0x94, 0x95, 0x96,
		0x97, 0x98, 0x99, 0x9a, 0xa2, 0xa3, 0xa4, 0xa5,
		0xa6, 0xa7, 0xa8, 0xa9, 0xaa, 0xb2, 0xb3, 0xb4,
		0xb5, 0xb6, 0xb7, 0xb8, 0xb9, 0xba, 0xc2, 0xc3,
		0xc4, 0xc5, 0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xd2,
		0xd3, 0xd4, 0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xda,
		0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9,
		0xea, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8,
		0xf9, 0xfa
	};
}
JpegEncoder::JpegEncoder()
	:w(0), h(0), rgbbuffer(0)
{
	init_HuffmanTable();  // 初始化静态表格
}

JpegEncoder::~JpegEncoder()
{
	clean();
}

void JpegEncoder::clean(void)
{
	if (rgbbuffer) delete[] rgbbuffer;
	rgbbuffer = 0;
	w = 0;
	h = 0;
}

bool JpegEncoder::read(const char *filename)   // 读取bmp文件
{
	clean();

#pragma pack(push, 2)   // bmp格式
	typedef struct {
		unsigned short	bfType;
		unsigned int	bfSize;
		unsigned short	bfReserved1;
		unsigned short	bfReserved2;
		unsigned int	bfOffBits;
	} BITMAPFILEHEADER;

	typedef struct {
		unsigned int	biSize;
		int				biWidth;
		int				biHeight;
		unsigned short	biPlanes;
		unsigned short	biBitCount;
		unsigned int	biCompression;
		unsigned int	biSizeImage;
		int				biXPelsPerMeter;
		int				biYPelsPerMeter;
		unsigned int	biClrUsed;
		unsigned int	biClrImportant;
	} BITMAPINFOHEADER;
#pragma pack(pop)

	FILE* file = fopen(filename, "rb");
	if (file == 0)
	{
		cout << "文件打开错误" << '\n';
		return false;
	}
	bool flag = false;

	do
	{
		BITMAPFILEHEADER fileHeader;
		BITMAPINFOHEADER infoHeader;

		if (1 != fread(&fileHeader, sizeof(fileHeader), 1, file)) break;
		if (fileHeader.bfType != 0x4D42) break;
		if (1 != fread(&infoHeader, sizeof(infoHeader), 1, file)) break;
		if (infoHeader.biBitCount != 24 || infoHeader.biCompression != 0) break;
		int width = infoHeader.biWidth;
		int height = infoHeader.biHeight < 0 ? (-infoHeader.biHeight) : infoHeader.biHeight;
		if ((width & 7) != 0 || (height & 7) != 0)
		{
			cout << "打开文件需要是8的倍数" << '\n';
			break;   // 打开文件需要是8的倍数
		}


		int bmpSize = width * height * 3;

		unsigned char* buffer = new unsigned char[bmpSize];
		if (buffer == 0) break;

		fseek(file, fileHeader.bfOffBits, SEEK_SET);

		if (infoHeader.biHeight > 0)
		{
			for (int i = 0; i < height; i++)
			{
				if (width != fread(buffer + (height - 1 - i) * width * 3, 3, width, file))
				{
					delete[] buffer; buffer = 0;
					break;
				}
			}
		}
		else
		{
			if (width * height != fread(buffer, 3, width * height, file))
			{
				delete[] buffer; buffer = 0;
				break;
			}
		}
		rgbbuffer = buffer;
		w = width;
		h = height;
		flag = true;
	} while (false);
	fclose(file);
	file = 0;
	return flag;
}

bool JpegEncoder::encode(const char *filename,int qualityFactor)
{
	if (rgbbuffer == 0 || h == 0 || w == 0)
	{
		return false;
	}
	FILE* file = fopen(filename, "wb");
	if (file == 0) return false;

	initQualityTable(qualityFactor); // 初始化量化表

	write_header(file);

	short prev_DC_Y = 0, prev_DC_Cb = 0, prev_DC_Cr = 0;

	int newByte = 0, newBytePos = 7;
	for (int ypos = 0; ypos < h; ypos+=8)
	{
		for (int xpos = 0; xpos < w; xpos+=8)
		{
			char yData[64], cbData[64], crData[64];
			short yQuant[64], cbQuant[64], crQuant[64];

			ConvertColor(xpos, ypos, yData, cbData, crData);

			BitString outputBitString[128];
			int bitStringCounts;

			// Y 
			DCT(yData, yQuant);
			huffmanCoding(yQuant, prev_DC_Y, Y_DC_Huffman_Table, Y_AC_Huffman_Table, outputBitString, bitStringCounts);
			write_bitString(outputBitString, bitStringCounts, newByte, newBytePos, file);


			// Cr
			DCT(cbData, cbQuant);
			huffmanCoding(cbQuant, prev_DC_Cb, CbCr_DC_Huffman_Table, CbCr_AC_Huffman_Table, outputBitString, bitStringCounts);
			write_bitString(outputBitString, bitStringCounts, newByte, newBytePos, file);

			// Cr
			DCT(crData, crQuant);
			huffmanCoding(crQuant, prev_DC_Cr, CbCr_DC_Huffman_Table, CbCr_AC_Huffman_Table, outputBitString, bitStringCounts);
			write_bitString(outputBitString, bitStringCounts, newByte, newBytePos, file);
		}
	}
	flush_bits(newByte, newBytePos, file);
	write_word(0xFFD9, file);
	fclose(file);
	return true;
}
void JpegEncoder::flush_bits(int& newByte, int& newBytePos, FILE* file)
{
	if (newBytePos < 7)
	{
		write_byte((unsigned char)(newByte), file);
		if (newByte == 0xFF)
		{
			write_byte((unsigned char)(0x00), file);
		}
	}
}
void JpegEncoder::init_HuffmanTable()
{
	memset(&Y_DC_Huffman_Table, 0, sizeof(Y_DC_Huffman_Table));
	build_HuffmanTable(Standard_DC_Luminance_NRCodes, Standard_DC_Luminance_Values, Y_DC_Huffman_Table);

	memset(&Y_AC_Huffman_Table, 0, sizeof(Y_AC_Huffman_Table));
	build_HuffmanTable(Standard_AC_Luminance_NRCodes, Standard_AC_Luminance_Values, Y_AC_Huffman_Table);

	memset(&CbCr_DC_Huffman_Table, 0, sizeof(CbCr_DC_Huffman_Table));
	build_HuffmanTable(Standard_DC_Chrominance_NRCodes, Standard_DC_Chrominance_Values, CbCr_DC_Huffman_Table);

	memset(&CbCr_AC_Huffman_Table, 0, sizeof(CbCr_AC_Huffman_Table));
	build_HuffmanTable(Standard_AC_Chrominance_NRCodes, Standard_AC_Chrominance_Values, CbCr_AC_Huffman_Table);
}

JpegEncoder::BitString JpegEncoder::getBitCode(int value)
{
	BitString ret;
	int v = (value > 0) ? value : -value;
	int length = 0;
	for (length = 0; v; v >>= 1) length++;
	ret.value = value > 0 ? value : ((1 << length) + value - 1);
	ret.length = length;
	return ret;
}
void JpegEncoder::initQualityTable(int qualityFactor)
{
	if (qualityFactor <= 0) qualityFactor = 1;
	if (qualityFactor > 100) qualityFactor = 100;
	int scale;
	if (qualityFactor < 50)
		scale = 5000 / qualityFactor;
	else
		scale = 200 - qualityFactor * 2;

	for (int i = 0; i < 64; i++)
	{
		int temp = (Luminance_Quantization_Table[i] * scale + 50) / 100;
		if (temp <= 0) temp = 1;
		if (temp > 255) temp = 255;
		Ytable[ZigZag[i]] = (unsigned char)temp;

		temp = (Chrominance_Quantization_Table[i] * scale + 50) / 100;
		if (temp <= 0)  temp = 1;
		if (temp > 255) temp = 255;
		CbCrtable[ZigZag[i]] = (unsigned char)temp;
	}
}

void JpegEncoder::build_HuffmanTable(const char* nr_codes, const unsigned char* std_table, BitString* huffman_table)
{
	unsigned int code = 0;
	int pos = 0;

	for (int k = 1; k <= 16; k++)
	{
		int num_codes = nr_codes[k - 1];
		for (int i = 0; i < num_codes; i++)
		{
			unsigned char symbol = std_table[pos];
			huffman_table[symbol].value = code;
			huffman_table[symbol].length = k;
			code++;
			pos++;
		}
		code <<= 1;
	}
}

void JpegEncoder::write_byte(unsigned char value, FILE* file)
{
	write(&value, 1, file);
}
void JpegEncoder::write_word(unsigned short f_value, FILE* file)
{
	unsigned int value = ((f_value >> 8) & 0xFF) | ((f_value & 0xFF) << 8);
	write(&value, 2, file);
}
void JpegEncoder::write(const void* p, int byteSize, FILE* file)
{
	fwrite(p, 1, byteSize, file);
}


void JpegEncoder::huffmanCoding(const short* DU, short& prevDC, const BitString* HTDC, const BitString* HTAC,BitString* outputBitString, int& bitStringCounts)
{
	BitString EOB = HTAC[0x00];
	BitString SIXTEEN_ZEROS = HTAC[0xF0];

	int index = 0;

	int dcDiff = (int)(DU[0] - prevDC);  //DC 编码
	prevDC = DU[0];

	if (dcDiff == 0)
	{
		outputBitString[index++] = HTDC[0];
	}
	else
	{
		BitString res = getBitCode(dcDiff);
		outputBitString[index++] = HTDC[res.length];
		outputBitString[index++] = res;
	}

	int endpos = 63;
	while ((endpos > 0) && (DU[endpos] == 0)) endpos--;

	int i = 1;
	while (i <= endpos)
	{
		int zeroCounts = 0;
		while (i <= endpos && DU[i] == 0)
		{
			zeroCounts++;
			i++;
		}

		while (zeroCounts >= 16)
		{
			outputBitString[index++] = SIXTEEN_ZEROS;
			zeroCounts -= 16;
		}

		if (i > endpos)
		{
			break;
		}

		BitString res = getBitCode(DU[i]);

		outputBitString[index++] = HTAC[(zeroCounts << 4) | res.length];
		outputBitString[index++] = res;
		i++;
	}
	if (endpos != 63)
		outputBitString[index++] = EOB;
	bitStringCounts = index;
}

void JpegEncoder::write_bitString(const BitString* bs, int counts, int& newByte, int& newBytePos, FILE* file)
{
	for (int i = 0; i < counts; i++)
	{
		int value = bs[i].value;
		int length = bs[i].length;

		for (int j = length - 1; j >= 0; j--)
		{
			int bit = (value >> j) & 1;
			if (bit)
			{
				newByte |= (1 << newBytePos);
			}
			newBytePos--;
			if (newBytePos < 0)
			{
				write_byte((unsigned char)newByte, file);
				if (newByte == 0xFF)
				{
					write_byte(0x00, file);
				}
				newBytePos = 7;
				newByte = 0;
			}
		}
	}
}
void JpegEncoder::ConvertColor(int xPos, int yPos, char* yData, char* cbData, char* crData)
{
	for (int y = 0; y < 8; y++)
	{
		unsigned char* p = rgbbuffer + (y + yPos) * w * 3 + xPos * 3;
		for (int x = 0; x < 8; x++)
		{
			unsigned char B = *p++;
			unsigned char G = *p++;
			unsigned char R = *p++;

			double Y = 0.299f * R + 0.587f * G + 0.114f * B;
			double Cb = -0.168736f * R - 0.331264f * G + 0.5f * B;
			double Cr = 0.5f * R - 0.418688f * G - 0.081312f * B;

			yData[y * 8 + x] = (char)(Y - 128);
			cbData[y * 8 + x] = (char)Cb;
			crData[y * 8 + x] = (char)Cr;
		}
	}
}

void JpegEncoder::DCT(const char* block_data, short* Data)
{
	const double PI = 3.14159265358979323846;
	for (int v = 0; v < 8; v++)
	{
		for (int u = 0; u < 8; u++)
		{
			double alpha_u = (u == 0) ? 1 / sqrt(8.0f) : 0.5f;
			double alpha_v = (v == 0) ? 1 / sqrt(8.0f) : 0.5f;

			double temp = 0.f;
			for (int x = 0; x < 8; x++)
			{
				for (int y = 0; y < 8; y++)
				{
					double data = block_data[y * 8 + x];

					data *= cos((2 * x + 1) * u * PI / 16.0f);
					data *= cos((2 * y + 1) * v * PI / 16.0f);

					temp += data;
				}
			}
			temp *= alpha_u * alpha_v / Ytable[ZigZag[v * 8 + u]];
			Data[ZigZag[v * 8 + u]] = (short)((short)(temp + 16384.5) - 16384);
		}
	}
}


void JpegEncoder::write_header(FILE* file)
{
	//SOI
	write_word(0xFFD8, file);

	//APPO
	write_word(0xFFE0, file);
	write_word(16, file);
	write("JFIF", 5, file);
	write_byte(1, file);
	write_byte(1, file);
	write_byte(0, file);
	write_word(1, file);
	write_word(1, file);
	write_byte(0, file);
	write_byte(0, file);


	//DQT
	write_word(0xFFDB, file);
	write_word(132, file);
	write_byte(0, file);
	write(Ytable, 64, file);
	write_byte(1, file);
	write(CbCrtable, 64, file);

	//SOFO
	write_word(0xFFC0, file);
	write_word(17, file);
	write_byte(8, file);
	write_word(h & 0xFFFF, file);
	write_word(w & 0xFFFF, file);
	write_byte(3, file);

	write_byte(1, file);
	write_byte(0x11, file);
	write_byte(0, file);

	write_byte(2, file);
	write_byte(0x11, file);
	write_byte(1, file);

	write_byte(3, file);
	write_byte(0x11, file);
	write_byte(1, file);

	//DHT
	write_word(0xFFC4, file);
	write_word(0x01A2, file);
	write_byte(0, file);

	write(Standard_DC_Luminance_NRCodes, sizeof(Standard_DC_Luminance_NRCodes), file);
	write(Standard_DC_Luminance_Values, sizeof(Standard_DC_Luminance_Values), file);
	write_byte(0x10, file);
	write(Standard_AC_Luminance_NRCodes, sizeof(Standard_AC_Luminance_NRCodes), file);
	write(Standard_AC_Luminance_Values, sizeof(Standard_AC_Luminance_Values), file);
	write_byte(0x01, file);
	write(Standard_DC_Chrominance_NRCodes, sizeof(Standard_DC_Chrominance_NRCodes), file);
	write(Standard_DC_Chrominance_Values, sizeof(Standard_DC_Chrominance_Values), file);
	write_byte(0x11, file);
	write(Standard_AC_Chrominance_NRCodes, sizeof(Standard_AC_Chrominance_NRCodes), file);
	write(Standard_AC_Chrominance_Values, sizeof(Standard_AC_Chrominance_Values), file);


	//SOS
	write_word(0xFFDA, file);	
	write_word(12, file);
	write_byte(3, file);

	write_byte(1, file);
	write_byte(0, file);

	write_byte(2, file);
	write_byte(0x11, file);

	write_byte(3, file);
	write_byte(0x11, file);

	write_byte(0, file);
	write_byte(0x3F, file);
	write_byte(0, file);

}