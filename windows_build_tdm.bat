cd src
gcc main.c screenshot_windows.c lodepng.c -ld3d11 -lgdi32 -ldwmapi -o ../build/timelapse.exe -g