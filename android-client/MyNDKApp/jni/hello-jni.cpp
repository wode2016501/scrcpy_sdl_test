/*
 * Copyright (C) 2009 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *adb forward tcp:1234 localabstract:scrcpy
 */
#include <string.h>
#include <jni.h>
#include <assert.h>
#include <pthread.h>
#include <fcntl.h>
#include <android/native_window_jni.h>
#include "media/NdkMediaCodec.h"
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

/* This is a trivial JNI example where we use a native method
 * to return a new VM String. See the corresponding Java source
 * file located at:
 *
 *   apps/samples/hello-jni/project/src/com/example/hellojni/HelloJni.java
 */
#include <android/log.h>

#define LOG_TAG "尼玛"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

static ANativeWindow *theNativeWindow = 0;
void *aaaaa(void *a);
extern "C"
{
	int tcp_connect(char *ip, int port);
	int readyz(int fd, char *buf, int size_max); // 读取一帧数据
	//int ntohl(int ); 
	int read_(int fd, char *buf, size_t size, int max_size); 
	char *malloc(int);
	//int usleep(int);
	//int read(int, char *, int);
	JNIEXPORT void JNICALL Java_com_mycompany_myndkapp_HelloJni_startPreview(JNIEnv *env, jclass clazz, jobject surface);
}

JNIEXPORT void JNICALL Java_com_mycompany_myndkapp_HelloJni_startPreview(JNIEnv *env, jclass clazz, jobject surface)
{
	if (theNativeWindow == 0)
	{
		theNativeWindow = ANativeWindow_fromSurface(env, surface);
		int ret = 0;
		pthread_t pid;
		void *a = 0;
		LOGI("开启创建景程");
		if ((ret = pthread_create(&pid, NULL, aaaaa, a)) != 0)
		{
			// LOGD("thread_create err\n");
		}
	}
}

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
        LOGI("Socket创建失败");
        return -1; 
    }
    LOGI("客户端Socket创建成功\n");

    // 配置服务器地址信息
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = inet_addr(ip);

    // 连接到服务器
    ret = connect(client_socket, (struct sockaddr *)&server_addr, sizeof(server_addr));
    if (ret == -1)
    {
        LOGI("连接服务器失败");
        close(client_socket);
        return -1; 
    }
    LOGI("成功连接到服务器 %s:%d\n", ip, port);
    return client_socket;
}

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
int readyz(int fd, char *buf, int size_max) // 读取一帧数据
{
	int ret = 0;
	int size = 0;
	if(sizeof(long)==4)
		ret = read_(fd, buf, sizeof(long long), size_max); // pts
		else
	ret = read_(fd, buf, sizeof(long), size_max); // pts
	if (ret != 8)
	{
		LOGI("read size 8 \n");
		return -1; /* EOF */
	}
	ret = read_(fd, (char *) &size, sizeof(int), size_max);
	if (ret != 4)
	{
		LOGI("read size 4 failed:\n");
		return -1; /* EOF */
	}
	size = ntohl(size);
	if (size > 0)
	{
		LOGI("size: %d %#x\n", size, size);
		ret = read_(fd, buf, size, size_max);
		LOGI("n: %d\n", ret);
		if (ret == size)
		{
			return ret;
		}
	}
	return -1;
}
void *aaaaa(void *a)
{
	LOGI("开始打开文件");
	int fd = tcp_connect("192.168.43.1",9999);//open("/sdcard/k.h264", 0);
	if (fd < 0)
	{
		LOGI("无法打开文件");
		return 0;
	}
	char *buff = malloc(1024*1024 );
	LOGI("开文件成功%s", buff);
	int rets = read_(fd, buff, 77, 1024);
	if(rets != 77)
	{
		LOGI("读取文件失败77");
		return 0;
	}
	AMediaCodec *pMediaCodec;
	AMediaFormat *format;
	pMediaCodec = AMediaCodec_createDecoderByType("video/avc"); // h264
	format = AMediaFormat_new();
	AMediaFormat_setString(format, "mime", "video/avc");
	AMediaFormat_setInt32(format, AMEDIAFORMAT_KEY_WIDTH, 2376);
	AMediaFormat_setInt32(format, AMEDIAFORMAT_KEY_HEIGHT, 1080);
	media_status_t status = AMediaCodec_configure(pMediaCodec, format, theNativeWindow, NULL, 0);
	AMediaCodec_start(pMediaCodec);
	int i = 0,zi=0;
	while (1)
	{
		usleep(1000);
		ssize_t bufidx = AMediaCodec_dequeueInputBuffer(pMediaCodec, 2000);
		if (bufidx >= 0)
		{
			size_t bufsize;
			uint8_t *buf = AMediaCodec_getInputBuffer(pMediaCodec, bufidx, &bufsize);
			int size = readyz(fd, (char *) buf, bufsize);
			if(size==-1)
				break; 
				//LOGI("读取第%d帧,size=%d", zi++, size);
			long presentationTimeUs = 0; // pts
			AMediaCodec_queueInputBuffer(pMediaCodec, bufidx, 0, size, presentationTimeUs, 0);
			AMediaCodecBufferInfo info;
			bufidx = AMediaCodec_dequeueOutputBuffer(pMediaCodec, &info, 2000);
			if (bufidx >= 0)
			{
				//LOGI("发送第%d帧,size=%d", i++, size);
				buf = AMediaCodec_getOutputBuffer(pMediaCodec, bufidx, &bufsize);
				AMediaCodec_releaseOutputBuffer(pMediaCodec, bufidx, true);
			}
		}
	}
}
