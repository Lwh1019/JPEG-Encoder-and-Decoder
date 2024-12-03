#define _CRT_SECURE_NO_WARNINGS
#include "JPEG.h"
#include <bits/stdc++.h>
#include "JpegEncoder.h"
#include "JpegDecoder.h"
#include <jpeglib.h>
#include <QWidget>
#include <QPushButton>
#include <QVBoxLayout>
#include <QPainter>
#include <QFileDialog>
#include <QDebug>
#include <QThread>
#include <QProgressDialog>
#include <qmessagebox.h>
#include <QtConcurrent/QtConcurrent>
extern "C"
{
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
}

using namespace std;

QString filepath;
int qualityFactor = 10;
JPEG::JPEG(QWidget* parent)
    : QWidget(parent),
    ui(new Ui::JPEGClass)
{
    ui->setupUi(this);
    connect(ui->openvideo, &QPushButton::clicked, this, &JPEG::openVideo);
    connect(ui->getflash, &QPushButton::clicked, this, &JPEG::openflash);
    connect(ui->JPEG_B, &QPushButton::clicked, this, &JPEG::JPEGBian);
    connect(ui->JPEG_J, &QPushButton::clicked, this, &JPEG::JPEGJie);
    connect(ui->JPEG_B_2, &QPushButton::clicked, this, &JPEG::JPEGBian_dat);
    connect(ui->JPEG_J_2, &QPushButton::clicked, this, &JPEG::JPEGJie_dat);
    connect(ui->horizontalSlider, &QSlider::valueChanged, this, &JPEG::updateText);


    ui->horizontalSlider->setRange(0, 100);
    ui->horizontalSlider->setValue(10);
    qualityFactor = ui->horizontalSlider->value();

}
JPEG::~JPEG()
{
}
void JPEG::updateText()
{
    ui->value->setText(QString::number(ui->horizontalSlider->value()));
}
void JPEG::openVideo()
{
    filepath = QFileDialog::getOpenFileName(this, "选择视频文件", "", "视频文件 (*.mp4 *.avi *.mkv *.mov)");
    if (!filepath.isEmpty())
    {
        QMessageBox::information(this, "成功", "视频已加载！");
    }
    else
    {
        QMessageBox::warning(this, "错误", "未选中视频失败！");
    }
}
void saveFrameAsImage(AVFrame* frame, int width, int height, const QString& fileName)
{
    QImage image(frame->data[0], width, height, frame->linesize[0], QImage::Format_RGB888);
    image.save(fileName);
}
void JPEG::openflash()
{
    if (filepath.isEmpty())
    {
        QMessageBox::warning(this, "错误", "请先选中视频！");
        return;
    }

    QProgressDialog* progressDialog = new QProgressDialog("正在提取关键帧，请稍候...", nullptr, 0, 0, this);
    progressDialog->setWindowModality(Qt::WindowModal);
    progressDialog->setCancelButton(nullptr);
    progressDialog->setRange(0, 0);
    progressDialog->setValue(0);
    progressDialog->show();

    QtConcurrent::run([=]() {
        AVFormatContext* formatContext = avformat_alloc_context();
        if (avformat_open_input(&formatContext, filepath.toStdString().c_str(), nullptr, nullptr) != 0)
        {
            QMessageBox::warning(this, "错误", "无法打开视频文件！");
            return;
        }
        if (avformat_find_stream_info(formatContext, nullptr) < 0)
        {
            QMessageBox::warning(this, "错误", "无法找到视频流信息！");
            avformat_close_input(&formatContext);
            return;
        }

        int videoStreamIndex = -1;
        for (unsigned int i = 0; i < formatContext->nb_streams; i++)
        {
            if (formatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
            {
                videoStreamIndex = i;
                break;
            }
        }
        if (videoStreamIndex == -1)
        {
            QMessageBox::warning(this, "错误", "无法找到视频流信息！");
            avformat_close_input(&formatContext);
            return;
        }

        AVCodecParameters* codecParams = formatContext->streams[videoStreamIndex]->codecpar;
        const AVCodec* codec = avcodec_find_decoder(codecParams->codec_id);
        if (!codec) {
            QMessageBox::warning(this, "错误", "无法找到解码器！");
            avformat_close_input(&formatContext);
            return;
        }

        AVCodecContext* codecContext = avcodec_alloc_context3(codec);
        avcodec_parameters_to_context(codecContext, codecParams);
        if (avcodec_open2(codecContext, codec, nullptr) < 0)
        {
            QMessageBox::warning(this, "错误", "无法打开解码器！");
            avcodec_free_context(&codecContext);
            avformat_close_input(&formatContext);
            return;
        }

        AVPacket* packet = av_packet_alloc();
        AVFrame* frame = av_frame_alloc();
        AVFrame* rgbFrame = av_frame_alloc();

        SwsContext* swsContext = sws_getContext(codecParams->width, codecParams->height, codecContext->pix_fmt,
            codecParams->width, codecParams->height, AV_PIX_FMT_RGB24,
            SWS_BICUBIC, nullptr, nullptr, nullptr);

        int bufferSize = av_image_get_buffer_size(AV_PIX_FMT_RGB24, codecParams->width, codecParams->height, 1);
        uint8_t* buffer = (uint8_t*)av_malloc(bufferSize * sizeof(uint8_t));
        av_image_fill_arrays(rgbFrame->data, rgbFrame->linesize, buffer, AV_PIX_FMT_RGB24,
            codecParams->width, codecParams->height, 1);

        int frameIndex = 0;
        int totalFrames = formatContext->streams[videoStreamIndex]->duration;

        while (av_read_frame(formatContext, packet) >= 0)
        {
            if (packet->stream_index == videoStreamIndex) {
                if (avcodec_send_packet(codecContext, packet) == 0)
                {
                    while (avcodec_receive_frame(codecContext, frame) == 0)
                    {
                        if (frame->flags & AV_FRAME_FLAG_KEY)
                        {
                            sws_scale(swsContext, frame->data, frame->linesize, 0, codecParams->height,
                                rgbFrame->data, rgbFrame->linesize);
                            QString outputFileName = QString("frame%1.bmp").arg(++frameIndex);
                            saveFrameAsImage(rgbFrame, codecParams->width, codecParams->height, outputFileName);
                        }
                    }
                }
            }
            av_packet_unref(packet);
        }
        progressDialog->close();
        QMessageBox::information(this, "完成", "帧提取成功！");

        sws_freeContext(swsContext);
        av_free(buffer);
        av_frame_free(&rgbFrame);
        av_frame_free(&frame);
        av_packet_free(&packet);
        avcodec_free_context(&codecContext);
        avformat_close_input(&formatContext);
        });
}
int zigZagOrder[64] = {
    0, 1, 5, 6, 14, 15, 27, 28,
    2, 4, 7, 13, 16, 26, 29, 42,
    3, 8, 12, 17, 25, 30, 41, 43,
    9, 11, 18, 24, 31, 40, 44, 53,
    10, 19, 23, 32, 39, 45, 52, 54,
    20, 22, 33, 38, 46, 51, 55, 60,
    21, 34, 37, 47, 50, 56, 59, 61,
    35, 36, 48, 49, 57, 58, 62, 63 };
void ZigZagScan(const vector<vector<int>>& block, vector<int>& scannedData)
{
    scannedData.resize(64);
    for (int i = 0; i < 64; ++i)
    {
        int row = zigZagOrder[i] / 8;
        int col = zigZagOrder[i] % 8;
        scannedData[i] = block[row][col];
    }
}
void RunLengthEncode(const vector<int>& scannedData, vector<pair<int, int>>& rleData)
{
    int zeroCount = 0;
    for (size_t i = 1; i < scannedData.size(); ++i)
    {
        if (scannedData[i] == 0)
        {
            ++zeroCount;
        }
        else
        {
            while (zeroCount > 15)
            {
                rleData.push_back({ 15, 0 });
                zeroCount -= 16;
            }
            rleData.push_back({ zeroCount, scannedData[i] });
            zeroCount = 0;
        }
    }
    if (zeroCount > 0)
    {
        rleData.push_back({ 0, 0 });
    }
}
void RGBToYCbCr(unsigned char R, unsigned char G, unsigned char B, double& Y, double& Cb, double& Cr)
{
    Y = 0.299 * R + 0.587 * G + 0.114 * B;
    Cb = -0.1687 * R - 0.3313 * G + 0.5 * B + 128;
    Cr = 0.5 * R - 0.4187 * G - 0.0813 * B + 128;
}
void SubSample_420(const vector<vector<double>>& input, vector<vector<double>>& output)
{
    int inputHeight = input.size();
    int inputWidth = input[0].size();
    int outputHeight = output.size();
    int outputWidth = output[0].size();
    for (int i = 0; i < outputHeight; ++i)
    {
        for (int j = 0; j < outputWidth; ++j)
        {
            int srcI = i * 2;
            int srcJ = j * 2;
            output[i][j] = (input[srcI][srcJ] + input[srcI][srcJ + 1] + input[srcI + 1][srcJ] + input[srcI + 1][srcJ + 1]) / 4.0;
        }
    }
}
void DCT(const vector<vector<double>>& block, vector<vector<double>>& dctblock)
{
    int n = block.size();
    for (int u = 0; u < n; ++u)
    {
        for (int v = 0; v < n; ++v)
        {
            double sum = 0.0;
            for (int x = 0; x < n; ++x)
            {
                for (int y = 0; y < n; ++y)
                {
                    sum += block[x][y] * cos((2 * x + 1) * u * M_PI / (2 * n)) * cos((2 * y + 1) * v * M_PI / (2 * n));
                }
            }
            double cu = (u == 0) ? sqrt(1.0 / n) : sqrt(2.0 / n);
            double cv = (v == 0) ? sqrt(1.0 / n) : sqrt(2.0 / n);
            dctblock[u][v] = cu * cv * sum;
        }
    }
}
void IDCT(const vector<vector<double>>& dctBlock, vector<vector<double>>& block)
{
    int N = dctBlock.size();
    for (int x = 0; x < N; x++)
    {
        for (int y = 0; y < N; y++)
        {
            double sum = 0.0;
            for (int u = 0; u < N; ++u)
            {
                for (int v = 0; v < N; ++v)
                {
                    double cu = (u == 0) ? sqrt(1.0 / N) : sqrt(2.0 / N);
                    double cv = (v == 0) ? sqrt(1.0 / N) : sqrt(2.0 / N);
                    sum += cu * cv * dctBlock[u][v] * cos((2 * x + 1) * u * M_PI / (2 * N)) * cos((2 * y + 1) * v * M_PI / (2 * N));
                }
            }
            block[x][y] = sum;
        }
    }
}
int light[8][8] =
{
    {16, 11, 10, 16, 24, 40, 51, 61},
    {12, 12, 14, 19, 26, 58, 60, 55},
    {14, 13, 16, 24, 40, 57, 69, 56},
    {14, 17, 22, 29, 51, 87, 80, 62},
    {18, 22, 37, 56, 68, 109, 103, 77},
    {24, 35, 55, 64, 81, 104, 113, 92},
    {49, 64, 78, 87, 103, 121, 120, 101},
    {72, 92, 95, 98, 112, 100, 103, 99} };
int color[8][8] =
{
    {17, 18, 24, 47, 99, 99, 99, 99},
    {18, 21, 26, 66, 99, 99, 99, 99},
    {24, 26, 56, 99, 99, 99, 99, 99},
    {47, 66, 99, 99, 99, 99, 99, 99},
    {99, 99, 99, 99, 99, 99, 99, 99},
    {99, 99, 99, 99, 99, 99, 99, 99},
    {99, 99, 99, 99, 99, 99, 99, 99},
    {99, 99, 99, 99, 99, 99, 99, 99} };
void AdjustQuantizationTable(int baseTable[8][8], int adjustedTable[8][8], int qualityFactor)
{
    for (int i = 0; i < 8; ++i)
    {
        for (int j = 0; j < 8; ++j)
        {
            adjustedTable[i][j] = std::max(1, std::min(255, (baseTable[i][j] * 50 + qualityFactor / 2) / qualityFactor));
        }
    }
}
void Quantize(const vector<vector<double>>& dctBlock, vector<vector<int>>& quantizedBlock, int quantTable[8][8])
{
    for (int i = 0; i < 8; i++)
    {
        for (int j = 0; j < 8; j++)
        {
            quantizedBlock[i][j] = round(dctBlock[i][j] / quantTable[i][j]);
        }
    }
}
void DeQuantize(const vector<vector<int>>& quantizedBlock, vector<vector<double>>& dctBlock, int quantTable[8][8])
{
    for (int i = 0; i < 8; ++i)
    {
        for (int j = 0; j < 8; ++j)
        {
            dctBlock[i][j] = quantizedBlock[i][j] * quantTable[i][j];
        }
    }
}
void JPEG::JPEGBian()
{
    QString filePath = QFileDialog::getOpenFileName(this, "选择帧图像", "", "图像文件 (*.bmp)");
    if (filePath.isEmpty())
    {
        QMessageBox::warning(this, "错误", "未选择图像文件！");
        return;
    }
    JpegEncoder encoder;
    encoder.read(filePath.toLocal8Bit().constData());
    QString savePath = QFileDialog::getSaveFileName(this, "保存编码图像", "", "图像文件 (*.jpg)");
    qualityFactor = ui->horizontalSlider->value();
    if ( qualityFactor < 1 || qualityFactor > 100)
    {
        QMessageBox::warning(this, "无效输入", "请输入有效的质量因子（1~100）。");
        return;
    }
    encoder.encode(savePath.toLocal8Bit().constData(), qualityFactor);
    QMessageBox::information(this, "完成", "JPG图像已保存！");
}
void JPEG::JPEGJie()
{
    QString filePath = QFileDialog::getOpenFileName(this, "选择图像", "", "图像文件 (*.jpg)");
    if (filePath.isEmpty())
    {
        QMessageBox::warning(this, "错误", "未选择图像文件！");
        return;
    }
    JpegDecoder decoder;
    decoder.read(filePath.toLocal8Bit().constData());
    QString savePath = QFileDialog::getSaveFileName(this, "保存编码图像", "", "图像文件 (*.bmp)");
    qualityFactor = ui->horizontalSlider->value();
    if (qualityFactor < 1 || qualityFactor > 100)
    {
        QMessageBox::warning(this, "无效输入", "请输入有效的质量因子（1~100）。");
        return;
    }
    decoder.decode(savePath.toLocal8Bit().constData());
    QMessageBox::information(this, "完成", "BMP图像已保存！");
}

void JPEG::JPEGBian_dat()
{
    qualityFactor = ui->horizontalSlider->value();
    if (qualityFactor < 1 || qualityFactor > 100)
    {
        QMessageBox::warning(this, "无效输入", "请输入有效的质量因子（1~100）。");
        return;
    }
    QString filePath = QFileDialog::getOpenFileName(this, "选择帧图像", "", "图像文件 (*.png *.bmp)");
    if (filePath.isEmpty())
    {
        QMessageBox::warning(this, "错误", "未选择图像文件！");
        return;
    }

    QImage image(filePath);
    if (image.isNull())
    {
        QMessageBox::warning(this, "错误", "图像加载失败！");
        return;
    }

    image = image.convertToFormat(QImage::Format_RGB888);
    int width = image.width();
    int height = image.height();

    int newWidth = (width + 15) / 16 * 16;
    int newHeight = (height + 15) / 16 * 16;

    QImage paddedImage(newWidth, newHeight, QImage::Format_RGB888);
    paddedImage.fill(Qt::black);
    QPainter painter(&paddedImage);
    painter.drawImage(0, 0, image);
    painter.end();

    vector<vector<double>> Y(newHeight, vector<double>(newWidth));
    vector<vector<double>> Cb(newHeight, vector<double>(newWidth));
    vector<vector<double>> Cr(newHeight, vector<double>(newWidth));

    for (int y = 0; y < newHeight; ++y)
    {
        for (int x = 0; x < newWidth; ++x)
        {
            QRgb pixel = paddedImage.pixel(x, y);
            unsigned char R = qRed(pixel);
            unsigned char G = qGreen(pixel);
            unsigned char B = qBlue(pixel);
            double yVal, cbVal, crVal;
            RGBToYCbCr(R, G, B, yVal, cbVal, crVal);
            Y[y][x] = yVal - 128; // 级别偏移
            Cb[y][x] = cbVal;
            Cr[y][x] = crVal;
        }
    }

    int subsampledHeight = newHeight / 2;
    int subsampledWidth = newWidth / 2;
    vector<vector<double>> CbSub(subsampledHeight, vector<double>(subsampledWidth));
    vector<vector<double>> CrSub(subsampledHeight, vector<double>(subsampledWidth));
    SubSample_420(Cb, CbSub);
    SubSample_420(Cr, CrSub);

    vector<vector<int>> quantizedDataY;
    vector<vector<int>> quantizedDataCb;
    vector<vector<int>> quantizedDataCr;

    for (int yBlock = 0; yBlock < newHeight; yBlock += 8)
    {
        for (int xBlock = 0; xBlock < newWidth; xBlock += 8)
        {
            vector<vector<double>> block(8, vector<double>(8));
            for (int y = 0; y < 8; ++y)
            {
                for (int x = 0; x < 8; ++x)
                {
                    block[y][x] = Y[yBlock + y][xBlock + x];
                }
            }
            vector<vector<double>> dctBlock(8, vector<double>(8));
            DCT(block, dctBlock);

            int adjustedLight[8][8];
            int adjustedColor[8][8];
            AdjustQuantizationTable(light, adjustedLight, qualityFactor);
            AdjustQuantizationTable(color, adjustedColor, qualityFactor);

            vector<vector<int>> quantizedBlock(8, vector<int>(8));
            Quantize(dctBlock, quantizedBlock, adjustedLight);

            vector<int> scannedData;
            ZigZagScan(quantizedBlock, scannedData);
            quantizedDataY.push_back(scannedData);
        }
    }

    for (int yBlock = 0; yBlock < subsampledHeight; yBlock += 8)
    {
        for (int xBlock = 0; xBlock < subsampledWidth; xBlock += 8)
        {
            vector<vector<double>> block(8, vector<double>(8));
            for (int y = 0; y < 8; ++y)
            {
                for (int x = 0; x < 8; ++x)
                {
                    block[y][x] = CbSub[yBlock + y][xBlock + x];
                }
            }
            vector<vector<double>> dctBlock(8, vector<double>(8));
            DCT(block, dctBlock);

            int adjustedLight[8][8];
            int adjustedColor[8][8];
            AdjustQuantizationTable(light, adjustedLight, qualityFactor);
            AdjustQuantizationTable(color, adjustedColor, qualityFactor);

            vector<vector<int>> quantizedBlock(8, vector<int>(8));
            Quantize(dctBlock, quantizedBlock, adjustedColor);

            vector<int> scannedData;
            ZigZagScan(quantizedBlock, scannedData);
            quantizedDataCb.push_back(scannedData);
        }
    }

    for (int yBlock = 0; yBlock < subsampledHeight; yBlock += 8)
    {
        for (int xBlock = 0; xBlock < subsampledWidth; xBlock += 8)
        {
            vector<vector<double>> block(8, vector<double>(8));
            for (int y = 0; y < 8; ++y)
            {
                for (int x = 0; x < 8; ++x)
                {
                    block[y][x] = CrSub[yBlock + y][xBlock + x];
                }
            }
            vector<vector<double>> dctBlock(8, vector<double>(8));
            DCT(block, dctBlock);

            int adjustedLight[8][8];
            int adjustedColor[8][8];
            AdjustQuantizationTable(light, adjustedLight, qualityFactor);
            AdjustQuantizationTable(color, adjustedColor, qualityFactor);

            vector<vector<int>> quantizedBlock(8, vector<int>(8));
            Quantize(dctBlock, quantizedBlock, adjustedColor);

            vector<int> scannedData;
            ZigZagScan(quantizedBlock, scannedData);
            quantizedDataCr.push_back(scannedData);
        }
    }

    QString saveFilePath = QFileDialog::getSaveFileName(this, "保存编码数据", "", "编码数据文件 (*.dat)");
    if (saveFilePath.isEmpty())
    {
        QMessageBox::warning(this, "错误", "未选择保存路径！");
        return;
    }
    QFile file(saveFilePath);
    if (!file.open(QIODevice::WriteOnly))
    {
        QMessageBox::warning(this, "错误", "无法打开保存文件！");
        return;
    }
    QDataStream out(&file);
    out.setVersion(QDataStream::Qt_5_9);

    out << width << height;

    out << newWidth << newHeight;

    int yBlockCount = quantizedDataY.size();
    out << yBlockCount;

    for (const auto& block : quantizedDataY)
    {
        for (int val : block)
        {
            out << val;
        }
    }

    int cbBlockCount = quantizedDataCb.size();
    out << cbBlockCount;

    for (const auto& block : quantizedDataCb)
    {
        for (int val : block)
        {
            out << val;
        }
    }

    int crBlockCount = quantizedDataCr.size();
    out << crBlockCount;

    for (const auto& block : quantizedDataCr)
    {
        for (int val : block)
        {
            out << val;
        }
    }

    file.close();

    QMessageBox::information(this, "完成", "编码数据已保存！");
}
void JPEG::JPEGJie_dat()
{
    qualityFactor = ui->horizontalSlider->value();
    if (qualityFactor < 1 || qualityFactor > 100)
    {
        QMessageBox::warning(this, "无效输入", "请输入有效的质量因子（1~100）。");
        return;
    }
    QString encodedFilePath = QFileDialog::getOpenFileName(this, "选择编码数据文件", "", "编码数据文件 (*.dat)");
    if (encodedFilePath.isEmpty())
    {
        QMessageBox::warning(this, "错误", "未选择编码数据文件！");
        return;
    }

    QFile file(encodedFilePath);
    if (!file.open(QIODevice::ReadOnly))
    {
        QMessageBox::warning(this, "错误", "无法打开编码数据文件！");
        return;
    }
    QDataStream in(&file);
    in.setVersion(QDataStream::Qt_5_9);

    int width, height;
    in >> width >> height;

    int newWidth, newHeight;
    in >> newWidth >> newHeight;

    int yBlockCount;
    in >> yBlockCount;

    vector<vector<int>> quantizedDataY(yBlockCount, vector<int>(64));
    for (int i = 0; i < yBlockCount; ++i)
    {
        for (int j = 0; j < 64; ++j)
        {
            in >> quantizedDataY[i][j];
        }
    }

    int cbBlockCount;
    in >> cbBlockCount;

    vector<vector<int>> quantizedDataCb(cbBlockCount, vector<int>(64));
    for (int i = 0; i < cbBlockCount; ++i)
    {
        for (int j = 0; j < 64; ++j)
        {
            in >> quantizedDataCb[i][j];
        }
    }

    int crBlockCount;
    in >> crBlockCount;

    vector<vector<int>> quantizedDataCr(crBlockCount, vector<int>(64));
    for (int i = 0; i < crBlockCount; ++i)
    {
        for (int j = 0; j < 64; ++j)
        {
            in >> quantizedDataCr[i][j];
        }
    }

    file.close();

    vector<vector<double>> Y(newHeight, vector<double>(newWidth));
    int subsampledHeight = newHeight / 2;
    int subsampledWidth = newWidth / 2;
    vector<vector<double>> CbSub(subsampledHeight, vector<double>(subsampledWidth));
    vector<vector<double>> CrSub(subsampledHeight, vector<double>(subsampledWidth));

    int index = 0;
    for (int yBlock = 0; yBlock < newHeight; yBlock += 8)
    {
        for (int xBlock = 0; xBlock < newWidth; xBlock += 8)
        {
            vector<int> scannedData = quantizedDataY[index++];
            vector<vector<int>> quantizedBlock(8, vector<int>(8));
            for (int i = 0; i < 64; ++i)
            {
                int row = zigZagOrder[i] / 8;
                int col = zigZagOrder[i] % 8;
                quantizedBlock[row][col] = scannedData[i];
            }
            int adjustedLight[8][8];
            int adjustedColor[8][8];
            AdjustQuantizationTable(light, adjustedLight, qualityFactor);
            AdjustQuantizationTable(color, adjustedColor, qualityFactor);

            vector<vector<double>> dequantizedBlock(8, vector<double>(8));
            DeQuantize(quantizedBlock, dequantizedBlock, adjustedLight);

            vector<vector<double>> block(8, vector<double>(8));
            IDCT(dequantizedBlock, block);

            for (int y = 0; y < 8; ++y)
            {
                for (int x = 0; x < 8; ++x)
                {
                    Y[yBlock + y][xBlock + x] = block[y][x];
                }
            }
        }
    }

    index = 0;
    for (int yBlock = 0; yBlock < subsampledHeight; yBlock += 8)
    {
        for (int xBlock = 0; xBlock < subsampledWidth; xBlock += 8)
        {
            vector<int> scannedData = quantizedDataCb[index++];
            vector<vector<int>> quantizedBlock(8, vector<int>(8));
            for (int i = 0; i < 64; ++i)
            {
                int row = zigZagOrder[i] / 8;
                int col = zigZagOrder[i] % 8;
                quantizedBlock[row][col] = scannedData[i];
            }
            int adjustedLight[8][8];
            int adjustedColor[8][8];
            AdjustQuantizationTable(light, adjustedLight, qualityFactor);
            AdjustQuantizationTable(color, adjustedColor, qualityFactor);
            vector<vector<double>> dequantizedBlock(8, vector<double>(8));
            DeQuantize(quantizedBlock, dequantizedBlock, adjustedColor);

            vector<vector<double>> block(8, vector<double>(8));
            IDCT(dequantizedBlock, block);

            for (int y = 0; y < 8; ++y)
            {
                for (int x = 0; x < 8; ++x)
                {
                    CbSub[yBlock + y][xBlock + x] = block[y][x];
                }
            }
        }
    }

    index = 0;
    for (int yBlock = 0; yBlock < subsampledHeight; yBlock += 8)
    {
        for (int xBlock = 0; xBlock < subsampledWidth; xBlock += 8)
        {
            vector<int> scannedData = quantizedDataCr[index++];
            vector<vector<int>> quantizedBlock(8, vector<int>(8));
            for (int i = 0; i < 64; ++i)
            {
                int row = zigZagOrder[i] / 8;
                int col = zigZagOrder[i] % 8;
                quantizedBlock[row][col] = scannedData[i];
            }
            int adjustedLight[8][8];
            int adjustedColor[8][8];
            AdjustQuantizationTable(light, adjustedLight, qualityFactor);
            AdjustQuantizationTable(color, adjustedColor, qualityFactor);
            vector<vector<double>> dequantizedBlock(8, vector<double>(8));
            DeQuantize(quantizedBlock, dequantizedBlock, adjustedColor);

            vector<vector<double>> block(8, vector<double>(8));
            IDCT(dequantizedBlock, block);

            for (int y = 0; y < 8; ++y)
            {
                for (int x = 0; x < 8; ++x)
                {
                    CrSub[yBlock + y][xBlock + x] = block[y][x];
                }
            }
        }
    }

    vector<vector<double>> Cb(newHeight, vector<double>(newWidth));
    vector<vector<double>> Cr(newHeight, vector<double>(newWidth));
    for (int y = 0; y < newHeight; ++y)
    {
        for (int x = 0; x < newWidth; ++x)
        {
            Cb[y][x] = CbSub[y / 2][x / 2];
            Cr[y][x] = CrSub[y / 2][x / 2];
        }
    }

    QImage resultImage(newWidth, newHeight, QImage::Format_RGB888);
    for (int y = 0; y < newHeight; ++y)
    {
        for (int x = 0; x < newWidth; ++x)
        {
            double Y_value = Y[y][x] + 128;
            double Cb_value = Cb[y][x] - 128;
            double Cr_value = Cr[y][x] - 128;

            int R = std::clamp(static_cast<int>(Y_value + 1.402 * Cr_value), 0, 255);
            int G = std::clamp(static_cast<int>(Y_value - 0.344136 * Cb_value - 0.714136 * Cr_value), 0, 255);
            int B = std::clamp(static_cast<int>(Y_value + 1.772 * Cb_value), 0, 255);

            resultImage.setPixel(x, y, qRgb(R, G, B));
        }
    }

    QImage finalImage = resultImage.copy(0, 0, width, height);

    QString savePath = QFileDialog::getSaveFileName(this, "保存解码图像", "", "图像文件 (*.bmp)");
    if (!savePath.isEmpty())
    {
        if (finalImage.save(savePath))
        {
            QMessageBox::information(this, "完成", "图像已解码并保存！");
        }
        else
        {
            QMessageBox::warning(this, "错误", "保存解码图像失败！");
        }
    }
}