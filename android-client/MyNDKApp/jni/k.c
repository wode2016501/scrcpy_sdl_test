
/*'/opt/android-sdk/ndk/android-ndk-r28-beta1/toolchains/llvm/prebuilt/linux-x86_64/bin/aarch64-linux-android23-clang'  4.c  -lm -lOpenSLES -llog
ffmpeg -i k.mkv -f s16le -acodec pcm_s16le -ar 48000 -ac 2  k.pcm
adb -s LGH9309781d2db push k.pcm /sdcard
adb -s LGH9309781d2db  push a.out /data/local/tmp/a && adb -s LGH9309781d2db  shell  /data/local/tmp/a
*/
#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>
#include <jni.h>
#include <android/log.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <math.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define LOG_TAG "尼玛audio"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

typedef struct {
    SLObjectItf engineObject;
    SLEngineItf engineEngine;
    SLObjectItf outputMixObject;
    SLObjectItf playerObject;
    SLPlayItf playerPlay;
    SLAndroidSimpleBufferQueueItf bufferQueue;
    
    // PCM数据参数
    int sampleRate;
    int channelCount;
    int bitsPerSample;
    
    // 播放状态
    int isPlaying;
    int isInitialized;
    
    // 线程相关
    pthread_t modifyThread;
    int shouldStopThread;
} PCMPlayer;

static PCMPlayer g_player;
static short* pcmBuffer = NULL;  // 预分配缓冲区




int fd=0;
//44100hz*16*2/8=176400
int pcmBufferSize=176400;
long pts=0;
/*
int read_(int fd, void *p, int size, int size_max)
{
    if (size > size_max)
        return -1;
    int y = size;
    int ret = 0;
    while (y > 0)
    {
        ret = read(fd, p, y);
        if (ret < 0)
            return -1;
        y -= ret;
        p += ret;
    }
    return size;
}
*/ 



int read_(int fd, char *buf, size_t size, int max_size)
{

	if (size > max_size)
		return -1;
	int y = size;
	int ret = 0;
	while (y > 0)
	{
		ret = read(fd, buf, y);
		if (ret < 1)
		{
			LOGI("read errno");
			return -1;
		}
		buf += ret;
		y -= ret;
	}

	return size;
}
int readyz(int fd, void *buf, int size_max) // 读取一帧数据
{
	int ret = 0;
	int size = 0;
    ret = read_(fd, buf, sizeof(long long), size_max); // pts

	if (ret != 8)
	{
		LOGI("read size 8=%ld 错误\n",sizeof(long long));
		return -1; /* EOF */
	}
	ret = read_(fd, (char *) &size, sizeof(int), size_max);
	if (ret != 4)
	{
		LOGI("read size 4 错误:\n");
		return -1; /* EOF */
	}
	size = ntohl(size);
	if (size > 0)
	{
		// LOGI("size: %d %#x\n", size, size);
		ret = read_(fd, buf, size, size_max);
		// LOGI("n: %d\n", ret);
		if (ret == size)
		{
			return ret;
		}
	}
	LOGI("read size %d 错误:\n",size);
	return -1;
}



static void bufferQueueCallback(SLAndroidSimpleBufferQueueItf bq, void* context) {

    PCMPlayer* player = (PCMPlayer*)context;
   // if (!player || !player->isPlaying) return;
    
   int ret=0;
    if (pcmBuffer&&fd!=-1) {
		/*
        int ret=read_(fd,&pts,4,pcmBufferSize);
        ret=read_(fd,&pts,4,pcmBufferSize);//兼容32位系统  pts無用
        ret=read_(fd,&len,4,pcmBufferSize);
        if (ret != 4) {
            LOGE("Failed to read PCM data");
            close(fd);
            fd=-1;
            return;
        }
        len=ntohl(len);
        if(len>pcmBufferSize||len<0){
            LOGI("Failed to read PCM data");
            close(fd);
            fd=-1;
            return;
        }*/
        ret=readyz(fd,pcmBuffer, pcmBufferSize); // 读取PCM数据到缓冲区
        if (ret < 0) {
            LOGE("Failed to read PCM data");
            close(fd);
            fd=-1;
            return;
        }
    }
        (*bq)->Enqueue(bq, pcmBuffer, ret);
    
}

int initPlayer() {
    SLresult result;
    
    // 初始化线程控制标志
    g_player.shouldStopThread = 0;
    
    // 创建引擎
    result = slCreateEngine(&g_player.engineObject, 0, NULL, 0, NULL, NULL);
    if (result != SL_RESULT_SUCCESS) {
        LOGE("Failed to create engine: %d", result);
        return 0;
    }
    
    result = (*g_player.engineObject)->Realize(g_player.engineObject, SL_BOOLEAN_FALSE);
    if (result != SL_RESULT_SUCCESS) {
        LOGE("Failed to realize engine: %d", result);
        return 0;
    }
    
    result = (*g_player.engineObject)->GetInterface(g_player.engineObject, SL_IID_ENGINE, &g_player.engineEngine);
    if (result != SL_RESULT_SUCCESS) {
        LOGE("Failed to get engine interface: %d", result);
        return 0;
    }
    
    // 创建输出混音器
    result = (*g_player.engineEngine)->CreateOutputMix(g_player.engineEngine, &g_player.outputMixObject, 0, NULL, NULL);
    if (result != SL_RESULT_SUCCESS) {
        LOGE("Failed to create output mix: %d", result);
        return 0;
    }
    
    result = (*g_player.outputMixObject)->Realize(g_player.outputMixObject, SL_BOOLEAN_FALSE);
    if (result != SL_RESULT_SUCCESS) {
        LOGE("Failed to realize output mix: %d", result);
        return 0;
    }
    
    // 配置PCM参数
    g_player.sampleRate = 48000;
    g_player.channelCount = 2;
    g_player.bitsPerSample = 16;
    
    // 配置音频源
    SLDataLocator_AndroidSimpleBufferQueue loc_bufq = {
        SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE,
        2
    };
    
    SLDataFormat_PCM format_pcm = {
        SL_DATAFORMAT_PCM,
        (SLuint32)g_player.channelCount,
        (SLuint32)g_player.sampleRate * 1000, // 转换为millihertz
        16,//SL_PCMSAMPLEFORMAT_FIXED_16,
        SL_PCMSAMPLEFORMAT_FIXED_16,
        g_player.channelCount == 1 ? SL_SPEAKER_FRONT_CENTER : (SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT),
        SL_BYTEORDER_LITTLEENDIAN
    };
    
    SLDataSource audioSrc = {&loc_bufq, &format_pcm};
    
    // 配置音频接收器
    SLDataLocator_OutputMix loc_outmix = {
        SL_DATALOCATOR_OUTPUTMIX,
        g_player.outputMixObject
    };
    
    SLDataSink audioSnk = {&loc_outmix, NULL};
    
    // 创建音频播放器
    const SLInterfaceID ids[] = {SL_IID_ANDROIDSIMPLEBUFFERQUEUE};
    const SLboolean req[] = {SL_BOOLEAN_TRUE};
    
    result = (*g_player.engineEngine)->CreateAudioPlayer(g_player.engineEngine, &g_player.playerObject,
        &audioSrc, &audioSnk, 1, ids, req);
    if (result != SL_RESULT_SUCCESS) {
        LOGE("Failed to create audio player: %d", result);
        return 0;
    }
    
    result = (*g_player.playerObject)->Realize(g_player.playerObject, SL_BOOLEAN_FALSE);
    if (result != SL_RESULT_SUCCESS) {
        LOGE("Failed to realize audio player: %d", result);
        return 0;
    }
    
    result = (*g_player.playerObject)->GetInterface(g_player.playerObject, SL_IID_PLAY, &g_player.playerPlay);
    if (result != SL_RESULT_SUCCESS) {
        LOGE("Failed to get play interface: %d", result);
        return 0;
    }
    
    result = (*g_player.playerObject)->GetInterface(g_player.playerObject, SL_IID_ANDROIDSIMPLEBUFFERQUEUE, &g_player.bufferQueue);
    if (result != SL_RESULT_SUCCESS) {
        LOGE("Failed to get buffer queue interface: %d", result);
        return 0;
    }
    
    // 注册回调函数
    result = (*g_player.bufferQueue)->RegisterCallback(g_player.bufferQueue, bufferQueueCallback, &g_player);
    if (result != SL_RESULT_SUCCESS) {
        LOGE("Failed to register callback: %d", result);
        return 0;
    }

    // 预分配缓冲区
    pcmBuffer = malloc(pcmBufferSize);
    if (!pcmBuffer) {
        LOGE("Failed to allocate buffer");
        return 0;
    }

    g_player.isInitialized = 1;
    LOGI("PCM player initialized successfully");
    return 1;
}

void startPlayer() {
    if (!g_player.isInitialized) {
        LOGE("Player not initialized");
        return;
    }

    // 设置播放状态
    g_player.isPlaying = 1;
/*
    // 创建修改频率的线程
    if (pthread_create(&g_player.modifyThread, NULL, modifyFrequencyThread, NULL) != 0) {
        LOGE("Failed to create modify thread");
        return;
    }
*/
    // 首次入队数据
    bufferQueueCallback(g_player.bufferQueue, &g_player);

    // 开始播放
    SLresult result = (*g_player.playerPlay)->SetPlayState(g_player.playerPlay, SL_PLAYSTATE_PLAYING);
    if (result != SL_RESULT_SUCCESS) {
        LOGE("Failed to start playback: %d", result);
    }
}

void stopPlayer() {
    if (!g_player.isInitialized) return;

    g_player.isPlaying = 0;
    (*g_player.playerPlay)->SetPlayState(g_player.playerPlay, SL_PLAYSTATE_STOPPED);
    
    // 停止并等待修改线程结束
    g_player.shouldStopThread = 1;
    pthread_join(g_player.modifyThread, NULL);
}

void cleanup() {
    if (pcmBuffer) {
        free(pcmBuffer);
        pcmBuffer = NULL;
    }

    if (g_player.playerObject) {
        (*g_player.playerObject)->Destroy(g_player.playerObject);
        g_player.playerObject = NULL;
        g_player.playerPlay = NULL;
        g_player.bufferQueue = NULL;
    }

    if (g_player.outputMixObject) {
        (*g_player.outputMixObject)->Destroy(g_player.outputMixObject);
        g_player.outputMixObject = NULL;
    }

    if (g_player.engineObject) {
        (*g_player.engineObject)->Destroy(g_player.engineObject);
        g_player.engineObject = NULL;
        g_player.engineEngine = NULL;
    }

    g_player.isInitialized = 0;
}
  //int open(const char *pathname, int flags);


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
int mainaudio() {
	int main(); 
	main(); 
}
int main() {
  
    fd= //open("/sdcard/k.pcm", 0);
    tcp_connect("192.168.216.125",9998);
    //printf("fd=%d\n",fd);
    if(fd<0){
        LOGE("open error\n");
        return 0;
    }
    char buf[69];
    int ret= read_(fd, buf, 69,69);
    if(ret<0)
    {
        printf("read error\n");
        return 0;
    }
    printf("open success\n");
    if (!initPlayer()) {
        LOGE("Failed to initialize player");
        return 1;
    }

    startPlayer();

    // 播放5秒
	/*
	
   sleep(150);

    stopPlayer();
    cleanup();
*/
    return 0;
}
