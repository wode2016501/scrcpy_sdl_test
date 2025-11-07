/*
shell: adb forward tcp:1234 localabstract:scrcpy
shell: adb push /usr/local/share/scrcpy/scrcpy-server /data/local/tmp/scrcpy-server-manual.jar
shell: adb shell CLASSPATH=/data/local/tmp/scrcpy-server-manual.jar app_process / com.genymobile.scrcpy.Server 3.3.3 tunnel_forward=true audio=false control=false cleanup=false  max_size=1920
shell: gcc c_1z_h264_sdl.c  -lavcodec -lavformat -lavutil -lswscale -lSDL2
shell: ./a.out 
*/
// gcc c_1z_h264_sdl.c  -lavcodec -lavformat -lavutil -lswscale -lSDL2
//  ./a.out
#include <stdio.h>
#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

// extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
//}
#define H264_FILE "k.h264"

// 全局变量
AVCodecContext *codec_ctx = NULL;
AVCodec *codec = NULL;
AVFrame *frame = NULL, *frame_yuv = NULL;
SDL_Window *window = NULL;
SDL_Renderer *renderer = NULL;
SDL_Texture *texture = NULL;
struct SwsContext *sws_ctx = NULL;

int tcp_connect(char *ip, int port)
{
    int client_socket;
    struct sockaddr_in server_addr;
    int ret;

    // 设置服务器地址和端口

    // 创建Socket
    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket == -1)
    {
        perror("Socket创建失败");
        exit(EXIT_FAILURE);
    }
    printf("客户端Socket创建成功\n");

    // 配置服务器地址信息
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = inet_addr(ip);

    // 连接到服务器
    ret = connect(client_socket, (struct sockaddr *)&server_addr, sizeof(server_addr));
    if (ret == -1)
    {
        perror("连接服务器失败");
        close(client_socket);
        exit(EXIT_FAILURE);
    }
    printf("成功连接到服务器 %s:%d\n", ip, port);
    return client_socket;
}

// 初始化FFmpeg解码器
int init_decoder()
{
    // 查找H264解码器
    codec = avcodec_find_decoder(AV_CODEC_ID_H264);
    if (!codec)
    {
        printf("找不到H264解码器\n");
        return -1;
    }

    // 创建解码器上下文
    codec_ctx = avcodec_alloc_context3(codec);
    if (!codec_ctx)
    {
        printf("无法分配解码器上下文\n");
        return -1;
    }

    // 打开解码器
    if (avcodec_open2(codec_ctx, codec, NULL) < 0)
    {
        printf("无法打开解码器\n");
        return -1;
    }

    frame = av_frame_alloc();
    frame_yuv = av_frame_alloc();

    if (!frame || !frame_yuv)
    {
        printf("无法分配帧\n");
        return -1;
    }

    return 0;
}

// 初始化SDL
int init_sdl(int width, int height)
{
    if (SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        printf("SDL初始化失败: %s\n", SDL_GetError());
        return -1;
    }

    // 创建窗口
    window = SDL_CreateWindow("H264解码显示",
                              SDL_WINDOWPOS_UNDEFINED,
                              SDL_WINDOWPOS_UNDEFINED,
                              width, height,
                              SDL_WINDOW_SHOWN);
    if (!window)
    {
        printf("无法创建窗口: %s\n", SDL_GetError());
        return -1;
    }

    // 创建渲染器
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer)
    {
        printf("无法创建渲染器: %s\n", SDL_GetError());
        return -1;
    }

    // 创建纹理（YUV420P格式）
    texture = SDL_CreateTexture(renderer,
                                SDL_PIXELFORMAT_YV12,
                                SDL_TEXTUREACCESS_STREAMING,
                                width, height);
    if (!texture)
    {
        printf("无法创建纹理: %s\n", SDL_GetError());
        return -1;
    }

    return 0;
}

// 解码并显示一帧
int decode_and_display_frame(const uint8_t *data, int size)
{
    AVPacket pkt;
    int ret;

    av_init_packet(&pkt);
    pkt.data = (uint8_t *)data;
    pkt.size = size;
    printf("pkt.size=%d\n", pkt.size);
    for (int i = 0; i < 4; i++)
    {
        printf("%0x ", data[i]);
    }
    printf("%#x\n",data[30+5]);
    // 发送数据包到解码器
    ret = avcodec_send_packet(codec_ctx, &pkt);
    if (ret < 0)
    {
        printf("发送数据包失败\n");
        return -1;
    }

    // 接收解码后的帧
    ret = avcodec_receive_frame(codec_ctx, frame);
    if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
    {
        return -1;
    }
    else if (ret < 0)
    {
        printf("解码错误\n");
        return -1;
    }

    // 配置图像转换上下文
    sws_ctx = sws_getCachedContext(sws_ctx,
                                   frame->width, frame->height, codec_ctx->pix_fmt,
                                   frame->width, frame->height, AV_PIX_FMT_YUV420P,
                                   SWS_BILINEAR, NULL, NULL, NULL);

    if (!sws_ctx)
    {
        printf("无法创建转换上下文\n");
        return -1;
    }

    // 分配YUV帧缓冲区
    if (av_image_alloc(frame_yuv->data, frame_yuv->linesize,
                       frame->width, frame->height, AV_PIX_FMT_YUV420P, 1) < 0)
    {
        printf("无法分配YUV图像\n");
        return -1;
    }

    // 转换图像格式
    sws_scale(sws_ctx, (const uint8_t *const *)frame->data,
              frame->linesize, 0, frame->height,
              frame_yuv->data, frame_yuv->linesize);

    // 更新SDL纹理
    SDL_UpdateYUVTexture(texture, NULL,
                         frame_yuv->data[0], frame_yuv->linesize[0],  // Y分量
                         frame_yuv->data[1], frame_yuv->linesize[1],  // U分量
                         frame_yuv->data[2], frame_yuv->linesize[2]); // V分量

    // 渲染显示
    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, texture, NULL, NULL);
    SDL_RenderPresent(renderer);

    // 清理
    av_freep(&frame_yuv->data[0]);
    av_packet_unref(&pkt);

    return 0;
}

void cleanup()
{
    if (sws_ctx)
        sws_freeContext(sws_ctx);
    if (frame)
        av_frame_free(&frame);
    if (frame_yuv)
        av_frame_free(&frame_yuv);
    if (codec_ctx)
        avcodec_free_context(&codec_ctx);
    if (texture)
        SDL_DestroyTexture(texture);
    if (renderer)
        SDL_DestroyRenderer(renderer);
    if (window)
        SDL_DestroyWindow(window);
    SDL_Quit();
}

int main(int argc, char *argv[])
{
    // FILE *file;
    uint8_t buffer[1024 * 1024]; //= malloc (1024 * 1024*888); // 1MB缓冲区
    if (!buffer)
    {
        fprintf(stderr, "无法分配缓冲区\n");
        return -1;
    }
    int buffer_size;
    // 注册所有组件
    av_register_all();

    // 初始化解码器
    if (init_decoder() < 0)
    {
        printf("解码器初始化失败\n");
        return -1;
    }

    // 打开H264文件
    // file = fopen(H264_FILE, "rb");
    // int fd = open(H264_FILE, 0);
    int fd = tcp_connect("127.0.0.1", 1234);
    if (fd < 0)
    {
        printf("无法打开文件: %s\n", H264_FILE);
        return -1;
    }
    // int ret = lseek(fd, 77, SEEK_CUR);
    int ret = read_(fd, buffer, 77);
    if (ret != 77)
    {
        printf("读取77失败\n");
        return -1;
    }
    // 假设第一帧的尺寸（实际应该从码流中解析）
    int width = 2376, height = 1080;

    // 初始化SDL
    if (init_sdl(width, height) < 0)
    {
        printf("SDL初始化失败\n");

        return -1;
    }

    char *p = buffer;

    int zzsize = 0;
    int isize = 0;

    // 读取H264数据包
    // ret = lseek(fd, 8, SEEK_CUR);      // pts long
    ret = read_(fd, buffer, 8);
    if (ret != 8)
    {
        printf("读8失败\n");
        return -1;
    }
    buffer_size = read_(fd, &isize, 4); // size int
    isize = ntohl(isize);
    printf("size:%d\n", isize);
    // 读取一帧数据（简化处理，实际应该按NALU单元读取）
    buffer_size = read_(fd, buffer, isize);
    if (buffer_size != isize)
    {
        printf("文件读取失败\n");
        close(fd);
        return -1;
    }
    p += isize;


int psize = 0;
/*
    // 读取H264数据包
    // ret = lseek(fd, 8, SEEK_CUR);      // pts long
    ret = read_(fd, p, 8);
    if (ret != 8)
    {
        printf("读8失败\n");
        return -1;
    }
    buffer_size = read_(fd, &psize, 4); // size int
    psize = ntohl(psize);
    printf("size:%d\n", psize);
    // 读取一帧数据（简化处理，实际应该按NALU单元读取）
    buffer_size = read_(fd, p, psize);
    if (buffer_size != psize)
    {
        printf("文件读取失败\n");
        close(fd);
        return -1;
    }
    p += psize;

*/


    int size = 0;

    SDL_Event event;
    int quit = 0;

    // 解码并显示

    printf("解码显示成功！\n");
    // 等待用户按键退出

    while (!quit)
    {
        // 读取H264数据包
        // ret = lseek(fd, 8, SEEK_CUR);     // pts long
        ret = read_(fd, p, 8);
        if (ret != 8)
        {
            printf("读8失败\n");
            return -1;
        }
        buffer_size = read_(fd, &size, 4); // size int
        size = ntohl(size);
        printf("size:%d\n", size);
        // 读取一帧数据（简化处理，实际应该按NALU单元读取）
        buffer_size = read_(fd, p, size);
        if (buffer_size != size)
        {
            printf("文件读取失败\n");
            close(fd);
            return -1;
        }
        zzsize += buffer_size + isize+psize;
        if (decode_and_display_frame(buffer, zzsize) != 0)
            break;
        zzsize = 0;
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_QUIT ||
                (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE))
            {
                quit = 1;
            }
        }
       // SDL_Delay(1000);
    }

    close(fd);
    cleanup();
    return 0;
}

int read_(int fd, void *p, int size){
int y=size;
int ret=0;
while (y>0)
{
    ret = read(fd, p, y);
    if (ret < 0)
    return ret;
    y -= ret;
    p += ret;
    /* code */
}
return size;
}