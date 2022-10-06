#include "publish_stream.h"

#define FILE_NAME "../data/Iphone14_trailer.mp4"
#define FILE_NAME1 "../data/China_trailer.mp4"
#define RTSP_URL "rtsp://10.33.96.166:8554/mystream"

int main()
{
    AVPusher *avpusher_ = new AVPusher();
    int ret;
    // av_register_all();(弃用已久)
    avformat_network_init();

    if((ret = avpusher_->findlocalstreaminfo(FILE_NAME1)) < 0)
    {
        delete avpusher_;
        fprintf(stderr,"findlocalstreaminfo failed!");
        return 0;
    }
    avpusher_->connect(RTSP_URL);
    avpusher_->pushstream();
    return 0;
}
