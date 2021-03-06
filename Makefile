CC = g++
LIB = -lrtmp -lssl -lcrypto -lz 
RUNLIBPATH=/opt/source/rtmpdump/librtmp
LIBPATH=-L../rtmpdump/librtmp 
CINC = -I/opt/source/rtmpdump/librtmp 
CXXFLAGS = -Wall -g -std=c++11 -O0  $(CINC)
LDFLAGS = -Wl,-rpath,$(RUNLIBPATH)

target = rtmp_pulish

objects = main.o rtmp_h264_stream.o sps_pps.o
all: $(target)
$(target): $(objects)
	make -f MakeFile_flv
	make -f MakeFile_mp4
	$(CC) -o $@ $^ $(LIBPATH) $(LIB) $(LDFLAGS) 
$(objects): %.o: %.cpp
	$(CC) -c $(CXXFLAGS) $< -o $@
clean:
	rm -rf *.o $(target)
	make clean -f MakeFile_flv
	make clean -f MakeFile_mp4
