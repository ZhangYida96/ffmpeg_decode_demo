#ifndef OPENCVIDEO_MASTER_H264DECODER_H
#define OPENCVIDEO_MASTER_H264DECODER_H

#include <stdio.h>
#include <unistd.h>
#include <iostream>

// Opencv
#include <opencv/cv.h>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

extern "C" {
//#include "libavutil/avutil.h"
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
//新版里的图像转换结构需要引入的头文件
#include "libswscale/swscale.h"
}

#if LIBAVCODEC_VERSION_INT < AV_VERSION_INT(55,28,1)
#define av_frame_alloc  avcodec_alloc_frame
#endif

class H264Decoder
{

public:
    FILE *fp_in;

    AVFormatContext *pFormatCtx;  //总的结构


    //解码相关
    AVCodec         *pCodec;
    AVCodecContext  *pCodecCtx;

    AVPacket        *packet;      //解码前数据
    AVFrame         *pFrame,*pFrameRGB;//解码后数据



    int numBytes;
    int videoindex;


    //读内存的方式解码
    const AVCodec *codec;
    AVCodecContext *c = nullptr;
    int frame_count;
    AVFrame *frame;
    AVPacket avpkt;
    AVFrame *pFrameBGR;

    int BGRsize;


    //YUV-BGR转换 两种方式公用的
    struct SwsContext *img_convert_ctx;
    unsigned char *out_buffer;

public :

    H264Decoder();
    //读取文件/网络
    int init();
    int decode(cv::Mat &mat_img);

    //从内存中读取
    int init_read();
    int decode_read(unsigned char *inputbuf, size_t size, cv::Mat &mat_img);


};

#endif //OPENCVIDEO_MASTER_H264DECODER_H
