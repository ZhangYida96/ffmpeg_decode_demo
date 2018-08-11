#include "H264Decoder.h"



H264Decoder::H264Decoder()
{
    init();
//  init_read();
}

int H264Decoder::init()
{

  pCodecCtx= NULL;
//  char filepath[1024] = "rtsp://184.72.239.149/vod/mp4://BigBuckBunny_175k.mov";
//  char filepath[1024] = "/home/only/264rawFrame/outputFrame0";
  char filepath[1024] = "udp://226.0.0.80:8001";
//  char filepath[1024] = "/home/only/test.265";
  av_register_all(); //初始化FFMPEG

  avformat_network_init();

  pFormatCtx = avformat_alloc_context();

  std::cout<<"avformat_network_init.. \n";

//    AVDictionary* options = NULL;
//    av_dict_set(&options, "buffer_size", "102400", 0); //设置缓存大小，1080p可将值调大
//    av_dict_set(&options, "rtsp_transport", "tcp", 0); //以udp方式打开，如果以tcp方式打开将udp替换为tcp
//    av_dict_set(&options, "stimeout", "2000000", 0); //设置超时断开连接时间，单位微秒
//    av_dict_set(&options, "max_delay", "500000", 0); //设置最大时延

  //=================================打开网络流或文件流 ============================//
  if (avformat_open_input(&pFormatCtx, filepath, NULL, NULL) != 0)
  {
      std::cout<<"Couldn't open input stream \n";
      return 0;
  }
  //===================================查找码流信息================================//
  if (avformat_find_stream_info(pFormatCtx, NULL)<0)
  {
      std::cout<<"Couldn't find stream information";
      return 0;
  }
  //=============================查找码流中是否有视频流=============================//
  videoindex = -1;
  for (int i = 0; i<pFormatCtx->nb_streams; i++)  //对码流中的每个elements
  {
      if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
      {
          videoindex = i;
          break;
      }
  }
  if (videoindex == -1)
  {
      printf("Didn't find a video stream.\n");
      return 0;
  }


  //=================================  查找解码器 ===================================//
  pCodecCtx = pFormatCtx->streams[videoindex]->codec;

  pCodec = avcodec_find_decoder(pCodecCtx->codec_id);

  if (pCodec == NULL)
  {
      printf("Codec not found.\n");
      return 0;
  }

  //================================  打开解码器 ===================================//
  if (avcodec_open2(pCodecCtx, pCodec, NULL)<0)
  {
      printf("Could not open codec.\n");
      return 0;
  }

  //==================================== 分配空间 ==================================//
  pFrame    = av_frame_alloc();
  pFrameRGB = av_frame_alloc();
  //一帧图像数据大小
  numBytes = avpicture_get_size(AV_PIX_FMT_BGR24, pCodecCtx->width,pCodecCtx->height);
  out_buffer = (unsigned char *) av_malloc(numBytes * sizeof(unsigned char));


  //会将pFrameRGB的数据按RGB格式自动"关联"到buffer  即pFrameRGB中的数据改变了 out_buffer中的数据也会相应的改变
  avpicture_fill((AVPicture *)pFrameRGB, out_buffer, AV_PIX_FMT_BGR24, pCodecCtx->width, pCodecCtx->height);

  //Output Info---输出一些文件信息
  printf("---------------- File Information ---------------\n");
  av_dump_format(pFormatCtx, 0, filepath, 0);
  printf("-------------------------------------------------\n");

  //================================ 设置数据转换参数 ================================//

  img_convert_ctx = sws_getContext(pCodecCtx->width, pCodecCtx->height, pCodecCtx->pix_fmt,
      pCodecCtx->width, pCodecCtx->height, AV_PIX_FMT_BGR24, SWS_BICUBIC, NULL, NULL, NULL);

  //=========================== 分配AVPacket结构体 ===============================//
  int y_size = pCodecCtx->width * pCodecCtx->height;
  packet = (AVPacket *) malloc(sizeof(AVPacket)); //分配一个packet
  av_new_packet(packet, y_size); //分配packet的数据


}

int H264Decoder::decode(cv::Mat &mat_img)
{

  //===========================  读取视频信息 ===============================//
  if (av_read_frame(pFormatCtx, packet) < 0) //读取的是一帧视频  数据存入一个AVPacket的结构中
  {
//      std::cout  << "no read frame..\n" ;
      return 0;
  }
  //此时数据存储在packet中

  //=========================== 对视频数据进行解码 ===============================//


  int got_picture;
  if (packet->stream_index == videoindex)
  {
      //视频解码函数  解码之后的数据存储在 pFrame中
      int ret = avcodec_decode_video2(pCodecCtx, pFrame, &got_picture, packet);
      if (ret < 0)
      {
          std::cout  << "avcodec_decode_video2 no find \n" ;
          return 0;
      }
      std::cout<<"pCodecCtx:"<<"\n";

      //=========================== YUV=>RGB ===============================//
      if (got_picture)
      {
          //转换一帧图像
          sws_scale(img_convert_ctx,(const uint8_t *const *)pFrame->data, pFrame->linesize, 0, pCodecCtx->height,  //源
                    pFrameRGB->data, pFrameRGB->linesize);                                 //目的


          //生成相同大小的Mat
          mat_img.create(cv::Size(pCodecCtx->width, pCodecCtx->height), CV_8UC3);
          memcpy(mat_img.data, (uchar *)out_buffer, numBytes);
          std::cout<<"get one pic... \n";
          return 1;
      }
      else
      {
        return 0;
      }
  }
  else
  {
    std::cout<<"no decode... \n";
    return 0;
  }



}

int H264Decoder::init_read()
{

  avcodec_register_all();
  av_init_packet(&avpkt);

  //选择解码器
  codec = avcodec_find_decoder(AV_CODEC_ID_H264);
//  codec = avcodec_find_decoder(AV_CODEC_ID_HEVC);


  if (!codec) {
      fprintf(stderr, "Codec not found\n");
      exit(1);
  }
  c = avcodec_alloc_context3(codec);
  if (!c) {
      fprintf(stderr, "Could not allocate video codec context\n");
      exit(1);
  }

  if (avcodec_open2(c, codec, NULL) < 0) {
      fprintf(stderr, "Could not open codec\n");
      exit(1);
  }

  frame = av_frame_alloc();
  if (!frame) {
      fprintf(stderr, "Could not allocate video frame\n");
      exit(1);
  }

  frame_count = 0;

  //存储解码后转换的RGB数据
  pFrameBGR = av_frame_alloc();


}

int H264Decoder::decode_read(unsigned char *inputbuf, size_t size,cv::Mat &mat_img)
{
  avpkt.size = size;
  if(avpkt.size == 0)
      return 0;

  avpkt.data = inputbuf;

  int len, got_frame;


  len = avcodec_decode_video2(c, frame, &got_frame, &avpkt);
  if (len < 0)
  {

      fprintf(stderr, "Error while decoding frame %d\n", frame_count);
      frame_count++;

      return 0;
  }
  if(out_buffer == nullptr)
  {
      BGRsize = avpicture_get_size(AV_PIX_FMT_BGR24, c->width,
                                   c->height);
      out_buffer = (uint8_t *) av_malloc(BGRsize);
      avpicture_fill((AVPicture *) pFrameBGR, out_buffer, AV_PIX_FMT_BGR24,
                     c->width, c->height);

      img_convert_ctx =
              sws_getContext(c->width, c->height, c->pix_fmt,
                             c->width, c->height, AV_PIX_FMT_BGR24, SWS_BICUBIC, NULL, NULL,
                             NULL);
      mat_img.create(cv::Size(c->width, c->height), CV_8UC3);

  }
  if (avpkt.data)
  {
      avpkt.size -= len;
      avpkt.data += len;
  }
  if (got_frame)
  {
      sws_scale(img_convert_ctx, (const uint8_t *const *)frame->data,
                frame->linesize, 0, c->height, pFrameBGR->data, pFrameBGR->linesize);

      memcpy(mat_img.data, out_buffer, BGRsize);

//        printf("decoding frame: %d\n",frame_count);
      frame_count++;
      return 1;
  }
  else
  {
      return 0;
  }


}
/*
 * //使用读取内存的方式的main函数需要执行的指令
//  std::ifstream fin("/home/only/test.265", std::ios_base::binary);
//  std::cout<<"1.. \n";
//  fin.seekg(0, std::ios::end);
//  int len = fin.tellg();
//  fin.seekg(0, std::ios::beg);

//  fin.read((char *) buf, len);
//  if(mDecoder.decode_read(buf, len,mimg)==1)
//  {
//    std::cout<<"show.. \n";
//    //显示解码之后的图片
//    cv::imshow("decode",mimg);
//    cv::waitKey(1);
//  }

//  for (int j = 88; j < 119; j++)
//  {
//      std::ifstream fin("/home/only/264rawFrame/outputFrame"
//                        + std::to_string(j), std::ios_base::binary);
//      fin.seekg(0, std::ios::end);
//      int len = fin.tellg();
//      fin.seekg(0, std::ios::beg);

//      fin.read((char *) buf, len);
//      if(mDecoder.decode_read(buf, len,mimg)==1)
//      {
//        std::cout<<"show.. \n";
//        //显示解码之后的图片
//        cv::imshow("decode",mimg);
//        cv::waitKey(1);
//      }

//  }
*/



