#define _CRT_SECURE_NO_WARNINGS
#include "JpegDecoder.h"

namespace {
    // 标准 DC 亮度 Huffman 表
    const unsigned char Standard_DC_Luminance_NRCodes[] = { 0, 0, 1, 5, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0 };
    const unsigned char Standard_DC_Luminance_Values[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 };

    // 标准 DC 色度 Huffman 表
    const unsigned char Standard_DC_Chrominance_NRCodes[] = { 0, 0, 3, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0 };
    const unsigned char Standard_DC_Chrominance_Values[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 };

    // 标准 AC 亮度 Huffman 表
    const unsigned char Standard_AC_Luminance_NRCodes[] = { 0, 2, 1, 3, 3, 2, 4, 3, 5, 5, 4, 4, 0, 0, 1, 0x7d };
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
        0x7a, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88,
        0x89, 0x8a, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97,
        0x98, 0x99, 0x9a, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6,
        0xa7, 0xa8, 0xa9, 0xaa, 0xb2, 0xb3, 0xb4, 0xb5,
        0xb6, 0xb7, 0xb8, 0xb9, 0xba, 0xc2, 0xc3, 0xc4,
        0xc5, 0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xd2, 0xd3,
        0xd4, 0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xda, 0xe2,
        0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9, 0xea,
        0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8, 0xf9,
        0xfa
    };

    // 标准 AC 色度 Huffman 表
    const unsigned char Standard_AC_Chrominance_NRCodes[] = { 0, 2, 1, 2, 4, 4, 3, 4, 7, 5, 4, 4, 0, 1, 2, 0x77 };
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
}

JpegDecoder::JpegDecoder()
    : w(0), h(0), rgbbuffer(nullptr), numComponents(0)
{
    memset(Ytable, 0, sizeof(Ytable));
    memset(CbCrtable, 0, sizeof(CbCrtable));
    initializeHuffmanTables();
}


JpegDecoder::~JpegDecoder()
{
    clean();
}


void JpegDecoder::clean()
{
    if (rgbbuffer)
    {
        delete[] rgbbuffer;
        rgbbuffer = nullptr;
    }
    w = 0;
    h = 0;
    numComponents = 0;
}

// 初始化标准 Huffman 表
void JpegDecoder::initializeHuffmanTables()
{
    buildHuffmanTable(Standard_DC_Luminance_NRCodes, Standard_DC_Luminance_Values, Y_DC_Huffman_Table);
    buildHuffmanTable(Standard_AC_Luminance_NRCodes, Standard_AC_Luminance_Values, Y_AC_Huffman_Table);
    buildHuffmanTable(Standard_DC_Chrominance_NRCodes, Standard_DC_Chrominance_Values, CbCr_DC_Huffman_Table);
    buildHuffmanTable(Standard_AC_Chrominance_NRCodes, Standard_AC_Chrominance_Values, CbCr_AC_Huffman_Table);
}

// 构建 Huffman 表
void JpegDecoder::buildHuffmanTable(const unsigned char* bits, const unsigned char* huffval, HuffmanTable& table)
{
    int p = 0;
    unsigned int code = 0;
    int si = 1;

    for (int l = 1; l <= 16; l++)
    {
        int num_codes = bits[l - 1];
        for (int i = 0; i < num_codes; i++)
        {
            if (p >= 256)
            {
                return;
            }
            table.huffsize[p++] = l;
        }
    }
    table.huffsize[p] = 0;

    p = 0;
    code = 0;
    si = table.huffsize[0];
    if (si == 0)
    {
        return;
    }

    while (table.huffsize[p])
    {
        while (table.huffsize[p] == si)
        {
            if (p >= 256)
            {
                return;
            }
            table.huffcode[p] = code;
            code++;
            p++;
            if (p >= 256) break;
        }
        code <<= 1; 
        si++;
        if (si > 16) break;
    }

    copy(huffval, huffval + p, table.huffval);

    int idx = 0;
    for (int l = 1; l <= 16; l++)
    {
        int num_codes = bits[l - 1];
        if (num_codes > 0)
        {
            table.valptr[l] = idx;
            table.mincode[l] = table.huffcode[idx];
            table.maxcode[l] = table.huffcode[idx + num_codes - 1];
            idx += num_codes;
        }
        else
        {
            table.valptr[l] = -1;
            table.mincode[l] = -1;
            table.maxcode[l] = -1;
        }
    }
    table.maxcode[17] = 0xFFFFF; 
}
int JpegDecoder::huffmanExtend(int v, int t)
{
    if (t == 0) return 0;
    int vt = 1 << (t - 1);
    if (v < vt)
        return v + (-1 << t) + 1;
    else
        return v;
}
int JpegDecoder::readBits(FILE* file, int nbits, int& bitBuffer, int& bitCount)
{
    int v = 0;
    while (nbits > 0)
    {
        if (bitCount == 0)
        {
            int c = fgetc(file);
            if (c == EOF)
            {
                return -1;
            }
            if (c == 0xFF)
            {
                int next = fgetc(file);
                if (next != 0x00)
                {
                    return -1;
                }
            }
            bitBuffer = c;
            bitCount = 8;
        }
        v = (v << 1) | ((bitBuffer >> (bitCount - 1)) & 1);
        bitCount--;
        nbits--;
    }
    return v;
}

int JpegDecoder::decodeHuffmanSymbol(FILE* file, HuffmanTable& htable, int& bitBuffer, int& bitCount)
{
    int code = 0;
    int l = 1;
    while (l <= 16)
    {
        if (bitCount == 0)
        {
            int c = fgetc(file);
            if (c == EOF)
            {
                return -1;
            }
            if (c == 0xFF)
            {
                int next = fgetc(file);
                if (next != 0x00)
                {
                    return -1;
                }
            }
            bitBuffer = c;
            bitCount = 8;
        }
        code = (code << 1) | ((bitBuffer >> (bitCount - 1)) & 1);
        bitCount--;

        if (code >= htable.mincode[l] && code <= htable.maxcode[l])
        {
            int index = htable.valptr[l] + (code - htable.mincode[l]);
            return htable.huffval[index];
        }
        l++;
    }
    return -1;
}

void JpegDecoder::reorderZigZag(short* block)
{
    const char ZigZagOrder[64] =
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

    short reordered[64];
    for (int i = 0; i < 64; i++)
    {
        reordered[i] = block[ZigZagOrder[i]];
    }
    memcpy(block, reordered, sizeof(reordered));
}

bool JpegDecoder::decodeBlock(FILE* file, HuffmanTable& htdc, HuffmanTable& htac, int& prevDC, unsigned char* qtable, short* block, int& bitBuffer, int& bitCount)
{
    cout << "Decoding new block..." << endl;
    // 解码 DC 系数
    int s = decodeHuffmanSymbol(file, htdc, bitBuffer, bitCount);
    if (s == -1)
    {
        cout << "Failed to decode DC coefficient." << endl;
        return false;
    }
    int diff = 0;
    if (s != 0)
    {
        int bits = readBits(file, s, bitBuffer, bitCount);
        if (bits == -1) return false;
        diff = huffmanExtend(bits, s);
    }
    int DC = prevDC + diff;
    prevDC = DC;
    block[0] = DC;

    // 解码 AC 系数
    int k = 1;
    while (k < 64)
    {
        int rs = decodeHuffmanSymbol(file, htac, bitBuffer, bitCount);
        if (rs == -1) return false;
        if (rs == 0)
        {
            // EOB
            while (k < 64)
            {
                block[k++] = 0;
            }
            break;
        }
        int r = rs >> 4;
        int s_ac = rs & 0x0F;
        if (s_ac == 0)
        {
            // ZLE
            k += 16;
            if (k >= 64)
            {
                cout << "Failed to decode AC coefficient at position k=" << k << endl;
                return false;
            }
            continue;
        }
        k += r;
        if (k >= 64)
        {
            cout << "Failed to decode AC coefficient at position k=" << k << endl;
            return false;
        }
        int bits = readBits(file, s_ac, bitBuffer, bitCount);
        if (bits == -1) return false;
        int ac = huffmanExtend(bits, s_ac);
        block[k++] = ac;
    }
    cout << "Block decoded successfully." << endl;
    return true;
}

void JpegDecoder::dequantize(short* block, unsigned char* qtable)
{
    for (int i = 0; i < 64; i++)
    {
        block[i] *= qtable[i];
    }
}

void JpegDecoder::inverseDCT(short* block, double* fblock)
{
    const double PI = 3.14159265358979323846;
    for (int y = 0; y < 8; y++)
    {
        for (int x = 0; x < 8; x++)
        {
            double sum = 0.0;
            for (int v = 0; v < 8; v++)
            {
                for (int u = 0; u < 8; u++)
                {
                    double Cu = (u == 0) ? (1.0 / sqrt(2.0)) : 1.0;
                    double Cv = (v == 0) ? (1.0 / sqrt(2.0)) : 1.0;
                    double dctCoeff = block[v * 8 + u];
                    sum += Cu * Cv * dctCoeff *
                        cos(((2 * x + 1) * u * PI) / 16.0) *
                        cos(((2 * y + 1) * v * PI) / 16.0);
                }
            }
            fblock[y * 8 + x] = sum / 4.0;
        }
    }
}

// 升采样（双线性插值）
void JpegDecoder::upsampleChroma(double* chroma, unsigned char* upsampledChroma, int origWidth, int origHeight, int targetWidth, int targetHeight)
{
    for (int y = 0; y < targetHeight; y++)
    {
        for (int x = 0; x < targetWidth; x++)
        {

            double srcX = ((double)x / targetWidth) * origWidth;
            double srcY = ((double)y / targetHeight) * origHeight;

            int x0 = floor(srcX);
            int x1 = min(x0 + 1, origWidth - 1);
            int y0 = floor(srcY);
            int y1 = min(y0 + 1, origHeight - 1);

            double dx = srcX - x0;
            double dy = srcY - y0;
            double val = (1 - dx) * (1 - dy) * chroma[y0 * origWidth + x0] +
                dx * (1 - dy) * chroma[y0 * origWidth + x1] +
                (1 - dx) * dy * chroma[y1 * origWidth + x0] +
                dx * dy * chroma[y1 * origWidth + x1];

            upsampledChroma[y * targetWidth + x] = static_cast<unsigned char>(min(max((int)(val + 0.5), 0), 255));
        }
    }
}
bool JpegDecoder::parseJPEGFile(FILE* file)
{
    unsigned char marker1 = fgetc(file);
    unsigned char marker2 = fgetc(file);
    if (marker1 != 0xFF || marker2 != 0xD8)
    {
        cout << "dd" << endl;
        return false;
    }

    // 读取标记
    bool done = false;
    while (!done)
    {
        // 读取下一个标记
        int marker = 0;
        while (true)
        {
            marker1 = fgetc(file);
            if (marker1 != 0xFF)
            {
                cout << "ee" << endl;
                return false;
            }
            marker2 = fgetc(file);
            if (marker2 != 0xFF)
            {
                marker = 0xFF00 | marker2;
                break;
            }
        }

        switch (marker)
        {
        case 0xFFDB: // DQT
        {
            unsigned short length = (fgetc(file) << 8) | fgetc(file);
            length -= 2;
            while (length > 0)
            {
                unsigned char qtInfo = fgetc(file);
                length--;

                unsigned char* qtable = (qtInfo & 0x0F) == 0 ? Ytable : CbCrtable;
                if ((qtInfo >> 4) == 0)
                {
                    fread(qtable, 1, 64, file);
                    length -= 64;
                }
                else
                {
                    return false;
                }
            }
        }
        break;

        case 0xFFC0: // SOF0
        {
            unsigned short length = (fgetc(file) << 8) | fgetc(file);
            unsigned char precision = fgetc(file);

            h = (fgetc(file) << 8) | fgetc(file);
            w = (fgetc(file) << 8) | fgetc(file);

            numComponents = fgetc(file);
            if (numComponents > 3)
            {
                cout << "xx" << endl;
                return false;
            }

            for (int i = 0; i < numComponents; i++)
            {
                components[i].id = fgetc(file);
                unsigned char sampling = fgetc(file);
                components[i].h = (sampling >> 4) & 0x0F;
                components[i].v = sampling & 0x0F;
                components[i].quantTable = fgetc(file);
            }

            length -= 8 + numComponents * 3;
            if (length > 0)
            {
                fseek(file, length, SEEK_CUR);
            }
        }
        break;

        case 0xFFC4: // DHT
        {
            unsigned short length = (fgetc(file) << 8) | fgetc(file);
            length -= 2;
            while (length > 0)
            {
                unsigned char htInfo = fgetc(file);
                length--;

                unsigned char bits[16];
                fread(bits, 1, 16, file);
                length -= 16;

                int totalValues = 0;
                for (int i = 0; i < 16; i++)
                {
                    totalValues += bits[i];
                }

                unsigned char* huffval = new unsigned char[totalValues];
                fread(huffval, 1, totalValues, file);
                length -= totalValues;

                if ((htInfo & 0x10) == 0) // DC Huffman table
                {
                    if ((htInfo & 0x0F) == 0)
                    {
                        buildHuffmanTable(bits, huffval, Y_DC_Huffman_Table);
                    }
                    else
                    {
                        buildHuffmanTable(bits, huffval, CbCr_DC_Huffman_Table);
                    }
                }
                else // AC Huffman table
                {
                    if ((htInfo & 0x0F) == 0)
                    {
                        buildHuffmanTable(bits, huffval, Y_AC_Huffman_Table);
                    }
                    else
                    {
                        buildHuffmanTable(bits, huffval, CbCr_AC_Huffman_Table);
                    }
                }

                delete[] huffval;
            }
        }
        break;

        case 0xFFDA: // SOS
        {
            unsigned short length = (fgetc(file) << 8) | fgetc(file);
            unsigned char numComponentsScan = fgetc(file);
            if (numComponentsScan != numComponents)
            {
                cout << "zz" << endl;
                return false;
            }

            // 读取每个扫描组件的配置
            for (int i = 0; i < numComponentsScan; i++)
            {
                unsigned char id = fgetc(file);
                unsigned char table = fgetc(file);
            }

            unsigned char spectralStart = fgetc(file);
            unsigned char spectralEnd = fgetc(file);
            unsigned char successiveApprox = fgetc(file);

            if (spectralStart != 0 || spectralEnd != 63 || successiveApprox != 0)
            {
                cout << "vv" << endl;
                return false;
            }

            if (!decodeImageData(file))
            {
                cout << "yy" << endl;
                return false;
            }

            done = true;
        }
        break;

        case 0xFFD9: // EOI
        {
            done = true;
        }
        break;

        default:
        {
            unsigned short length = (fgetc(file) << 8) | fgetc(file);
            length -= 2;
            fseek(file, length, SEEK_CUR);
        }
        break;
        }
    }

    return true;
}

bool JpegDecoder::decodeImageData(FILE* file)
{
    // 确定最大水平和垂直采样因子
    int maxH = 1, maxV = 1;
    for (int i = 0; i < numComponents; i++)
    {
        if (components[i].h > maxH)
            maxH = components[i].h;
        if (components[i].v > maxV)
            maxV = components[i].v;
    }

    // 计算 MCU 的数量
    int MCU_width = 8 * maxH;
    int MCU_height = 8 * maxV;

    int MCU_count_x = (w + MCU_width - 1) / MCU_width;
    int MCU_count_y = (h + MCU_height - 1) / MCU_height;

    // 计算色度分量的尺寸
    int CbCr_h = 8 * components[1].h;
    int CbCr_v = 8 * components[1].v;
    int CbCr_width = MCU_count_x * CbCr_h;
    int CbCr_height = MCU_count_y * CbCr_v;

    // 分配 Y、Cb、Cr 缓冲区
    unsigned char* Y_buffer = new unsigned char[MCU_count_x * MCU_width * MCU_count_y * MCU_height];
    unsigned char* Cb_buffer = new unsigned char[CbCr_width * CbCr_height];
    unsigned char* Cr_buffer = new unsigned char[CbCr_width * CbCr_height];
    if (!Y_buffer || !Cb_buffer || !Cr_buffer)
    {
        cout << "nn" << endl;
        delete[] Y_buffer;
        delete[] Cb_buffer;
        delete[] Cr_buffer;
        return false;
    }
    memset(Y_buffer, 0, MCU_count_x * MCU_width * MCU_count_y * MCU_height);
    memset(Cb_buffer, 0, CbCr_width * CbCr_height);
    memset(Cr_buffer, 0, CbCr_width * CbCr_height);

    // 初始化前一个 DC 系数
    int prevDC_Y = 0;
    int prevDC_Cb = 0;
    int prevDC_Cr = 0;

    int bitBuffer = 0;
    int bitCount = 0;

    // 逐个 MCU 进行解码
    for (int y = 0; y < MCU_count_y; y++)
    {
        for (int x = 0; x < MCU_count_x; x++)
        {
            // 解码每个组件
            for (int c = 0; c < numComponents; c++)
            {
                HuffmanTable& htdc = (c == 0) ? Y_DC_Huffman_Table : CbCr_DC_Huffman_Table;
                HuffmanTable& htac = (c == 0) ? Y_AC_Huffman_Table : CbCr_AC_Huffman_Table;
                unsigned char* qtable = (c == 0) ? Ytable : CbCrtable;

                short block[64];
                memset(block, 0, sizeof(block));
                int& prevDC = (c == 0) ? prevDC_Y : ((c == 1) ? prevDC_Cb : prevDC_Cr);

                if (!decodeBlock(file, htdc, htac, prevDC, qtable, block, bitBuffer, bitCount))
                {
                    cout << "mm" << endl;
                    delete[] Y_buffer;
                    delete[] Cb_buffer;
                    delete[] Cr_buffer;
                    return false;
                }
                reorderZigZag(block);

                dequantize(block, qtable);

                double fblock[64];
                inverseDCT(block, fblock);

                // 写入 Y、Cb、Cr 缓冲区
                if (c == 0) // Y 组件
                {
                    for (int i = 0; i < 64; i++)
                    {
                        int xx = x * components[c].h * 8 + (i % 8) * components[c].h;
                        int yy = y * components[c].v * 8 + (i / 8) * components[c].v;
                        for (int hv = 0; hv < components[c].h; hv++)
                        {
                            for (int vv = 0; vv < components[c].v; vv++)
                            {
                                if ((xx + hv) < (MCU_count_x * MCU_width) && (yy + vv) < (MCU_count_y * MCU_height))
                                {
                                    Y_buffer[(yy + vv) * MCU_count_x * MCU_width + (xx + hv)] =
                                        static_cast<unsigned char>(min(max((int)(fblock[i] + 128.0), 0), 255));
                                }
                            }
                        }
                    }
                }
                else // Cb 或 Cr 组件
                {
                    for (int i = 0; i < 64; i++)
                    {
                        int xx = x * components[c].h * 8 + (i % 8) * components[c].h;
                        int yy = y * components[c].v * 8 + (i / 8) * components[c].v;
                        for (int hv = 0; hv < components[c].h; hv++)
                        {
                            for (int vv = 0; vv < components[c].v; vv++)
                            {
                                if ((xx + hv) < CbCr_width && (yy + vv) < CbCr_height)
                                {
                                    if (c == 1) // Cb
                                        Cb_buffer[(yy + vv) * CbCr_width + (xx + hv)] =
                                        static_cast<unsigned char>(min(max((int)(fblock[i] + 128.0), 0), 255));
                                    else // Cr
                                        Cr_buffer[(yy + vv) * CbCr_width + (xx + hv)] =
                                        static_cast<unsigned char>(min(max((int)(fblock[i] + 128.0), 0), 255));
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    // 升采样 Cb 和 Cr 分量到与 Y 相同的分辨率
    unsigned char* upsampledCb = new unsigned char[w * h];
    unsigned char* upsampledCr = new unsigned char[w * h];
    if (!upsampledCb || !upsampledCr)
    {
        cout << "kk" << endl;
        delete[] Y_buffer;
        delete[] Cb_buffer;
        delete[] Cr_buffer;
        delete[] upsampledCb;
        delete[] upsampledCr;
        return false;
    }

    // 判断色度分量是否需要升采样
    if (components[1].h == maxH && components[1].v == maxV)
    {
        // 4:4:4，不需要升采样
        memcpy(upsampledCb, Cb_buffer, w * h);
        memcpy(upsampledCr, Cr_buffer, w * h);
    }
    else
    {
        // 需要升采样
        upsampleChroma((double*)Cb_buffer, upsampledCb, CbCr_width, CbCr_height, w, h);
        upsampleChroma((double*)Cr_buffer, upsampledCr, CbCr_width, CbCr_height, w, h);
    }

    rgbbuffer = new unsigned char[w * h * 3];
    if (!rgbbuffer)
    {
        cout << "ll" << endl;
        delete[] Y_buffer;
        delete[] Cb_buffer;
        delete[] Cr_buffer;
        delete[] upsampledCb;
        delete[] upsampledCr;
        return false;
    }

    // 转换 YCbCr 到 RGB
    for (int y = 0; y < h; y++)
    {
        for (int x = 0; x < w; x++)
        {
            int index = (y * w + x) * 3;

            double Y_val = (double)Y_buffer[y * w + x];
            double Cb = (double)upsampledCb[y * w + x] - 128.0;
            double Cr = (double)upsampledCr[y * w + x] - 128.0;

            double R = Y_val + 1.402 * Cr;
            double G = Y_val - 0.344136 * Cb - 0.714136 * Cr;
            double B = Y_val + 1.772 * Cb;

            R = min(max(R, 0.0), 255.0);
            G = min(max(G, 0.0), 255.0);
            B = min(max(B, 0.0), 255.0);

            rgbbuffer[index + 0] = static_cast<unsigned char>(B);
            rgbbuffer[index + 1] = static_cast<unsigned char>(G);
            rgbbuffer[index + 2] = static_cast<unsigned char>(R);
        }
    }
    return true;
}
bool JpegDecoder::read(const char* filename)
{
    clean();
    FILE* file = fopen(filename, "rb");
    if (!file)
    {
        cout << "ii" << filename << endl;
        return false;
    }
    bool result = parseJPEGFile(file);
    fclose(file);
    return result;
}

bool JpegDecoder::decode(const char* filename)
{
    if (!rgbbuffer || w == 0 || h == 0)
    {
        cout << "pp" << endl;
        return false;
    }

    FILE* file = fopen(filename, "wb");
    if (!file)
    {
        cout << "无法创建文件：" << filename << endl;
        return false;
    }

#pragma pack(push, 2)
    typedef struct
    {
        unsigned short bfType;
        unsigned int bfSize;
        unsigned short bfReserved1;
        unsigned short bfReserved2;
        unsigned int bfOffBits;
    } BITMAPFILEHEADER;

    typedef struct
    {
        unsigned int biSize;
        int biWidth;
        int biHeight;
        unsigned short biPlanes;
        unsigned short biBitCount;
        unsigned int biCompression;
        unsigned int biSizeImage;
        int biXPelsPerMeter;
        int biYPelsPerMeter;
        unsigned int biClrUsed;
        unsigned int biClrImportant;
    } BITMAPINFOHEADER;
#pragma pack(pop)

    BITMAPFILEHEADER fileHeader;
    BITMAPINFOHEADER infoHeader;

    fileHeader.bfType = 0x4D42; 
    fileHeader.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
    fileHeader.bfReserved1 = 0;
    fileHeader.bfReserved2 = 0;
    fileHeader.bfSize = fileHeader.bfOffBits + w * h * 3;

    infoHeader.biSize = sizeof(BITMAPINFOHEADER);
    infoHeader.biWidth = w;
    infoHeader.biHeight = -h; 
    infoHeader.biPlanes = 1;
    infoHeader.biBitCount = 24;
    infoHeader.biCompression = 0;
    infoHeader.biSizeImage = w * h * 3;
    infoHeader.biXPelsPerMeter = 0;
    infoHeader.biYPelsPerMeter = 0;
    infoHeader.biClrUsed = 0;
    infoHeader.biClrImportant = 0;

    fwrite(&fileHeader, sizeof(fileHeader), 1, file);
    fwrite(&infoHeader, sizeof(infoHeader), 1, file);

    fwrite(rgbbuffer, 1, w * h * 3, file);

    fclose(file);
    return true;
}
