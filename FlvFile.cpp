/*************************************************************************
    > File Name: FlvFile.cpp
    > Author: pzx
    > Created Time: 2019年02月21日 星期四 15时42分31秒
************************************************************************/
#include "FlvFile.h"
#include <iostream>
#include <vector>
#include <string>
#include "ByteUtil.h"
#include "Amf.h"


#define CheckBuffer(x) { if ((nBufSize-nOffset) < (x)) \
    { *usedLen = nOffset; return NULL;} }
#define TAG_HEADER_SIZE 15
#define FLV_HEADER_SIZE 9

#define VIDEO_CODEC_ID_JPEG 1
#define VIDEO_CODEC_ID_H263 2
#define VIDEO_CODEC_ID_SCREEN 3
#define VIDEO_CODEC_ID_VP6 4
#define VIDEO_CODEC_ID_VPA 5
#define VIDEO_CODEC_ID_SCREEN2 6
#define VIDEO_CODEC_ID_AVC 7   // H264

#define VIDEO_FRAME_TYPE_KEYFRAME 1
#define VIDEO_FRAME_TYPE_INTERFRAME 2

#define AUDIO_CODEC_ID_ADPCM 1
#define AUDIO_CODEC_ID_MP3 2
#define AUDIO_CODEC_ID_LINEPCM 3
#define AUDIO_CODEC_ID_AAC 10

#define AVC_PACKET_TYPE_SEQUENCE_HEADER 0
#define AVC_PACKET_TYPE_AVC_NALU 1
#define AVC_PACKET_TYPE_SEQUENCE_END 2


int AudioTag::aacProfile_;
int AudioTag::sampleRateIndex_;
int AudioTag::channelConfig_;

void Tag::init(TagHeader *pHeader, unsigned char *pBuf, uint32_t nLeftLen)
{
    memcpy(&tagHeader_, pHeader, sizeof(TagHeader));

   /*
    tagHeaderData_.data_ = new unsigned char[11];
    memcpy(tagHeaderData_.data_, pBuf, 11);
    */

    tagData_.data_ = new unsigned char[tagHeader_.dataSize_];
    memcpy(tagData_.data_, pBuf + 11, tagHeader_.dataSize_);
}

AudioTag::AudioTag(TagHeader *pHeader, unsigned char *pBuf, uint32_t nLeftLen,
        FlvFile *pParser)
{
    init(pHeader, pBuf, nLeftLen);
    unsigned char *pd = tagData_.data_;
    soundFormat_ = (SoundFormat)((pd[0] & 0xf0) >> 4);
    soundRate_ = (SoundRate)((pd[0] & 0x0c) >> 2);
    soundSize_ = (SoundSize)((pd[0] & 0x02) >> 1);
    soundType_ = (SoundType)(pd[0] & 0x01);
    // debug("soundFormat_:%d soundRate_:%d soundSize_:%d soundType_:%d", soundFormat_, soundRate_, soundSize_, soundType_);
    if (soundFormat_ == 10) {  // AAC
        // 解析aactag
        parseAACTag(pParser);
    }
}

int AudioTag::parseAACTag(FlvFile *pParser)
{
    unsigned char *pd = tagData_.data_;
    int nAACPacketType = pd[1];
    dts_ = tagHeader_.totalTimeStamp_;
    pts_ = dts_;

    if (nAACPacketType == 0) {
        // 解析aac header
        parseAudioSpecificConfig(pParser, pd);
    } else if (nAACPacketType == 1) {
        parseRawAAC(pParser, pd);
    } else {
    }
    return 1;
}

int AudioTag::parseAudioSpecificConfig(FlvFile *pParser,
        unsigned char *pTagData) {
    unsigned char *pd = tagData_.data_;
    dts_ = tagHeader_.totalTimeStamp_;
    pts_ = dts_;
    aacProfile_ = ((pd[2]&0xf8)>>3) - 1;
    sampleRateIndex_ = ((pd[2] & 0x07) << 1) | (pd[3] >> 7);
    channelConfig_ = (pd[3]>>3) & 0x0f;

    mediaData_.data_ = NULL;
    mediaData_.size_ = 0;

    return 1;
}

int AudioTag::parseRawAAC(FlvFile *pParser, unsigned char *pTagData) {
    uint64_t bits = 0;
    int dataSize = tagHeader_.dataSize_ - 2;

    WriteU64(&bits, 12, 0xFFF);
    WriteU64(&bits, 1, 0);
    WriteU64(&bits, 2, 0);
    WriteU64(&bits, 1, 1);
    WriteU64(&bits, 2, aacProfile_);
    WriteU64(&bits, 4, sampleRateIndex_);
    WriteU64(&bits, 1, 0);
    WriteU64(&bits, 3, channelConfig_);
    WriteU64(&bits, 1, 0);
    WriteU64(&bits, 1, 0);
    WriteU64(&bits, 1, 0);
    WriteU64(&bits, 1, 0);
    WriteU64(&bits, 13, 7 + dataSize);
    WriteU64(&bits, 11, 0x7FF);
    WriteU64(&bits, 2, 0);

    mediaData_.size_ = 7 + dataSize;
    mediaData_.data_ = new unsigned char[mediaData_.size_];
    unsigned char p64[8];
    p64[0] = (unsigned char)(bits >> 56);
    p64[1] = (unsigned char)(bits >> 48);
    p64[2] = (unsigned char)(bits >> 40);
    p64[3] = (unsigned char)(bits >> 32);
    p64[4] = (unsigned char)(bits >> 24);
    p64[5] = (unsigned char)(bits >> 16);
    p64[6] = (unsigned char)(bits >> 8);
    p64[7] = (unsigned char)(bits);
    memcpy(mediaData_.data_, p64+1, 7);
    memcpy(mediaData_.data_ + 7, pTagData + 2, dataSize);
    return 1;
}

int VideoTag::ParseH264Tag(FlvFile *file) {
    // VIDEO TAG:
    // frametype:4bit codecid:4bit avcpackettype:1byte compositiontime:3byte
    // 总共 5byte
    unsigned char *pd = tagData_.data_;
    // 3字节
    avcPacketType_ = (AVCPacketType)pd[1];
    // 占三个字节
    compositionTime_ = ShowU24(pd + 2);

    dts_ = tagHeader_.totalTimeStamp_;
    pts_ = dts_ + compositionTime_;
    if(frameType_ == FRAME_TYPE_KEY) {
        //
    }

    if (avcPacketType_ == AVC_PACKET_TYPE_SEQUENCE_HEADER) {
        ParseH264Configuration(file, pd);
    }
    else if (avcPacketType_ == AVC_PACKET_TYPE_NALU)
    {
        ParseNalu(file, pd);
    }
    else if(avcPacketType_ == AVC_PACKET_TYPE_SEQUENCE_END) {
        debug("xxxxxxavcpacket_type:%d\n", avcPacketType_);
    }
    return 1;
}

int VideoTag::ParseH264Configuration(FlvFile *pParser, unsigned char *pTagData) {
    // 从6byte开始为avcheader的数据
    unsigned char *pd = pTagData;
    // 表示NALU length的长度所占的字节数
    pParser->nalUnitLength_ = (pd[9] & 0x03) + 1;
    debug("nalu length:%d", pParser->nalUnitLength_);

    int sps_size = 0;
    int pps_size = 0;
    int sps_num = 0;
    int pps_num = 0;
    // pps sps都只有一个
    sps_num = ShowU8(pd + 10) & 0x1f;
    sps_size = ShowU16(pd + 11);
    debug("sps_num:%d sps_size:%d", sps_num, sps_size);
    pps_num = ShowU8(pd + 11 + (2 + sps_size));
    debug("pps num:%d", pps_num);
    pps_size = ShowU16(pd + 11 + (2 + sps_size) + 1);
    debug("pps size:%d", pps_size);
    // 4字节的startcode
    mediaData_.size_ = 4 + sps_size + 4 + pps_size;
    mediaData_.data_ = new unsigned char[mediaData_.size_];
    memcpy(mediaData_.data_, &h264StartCode, 4);
    memcpy(mediaData_.data_ + 4, pd + 11 + 2, sps_size);
    memcpy(mediaData_.data_ + 4 + sps_size, &h264StartCode, 4);
    memcpy(mediaData_.data_ + 4 + sps_size + 4, pd + 11 + 2 + sps_size + 2 + 1, pps_size);
    return 1;
}

// videotag
VideoTag::VideoTag(TagHeader *pHeader, unsigned char *pBuf, uint32_t nLeftLen,
        FlvFile *pParser) {
    // debug("videotag\n");
    init(pHeader, pBuf, nLeftLen);
    unsigned char *pd = tagData_.data_;
    // 帧类型
    frameType_ = FrameType((pd[0] & 0xf0) >> 4);
    // 编码id
    codecId_ = CodecId(pd[0] & 0x0f);
    if (tagHeader_.tagType_ == TAG_TYPE_VIDEO && codecId_ == CODEC_ID_AVC) {
        ParseH264Tag(pParser);
    }
}

// 解析NALU
int VideoTag::ParseNalu(FlvFile *pParser, unsigned char *pTagData) {
    unsigned char *pd = pTagData;
    size_t nOffset = 0;

    mediaData_.data_ = new unsigned char[tagHeader_.dataSize_+10];
    mediaData_.size_ = 0;

    nOffset = 5;
    while (1)  {
        if (nOffset >= tagHeader_.dataSize_)
            break;

        int nNaluLen;
        // 4个字节
        switch (pParser->nalUnitLength_)
        {
        case 4:
            nNaluLen = ShowU32(pd + nOffset);
            break;
        case 3:
            nNaluLen = ShowU24(pd + nOffset);
            break;
        case 2:
            nNaluLen = ShowU16(pd + nOffset);
            break;
        default:
            nNaluLen = ShowU8(pd + nOffset);
        }
        memcpy(mediaData_.data_ + mediaData_.size_, &h264StartCode, 4);
        memcpy(mediaData_.data_ + mediaData_.size_ + 4,
                pd + nOffset + pParser->nalUnitLength_, nNaluLen);
        mediaData_.size_ += (4 + nNaluLen);
        nOffset += (pParser->nalUnitLength_ + nNaluLen);
    }
    return 1;
}

ScriptTag::ScriptTag(TagHeader *pHeader, unsigned char *pBuf,
        uint32_t nLeftLen, FlvFile *pParser) {
    //debug("script tag\n");
    init(pHeader, pBuf, nLeftLen);

    unsigned char *pd = tagData_.data_;
    // 开始解析tag
    int i = 0;
    int type = pd[i++];
    //debug("amf type:%d\n", type);
    std::string key;
    std::string value;
    if(type == AMF_DATA_TYPE_STRING) {
        key = amfGetString(pd + i, nLeftLen - i);
    } else {
        return;
    }
    i += 2;
    i += key.length();

    // 解析metadata
    if(key == "onMetaData") {
        parseMetadata(pd + i, nLeftLen -i, "onMetadata");
    }
    debug("Metdata:width:%lf, height:%lf", width_, height_);
}

// 解析metadata
int ScriptTag::parseMetadata(unsigned char* data, uint32_t size,
        std::string key) {
    std::string valueString;
    double valueNum;
    int i = 0;
    int valueType = data[i++];
    int valueLen = 0;
    switch(valueType) {
        case AMF_DATA_TYPE_MIXEDARRAY: {
            int arrLen = ShowU32(data + i);
            i += 4;
            for(int j = 0; j < arrLen; ++j) {
                if(j != 0) {
                    ++i;
                }
                key = amfGetString(data + i, size-i);
                i += 2;
                i += key.length();
                valueLen = parseMetadata(data + i, size - i, key);
                i += valueLen;
            }
            break;
        }
        case AMF_DATA_TYPE_STRING: {
            valueString = amfGetString(data + i, size -i);
            i += 2;
            i += valueString.length();
            valueLen = valueString.length() + 2;
            break;
        }
        case AMF_DATA_TYPE_BOOL: {
            valueNum = data[i++];
            valueLen = 1;
            break;
        }
        case AMF_DATA_TYPE_NUMBER: {
            valueNum = ShowDouble(data + i);
            valueLen = 8;
            i += 8;
            break;
        }
        default:
        break;
    }
    if(key == "duration") {
        duration_ = valueNum;
    }
    else if(key == "width") {
        width_ = valueNum;
    }
    else if(key == "height") {
        height_ = valueNum;
    }
    else if(key == "videodatarate") {
        videoDataRate_ = valueNum;
    }
    else if(key == "framerate") {
        framerate_ = valueNum;
    }
    else if(key == "videocodecid") {
        videoCodecId_ = valueNum;
    }
    else if(key == "audiosamplerate") {
        audioSampleRate_ = valueNum;
    }
    else if(key == "audiosamplesize") {
        audioSampleSize_ = valueNum;
    }
    else if(key == "stereo") {
        stereo_ = valueNum;
    }
    else if(key == "audiocodecid") {
        audioCodecId_ = valueNum;
    }
    else if(key == "filesize") {
        fileSize_ = valueNum;
    }
    debug("key :%s value:%.2f", key.c_str(),valueNum);
    return valueLen;
}


FlvFile::FlvFile() {
    flvHeader_ = NULL;
    unusedLen_ = 0;
    memset(buf_, 0, sizeof(buf_));
    m_h264_fp = NULL;
    m_aac_fp = NULL;
    // h264解析器初始化
}

FlvFile::~FlvFile() {
    if (m_h264_fp) {
        fclose(m_h264_fp);
        m_h264_fp = NULL;
    }
    if (m_aac_fp) {
        fclose(m_aac_fp);
        m_aac_fp = NULL;
    }
    if (flvHeader_) {
        delete flvHeader_;
        flvHeader_ = NULL;
    }
}

int FlvFile::init(const std::string& fileName, const char* h264_file, const char* aac_file) {
    if(fileName.empty())     {
        debug("init file is eempty");
        return -1;
    }
    fileName_ = fileName;
    fs_.open(fileName_, std::ios_base::in | std::ios_base::binary);
    if (!fs_ || !fs_.is_open() || !fs_.good()) {
        debug("file is error");
        return -1;
    }
    m_h264_fp = fopen(h264_file, "wb");
    m_aac_fp = fopen(aac_file, "wb");
    return 0;
}

int FlvFile::destory() {
    // 关闭文件
    fs_.close();
    return 0;
}

Tag* FlvFile::parseFlv() {
    // 4M的buf
    int readLen = 0;
    int usedLen = 0;
    Tag* tag = nullptr;
    while (1) {
        // 读数据
        fs_.read(reinterpret_cast<char *>(buf_) + unusedLen_,
            sizeof(buf_) - unusedLen_);
        readLen = fs_.gcount();
        if (readLen == 0 && unusedLen_ == 0) {
            debug("file is end");
            return nullptr;
        }
        unusedLen_ += readLen;
        tag = parse(buf_, unusedLen_, &usedLen);
        if (!tag) break;
        if (unusedLen_ != usedLen) {
            memmove(buf_, buf_ + usedLen, unusedLen_ - usedLen);
        }
        if (m_h264_fp && tag->tagHeader_.tagType_ == TAG_TYPE_VIDEO) {
            fwrite(tag->mediaData_.data_, 1, tag->mediaData_.size_, m_h264_fp); 
        } else if (m_aac_fp && tag->tagHeader_.tagType_ == TAG_TYPE_AUDIO) {
             AudioTag *pAudioTag = reinterpret_cast<AudioTag *>(tag);
            if (pAudioTag->soundFormat_ == 10) {
                fwrite(tag->mediaData_.data_, 1, tag->mediaData_.size_, m_aac_fp); 
            }
        } else {
            debug("tag type=%d", tag->tagHeader_.tagType_);
        }
        unusedLen_ -= usedLen;
        delete tag;
    } 
    return tag;
}

/*
 * 参数一：数据
 * 参数二：数据的大小
 * 参数三：已经解析的数据
 */
Tag* FlvFile::parse(unsigned char *pBuf, int nBufSize, int *usedLen) {
    int nOffset = 0;
    // 初始化flvheader
    if (flvHeader_ == NULL) {
        CheckBuffer(FLV_HEADER_SIZE);
        // 保存flvheader
        flvHeader_ = CreateFlvHeader(pBuf+nOffset);
        nOffset += flvHeader_->headerSize_;
    }
    Tag* tag = nullptr;
    CheckBuffer(TAG_HEADER_SIZE);
    // int nPrevSize = ShowU32(pBuf + nOffset);
    nOffset += 4;
    tag = CreateTag(pBuf + nOffset, nBufSize-nOffset);
    if (tag == NULL) {
        debug("CreateTag failed");
        nOffset -= 4;
        *usedLen = nOffset;
        return tag;
    }
    nOffset += (11 + tag->tagHeader_.dataSize_);
    *usedLen = nOffset;
    return tag;
}


// 文件头
FlvHeader *FlvFile::CreateFlvHeader(unsigned char *pBuf) {
    FlvHeader *pHeader = new FlvHeader;
    pHeader->version_ = pBuf[3];
    pHeader->haveAudio_ = (pBuf[4] >> 2) & 0x01;
    pHeader->haveVideo_ = (pBuf[4] >> 0) & 0x01;
    pHeader->headerSize_ = ShowU32(pBuf + 5);

    pHeader->data_ = new unsigned char[pHeader->headerSize_];
    //拷贝包头的数据
    memcpy(pHeader->data_, pBuf, pHeader->headerSize_);
    return pHeader;
}

Tag *FlvFile::CreateTag(unsigned char *pBuf, size_t nLeftLen) {
    TagHeader header;
    header.tagType_ = (TagType)ShowU8(pBuf+0);
    header.dataSize_ = ShowU24(pBuf + 1);
    header.timeStamp_ = ShowU24(pBuf + 4);
    header.timeStampEx_ = ShowU8(pBuf + 7);
    header.streamId_ = ShowU24(pBuf + 8);
    header.totalTimeStamp_ = (unsigned int)((header.timeStampEx_ << 24)) +
        header.timeStamp_;
    if ((header.dataSize_ + 11) > nLeftLen) {
        debug("error header datasize:%d nLeftLen:%d", header.dataSize_, nLeftLen);
        return NULL;
    }

    Tag *pTag;
    switch (header.tagType_) {
    case TAG_TYPE_VIDEO:
        pTag = new VideoTag(&header, pBuf, nLeftLen, this);
        break;
    case TAG_TYPE_AUDIO:
        pTag = new AudioTag(&header, pBuf, nLeftLen, this);
        break;
    case TAG_TYPE_SCRIPT:
        pTag = new ScriptTag(&header, pBuf, nLeftLen, this);
        metadata_ = reinterpret_cast<ScriptTag*>(pTag);
        break;
    default:
        pTag = new Tag();
        pTag->init(&header, pBuf, nLeftLen);
    }
    return pTag;
}

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        std::cout << argv[0] <<" [file name]" << std::endl;
        return 0;
    }
    FlvFile file;
    file.init(argv[1], "test.h264", "test.aac");
    file.parseFlv();
    return 0;
}