#ifndef MP4_BOX_H
#define MP4_BOX_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "util.h"

typedef struct MP4_BOX_BASIC
{
    uint32_t box_length;
    char box_name[5];
} MP4_BOX_BASIC;

typedef struct Ftyp_Box
{
    MP4_BOX_BASIC box_basic;
    char major_brand[5];
    uint32_t minor_version;
    char *compatible_brands;
} Ftyp_Box;

typedef struct Free_Box
{
    MP4_BOX_BASIC box_basic;
} Free_Box;

typedef struct Mdat_Box
{
    MP4_BOX_BASIC box_basic;
} Mdat_Box;

typedef struct Mvhd_Box
{
    MP4_BOX_BASIC box_basic;
    uint8_t version;
    uint8_t flags[3];
    uint32_t create_time;       //生成时间
    uint32_t motify_time;       //修改时间
    uint32_t time_scale;    
    uint32_t duration;
    uint32_t preferred_rate;    //播放速度 
    uint16_t preferred_volume;  //播放声音
    uint8_t reserved[10];       //保留
    uint32_t matrix[9];         //矩阵
    uint32_t preview_time;      //预览时间
    uint32_t preview_duration;  
    uint32_t post_time;          
    uint32_t selection_time;
    uint32_t selection_duration;
    uint32_t current_time;      //当前时间
    uint32_t next_track_id;     //下一个Track ID
} Mvhd_Box;

typedef struct Data_Box
{
    MP4_BOX_BASIC box_basic;
} Data_Box;

typedef struct Tkhd_Box
{
    MP4_BOX_BASIC box_basic;
    uint8_t version;
    uint8_t flags[3];
    uint32_t create_time;
    uint32_t motify_time;
    uint32_t track_id;          //当前Track的id
    uint8_t reserved[4];
    uint32_t duration;
    uint8_t reserved2[8];
    uint16_t layer;
    uint16_t alternate_group;
    uint16_t volume;
    uint8_t reserved3[2];
    uint32_t matrix[9];
    uint32_t width;
    uint32_t height;
} Tkhd_Box;

typedef struct Elst_Entry
{
    uint32_t track_duration;
    uint32_t media_time;
    uint32_t media_rate;
} Elst_Entry;

typedef struct Elst_Box
{
    MP4_BOX_BASIC box_basic;
    uint8_t version;
    uint8_t flags[3];
    uint32_t entry_num;     //elst box中包含的条目数量
    Elst_Entry *entrys;     //包含的条目
} Elst_Box;

typedef struct Edts_Box
{
    MP4_BOX_BASIC box_basic;
    Elst_Box *elst_box;
} Edts_Box;

typedef struct Mdhd_Box
{
    MP4_BOX_BASIC box_basic;
    uint8_t version;
    uint8_t flags[3];
    uint32_t create_time;
    uint32_t motify_time;
    uint32_t time_scale;
    uint32_t duration;
    uint16_t language;
    uint16_t quality;
} Mdhd_Box;

typedef struct Iods_Box
{
    MP4_BOX_BASIC box_basic;
} Iods_Box;

typedef struct Hdlr_Box
{
    MP4_BOX_BASIC box_basic;
    uint8_t version;
    uint8_t flags[3];
    uint32_t component_type;
    uint32_t component_subtype;
    uint32_t component_manufacturer;
    uint32_t component_flags;
    uint32_t component_flags_mask;
    char *component_name;
} Hdlr_Box;

typedef struct Vmhd_Box
{
    MP4_BOX_BASIC box_basic;
    uint8_t version;
    uint8_t flags[3];
    uint16_t graphics_mode;
    uint16_t opcolor[3];
} Vmhd_Box;

typedef struct Smhd_Box
{
    MP4_BOX_BASIC box_basic;
    uint8_t version;
    uint8_t flags[3];
    uint16_t balance;
    uint8_t reserved[2];
} Smhd_Box;

typedef struct Url_Box
{
    MP4_BOX_BASIC box_basic;
    uint8_t version;
    uint8_t flags[3];
} Url_Box;

typedef struct Urn_Box
{
    MP4_BOX_BASIC box_basic;
    uint8_t version;
    uint8_t flags[3];
} Urn_Box;

typedef struct Dref_Box
{
    MP4_BOX_BASIC box_basic;
    uint8_t version;
    uint8_t flags[3];
    uint32_t entry_num;
    void *arg;
} Dref_Box;

// Data Information 
typedef struct Dinf_Box
{
    MP4_BOX_BASIC box_basic;
    Dref_Box *dref_box;
} Dinf_Box;

typedef struct SPS
{
    uint16_t sequenceParameterSetLength;
    uint8_t *sequenceParameterSetNALUnit;
}SPS;

typedef struct PPS
{
    uint16_t pictureParameterSetLength;
    uint8_t *pictureParameterSetNALUnit;
}PPS;

typedef struct Mp4_avcC_Box
{
	MP4_BOX_BASIC box_basic;
	uint8_t       configurationVersion;      //=1
	uint8_t       AVCProfileIndication;
	uint8_t       profile_compatibility;
	uint8_t       AVCLevelIndication;

	uint8_t       lengthSizeMinusOne;        // & 0x3,  ==2 bit
	uint8_t       numOfSequenceParameterSet; // & 0x1F  ==5bit

    SPS* sps;

	uint8_t       numOfPictureParameterSets;

    PPS* pps;
} Mp4_avcC_Box;

typedef struct Mp4_AVC1_Box 
{
    MP4_BOX_BASIC box_basic;
    char reserved[6];
    uint16_t data_reference_index;
    uint16_t pre_defined;
    uint16_t reserved1;
    uint32_t pre_defined1[3];
    uint16_t width;
    uint16_t height;
    uint32_t horiz_res;
    uint32_t vert_res;
    uint32_t reserved2;
    uint16_t frame_count;
    char compressor_name[33];
    uint8_t bit_depth;
    uint16_t pre_defined2;
	Mp4_avcC_Box *avcC;
} Mp4_AVC1_Box;

typedef struct Mp4_Esds_Box
{
    MP4_BOX_BASIC box_basic;

} Mp4_Esds_Box;

typedef struct Mp4_MP4A_Box
{
    MP4_BOX_BASIC box_basic;
    uint8_t reserved[6];
    uint16_t dref_id;
    uint16_t version;
    uint16_t revision_level;
    uint32_t vendor;
    uint16_t channels;
    uint16_t bits_per_coded_sample;
    uint16_t audio_cid;
    uint16_t packet_size;
    uint32_t sample_rate;
    Mp4_Esds_Box *esds_box;
} Mp4_MP4A_Box;

typedef struct Sample_Description
{
	struct Mp4_AVC1_Box *avc1;
	struct Mp4_MP4A_Box *mp4a;
} Sample_Description;

typedef struct Stsd_Box
{
    MP4_BOX_BASIC box_basic;
    uint8_t version;
    uint8_t flags[3];
    uint32_t number_of_entries;
    Sample_Description *sample_descriptions;
} Stsd_Box;

typedef struct Time_To_Sample_Entry
{
    uint32_t sample_count;
    uint32_t sample_duration;
} Time_To_Sample_Entry;

// Time-to-Sample atom存储了media sample的duration信息
typedef struct Stts_Box
{
    MP4_BOX_BASIC box_basic;
    uint8_t version;
    uint8_t flags[3];
    uint32_t number_of_entries;
    Time_To_Sample_Entry *time_to_sample_entrys;
} Stts_Box;

// Sync sample stom确定了media中的关键帧，也就是所谓的I帧，关键帧的解码不依赖B帧与P帧。
typedef struct Stss_Box
{
    MP4_BOX_BASIC box_basic;
    uint8_t version;
    uint8_t flags[3];
    uint8_t sync_sample_count;
    uint32_t *sync_sample_number;
} Stss_Box;

typedef struct Ctts_Box
{
    MP4_BOX_BASIC box_basic;
} Ctts_Box;

typedef struct Sample_To_Chunk_Entry
{
    uint32_t first_chunk;
    uint32_t samples_per_chunk;
    uint32_t sample_description_id;
} Sample_To_Chunk_Entry;

// Sample-to-Chunk Atoms
typedef struct Stsc_Box
{
    MP4_BOX_BASIC box_basic;
    uint8_t version;
    uint8_t flags[3];
    uint32_t sample_to_chunk_numbers;
    Sample_To_Chunk_Entry *sample_to_chunk_entrys;
} Stsc_Box;

// Sample Size atom 定义了每个sample的大小
typedef struct Stsz_Box
{
    MP4_BOX_BASIC box_basic;
    uint8_t version;
    uint8_t flags[3];
    uint32_t sample_size; //全部sample的数目，如果所有的sample有相同的长度，这个字段就是这个值，否则，这个字段的值就是0
    uint32_t sample_size_entry_number;
    uint32_t *sample_size_tables;
} Stsz_Box;

typedef struct Stco_Box
{
    MP4_BOX_BASIC box_basic;
    uint8_t version;
    uint8_t flags[3];
    uint32_t chunk_numbers;
    uint32_t *chunk_offset_tables;
} Stco_Box;

// Sample Table ，包含了从媒体时间到实际的Sample
typedef struct Stbl_Box
{
    MP4_BOX_BASIC box_basic;
    Stsd_Box *stsd_box;
    Stts_Box *stts_box;
    Stss_Box *stss_box;
    Ctts_Box *ctts_box;
    Stsc_Box *stsc_box;
    Stsz_Box *stsz_box;
    Stco_Box *stco_box;
} Stbl_Box;

typedef struct Minf_Box
{
    MP4_BOX_BASIC box_basic;
    void *m_hd;  //当Track时视频的时候，m_hd指向Vmhd_Box，当时音频时，m_hd指向Smhd_Box
    Dinf_Box *dinf_box;     
    Stbl_Box *stbl_box;
} Minf_Box;

typedef struct Mdia_Box
{
    MP4_BOX_BASIC box_basic;
    Mdhd_Box *mdhd_box;
    Hdlr_Box *hdlr_box;
    Minf_Box *minf_box;
} Mdia_Box;

typedef struct Trak_Box
{
    MP4_BOX_BASIC box_basic;
    Tkhd_Box *tkhd_box;
    Edts_Box *edts_box;
    Mdia_Box *mdia_box;
} Trak_Box;

typedef struct Moov_Box
{
    MP4_BOX_BASIC box_basic;
    Mvhd_Box *mvhd_box;
    Iods_Box *iods_box;
    Data_Box *data_box;
    int trak_index;
    Trak_Box track_box[3];
} Moov_Box;

typedef struct Mp4_File_Root_Box
{
    Ftyp_Box *ftyp_box;
    Moov_Box *moov_box;
    Mdat_Box *mdat_box;
    Free_Box *free_box;
} Mp4_File_Root_Box;

#endif
