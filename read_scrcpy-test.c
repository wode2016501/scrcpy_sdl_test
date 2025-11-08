/*
shell: adb forward tcp:1234 localabstract:scrcpy
shell: adb push /usr/local/share/scrcpy/scrcpy-server /data/local/tmp/scrcpy-server-manual.jar
shell: adb shell CLASSPATH=/data/local/tmp/scrcpy-server-manual.jar app_process / com.genymobile.scrcpy.Server 3.3.3 tunnel_forward=true audio=false control=false cleanup=false  max_size=1920
shell: nc localhost 1234 >k.h264
shell: gcc read_scrcpy.c
shell: ./a.out k.h264
*/

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
int isize = 0;
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
            perror("read");
            return -1;
        }
        buf += ret;
        y -= ret;
    }

    return size;
}

int main(int argc, char **argv)
{
    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s filename\n", argv[0]);
        return 1;
    }

    int fd = open(argv[1], O_RDONLY);
    if (fd == -1)
    {
        fprintf(stderr, "open '%s' failed: %s\n", argv[1], strerror(errno));
        return 2;
    }
    int size_max = 1024 * 1024;
    char buff[1024 * 1024];
    char *buf = buff;
    ssize_t n;
    n = read_(fd, buf, 77, size_max);
    if (n != 77)
    {
        fprintf(stderr, "read failed: %s\n", strerror(errno));
        close(fd);
        return 3;
    }
    n = readyz(fd, buf, size_max);
    if (n == -1)
        return -1;
    isize = n;
    buf += n;
    n = readyz(fd, buf, size_max);
    if (n == -1)
        return -1;
    int ret = 0;
    while (1)
    {
        ret = readyz(fd, buf, size_max);
        if (ret == -1)
            break;
        for (int i = 0; i < 5; i++)
        {
            printf("%0x ", buf[i]);
        }
        printf("\n");
        write(2, buff, ret + isize);
        break;
    }

    close(fd);
    return 0;
}

int readyz(int fd, char *buf, int size_max) // 读取一帧数据
{
    int ret = 0;
    int size = 0;
    ret = read_(fd, buf, sizeof(long), size_max); // pts
    if (ret != 8)
    {
        printf("read size 8 failed: %s\n", strerror(errno));
        return -1; /* EOF */
    }
    ret = read_(fd, &size, sizeof(int), size_max);
    if (ret != 4)
    {
        printf("read size 4 failed: %s\n", strerror(errno));
        return -1; /* EOF */
    }
    size = ntohl(size);
    if (size > 0)
    {
        printf("size: %d %#x\n", size, size);
        ret = read_(fd, buf, size, size_max);
        printf("n: %d\n", ret);
        if (ret == size)
        {
            return ret;
        }
    }
    return -1;
}