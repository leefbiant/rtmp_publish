#ifndef MP4_PARSE_H
#define MP4_PARSE_H

#include "mp4_box.h"

#define     MP4_BOX_SIZE        10

#define err_exit(str) \
	do \
	{ \
		perror(str); \
		exit(1); \
	}while(0) \

typedef struct Mp4_Parse_Enrty
{
    uint32_t type;
    Boolean (*mp4_read_box)(void *, FILE *);
} Mp4_Parse_Enrty;

typedef struct Mp4_Info
{
    char video_codec[100];
    char audio_codec[100];
    int MAX_BOX_SIZE;
    int video_duration;
    int delay_video_media_time;

    int audio_duration;
    int delay_audio_media_time;

} Mp4_Info;

Boolean mp4_read_ftyp_box(void *arg, FILE *mp4_file);
Boolean mp4_read_moov_box(void *arg, FILE *mp4_file);
Boolean mp4_read_free_box(void *arg, FILE *mp4_file);
Boolean mp4_read_mdat_box(void *arg, FILE *mp4_file);
Boolean mp4_read_mvhd_box(void *arg, FILE *mp4_file);
Boolean mp4_read_trak_box(void *arg, FILE *mp4_file);
Boolean mp4_read_tkhd_box(void *arg, FILE *mp4_file);
Boolean mp4_read_edts_box(void *arg, FILE *mp4_file);
Boolean mp4_read_mdia_box(void *arg, FILE *mp4_file);
Boolean mp4_read_elst_box(void *arg, FILE *mp4_file);
Boolean mp4_read_mdhd_box(void *arg, FILE *mp4_file);
Boolean mp4_read_hdlr_box(void *arg, FILE *mp4_file);
Boolean mp4_read_minf_box(void *arg, FILE *mp4_file);
Boolean mp4_read_vmhd_box(void *arg, FILE *mp4_file);
Boolean mp4_read_smhd_box(void *arg, FILE *mp4_file);
Boolean mp4_read_dinf_box(void *arg, FILE *mp4_file);
Boolean mp4_read_stbl_box(void *arg, FILE *mp4_file);
Boolean mp4_read_dref_box(void *arg, FILE *mp4_file);
Boolean mp4_read_url_box(void *arg, FILE *mp4_file);
Boolean mp4_read_stsd_box(void *arg, FILE *mp4_file);
Boolean mp4_read_stts_box(void *arg, FILE *mp4_file);
Boolean mp4_read_stss_box(void *arg, FILE *mp4_file);
Boolean mp4_read_ctts_box(void *arg, FILE *mp4_file);
Boolean mp4_read_stsc_box(void *arg, FILE *mp4_file);
Boolean mp4_read_stsz_box(void *arg, FILE *mp4_file);
Boolean mp4_read_stco_box(void *arg, FILE *mp4_file);
Boolean mp4_read_avc1_box(void *arg, FILE *mp4_file);
Boolean mp4_read_avcC_box(void *arg, FILE *mp4_file);
Boolean mp4_read_mp4a_box(void *arg, FILE *mp4_file);
Boolean mp4_read_esds_box(void *arg, FILE *mp4_file);
Boolean mp4_read_iods_box(void *arg, FILE *mp4_file);

Mp4_Parse_Enrty mp4_parse_table[] = {
    {MP4_TAG('f', 't', 'y', 'p'), mp4_read_ftyp_box},
    {MP4_TAG('m', 'o', 'o', 'v'), mp4_read_moov_box},
    {MP4_TAG('f', 'r', 'e', 'e'), mp4_read_free_box},
    {MP4_TAG('m', 'd', 'a', 't'), mp4_read_mdat_box},
    {MP4_TAG('m', 'v', 'h', 'd'), mp4_read_mvhd_box},
    {MP4_TAG('t', 'r', 'a', 'k'), mp4_read_trak_box},
    {MP4_TAG('t', 'k', 'h', 'd'), mp4_read_tkhd_box},
    {MP4_TAG('e', 'd', 't', 's'), mp4_read_edts_box},
    {MP4_TAG('m', 'd', 'i', 'a'), mp4_read_mdia_box},
    {MP4_TAG('e', 'l', 's', 't'), mp4_read_elst_box},
    {MP4_TAG('m', 'd', 'h', 'd'), mp4_read_mdhd_box},
    {MP4_TAG('h', 'd', 'l', 'r'), mp4_read_hdlr_box},
    {MP4_TAG('m', 'i', 'n', 'f'), mp4_read_minf_box},
    {MP4_TAG('v', 'm', 'h', 'd'), mp4_read_vmhd_box},
    {MP4_TAG('s', 'm', 'h', 'd'), mp4_read_smhd_box},
    {MP4_TAG('d', 'i', 'n', 'f'), mp4_read_dinf_box},
    {MP4_TAG('s', 't', 'b', 'l'), mp4_read_stbl_box},
    {MP4_TAG('d', 'r', 'e', 'f'), mp4_read_dref_box},
    {MP4_TAG('u', 'r', 'l', ' '), mp4_read_url_box },
    {MP4_TAG('s', 't', 's', 'd'), mp4_read_stsd_box},
    {MP4_TAG('s', 't', 't', 's'), mp4_read_stts_box},
    {MP4_TAG('s', 't', 's', 's'), mp4_read_stss_box},
    {MP4_TAG('c', 't', 't', 's'), mp4_read_ctts_box},
    {MP4_TAG('s', 't', 's', 'c'), mp4_read_stsc_box},
    {MP4_TAG('s', 't', 's', 'z'), mp4_read_stsz_box},
    {MP4_TAG('s', 't', 'c', 'o'), mp4_read_stco_box},
    {MP4_TAG('a', 'v', 'c', '1'), mp4_read_avc1_box},
    {MP4_TAG('a', 'v', 'c', 'C'), mp4_read_avcC_box},
    {MP4_TAG('m', 'p', '4', 'a'), mp4_read_mp4a_box},
    {MP4_TAG('e', 's', 'd', 's'), mp4_read_esds_box},
    {MP4_TAG('i', 'o', 'd', 's'), mp4_read_iods_box},
};
#endif