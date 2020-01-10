/***********************************************
Copyright           : 2015 leef.biant Inc.
Filename            : main.cpp
Author              : bt731001@163.com
Description         : ---
Create              : 2020-01-10 18:11:33
Last Modified       : 2020-01-10 18:11:33
***********************************************/

#include "rtmp_h264_stream.h"

void Using() {
  debug("Using rtmp h264file");
  _exit(0);
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        Using();
    }
    CRtmpH264Stream rtmp_stream(argv[1], argv[2]);
    rtmp_stream.Run();
    return 0;
}
