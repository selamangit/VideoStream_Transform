#include "publish_stream.h"

using namespace std;

AVPusher::AVPusher():isconnected_(false)
{
    
}

AVPusher::~AVPusher()
{

}

static double r2d(AVRational r)
{
	return r.num == 0 || r.den == 0 ? 0. : (double)r.num / (double)r.den;
}

int AVPusher::findlocalstreaminfo(string filename)
{
    int ret;
    // av_register_all();

    // avformat_network_init();

    // 打开封装格式，解封文件头
    fprintf(stdout,"avformat_open_input ……\n");
    if((ret = avformat_open_input(&pIfmtCtx_, filename.c_str(), NULL, NULL)) != 0)
    {
        // 如果pIfmtCtx_指向NULL,则avformat_open_input会为pIfmtCtx_分配空间并填入参数
        releaseobj();
        fprintf(stderr, "avformat_open_input failed\n");
        return -1;
    }
    fprintf(stdout,"avformat_open_input successed\n");
    // 获取流信息
    fprintf(stdout,"avformat_find_stream_info ……\n");
    if((ret = avformat_find_stream_info(pIfmtCtx_,0)) < 0)
    {
        releaseobj();
        fprintf(stderr, "avformat_find_stream_info failed\n");
        return -1;
    }
    fprintf(stdout,"avformat_find_stream_info successed\n");
    

    fprintf(stdout,"dump Inputcontext info  ……\n");
    av_dump_format(pIfmtCtx_, 0, filename.c_str(), 0);
    

    // 找到视频流的索引
    for(uint16_t i = 0; i < pIfmtCtx_->nb_streams; i++)
    {
        if(pIfmtCtx_->streams[i]->codec->codec_type = AVMEDIA_TYPE_VIDEO)
        {
            videostreamindex_ = i;
            break;
        }
    }

    AVStream *pIAvSt = pIfmtCtx_->streams[videostreamindex_];
    fprintf(stdout,"createvideodecoderctx ……\n");
    pVideoDecodecCtx_ = createvideodecoderctx(pIAvSt);
    if(pVideoDecodecCtx_ == nullptr)
    {
        releaseobj();
        fprintf(stderr,"createvideodecoderctx failed\n");
        return -1;
    }
    fprintf(stdout,"createvideodecoderctx successed\n");

    fprintf(stdout,"createvideoencoderctx ……\n");
    pVideoEncodecCtx_ = createvideoencoderctx(pIAvSt);
    if(pVideoEncodecCtx_ == nullptr)
    {
        releaseobj();
        fprintf(stderr,"createvideoencoderctx failed\n");
        return -1;
    }
    fprintf(stdout,"createvideoencoderctx successed\n");

}

// 配置编码器信息
AVCodecContext* AVPusher::createvideoencoderctx(AVStream *AvSt)
{
    fprintf(stdout,"avcodec_find_encoder ……\n");
    // 可以使用libx264编码器
    // const AVCodec* VideoEnCodec = avcodec_find_encoder_by_name("libx264");
    const AVCodec* VideoEnCodec  = avcodec_find_encoder(AV_CODEC_ID_H264);
    if(VideoEnCodec == nullptr)
    {
        fprintf(stderr,"avcodec_find_encoder failed\n");
        return nullptr;
    }
    fprintf(stdout,"avcodec_find_encoder successed\n");
    fprintf(stdout,"avcodec_alloc_context3 ……\n");
    AVCodecContext *VideoEncodecCtx = avcodec_alloc_context3(VideoEnCodec);
    if(VideoEncodecCtx == nullptr)
    {
        fprintf(stderr,"avcodec_alloc_context3 failed\n");
        return nullptr;
    }
    fprintf(stdout,"avcodec_alloc_context3 successed\n");
    /*
    avcodec_parameters_to_context(VideoEncodecCtx, AvSt->codecpar);
    不能再复制编码器的参数信息了，因为封装格式没有编码器信息？
    */
    VideoEncodecCtx->codec_type = AVMEDIA_TYPE_VIDEO;
    VideoEncodecCtx->pix_fmt = AV_PIX_FMT_YUV420P;
    VideoEncodecCtx->width = AvSt->codecpar->width;
    VideoEncodecCtx->height = AvSt->codecpar->height;
    VideoEncodecCtx->bit_rate = AvSt->codecpar->bit_rate;
    // libx264的平均码率可能是3000k
    // VideoEncodecCtx->bit_rate = 1024000*3; 
    // gop_size不知道是啥玩意，每2帧一个I帧
    VideoEncodecCtx->gop_size = 2;
    VideoEncodecCtx->framerate = {AvSt->r_frame_rate.num, AvSt->r_frame_rate.den};
    VideoEncodecCtx->time_base = {AvSt->time_base.num, AvSt->time_base.den};
    // |=是什么意思？这个似乎很重要
    VideoEncodecCtx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER; 
    // 一篇对编码器参数讲解比较多的文章
    // https://blog.csdn.net/qq_34940879/article/details/103808025
    av_opt_set(VideoEncodecCtx->priv_data, "lowres", "0", 0);  //强制低分辨率
    fprintf(stdout,"avcodec_open2 ……\n");
    if(avcodec_open2(VideoEncodecCtx,VideoEnCodec,NULL) < 0)
    {
        avcodec_free_context(&VideoEncodecCtx);
        fprintf(stdout,"avcodec_open2 failed\n");
        return nullptr;
    }
    fprintf(stdout,"avcodec_open2 successed\n");
    return VideoEncodecCtx;
    /*
    只有在复用即编码时才需要用户自己操作
    但目前版本在编码输出时，仍需要通过旧的codec自定义初始化后
    调用avcodec_parameters_from_context给新的赋值。
    */
}
// 配置解码器信息
// 有人说不需要在5.8的动态库版本中
// 解复用调用avformat_open_input的时候已经从流中获取到编解码器context
// 所以不需要再使用avcodec_parameters_to_context来拷贝一遍
AVCodecContext* AVPusher::createvideodecoderctx(AVStream *AvSt)
{
    fprintf(stdout,"avcodec_find_decoder ……\n");
    const AVCodec *VideoDeCodec = avcodec_find_decoder(AvSt->codecpar->codec_id);
    if(VideoDeCodec == nullptr)
    {
        fprintf(stderr,"avcodec_find_decoder failed\n");
        return nullptr;
    }
    fprintf(stdout,"avcodec_find_decoder successed\n");
    fprintf(stdout,"avcodec_alloc_context3 ……\n");
    AVCodecContext *VideoDecodecCtx = avcodec_alloc_context3(VideoDeCodec);
    if(VideoDecodecCtx == nullptr)
    {
        fprintf(stderr,"avcodec_alloc_context3 failed\n");
        return nullptr;
    }
    fprintf(stdout,"avcodec_alloc_context3 successed\n");
    // 将输入的解码器参数复制过来
    avcodec_parameters_to_context(VideoDecodecCtx, AvSt->codecpar);
    // 配置解码器时间基
    // num和den是什么？
    VideoDecodecCtx->time_base = {AvSt->time_base.num, AvSt->time_base.den};
    // 将VideoDeCodec解码器配置进VideoDecodecCtx容器管理
    fprintf(stdout,"avcodec_open2 ……\n");
    if(avcodec_open2(VideoDecodecCtx, VideoDeCodec, NULL) < 0)
    {
        fprintf(stderr,"avcodec_open2 failed\n");
        avcodec_free_context(&VideoDecodecCtx);
        return nullptr;
    }
    fprintf(stdout,"avcodec_open2 successed\n");
    return VideoDecodecCtx;
}

int AVPusher::connect(string dist)
{
    int ret;
    // 配置输出流
    // 分配输出上下文空间
    fprintf(stdout,"avformat_alloc_output_context2 ……\n");

    if((ret = avformat_alloc_output_context2(&pOfmtCtx_, NULL, "RTSP", dist.c_str())) < 0)
    {
        releaseobj();
        fprintf(stderr, "avformat_alloc_output_context2 failed\n");
        return -1;
    }
    fprintf(stdout,"avformat_alloc_output_context2 successed\n");
    pOfmtCtx_->max_interleave_delta = 1000000;
    pOfmtCtx_->max_delay = 1000000;


    // 遍历pIfmtCtx_（输入封装协议）的AVStream
    for(uint16_t i = 0; i < pIfmtCtx_->nb_streams; i++)
    {
        // 只做了视频的编码
        if(pIfmtCtx_->streams[i]->codecpar->codec_type != AVMEDIA_TYPE_VIDEO)
        {
            continue;
        }
        // 创建输出流
        AVStream *pOAvSt = avformat_new_stream(pOfmtCtx_, nullptr);        
        if(!pOAvSt)
        {
            fprintf(stderr, "avformat_new_stream failed\n");
            return -1;
        }
        fprintf(stdout,"avformat_new_stream successed\n");
        // 为输出流复制编码器的配置信息
        fprintf(stdout,"avcodec_parameters_from_context ……\n");

        if((ret = avcodec_parameters_from_context(pOAvSt->codecpar, pVideoEncodecCtx_)) < 0)
        {
            fprintf(stderr, "avcodec_parameters_from_context failed\n");
            return -1;
        }
        fprintf(stdout,"avcodec_parameters_from_context successed\n");
        pOAvSt->codec->codec_tag = 0;
        pOAvSt->time_base = pIfmtCtx_->streams[i]->time_base;
        pOAvSt->avg_frame_rate = pIfmtCtx_->streams[i]->avg_frame_rate;
        // if(pOfmtCtx_->oformat->flags & AVFMT_GLOBALHEADER)
        // {
        //     pOAvSt->codec->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
        // }
        
    }

    fprintf(stdout,"dump Outputcontext info  ……\n");
    av_dump_format(pOfmtCtx_, 0, dist.c_str(), 1);
    // (这样写是不可以的)
    // 当结构体定义AVFMT_NOFILE，将不需要调用avio_open和avio_close对I/O进行操作
    /*ret = avio_open(&pOfmtCtx_->pb, dist.c_str(), AVIO_FLAG_WRITE);

    if(!pOfmtCtx_->pb)
    {
        fprintf(stderr, "avio_open failed\n");
        return -1;
    }*/
    
    // (这样写是可以的，可以看到rtsp服务器里面有连接信息，也可以直接配置rtsp推流参数)
    // flags中已经设置AVFMT_NOFILE类型，则，不要设置pb值。
    // 因为复用器将按照自身的方式对I/O进行处理，而这个字段设置为空

    /*
    当我们调用av_register_all接口时，便会将ff_rtsp_demuxer 注册进来，可以看到flags设置为AVFMT_NOFILE
    rtsp不在协议族里面，因此是找不到URLProtocol的，因此不需要avio_open去配置URL协议
    rtsp的协议解析在avformat_alloc_output_context2就完成了
    */
    if(!(pOfmtCtx_->oformat->flags & AVFMT_NOFILE))
    {
        fprintf(stdout,"avio_open ……\n");
        // rtsp不在协议族里面，因此是找不到URLProtocol的，需要avio_open去配置URL协议
        ret = avio_open(&pOfmtCtx_->pb, dist.c_str(), AVIO_FLAG_WRITE);
        if(!pOfmtCtx_->pb)
        {
            fprintf(stderr, "avio_open failed\n");
            return -1;
        }
    }
    else{
		fprintf(stdout,"not !(ofmtCtx_->flags & AVFMT_NOFILE)---------\n");
	}

    char sdp[2048];
    ret = av_sdp_create(&pOfmtCtx_,1, sdp, sizeof(sdp));
	if(ret != 0)
    {
        printf("sdp:error");
		return -1;
    }
	printf("SDP:\n %s\n",sdp);

    // 配置rtsp推流参数，配置上TCP之后RTP数据包以TCP的方式传输，可以在rtsp服务器端看到
    // 如果不指定tcp将以udp的形式推流
    av_dict_set(&pOfmtOpt_, "stimeout", std::to_string(2 * 1000000).c_str(), 0);
    av_dict_set(&pOfmtOpt_, "rtsp_transport", "tcp", 0);

    pOfmtCtx_->video_codec_id = pOfmtCtx_->oformat->video_codec;

    // 写入文件头信息，此时与服务器连接起来了
    fprintf(stdout, "avformat_write_header ……\n");
    if((ret = avformat_write_header(pOfmtCtx_, &pOfmtOpt_)) < 0)
    {
        fprintf(stderr, "avformat_write_header failed\n");
        return -1;
    }
    fprintf(stdout, "avformat_write_header successed\n");
    isconnected_ = true;
}
int AVPusher::decodepackettoframe(AVFrame* outframe ,AVCodecContext* decodecctx, AVPacket pkt)
{
    int ret;
    if((ret = avcodec_send_packet(decodecctx, &pkt)) != 0)
    {
        return ret;
    }

    ret = avcodec_receive_frame(decodecctx, outframe);
    return ret;
}

int AVPusher::pushstream()
{
    if(!isconnected_)
    {
        fprintf(stderr, "have not connected\n");
        return -1;
    }
    int ret;
    // int srcH=1080,srcW=1920;
	// int img_size = srcW*srcH;
    // int linesize[4]={0};
    // uint8_t* data[4];
    // av_image_alloc(data, linesize, srcW, srcH, AV_PIX_FMT_BGR24, 1);
	// cv::Mat img=cv::Mat(cv::Size(srcW,srcH),CV_8UC3,data[0]);

    // uint64_t frame_index = 0;s
    AVPacket pkt;
    // SwsContext *ConvertCtxYUV2BGR;
    AVFrame *pOutFrame = av_frame_alloc();
    long long startTime = av_gettime();
    while(true)
    {
        if((ret = av_read_frame(pIfmtCtx_, &pkt)) < 0)
        {
            fprintf(stderr, "av_read_frame failed\n");
            av_packet_unref(&pkt);
            break;
        }        
        // 只发送视频packet
        if(pkt.stream_index != videostreamindex_)
        {
            av_packet_unref(&pkt);
            continue;
        }
        // 从输入的封装格式的packet中解码出原数据（YUV420）
        // if(pVideoDecodecCtx_ == nullptr)
        // {
        //     cout<<"1234"<<endl;
        //     return 0;
        // }
        ret = decodepackettoframe(pOutFrame, pVideoDecodecCtx_, pkt);

        av_packet_unref(&pkt);
        if(ret != 0)
        {
            continue;
        }
        // sws_scale(ConvertCtxYUV2BGR, (uint8_t const * const *)pOutFrame->data, pOutFrame->linesize, 0, srcH, data, linesize);

        // 编码frame发送packet
        AVPacket *sendpkt;
        sendpkt = av_packet_alloc();
        ret = avcodec_send_frame(pVideoEncodecCtx_, pOutFrame);
        if(ret == 0)
        {
            while(avcodec_receive_packet(pVideoEncodecCtx_, sendpkt) >= 0)
            {

                ret = sendstream(sendpkt);
            }
        }
        av_packet_free(&sendpkt);
    }
    fprintf(stdout, "pushstream done\n");
    return 0;
}
int calculatepts()
{
    // cout<<"pkt.pts:"<<pkt.pts<<endl;
    // // 如果pkt中的是没有封装格式的裸流（例如H.264裸流）是不包含PTS、DTS这些参数的。
    // if(pkt.pts == AV_NOPTS_VALUE)
    // {
    //     // 在发送这种数据的时候，需要自己计算并写入AVPacket的PTS，DTS，duration等参数。
    //     // 配置输入的封装格式的tb作为输出的tb
    //     AVRational tb = pIfmtCtx_->streams[videostreamindex_]->time_base;
    //     int64_t duration = (double)AV_TIME_BASE/av_q2d(pIfmtCtx_->streams[videostreamindex_]->r_frame_rate);

    // }
    // // pts dts是什么
    // // cout<<pkt.pts<<" "<<flush;

    // // 计算转换pts dts

    // AVRational itime = pIfmtCtx_->streams[pkt.stream_index]->time_base;
    // AVRational otime = pOfmtCtx_->streams[pkt.stream_index]->time_base;
    // // 不同封装格式具有不同的时间基，在转封装(将一种封装格式转换为另一种封装格式)过程中，时间基转换
    // pkt.pts = av_rescale_q_rnd(pkt.pts, itime, otime, (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
    // pkt.dts = av_rescale_q_rnd(pkt.dts, itime, otime, (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
    // pkt.duration = av_rescale_q(pkt.duration, itime, otime);
    // pkt.pos = -1;

    // // 视频帧推送速度
    // if(pIfmtCtx_->streams[pkt.stream_index]->codecpar->codec_type = AVMEDIA_TYPE_VIDEO)
    // {
    //     AVRational tb = pIfmtCtx_->streams[pkt.stream_index]->time_base;
    //     // 已经过去的时间
    //     long long now = av_gettime() - startTime;
    //     long long dts = 0;
    //     dts = pkt.dts * (1000 * 1000 * r2d(tb));
    //     if (dts > now)
    //         av_usleep(dts - now);
    // }
}
int AVPusher::sendstream(AVPacket *pkt)
{
    // 推送帧数据，多个流需要av_interleaved_write_frame
    // cout<<"1234"<<endl;
    if(pOfmtCtx_ ==nullptr)
    {
        return -1;
    }
    if(av_interleaved_write_frame(pOfmtCtx_, pkt))
    {
        return 0;
    }

}
int AVPusher::releaseobj()
{
    if(pIfmtCtx_ != nullptr)
    {
        avformat_close_input(&pIfmtCtx_);
    }
    if(pOfmtCtx_ != nullptr)
    {
        avformat_close_input(&pOfmtCtx_);
    }
    if(pVideoEncodecCtx_ != nullptr)
    {
        avcodec_free_context(&pVideoEncodecCtx_);
    }
    if(pVideoDecodecCtx_ != nullptr)
    {
        avcodec_free_context(&pVideoDecodecCtx_);
    }
}
