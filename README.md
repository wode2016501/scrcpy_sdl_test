shell: adb forward tcp:1234 localabstract:scrcpy

shell: adb push /usr/local/share/scrcpy/scrcpy-server /data/local/tmp/scrcpy-server-manual.jar


shell: adb shell CLASSPATH=/data/local/tmp/scrcpy-server-manual.jar app_process / com.genymobile.scrcpy.Server 3.3.3 tunnel_forward=true audio=false control=false cleanup=false  max_size=1920


shell: gcc c_1z_h264_sdl.c  -lavcodec -lavformat -lavutil -lswscale -lSDL2


shell: ./a.out 
