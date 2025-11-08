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

int read_fd(int fd, char *buf, size_t size, int max_size)
{
    int count = 0;
    int y = 0, n = 0;
    while (size > 0)
    {
        if (size > max_size)
            y = max_size;
        else
            y = size;
        n = read(fd, buf, y);
        if (n < 1)
        {
            perror("read");
            return -1;
        }
        if(count==0)    {
            for(int i=0;i<4;i++)
            {
                printf("%0x ", buf[i]);
            }
        }
        write(2, buf, n);
        size -= n;
        count++;
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

    char buf[4096];
    ssize_t n;
    n = read(fd, buf, 77);
    if (n != 77)
    {
        fprintf(stderr, "read failed: %s\n", strerror(errno));
        close(fd);
        return 3;
    }
    int size = 0;
    int size2 = 0;
    char *a=&size,*b=&size2;
    int count=0;
    while (1)
    {
         n = read(fd, buf, sizeof(long)); //pts
                 if (n == 0)
            break; /* EOF */
           
           n = read(fd, &size, sizeof(int));
                if (n == 0)
            break; // EOF
            size = ntohl(size);

        if (n == 4 && size > 0)
        {
            printf("size: %d %#x\n", size, size);
           // if(count<101)
            // n=lseek(fd, size, SEEK_CUR);
           // else
            n = read_fd(fd, buf, size, 4096);

            printf("n: %d\n", n);
            if (n == -1)
            {
                break;
            }
            count++;
            
            continue;
        }
    }

    close(fd);
    return 0;
}
