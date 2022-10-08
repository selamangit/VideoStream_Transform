#ifndef PUBLISH_STREAM_H
#define PUBLISH_STREAM_H

#define __STDC_CONSTANT_MACROS

extern "C"
{
    #include <libavdevice/avdevice.h>
    #include <libavformat/avformat.h>
    #include <libavcodec/avcodec.h>
    #include <libavutil/imgutils.h>
    #include <libswscale/swscale.h>
    #include <libavutil/frame.h>
    #include <libavutil/opt.h>
    #include <libavutil/time.h>
}

#include <opencv2/highgui.hpp>  //OpenCV highgui模块头文件
#include <opencv2/imgproc.hpp>  //OpenCV 图像处理头文件
#include <string>
#include <iostream>

using namespace std;

class AVPusher
{
    public:
        explicit AVPusher();
        ~AVPusher();
    public:
        bool isconnected_;
        int videostreamindex_;
        int audiostreamindex_;

        AVDictionary *pIfmtOpt_ = NULL;
        AVDictionary *pOfmtOpt_ = NULL;

        AVFormatContext *pIfmtCtx_ = NULL;
        AVFormatContext *pOfmtCtx_ = NULL;
        // 视频编解码器
        AVCodecContext *pVideoEncodecCtx_ = NULL;
        AVCodecContext *pVideoDecodecCtx_ = NULL;
        // 音频编解码器（先不做）
        AVCodecContext *pAudioEncodecCtx_ = NULL;
        AVCodecContext *pAudioDecodecCtx_ = NULL;

        int findlocalstreaminfo(string filename);
        AVCodecContext* createvideodecoderctx(AVStream *AvSt);
        AVCodecContext* createvideoencoderctx(AVStream *AvSt);
        
        int connect(string dist);
        int pushstream();
        int createinstreamdecodec();
        int createoutstreamencodec();
        int decodepackettoframe(AVFrame* outframe ,AVCodecContext* decodecctx, AVPacket pkt);
        int sendstream(AVPacket *pkt);
        int releaseobj();

};
#endif