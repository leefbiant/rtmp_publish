/***********************************************
Copyright           : 2015 leef.biant Inc.
Filename            : rtmp_h264_stream.h
Author              : bt731001@163.com
Description         : ---
Create              : 2020-01-10 15:35:48
Last Modified       : 2020-01-10 15:35:48
***********************************************/
#ifndef RTMP_PUBLISH_H
#define RTMP_PUBLISH_H

#include <stdio.h>
#include <string.h>

#include "util.h"

#include "rtmp.h"
#include "rtmp_sys.h"
#include "amf.h"
#include "sps_pps.h"

#define MAX_CACHE_SIZE (2*1024*1024)

enum {
	FLV_CODECID_H264 = 7,
};

// NALU单元
typedef struct _NaluUnit
{
	int type;
	int size;
	unsigned char *data;
    _NaluUnit() :data(NULL) {
    }
    ~_NaluUnit() {
        Destroy();
    }
    void Destroy() {
        if (data) {
            delete[] data;
            data = NULL;
        }
    }
}NaluUnit;

typedef struct _RTMPMetadata
{
	// video, must be h264 type
	unsigned int	nWidth;
	unsigned int	nHeight;
	unsigned int	nFrameRate;		// fps
	unsigned int	nVideoDataRate;	// bps
	unsigned int	nSpsLen;
	unsigned char	Sps[1024];
	unsigned int	nPpsLen;
	unsigned char	Pps[1024];

	// audio, must be aac type
	bool	        bHasAudio;
	unsigned int	nAudioSampleRate;
	unsigned int	nAudioSampleSize;
	unsigned int	nAudioChannels;
	char		    pAudioSpecCfg;
	unsigned int	nAudioSpecCfgLen;

} RTMPMetadata, *LPRTMPMetadata;


typedef struct Buff {
    uint8_t* buf;
    uint32_t size;
    uint32_t length; 
    Buff(int s) {
        length = 0;
        size = s > 0 ? s : 1;
        buf = new uint8_t[size];
    }
    ~Buff() {
        if (buf) {
            delete[] buf;
            buf = NULL;
        }
    }
}Buff;



class CRtmpH264Stream {
public:
    CRtmpH264Stream(const char* h_264_file, const char* url);
    virtual ~CRtmpH264Stream();
public:
    // 连接到RTMP Server
	bool Connect(const char* url);
    // 断开连接
	void Close();
    // 发送MetaData
	bool SendMetadata(LPRTMPMetadata lpMetaData);
    // 发送H264数据帧
	bool SendH264Packet(unsigned char *data,unsigned int size,bool bIsKeyFrame,unsigned int nTimeStamp);
    // 发送数据
    int SendPacket(unsigned int nPacketType,unsigned char *data,unsigned int size,unsigned int nTimestamp);

    bool SendVideoPacket(_NaluUnit* pnaul);

    bool ReadOneNaluFromBuf(NaluUnit &nalu);

    void Run();
private:
    FILE* m_fp; // h264 file;
    Buff m_cache;
    RTMP* m_prtmp;
    const char* m_url;
    RTMPMetadata m_meta_data;
    bool m_is_send_meta;
    uint32_t m_tick;
    uint32_t m_interval;
};
#endif
