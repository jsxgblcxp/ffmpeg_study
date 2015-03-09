gcc -l avformat -l avdevice -l avcodec -l avfilter  -l avutil -l swscale  -g   `sdl-config --cflags --libs ` tutorial01.c 
./a.out a.avi
rm a.out
