#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <ffmpeg/swscale.h>

int main()
{
        //____________________________ opening data  ___________________________
        av_register_all();

        AVFormatContext *pFileFormatCxt = NULL;

        // Open video file
        if(avformat_open_input(&pFileFormatCxt, argv[1], NULL, 0, NULL)!=0)
                return -1; // Couldn't open file

        // Retrieve stream information
        if(avformat_find_stream_info(pFileFormatCxt, NULL)<0)
                return -1; // Couldn't find stream information
        // Dump information about file onto standard error
        av_dump_format(pFileFormatCxt, 0, argv[1], 0);

        AVCodecContext *pCodecCtxOrig = NULL;
        AVCodecContext *pCodecCtx = NULL;

        // Find the first video stream
        int indexVideoStream;
        int videoStream=-1;
        for(indexVideoStream=0; indexVideoStream<pFileFormatCxt->nb_streams; indexVideoStream++)
                if(pFileFormatCxt->streams[indexVideoStream]->codec->codec_type==AVMEDIA_TYPE_VIDEO) {
                        videoStream=indexVideoStream;
                        break;
                }
        if(videoStream==-1)
                return -1; // Didn't find a video stream

        // Get a pointer to the codec context for the video stream
        pCodecCtx=pFileFormatCxt->streams[videoStream]->codec;
        AVCodec *pCodec = NULL;

        // Find the decoder for the video stream
        pCodec=avcodec_find_decoder(pCodecCtx->codec_id);
        if(pCodec==NULL) {
                fprintf(stderr, "Unsupported codec!\n");
                return -1; // Codec not found
        }
        // Copy context
        pCodecCtx = avcodec_alloc_context3(pCodec);
        if(avcodec_copy_context(pCodecCtx, pCodecCtxOrig) != 0) {
                fprintf(stderr, "Couldn't copy codec context");
                return -1; // Error copying codec context
        }
        // Open codec
        if(avcodec_open2(pCodecCtx, pCodec)<0)
                return -1; // Could not open codec

        //____________________________ stroring data  ___________________________
        // Allocate video frame
        AVFrame *pFrame = NULL;
        pFrame=av_frame_alloc();
        // Allocate an AVFrame structure
        pFrameRGB = av_frame_alloc();
        if(pFrameRGB==NULL)
                return -1;


        int numBytes;
        // Determine required buffer size and allocate buffer
        numBytes=avpicture_get_size( PIX_FMT_RGB24,
                                     pCodecCtx->width,
                                     pCodecCtx->height);

        uint8_t *buffer = NULL;
        buffer=(uint8_t *)av_malloc(numBytes*sizeof(uint8_t));

        // Assign appropriate parts of buffer to image planes in pFrameRGB
        // Note that pFrameRGB is an AVFrame, but AVFrame is a superset
        // of AVPicture

        avpicture_fill( (AVPicture *)pFrameRGB, /* read */
                        buffer,                 /* get */
                        PIX_FMT_RGB24,          /* from */
                        pCodecCtx->width,
                        pCodecCtx->height);


        //____________________________ reading data ____________________________ 
        struct SwsContext *sws_ctx = NULL;
        int frameFinished;
        AVPacket packet;
        // initialize SWS context for software scaling
        sws_ctx = sws_getContext( pCodecCtx->width,
                                  pCodecCtx->height,
                                  pCodecCtx->pix_fmt,
                                  pCodecCtx->width,
                                  pCodecCtx->height,
                                  PIX_FMT_RGB24,
                                  SWS_BILINEAR,
                                  NULL,
                                  NULL,
                                  NULL);

        indexPicture=0;
        while(av_read_frame(pFileFormatCxt, &packet)>=0) { /* 每次一次读似乎直接延后一个  , 自动的 */
                // Is this a packet from the video stream?
                if(packet.stream_index==videoStream) {
                        // Decode video frame
                        avcodec_decode_video2(pCodecCtx,
                                               pFrame,
                                               &frameFinished,
                                               &packet);

                        // Did we get a video frame?
                        if(frameFinished) {
                                // Convert the image from its native format to RGB
                                sws_scale( sws_ctx,                                     //c             the scaling context previously created with sws_getContext()
                                           (uint8_t const * const *)pFrame->data,       //srcSlice      the array containing the pointers to the planes of the source slice
                                           pFrame->linesize,                            //srcStride     the array containing the strides 跨度 for each plane of the source image
                                           0,                                           //srcSliceY     the position in the source image of the slice to process, 
                                                                                        //              that is the number (counted starting from zero) in the image of the first row of the slice
                                           pCodecCtx->height,                           //srcSliceH     the height of the source slice, that is the number of rows in the slice
                                           pFrameRGB->data,                             //dst           the array containing the pointers to the planes of the destination image
                                           pFrameRGB->linesize);                        //dstStride     the array containing the strides 跨度 for each plane of the destination image
                                //
                                //
                                //
                                // Save the frame to disk
                                //
                                if(++indexPicture<=5)
                                {
                                        SaveFrame( pFrameRGB,
                                                   pCodecCtx->width,
                                                   pCodecCtx->height,
                                                   indexPicture);

                                }
                        }
                }

                // Free the packet that was allocated by av_read_frame
                av_free_packet(&packet);
        }
        return 0;
}

void SaveFrame(AVFrame *pFrame, int frameWidth, int frameHeight, int frameIndex) {
                                                            //just used to make a name
        FILE *pPMMFile;
        char szFilename[32];

        // Open file
        sprintf(szFilename, "frame%d.ppm", frameIndex);
        pPMMFile=fopen(szFilename, "wb");
        if(pPMMFile==NULL)
                return;

        // Write header
        fprintf(pPMMFile, "P6\n%d %d\n255\n", frameWidth, frameHeight);
                //mark the file of ppm

        // Write pixel data
        int  lineIndex;
        for(lineIndex=0; lineIndex<frameHeight; lineIndex++)
                fwrite(pFrame->data[0] + lineIndex*pFrame->linesize[0] ,
                        1 ,
                        frameWidth*3 ,
                        pPMMFile);
                       /* write one line each time */
        fclose(pPMMFile);
}
