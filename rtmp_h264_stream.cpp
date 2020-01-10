/***********************************************
Copyright           : 2015 leef.biant Inc.
Filename            : rtmp_stream.cpp
Author              : bt731001@163.com
Description         : ---
Create              : 2020-01-10 15:48:10
Last Modified       : 2020-01-10 15:48:10
***********************************************/
#include "rtmp_h264_stream.h"


char * put_byte(char *output, uint8_t nVal) {
	output[0] = nVal;
	return output + 1;
}
char * put_be16(char *output, uint16_t nVal) {
	output[1] = nVal & 0xff;
	output[0] = nVal >> 8;
	return output + 2;
}
char * put_be24(char *output, uint32_t nVal) {
	output[2] = nVal & 0xff;
	output[1] = nVal >> 8;
	output[0] = nVal >> 16;
	return output + 3;
}
char * put_be32(char *output, uint32_t nVal) {
	output[3] = nVal & 0xff;
	output[2] = nVal >> 8;
	output[1] = nVal >> 16;
	output[0] = nVal >> 24;
	return output + 4;
}
char * put_be64(char *output, uint64_t nVal) {
	output = put_be32(output, nVal >> 32);
	output = put_be32(output, nVal);
	return output;
}
char * put_amf_string(char *c, const char *str) {
	uint16_t len = strlen(str);
	c = put_be16(c, len);
	memcpy(c, str, len);
	return c + len;
}
char * put_amf_double(char *c, double d) {
	*c++ = AMF_NUMBER; /* type: Number */
	{
		unsigned char *ci, *co;
		ci = (unsigned char *) &d;
		co = (unsigned char *) c;
		co[0] = ci[7];
		co[1] = ci[6];
		co[2] = ci[5];
		co[3] = ci[4];
		co[4] = ci[3];
		co[5] = ci[2];
		co[6] = ci[1];
		co[7] = ci[0];
	}
	return c + 8;
}

CRtmpH264Stream::CRtmpH264Stream(const char* h_264_file, const char* url) :m_cache(MAX_CACHE_SIZE) {
    m_url = url;
    memset(&m_meta_data, 0x00, sizeof(RTMPMetadata));
    m_is_send_meta = false;
    m_tick = 0;
    m_interval = 0;
    m_fp = fopen(h_264_file, "rb");
}

CRtmpH264Stream::~CRtmpH264Stream() {
    if (m_fp) {
        fclose(m_fp);
        m_fp = NULL;
    }
    Close();
}

bool CRtmpH264Stream::Connect(const char* url) {
    debug("conn rtmp svr:%s", url);
    m_prtmp = RTMP_Alloc();
    RTMP_Init(m_prtmp);

    if (RTMP_SetupURL(m_prtmp, (char*) url) == FALSE) {
		RTMP_Free(m_prtmp);
		return FALSE;
	}
	RTMP_EnableWrite(m_prtmp);
	if (RTMP_Connect(m_prtmp, NULL) == FALSE) {
		RTMP_Free(m_prtmp);
		return FALSE;
	}
	if (RTMP_ConnectStream(m_prtmp, 0) < 0) {
		RTMP_Close(m_prtmp);
		RTMP_Free(m_prtmp);
		return FALSE;
	}
    int ret = fread(m_cache.buf, 1, m_cache.size, m_fp);
    if (ret <= 0) {
        debug("read file failed:%s", strerror(errno));
        return false;
    }
    m_cache.length = ret;
	return TRUE;
}

void CRtmpH264Stream::Close() {
	if (m_prtmp) {
		RTMP_Close(m_prtmp);
		RTMP_Free(m_prtmp);
		m_prtmp = NULL;
	}
}

int CRtmpH264Stream::SendPacket(unsigned int nPacketType, unsigned char *data,
		unsigned int size, unsigned int nTimestamp) {
	if (m_prtmp == NULL) {
		return FALSE;
	}

	RTMPPacket packet;
	RTMPPacket_Reset(&packet);
	RTMPPacket_Alloc(&packet, size);

	packet.m_packetType = nPacketType;
	packet.m_nChannel = 0x04;
	packet.m_headerType = RTMP_PACKET_SIZE_LARGE;
	packet.m_nTimeStamp = nTimestamp;
	packet.m_nInfoField2 = m_prtmp->m_stream_id;
	packet.m_nBodySize = size;
	memcpy(packet.m_body, data, size);

	int nRet = RTMP_SendPacket(m_prtmp, &packet, 0);

	RTMPPacket_Free(&packet);

	return nRet;
}

bool CRtmpH264Stream::SendVideoPacket(_NaluUnit *pnaul) {
    do {
        if (m_is_send_meta)  break;
        if (pnaul->type != 0x7 && pnaul->type != 0x8) {
            break;
        }
        if (pnaul->type == 0x7) {
            m_meta_data.nSpsLen = pnaul->size;
            memcpy(m_meta_data.Sps, pnaul->data, pnaul->size);
        }
        if (pnaul->type == 0x8) {
            m_meta_data.nPpsLen = pnaul->size;
            memcpy(m_meta_data.Pps, pnaul->data, pnaul->size);
        }
        if (m_meta_data.nSpsLen) {
            get_bit_context buffer;
            SPS h264_sps_context;
            memset(&buffer, 0, sizeof(get_bit_context));
            buffer.buf = (uint8_t *)m_meta_data.Sps + 1;
            buffer.buf_size = m_meta_data.nSpsLen -1;
            if (0 != h264dec_seq_parameter_set(&buffer, &h264_sps_context)) {
                debug("parse sps failed");
                break;
            }

            m_meta_data.nWidth = (h264_sps_context.pic_width_in_mbs_minus1 + 1) * 16;
            m_meta_data.nHeight = (h264_sps_context.pic_height_in_map_units_minus1 + 1) * 16 * (2 - h264_sps_context.frame_mbs_only_flag);
            int timescale = h264_sps_context.vui_parameters.time_scale;
            int tick = h264_sps_context.vui_parameters.num_units_in_tick;
            float framerate = 0;
            h264_get_framerate(&framerate, &h264_sps_context);
            m_meta_data.nFrameRate = framerate;
            
        }
        if (m_meta_data.nSpsLen && m_meta_data.nPpsLen) {
            m_interval = 1000  / m_meta_data.nFrameRate;
            debug("init meta sucess with:%d height:%d framrate:%d interval:%d",
             m_meta_data.nWidth, m_meta_data.nHeight, m_meta_data.nFrameRate, m_interval);
            SendMetadata(&m_meta_data);
            m_is_send_meta = true;
        }
        return true;
    } while (0);
    do {
        if (!m_is_send_meta) break;
        if (pnaul->type == 0x7) break;
        if (pnaul->type == 0x8) break;
        bool bKeyframe = (pnaul->type == 0x05) ? TRUE : FALSE;
        SendH264Packet(pnaul->data, pnaul->size, bKeyframe, m_tick);
        m_tick  += m_interval;
    } while (0);
    return true;
}
bool CRtmpH264Stream::SendMetadata(LPRTMPMetadata lpMetaData) {
	if (lpMetaData == NULL) {
		return false;
	}
	char body[1024] = { 0 };
	;

	char * p = (char *) body;
	p = put_byte(p, AMF_STRING);
	p = put_amf_string(p, "@setDataFrame");

	p = put_byte(p, AMF_STRING);
	p = put_amf_string(p, "onMetaData");

	p = put_byte(p, AMF_OBJECT);
	p = put_amf_string(p, "copyright");
	p = put_byte(p, AMF_STRING);
	p = put_amf_string(p, "firehood");

	p = put_amf_string(p, "width");
	p = put_amf_double(p, lpMetaData->nWidth);

	p = put_amf_string(p, "height");
	p = put_amf_double(p, lpMetaData->nHeight);

	p = put_amf_string(p, "framerate");
	p = put_amf_double(p, lpMetaData->nFrameRate);

	p = put_amf_string(p, "videocodecid");
	p = put_amf_double(p, FLV_CODECID_H264);

	p = put_amf_string(p, "");
	p = put_byte(p, AMF_OBJECT_END);

	SendPacket(RTMP_PACKET_TYPE_INFO, (unsigned char*) body, p - body, 0);

	int i = 0;
	body[i++] = 0x17; // 1:keyframe  7:AVC
	body[i++] = 0x00; // AVC sequence header

	body[i++] = 0x00;
	body[i++] = 0x00;
	body[i++] = 0x00; // fill in 0;

	// AVCDecoderConfigurationRecord.
	body[i++] = 0x01; // configurationVersion
	body[i++] = lpMetaData->Sps[1]; // AVCProfileIndication
	body[i++] = lpMetaData->Sps[2]; // profile_compatibility
	body[i++] = lpMetaData->Sps[3]; // AVCLevelIndication 
	body[i++] = 0xff; // lengthSizeMinusOne

	// sps nums
	body[i++] = 0xE1; //&0x1f
	// sps data length
	body[i++] = lpMetaData->nSpsLen >> 8;
	body[i++] = lpMetaData->nSpsLen & 0xff;
	// sps data
	memcpy(&body[i], lpMetaData->Sps, lpMetaData->nSpsLen);
	i = i + lpMetaData->nSpsLen;

	// pps nums
	body[i++] = 0x01; //&0x1f
	// pps data length 
	body[i++] = lpMetaData->nPpsLen >> 8;
	body[i++] = lpMetaData->nPpsLen & 0xff;
	// sps data
	memcpy(&body[i], lpMetaData->Pps, lpMetaData->nPpsLen);
	i = i + lpMetaData->nPpsLen;

	bool ret = SendPacket(RTMP_PACKET_TYPE_VIDEO, (unsigned char*) body, i, 0);
    if (!ret) {
        return false;
    }
    m_is_send_meta = true;
    return true;
}

bool CRtmpH264Stream::SendH264Packet(unsigned char *data, unsigned int size,
		bool bIsKeyFrame, unsigned int nTimeStamp) {
	if (data == NULL && size < 11) {
		return false;
	}

	unsigned char *body = new unsigned char[size + 9];

	int i = 0;
	if (bIsKeyFrame) {
		body[i++] = 0x17;	// 1:Iframe  7:AVC
	} else {
		body[i++] = 0x27;	// 2:Pframe  7:AVC
	}
	body[i++] = 0x01;	// AVC NALU
	body[i++] = 0x00;
	body[i++] = 0x00;
	body[i++] = 0x00;

	// NALU size
	body[i++] = size >> 24;
	body[i++] = size >> 16;
	body[i++] = size >> 8;
	body[i++] = size & 0xff;

	// NALU data
	memcpy(&body[i], data, size);

	bool bRet = SendPacket(RTMP_PACKET_TYPE_VIDEO, body, i + size, nTimeStamp);

	delete[] body;

	return bRet;
}


bool CRtmpH264Stream::ReadOneNaluFromBuf(NaluUnit &nalu) {
    nalu.size = 0;
    uint32_t i = 0;
    while (i + 4 < m_cache.length) {
        if (m_cache.buf[i++] == 0x00 && m_cache.buf[i++] == 0x00 && m_cache.buf[i++] == 0x00 && m_cache.buf[i++] == 0x01) {
            uint32_t pos = i;
            while (pos + 4 < m_cache.length) {
                if (m_cache.buf[pos++] == 0x00 && m_cache.buf[pos++] == 0x00 && m_cache.buf[pos++] == 0x00 && m_cache.buf[pos++] == 0x01)  {
                    break;
                }
            }
            if (pos == m_cache.length) {
                nalu.size = pos - i;
            } else {
                nalu.size = pos - 4 - i;
            }
            nalu.data = new unsigned char[nalu.size];
            memcpy(nalu.data, &m_cache.buf[i], nalu.size);
            nalu.type = nalu.data[0] & 0x1f;
            memmove(m_cache.buf, m_cache.buf + nalu.size + 4, m_cache.length - nalu.size - 4);
            m_cache.length -= nalu.size + 4;
            break;
        } 
    }
    if (m_cache.length < m_cache.size / 3) {
        if (feof(m_fp)) {
            debug("file is end");
            return nalu.size > 0;
        }
        int ret = fread(m_cache.buf + m_cache.length, 1, m_cache.size - m_cache.length, m_fp);
        if (ret <= 0) {
            debug("read file failed:%s", strerror(errno));
            return false;
        }
        debug("read file %d", ret);
        m_cache.length += ret;
    }
    return nalu.size > 0;
}

void CRtmpH264Stream::Run() {
    if (!Connect(m_url)) {
        debug("not conn svr:%s", m_url);
        return;
    }
    RTMPMetadata metaData;
    memset(&metaData, 0, sizeof(RTMPMetadata)); 
    while (1) {
        _NaluUnit nalu;
        if (!ReadOneNaluFromBuf(nalu)) {
            debug("read naul failed");
            nalu.Destroy();
            break;
        }
        debug("find fream typp:%x size:%d", nalu.type, nalu.size);
        if (!SendVideoPacket(&nalu)) {
            debug("SendVideoPacket failed");
            break;
        }
        nalu.Destroy();
        usleep(m_interval * 1000);
    }
    Close();
    return;
}