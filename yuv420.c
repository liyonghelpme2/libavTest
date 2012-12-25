#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include "colorspace.h"

static void video_encode_example(const char *filename)
{
    av_register_all();
    AVCodec *codec;
    AVCodecContext *c= NULL;
    int i, out_size, size, x, y, outbuf_size;
    FILE *f;
    AVFrame *picture;
    uint8_t *outbuf, *picture_buf;

    printf("Video encoding\n");

    /* find the mpeg1 video encoder */
    codec = avcodec_find_encoder(CODEC_ID_MPEG1VIDEO);
    if (!codec) {
        fprintf(stderr, "codec not found\n");
        exit(1);
    }

    c = avcodec_alloc_context3(codec);
    picture= avcodec_alloc_frame();

    /* put sample parameters */
    c->bit_rate = 400000;
    /* resolution must be a multiple of two */
    c->width = 960;
    c->height = 640;
    /* frames per second */
    c->time_base= (AVRational){1, 25};
    c->gop_size = 10; /* emit one intra frame every ten frames */
    c->max_b_frames=1;
    c->pix_fmt = PIX_FMT_YUV420P;

    /* open it */
    if (avcodec_open(c, codec) < 0) {
        fprintf(stderr, "could not open codec\n");
        exit(1);
    }

    f = fopen(filename, "wb");
    if (!f) {
        fprintf(stderr, "could not open %s\n", filename);
        exit(1);
    }

    /* alloc image and output buffer */
    outbuf_size = 500000;
    outbuf = malloc(outbuf_size);
    size = c->width * c->height;
    
    //R G B 24 255, 0, 0
    //3 通道
    uint8_t *rgbbuffer = (uint8_t*)malloc(c->width*c->height*3);
    int j, k;
    int totalLen = c->width*c->height;
    memset(rgbbuffer, 0, totalLen*3);
    for(j = 0; j < totalLen; j++)
    {
        rgbbuffer[j*3+0] = 255;
        rgbbuffer[j*3+1] = 0;
        rgbbuffer[j*3+2] = 0;
    }
    AVFrame *pFrameRGB = avcodec_alloc_frame();
    //rgb data
    int ret = avpicture_fill((AVPicture *)pFrameRGB, rgbbuffer, PIX_FMT_RGB24, c->width, c->height);
    printf("fille picture of rgb value %d\n", ret);
    //YVU 采样 1 : 1

    
    //Y 4
    //U 2
    //V 2
    picture_buf = malloc((size * 3) / 2); /* size for YUV 420 */

    picture->data[0] = picture_buf;
    picture->data[1] = picture->data[0] + size;
    picture->data[2] = picture->data[1] + size / 4;
    //Y 解析度 是 U V 的两倍
    picture->linesize[0] = c->width;
    picture->linesize[1] = c->width / 2;
    picture->linesize[2] = c->width / 2;

    /* encode 1 second of video */
    //420 格式
    for(i=0;i< 40;i++) {
        fflush(stdout);
        /* prepare a dummy image */
        /* Y */
        for(y=0;y<c->height;y++) {
            for(x=0;x<c->width;x++) {
                //picture->data[0][y * picture->linesize[0] + x] = x + y + i * 3;
                /*
                int r = rgbbuffer[3*(y*c->width+x)+0];
                int g = rgbbuffer[3*(y*c->width+x)+1];
                int b = rgbbuffer[3*(y*c->width+x)+2];
                */
                int r, g, b;
                r = g = b = 0;
                if((x/10) % 3 == 0)
                    r = 255;
                if((x/10) % 3 == 1)
                    g = 255;
                if((x/10) % 3 == 2)
                    b = 255;
                picture->data[0][y*picture->linesize[0]+x] = RGB_TO_Y_CCIR(r, g, b);
            }
        }

        //2 * 2平均值0 1
        //           2 3
        /* Cb and Cr */
        for(y=0;y<c->height/2;y++) {
            for(x=0;x<c->width/2;x++) {
                /*
                int r = rgbbuffer[3*(y*2*c->width+x*2)+0];
                int g = rgbbuffer[3*(y*2*c->width+x*2)+1];
                int b = rgbbuffer[3*(y*2*c->width+x*2)+2];
                */
                int r, g, b;
                int col = x*2;
                r = g = b = 0;
                if((col/10) % 3 == 0)
                    r = 255;
                if((col/10) % 3 == 1)
                    g = 255;
                if((col/10) % 3 == 2)
                    b = 255;
                //picture->data[1][y * picture->linesize[1] + x] = 128 + y + i * 2;
                //picture->data[2][y * picture->linesize[2] + x] = 64 + x + i * 5;

                picture->data[1][y * picture->linesize[1] + x] = RGB_TO_U_CCIR(r, g, b, 0);
                picture->data[2][y * picture->linesize[2] + x] = RGB_TO_V_CCIR(r, g, b, 0);
            }
        }

        /* encode the image */
        out_size = avcodec_encode_video(c, outbuf, outbuf_size, picture);
        printf("encoding frame %3d (size=%5d)\n", i, out_size);
        fwrite(outbuf, 1, out_size, f);
    }

    /* get the delayed frames */
    for(; out_size; i++) {
        fflush(stdout);

        out_size = avcodec_encode_video(c, outbuf, outbuf_size, NULL);
        printf("write frame %3d (size=%5d)\n", i, out_size);
        fwrite(outbuf, 1, out_size, f);
    }

    /* add sequence end code to have a real mpeg file */
    outbuf[0] = 0x00;
    outbuf[1] = 0x00;
    outbuf[2] = 0x01;
    outbuf[3] = 0xb7;
    fwrite(outbuf, 1, 4, f);
    fclose(f);
    free(picture_buf);
    free(outbuf);

    avcodec_close(c);
    av_free(c);
    av_free(picture);
    printf("\n");
}
int main()
{
    video_encode_example("rgb.mp4");
}
