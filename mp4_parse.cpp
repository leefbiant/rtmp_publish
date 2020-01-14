#include "mp4_parse.h"

static Mp4_Info mp4_info;

Boolean mp4_read_basic_box(void *arg, FILE *mp4_file)
{
    MP4_BOX_BASIC *mp4_box_basic = (MP4_BOX_BASIC *)arg;
    uint32_t box_temp_size;
    fread(&box_temp_size, 1, 4, mp4_file);
    mp4_box_basic->box_length = get_big_endian32(box_temp_size);
    fread(mp4_box_basic->box_name, 1, 4, mp4_file);
    mp4_box_basic->box_name[4] = '\0';
    return True;
}

Boolean mp4_read_ftyp_box(void *arg, FILE *mp4_file)
{
    Mp4_File_Root_Box *mp4_file_root_box = (Mp4_File_Root_Box *)arg;
    Ftyp_Box *ftyp_box = (Ftyp_Box *)malloc(sizeof(Ftyp_Box));
    uint32_t minor_temp_version;

    mp4_read_basic_box(&(ftyp_box->box_basic), mp4_file);

    fread(ftyp_box->major_brand, 1, 4, mp4_file);

    fread(&minor_temp_version, 1, 4, mp4_file);
    ftyp_box->minor_version = get_big_endian32(minor_temp_version);
    ftyp_box->compatible_brands = (char *)malloc(ftyp_box->box_basic.box_length-16+1);
    fread(ftyp_box->compatible_brands, 1, ftyp_box->box_basic.box_length-16, mp4_file);
    ftyp_box->compatible_brands[ftyp_box->box_basic.box_length-16] = '\0';
    printf("box name: %s, %d, minor_version: %d, compatible_brands: %s\n", ftyp_box->box_basic.box_name,
        ftyp_box->box_basic.box_length, ftyp_box->minor_version, ftyp_box->compatible_brands);

    mp4_file_root_box->ftyp_box = ftyp_box;
    return True;
}

Boolean mp4_read_moov_box(void *arg, FILE *mp4_file)
{
    Mp4_File_Root_Box *mp4_file_root_box = (Mp4_File_Root_Box *)arg;
    Moov_Box *moov_box = (Moov_Box *)malloc(sizeof(Moov_Box));
    moov_box->trak_index = 0;
    uint32_t moov_current_pos = 0;
    uint32_t box_temp_size = 0;
    uint32_t box_size;
    uint32_t box_type;
    int i;
    
    mp4_read_basic_box(&(moov_box->box_basic), mp4_file);
    printf("box name: %s, %d\n", moov_box->box_basic.box_name, moov_box->box_basic.box_length);
    moov_current_pos += 8;
    while (moov_current_pos < moov_box->box_basic.box_length)
    {
        fread(&box_temp_size, 1, 4, mp4_file);
        box_size = get_big_endian32(box_temp_size);
        fread(&box_type, 1, 4, mp4_file);
        fseek(mp4_file, -8, SEEK_CUR);
        for (i = 0; i < mp4_info.MAX_BOX_SIZE; i++)
        {
            if (mp4_parse_table[i].type == box_type)
            {
                mp4_parse_table[i].mp4_read_box(moov_box, mp4_file);
                break;
            }
        }
        
        moov_current_pos += box_size;
    }
    mp4_file_root_box->moov_box = moov_box;
    return True;
}

Boolean mp4_read_free_box(void *arg, FILE *mp4_file)
{
    Mp4_File_Root_Box *mp4_file_root_box = (Mp4_File_Root_Box *)arg;
    Free_Box *free_box = (Free_Box *)malloc(sizeof(Free_Box));
    mp4_read_basic_box(&(free_box->box_basic), mp4_file);
    mp4_file_root_box->free_box = free_box;
    printf("box name: %s, %d\n", free_box->box_basic.box_name, free_box->box_basic.box_length);
    return True;
}

Boolean mp4_read_mdat_box(void *arg, FILE *mp4_file)
{
    Mp4_File_Root_Box *mp4_file_root_box = (Mp4_File_Root_Box *)arg;
    Mdat_Box *mdat_box = (Mdat_Box *)malloc(sizeof(Mdat_Box));
    mp4_read_basic_box(&(mdat_box->box_basic), mp4_file);
    printf("box name: %s, %d\n", mdat_box->box_basic.box_name, mdat_box->box_basic.box_length);
    fseek(mp4_file, mdat_box->box_basic.box_length-8, SEEK_CUR);
    mp4_file_root_box->mdat_box = mdat_box;
    return True;
}

Boolean mp4_read_mvhd_box(void *arg, FILE *mp4_file)
{
    Moov_Box *moov_box = (Moov_Box *)arg;
    Mvhd_Box *mvhd_box = (Mvhd_Box *)malloc(sizeof(Mvhd_Box));
    uint32_t temp32;
    uint16_t temp16;

    mp4_read_basic_box(&(mvhd_box->box_basic), mp4_file);
    printf("box name: %s, %d\n", mvhd_box->box_basic.box_name, mvhd_box->box_basic.box_length);
    fread(&(mvhd_box->version), 1, 1, mp4_file);
    fread(mvhd_box->flags, 1, 3, mp4_file);

    fread(&temp32, 1, 4, mp4_file);
    mvhd_box->create_time = get_big_endian32(temp32);

    fread(&temp32, 1, 4, mp4_file);
    mvhd_box->motify_time = get_big_endian32(temp32);

    fread(&temp32, 1, 4, mp4_file);
    mvhd_box->time_scale = get_big_endian32(temp32);

    fread(&temp32, 1, 4, mp4_file);
    mvhd_box->duration = get_big_endian32(temp32);

    fread(&temp32, 1, 4, mp4_file);
    mvhd_box->preferred_rate = get_big_endian32(temp32);

    fread(&temp16, 1, 2, mp4_file);
    mvhd_box->preferred_volume = get_big_endian16(temp16);
    
    fread(mvhd_box->reserved, 1, 10, mp4_file);
    fread(mvhd_box->matrix, 4, 9, mp4_file);

    fread(&temp32, 1, 4, mp4_file);
    mvhd_box->preview_time = get_big_endian32(temp32);

    fread(&temp32, 1, 4, mp4_file);
    mvhd_box->preview_duration = get_big_endian32(temp32);

    fread(&temp32, 1, 4, mp4_file);
    mvhd_box->post_time = get_big_endian32(temp32);

    fread(&temp32, 1, 4, mp4_file);
    mvhd_box->selection_time = get_big_endian32(temp32);

    fread(&temp32, 1, 4, mp4_file);
    mvhd_box->selection_duration = get_big_endian32(temp32);

    fread(&temp32, 1, 4, mp4_file);
    mvhd_box->current_time = get_big_endian32(temp32);

    fread(&temp32, 1, 4, mp4_file);
    mvhd_box->next_track_id = get_big_endian32(temp32);
    moov_box->mvhd_box = mvhd_box;
    printf("mvhd box track_id: %d\n", mvhd_box->next_track_id);
    return True;
}

Boolean mp4_read_iods_box(void *arg, FILE *mp4_file)
{
    Moov_Box *moov_box = (Moov_Box *)arg;
    Iods_Box *iods_box = (Iods_Box *)malloc(sizeof(Iods_Box));

    mp4_read_basic_box(&(iods_box->box_basic), mp4_file);
    printf("box name: %s, %d\n", iods_box->box_basic.box_name, iods_box->box_basic.box_length);
    fseek(mp4_file, iods_box->box_basic.box_length-8, SEEK_CUR);
    moov_box->iods_box = iods_box;
    return True;
}

Boolean mp4_read_trak_box(void *arg, FILE *mp4_file)
{
    Moov_Box *moov_box = (Moov_Box *)arg;
    Trak_Box *track_box = &(moov_box->track_box[moov_box->trak_index]);
    uint32_t trak_current_pos = 0;
    uint32_t box_temp_size = 0;
    uint32_t box_size;
    uint32_t box_type;
    int i;
    
    moov_box->trak_index ++;
    mp4_read_basic_box(&(track_box->box_basic), mp4_file);
    printf("box name: %s, %d\n", track_box->box_basic.box_name, track_box->box_basic.box_length);
    trak_current_pos += 8;
    while (trak_current_pos < track_box->box_basic.box_length)
    {
        fread(&box_temp_size, 1, 4, mp4_file);
        box_size = get_big_endian32(box_temp_size);
        fread(&box_type, 1, 4, mp4_file);
        fseek(mp4_file, -8, SEEK_CUR);
        for (i = 0; i < mp4_info.MAX_BOX_SIZE; i++)
        {
            if (mp4_parse_table[i].type == box_type)
            {
                mp4_parse_table[i].mp4_read_box(track_box, mp4_file);
                break;
            }
        }
        trak_current_pos += box_size;
    }
    
    return True;
}

Boolean mp4_read_tkhd_box(void *arg, FILE *mp4_file)
{
    Trak_Box *track_box = (Trak_Box *)arg;
    Tkhd_Box *tkhd_box = (Tkhd_Box *)malloc(sizeof(Tkhd_Box));
    uint32_t temp32;
    uint16_t temp16;

    mp4_read_basic_box(&(tkhd_box->box_basic), mp4_file);
    printf("box name: %s, %d\n", tkhd_box->box_basic.box_name, tkhd_box->box_basic.box_length);

    fread(&(tkhd_box->version), 1, 1, mp4_file);
    fread(tkhd_box->flags, 1, 3, mp4_file);

    fread(&temp32, 1, 4, mp4_file);
    tkhd_box->create_time = get_big_endian32(temp32);

    fread(&temp32, 1, 4, mp4_file);
    tkhd_box->motify_time = get_big_endian32(temp32);

    fread(&temp32, 1, 4, mp4_file);
    tkhd_box->track_id = get_big_endian32(temp32);

    fread(tkhd_box->reserved, 1, 4, mp4_file);

    fread(&temp32, 1, 4, mp4_file);
    tkhd_box->duration = get_big_endian32(temp32);

    fread(tkhd_box->reserved2, 1, 8, mp4_file);

    fread(&temp16, 1, 2, mp4_file);
    tkhd_box->layer = get_big_endian16(temp16);

    fread(&temp16, 1, 2, mp4_file);
    tkhd_box->alternate_group = get_big_endian16(temp16);

    fread(&temp16, 1, 2, mp4_file);
    tkhd_box->volume = get_big_endian16(temp16);

    fread(tkhd_box->reserved3, 1, 2, mp4_file);

    fread(tkhd_box->matrix, 4, 9, mp4_file);

    fread(&temp32, 1, 4, mp4_file);
    tkhd_box->width = get_big_endian32(temp32) >> 16;

    fread(&temp32, 1, 4, mp4_file);
    tkhd_box->height = get_big_endian32(temp32) >> 16;
    track_box->tkhd_box = tkhd_box;
    printf("width : %d, height : %d, track_id : %d\n", tkhd_box->width, tkhd_box->height, tkhd_box->track_id);
    return True;
}

Boolean mp4_read_edts_box(void *arg, FILE *mp4_file)
{
    Trak_Box *track_box = (Trak_Box *)arg;
    Edts_Box *edts_box = (Edts_Box *)malloc(sizeof(Edts_Box));
    uint32_t trak_current_pos = 0;
    uint32_t box_temp_size = 0;
    uint32_t box_size;
    uint32_t box_type;
    int i;

    mp4_read_basic_box(&(edts_box->box_basic), mp4_file);
    printf("box name: %s, %d\n", edts_box->box_basic.box_name, edts_box->box_basic.box_length);
    trak_current_pos += 8;
    while (trak_current_pos < edts_box->box_basic.box_length)
    {
        fread(&box_temp_size, 1, 4, mp4_file);
        box_size = get_big_endian32(box_temp_size);
        fread(&box_type, 1, 4, mp4_file);
        fseek(mp4_file, -8, SEEK_CUR);
        for (i = 0; i < mp4_info.MAX_BOX_SIZE; i++)
        {
            if (mp4_parse_table[i].type == box_type)
            {
                mp4_parse_table[i].mp4_read_box(edts_box, mp4_file);
                break;
            }
        }
        trak_current_pos += box_size;
    }
    track_box->edts_box = edts_box;

    return True;
}

Boolean mp4_read_elst_box(void *arg, FILE *mp4_file)
{
    Edts_Box *edts_box = (Edts_Box *)arg;
    Elst_Box *elst_box = (Elst_Box *)malloc(sizeof(Elst_Box));
    uint32_t entry_num_temp;
    uint32_t temp32;
    uint32_t i;

    mp4_read_basic_box(&(elst_box->box_basic), mp4_file);
    printf("box name: %s, %d\n", elst_box->box_basic.box_name, elst_box->box_basic.box_length);

    fread(&(elst_box->version), 1, 1, mp4_file);
    fread(elst_box->flags, 1, 3, mp4_file);

    fread(&entry_num_temp, 1, 4, mp4_file);
    elst_box->entry_num = get_big_endian32(entry_num_temp);
    elst_box->entrys = (Elst_Entry *)malloc((elst_box->entry_num)*sizeof(Elst_Entry));
    for (i = 0; i < elst_box->entry_num; i++)
    {
        fread(&temp32, 1, 4, mp4_file);
        elst_box->entrys[i].track_duration = get_big_endian32(temp32);
        fread(&temp32, 1, 4, mp4_file);
        elst_box->entrys[i].media_time = get_big_endian32(temp32);
        fread(&temp32, 1, 4, mp4_file);
        elst_box->entrys[i].media_rate = get_big_endian32(temp32);
        printf("track duration: %d, media time: %d, media_rate: %d\n", elst_box->entrys[i].track_duration, 
            elst_box->entrys[i].media_time, elst_box->entrys[i].media_rate);
    }
    edts_box->elst_box = elst_box;
    return True;
}

Boolean mp4_read_mdia_box(void *arg, FILE *mp4_file)
{
    Trak_Box *track_box = (Trak_Box *)arg;
    Mdia_Box *mdia_box = (Mdia_Box *)malloc(sizeof(Mdia_Box));
    uint32_t current_mdia_pos = 0;
    uint32_t box_temp_size = 0;
    uint32_t box_size;
    uint32_t box_type;
    int i;

    mp4_read_basic_box(&(mdia_box->box_basic), mp4_file);
    printf("box name: %s, %d\n", mdia_box->box_basic.box_name, mdia_box->box_basic.box_length);

    current_mdia_pos += 8;

    while (current_mdia_pos < mdia_box->box_basic.box_length)
    {
        fread(&box_temp_size, 1, 4, mp4_file);
        box_size = get_big_endian32(box_temp_size);
        fread(&box_type, 1, 4, mp4_file);
        fseek(mp4_file, -8, SEEK_CUR);
        printf("box_size = %d\n", box_size);
        for (i = 0; i < mp4_info.MAX_BOX_SIZE; i++)
        {
            if (mp4_parse_table[i].type == box_type)
            {
                mp4_parse_table[i].mp4_read_box(mdia_box, mp4_file);
                break;
            }
        }
        current_mdia_pos += box_size;
    }
    track_box->mdia_box = mdia_box;
    return True;
}

Boolean mp4_read_mdhd_box(void *arg, FILE *mp4_file)
{
    Mdia_Box *mdia_box = (Mdia_Box *)arg;
    Mdhd_Box *mdhd_box = (Mdhd_Box *)malloc(sizeof(Mdhd_Box));
    uint32_t temp32;
    uint16_t temp16;

    mp4_read_basic_box(&(mdhd_box->box_basic), mp4_file);
    printf("box name: %s, %d\n", mdhd_box->box_basic.box_name, mdhd_box->box_basic.box_length);
    fread(&(mdhd_box->version), 1, 1, mp4_file);
    fread(mdhd_box->flags, 1, 3, mp4_file);
    fread(&temp32, 1, 4, mp4_file);
    mdhd_box->create_time = get_big_endian32(temp32);

    fread(&temp32, 1, 4, mp4_file);
    mdhd_box->motify_time = get_big_endian32(temp32);

    fread(&temp32, 1, 4, mp4_file);
    mdhd_box->time_scale = get_big_endian32(temp32);

    fread(&temp32, 1, 4, mp4_file);
    mdhd_box->duration = get_big_endian32(temp32);

    fread(&temp16, 1, 2, mp4_file);
    mdhd_box->language = get_big_endian16(temp16);

    fread(&temp16, 1, 2, mp4_file);
    mdhd_box->quality = get_big_endian16(temp16);
    mdia_box->mdhd_box = mdhd_box;
    return True;
}

Boolean mp4_read_hdlr_box(void *arg, FILE *mp4_file)
{
    Mdia_Box *mdia_box = (Mdia_Box *)arg;
    Hdlr_Box *hdlr_box = (Hdlr_Box *)malloc(sizeof(Hdlr_Box));
    uint32_t temp32;

    mp4_read_basic_box(&(hdlr_box->box_basic), mp4_file);
    printf("box name: %s, %d\n", hdlr_box->box_basic.box_name, hdlr_box->box_basic.box_length);

    fread(&(hdlr_box->version), 1, 1, mp4_file);
    fread(hdlr_box->flags, 1, 3, mp4_file);

    fread(&temp32, 1, 4, mp4_file);
    hdlr_box->component_type = get_big_endian32(temp32);

    fread(&hdlr_box->component_subtype, 1, 4, mp4_file);

    fread(&temp32, 1, 4, mp4_file);
    hdlr_box->component_manufacturer = get_big_endian32(temp32);

    fread(&temp32, 1, 4, mp4_file);
    hdlr_box->component_flags = get_big_endian32(temp32);

    fread(&temp32, 1, 4, mp4_file);
    hdlr_box->component_flags_mask = get_big_endian32(temp32);
    hdlr_box->component_name = (char *)malloc((hdlr_box->box_basic.box_length-31)*sizeof(char));
    fread(hdlr_box->component_name, 1, hdlr_box->box_basic.box_length-32, mp4_file);
    mdia_box->hdlr_box = hdlr_box;
    return True;
}

Boolean mp4_read_minf_box(void *arg, FILE *mp4_file)
{
    Mdia_Box *mdia_box = (Mdia_Box *)arg;
    Minf_Box *minf_box = (Minf_Box *)malloc(sizeof(Minf_Box));
    uint32_t current_minf_pos = 0;
    uint32_t box_temp_size = 0;
    uint32_t box_size;
    uint32_t box_type;
    int i;

    mp4_read_basic_box(&(minf_box->box_basic), mp4_file);
    printf("box name: %s, %d\n", minf_box->box_basic.box_name, minf_box->box_basic.box_length);

    current_minf_pos += 8;
    while (current_minf_pos < minf_box->box_basic.box_length)
    {
        fread(&box_temp_size, 1, 4, mp4_file);
        box_size = get_big_endian32(box_temp_size);
        fread(&box_type, 1, 4, mp4_file);
        fseek(mp4_file, -8, SEEK_CUR);
        for (i = 0; i < mp4_info.MAX_BOX_SIZE; i++)
        {
            if (mp4_parse_table[i].type == box_type)
            {
                mp4_parse_table[i].mp4_read_box(minf_box, mp4_file);
                break;
            }
        }
        current_minf_pos += box_size;
    }
    mdia_box->minf_box = minf_box;
    return True;
}

Boolean mp4_read_vmhd_box(void *arg, FILE *mp4_file)
{
    Minf_Box *minf_box = (Minf_Box *)arg;
    Vmhd_Box *vmhd_box = (Vmhd_Box *)malloc(sizeof(Vmhd_Box));
    uint16_t temp16;

    mp4_read_basic_box(&(vmhd_box->box_basic), mp4_file);
    printf("box name: %s, %d\n", vmhd_box->box_basic.box_name, vmhd_box->box_basic.box_length);
    fread(&(vmhd_box->version), 1, 1, mp4_file);
    fread(vmhd_box->flags, 1, 3, mp4_file);

    fread(&temp16, 1, 2, mp4_file);
    vmhd_box->graphics_mode = get_big_endian16(temp16);

    fread(&temp16, 1, 2, mp4_file);
    vmhd_box->opcolor[0] = get_big_endian16(temp16);

    fread(&temp16, 1, 2, mp4_file);
    vmhd_box->opcolor[1] = get_big_endian16(temp16);

    fread(&temp16, 1, 2, mp4_file);
    vmhd_box->opcolor[2] = get_big_endian16(temp16);

    minf_box->m_hd = vmhd_box;
    return True;
}

Boolean mp4_read_smhd_box(void *arg, FILE *mp4_file)
{
    Minf_Box *minf_box = (Minf_Box *)malloc(sizeof(Minf_Box));
    Smhd_Box *smhd_box = (Smhd_Box *)malloc(sizeof(Smhd_Box));
    mp4_read_basic_box(&(smhd_box->box_basic), mp4_file);
    printf("box name: %s, %d\n", smhd_box->box_basic.box_name, smhd_box->box_basic.box_length);
    fread(&(smhd_box->version), 1, 1, mp4_file);
    fread(smhd_box->flags, 1, 3, mp4_file);

    smhd_box->balance = read_uint16_big(mp4_file);
    fread(smhd_box->reserved, 1, 2, mp4_file);

    minf_box->m_hd = smhd_box;
    return True;
}

Boolean mp4_read_dinf_box(void *arg, FILE *mp4_file)
{
    Minf_Box *minf_box = (Minf_Box *)arg;
    Dinf_Box *dinf_box = (Dinf_Box *)malloc(sizeof(Dinf_Box));
    uint32_t box_temp_size;
    uint32_t box_size;
    uint32_t box_type;
    int i;

    mp4_read_basic_box(&(dinf_box->box_basic), mp4_file);
    printf("box name: %s, %d\n", dinf_box->box_basic.box_name, dinf_box->box_basic.box_length);

    fread(&box_temp_size, 1, 4, mp4_file);
    box_size = get_big_endian32(box_temp_size);
    fread(&box_type, 1, 4, mp4_file);
    fseek(mp4_file, -8, SEEK_CUR);
    for (i = 0; i < mp4_info.MAX_BOX_SIZE; i++)
    {
        if (mp4_parse_table[i].type == box_type)
        {
            mp4_parse_table[i].mp4_read_box(dinf_box, mp4_file);
            break;
        }
    }

    minf_box->dinf_box = dinf_box;
    return True;
}

Boolean mp4_read_dref_box(void *arg, FILE *mp4_file)
{
    Dinf_Box *dinf_box = (Dinf_Box *)arg;
    Dref_Box *dref_box = (Dref_Box *)malloc(sizeof(Dref_Box));
    uint32_t temp32;
    uint32_t box_temp_size;
    uint32_t box_size;
    uint32_t box_type;
    int i;
    long current_pos;

    mp4_read_basic_box(&(dref_box->box_basic), mp4_file);
    printf("box name: %s, %d\n", dref_box->box_basic.box_name, dref_box->box_basic.box_length);

    fread(&(dref_box->version), 1, 1, mp4_file);
    fread(dref_box->flags, 1, 3, mp4_file);

    fread(&temp32, 1, 4, mp4_file);
    dref_box->entry_num = get_big_endian32(temp32);

    current_pos = ftell(mp4_file);

    fread(&box_temp_size, 1, 4, mp4_file);
    box_size = get_big_endian32(box_temp_size);
    fread(&box_type, 1, 4, mp4_file);
    
    fseek(mp4_file, -8, SEEK_CUR);
    for (i = 0; i < mp4_info.MAX_BOX_SIZE; i++)
    {
        if (mp4_parse_table[i].type == box_type)
        {
            mp4_parse_table[i].mp4_read_box(dref_box, mp4_file);
            break;
        }
    }
    if (ftell(mp4_file) == current_pos)
        fseek(mp4_file, box_size, SEEK_CUR);

    dinf_box->dref_box = dref_box;

    return True;
}

Boolean mp4_read_url_box(void *arg, FILE *mp4_file)
{
    Dref_Box *dref_box = (Dref_Box *)arg;
    Url_Box *url_box = (Url_Box *)malloc(sizeof(Url_Box));

    mp4_read_basic_box(&(url_box->box_basic), mp4_file);
    printf("box name: %s, %d\n", url_box->box_basic.box_name, url_box->box_basic.box_length);
    fread(&(url_box->version), 1, 1, mp4_file);
    fread(url_box->flags, 1, 3, mp4_file);
    dref_box->arg = url_box;
    return True;
}

Boolean mp4_read_stbl_box(void *arg, FILE *mp4_file)
{
    Minf_Box *minf_box = (Minf_Box *)arg;
    Stbl_Box *stbl_box = (Stbl_Box *)malloc(sizeof(Stbl_Box));
    uint32_t box_temp_size;
    uint32_t current_stbl_pos;
    uint32_t box_size;
    uint32_t box_type;
    int i;

    mp4_read_basic_box(&(stbl_box->box_basic), mp4_file);
    printf("box name: %s, %d\n", stbl_box->box_basic.box_name, stbl_box->box_basic.box_length);

    current_stbl_pos += 8;
    while (current_stbl_pos < stbl_box->box_basic.box_length)
    {
        fread(&box_temp_size, 1, 4, mp4_file);
        box_size = get_big_endian32(box_temp_size);
        fread(&box_type, 1, 4, mp4_file);
        fseek(mp4_file, -8, SEEK_CUR);
        for (i = 0; i < mp4_info.MAX_BOX_SIZE; i++)
        {
            if (mp4_parse_table[i].type == box_type)
            {
                mp4_parse_table[i].mp4_read_box(stbl_box, mp4_file);
                break;
            }
        }
        current_stbl_pos += box_size;
    }
    minf_box->stbl_box = stbl_box;
    return True;
}

Boolean mp4_read_stsd_box(void *arg, FILE *mp4_file)
{
    Stbl_Box *stbl_box = (Stbl_Box *)arg;
    Stsd_Box *stsd_box = (Stsd_Box *)malloc(sizeof(Stsd_Box));
    uint32_t temp32;
    uint32_t box_size;
    uint32_t box_type;
    uint32_t i, j;

    mp4_read_basic_box(&(stsd_box->box_basic), mp4_file);
    printf("box name: %s, %d\n", stsd_box->box_basic.box_name, stsd_box->box_basic.box_length);
    fread(&(stsd_box->version), 1, 1, mp4_file);
    fread(stsd_box->flags, 1, 3, mp4_file);

    fread(&temp32, 1, 4, mp4_file);
    stsd_box->number_of_entries = get_big_endian32(temp32);

    stsd_box->sample_descriptions = (Sample_Description *)malloc(sizeof(Sample_Description) * stsd_box->number_of_entries);    
    for (i = 0;i < stsd_box->number_of_entries; i++)
    {
        fread(&temp32, 1, 4, mp4_file);
        box_size = get_big_endian32(temp32);
        fread(&box_type, 1, 4, mp4_file);
        fseek(mp4_file, -8, SEEK_CUR);
        for (j = 0; j < mp4_info.MAX_BOX_SIZE; j++)
        {
            if (mp4_parse_table[j].type == box_type)
            {
                mp4_parse_table[j].mp4_read_box(&(stsd_box->sample_descriptions[i]), mp4_file);
                break;
            }
        }
    }
    stbl_box->stsd_box = stsd_box;
    return True;
}

Boolean mp4_read_avc1_box(void *arg, FILE *mp4_file)
{
    Sample_Description *sample_description = (Sample_Description *)arg;
    Mp4_AVC1_Box *avc1_box = (Mp4_AVC1_Box *)malloc(sizeof(Mp4_AVC1_Box));
    uint32_t box_size;
    uint32_t box_type;
    int i;
    long avc1_pos_start;

    avc1_pos_start = ftell(mp4_file);
    mp4_read_basic_box(&(avc1_box->box_basic), mp4_file);
    printf("box name: %s, %d\n", avc1_box->box_basic.box_name, avc1_box->box_basic.box_length);

    fread(avc1_box->reserved, sizeof(avc1_box->reserved), 1, mp4_file);
    avc1_box->data_reference_index = read_uint16_big(mp4_file);
    fread(&(avc1_box->pre_defined), 2, 1, mp4_file);
    fread(&(avc1_box->reserved1), 2, 1, mp4_file);
    fread(avc1_box->pre_defined1, 4, 3, mp4_file);
    avc1_box->width = read_uint16_big(mp4_file);
    avc1_box->height = read_uint16_big(mp4_file);
    avc1_box->horiz_res = read_uint32_big(mp4_file);
    avc1_box->vert_res = read_uint32_big(mp4_file);
    fread(&(avc1_box->reserved2), 4, 1, mp4_file);
    avc1_box->frame_count = read_uint16_big(mp4_file);
    
    fread(avc1_box->compressor_name, 1, sizeof(avc1_box->compressor_name), mp4_file);
    fread(&(avc1_box->bit_depth), 1, 1, mp4_file);
    fread(&(avc1_box->pre_defined2), 2, 1, mp4_file);

    box_size = read_uint32_big(mp4_file);
    fread(&box_type, 4, 1, mp4_file);
    fseek(mp4_file, -8, SEEK_CUR);
    for(i = 0; i < mp4_info.MAX_BOX_SIZE; i++)
    {
        if (mp4_parse_table[i].type == box_type)
        {
            mp4_parse_table[i].mp4_read_box(avc1_box, mp4_file);
            break;
        }
    }
    uint32_t sub_pos = avc1_box->box_basic.box_length - (ftell(mp4_file) - avc1_pos_start);
    if (sub_pos > 0)
        fseek(mp4_file, sub_pos, SEEK_CUR);
    sample_description->avc1 = avc1_box;
    if (!sample_description->avc1->avcC)
        printf("sample_description->avc1->avcC is null\n");

    return True;
}

Boolean mp4_read_avcC_box(void *arg, FILE *mp4_file)
{
    Mp4_AVC1_Box *avc1_box = (Mp4_AVC1_Box *)arg;
    Mp4_avcC_Box *avcc_box = (Mp4_avcC_Box *)malloc(sizeof(Mp4_avcC_Box));
    uint8_t uint8;
    int i;

    mp4_read_basic_box(&(avcc_box->box_basic), mp4_file);
    printf("box name: %s, %d\n", avcc_box->box_basic.box_name, avcc_box->box_basic.box_length);

    fread(&(avcc_box->configurationVersion), 1, 1, mp4_file);
    fread(&(avcc_box->AVCProfileIndication), 1, 1, mp4_file);
    fread(&(avcc_box->profile_compatibility), 1, 1, mp4_file);
    fread(&(avcc_box->AVCLevelIndication), 1, 1, mp4_file);
    fread(&uint8, 1, 1, mp4_file);
    avcc_box->lengthSizeMinusOne = 0x3 & uint8;
    fread(&uint8, 1, 1, mp4_file);
    avcc_box->numOfSequenceParameterSet = 0x1f & uint8;
    avcc_box->sps = (SPS *)malloc(sizeof(SPS) * avcc_box->numOfSequenceParameterSet);
    for (i = 0; i < avcc_box->numOfSequenceParameterSet; i++)
    {
        avcc_box->sps[i].sequenceParameterSetLength = read_uint16_big(mp4_file);
        
        avcc_box->sps[i].sequenceParameterSetNALUnit = (uint8_t *)malloc(sizeof(uint8_t) * avcc_box->sps[i].sequenceParameterSetLength);

        fread((avcc_box->sps[i].sequenceParameterSetNALUnit),
              (avcc_box->sps[i].sequenceParameterSetLength), 1, mp4_file);
    }

    fread(&(avcc_box->numOfPictureParameterSets), 1, 1, mp4_file);
    avcc_box->pps = (PPS *)malloc(sizeof(PPS) * avcc_box->numOfPictureParameterSets);
    for (i = 0; i < avcc_box->numOfPictureParameterSets; i++)
    {
        avcc_box->pps[i].pictureParameterSetLength = read_uint16_big(mp4_file);
        avcc_box->pps[i].pictureParameterSetNALUnit = (uint8_t *)malloc(sizeof(uint8_t) * avcc_box->pps[i].pictureParameterSetLength);
        fread((avcc_box->pps[i].pictureParameterSetNALUnit),
              (avcc_box->pps[i].pictureParameterSetLength), 1, mp4_file);
    }
    avc1_box->avcC = avcc_box;
    if (!avc1_box->avcC)
        printf("avc1_box->avcC is null\n");
    return True;
}

Boolean mp4_read_mp4a_box(void *arg, FILE *mp4_file)
{
    Sample_Description *sample_description = (Sample_Description *)arg;
    Mp4_MP4A_Box *mp4a_box = (Mp4_MP4A_Box *)malloc(sizeof(Mp4_MP4A_Box));
    uint32_t box_size;
    uint32_t box_type;
    int i;

    mp4_read_basic_box(&(mp4a_box->box_basic), mp4_file);
    printf("box name: %s, %d\n", mp4a_box->box_basic.box_name, mp4a_box->box_basic.box_length);
    
    if (mp4a_box->box_basic.box_length >= 16)
    {
        fread(mp4a_box->reserved, 1, 6, mp4_file);
        mp4a_box->dref_id = read_uint16_big(mp4_file);
    }
    else if (mp4a_box->box_basic.box_length <= 7)
    {
        printf("invalid size %d in stsd\n", mp4a_box->box_basic.box_length);
        return False;
    }
    mp4a_box->version = read_uint16_big(mp4_file);
    mp4a_box->revision_level = read_uint16_big(mp4_file);
    mp4a_box->vendor = read_uint32_big(mp4_file);
    mp4a_box->channels = read_uint16_big(mp4_file);
    mp4a_box->bits_per_coded_sample = read_uint16_big(mp4_file);
    mp4a_box->audio_cid = read_uint16_big(mp4_file);
    mp4a_box->packet_size = read_uint16_big(mp4_file);
    mp4a_box->sample_rate = read_uint32_big(mp4_file) >> 16;
    printf("channels = %d, bits_per_coded_sample = %d, sample_rate = %d\n", mp4a_box->channels, 
        mp4a_box->bits_per_coded_sample, mp4a_box->sample_rate);

    box_size = read_uint32_big(mp4_file);
    fread(&box_type, 4, 1, mp4_file);
    fseek(mp4_file, -8, SEEK_CUR);
    for (i = 0; i < mp4_info.MAX_BOX_SIZE; i++)
    {
        if (mp4_parse_table[i].type == box_type)
        {
            mp4_parse_table[i].mp4_read_box(mp4a_box, mp4_file);
            break;
        }
    }

    sample_description->mp4a = mp4a_box;
    return True;
}

Boolean mp4_read_esds_box(void *arg, FILE *mp4_file)
{
    Mp4_MP4A_Box *mp4a_box = (Mp4_MP4A_Box *)arg;
    Mp4_Esds_Box *esds_box = (Mp4_Esds_Box *)malloc(sizeof(Mp4_Esds_Box));

    mp4_read_basic_box(&(esds_box->box_basic), mp4_file);
    printf("box name: %s, %d\n", esds_box->box_basic.box_name, esds_box->box_basic.box_length);
    fseek(mp4_file, esds_box->box_basic.box_length-8, SEEK_CUR);
    mp4a_box->esds_box = esds_box;
    return True;
}

Boolean mp4_read_stts_box(void *arg, FILE *mp4_file)
{
    Stbl_Box *stbl_box = (Stbl_Box *)arg;
    Stts_Box *stts_box = (Stts_Box *)malloc(sizeof(Stts_Box));
    int i;

    mp4_read_basic_box(&(stts_box->box_basic), mp4_file);
    printf("box name: %s, %d\n", stts_box->box_basic.box_name, stts_box->box_basic.box_length);
    fread(&stts_box->version, 1, 1, mp4_file);
    fread(&stts_box->flags, 1, 3, mp4_file);
    stts_box->number_of_entries = read_uint32_big(mp4_file);
    stts_box->time_to_sample_entrys = (Time_To_Sample_Entry *)malloc(sizeof(Time_To_Sample_Entry) * stts_box->number_of_entries);
    for (i = 0; i < stts_box->number_of_entries; i++)
    {
        stts_box->time_to_sample_entrys[i].sample_count = read_uint32_big(mp4_file);
        stts_box->time_to_sample_entrys[i].sample_duration = read_uint32_big(mp4_file);
    }

    stbl_box->stts_box = stts_box;
    return True;
}

Boolean mp4_read_stss_box(void *arg, FILE *mp4_file)
{
    Stbl_Box *stbl_box = (Stbl_Box *)arg;
    Stss_Box *stss_box = (Stss_Box *)malloc(sizeof(Stss_Box));
    int i;
    long stss_start_pos;
    long stss_end_pos;

    stss_start_pos = ftell(mp4_file);
    mp4_read_basic_box(&(stss_box->box_basic), mp4_file);
    printf("box name: %s, %d\n", stss_box->box_basic.box_name, stss_box->box_basic.box_length);
    fread(&stss_box->version, 1, 1, mp4_file);
    fread(stss_box->flags, 1, 3, mp4_file);
    stss_box->sync_sample_count = read_uint32_big2(mp4_file);
    printf("sync_sample_count = %d\n", stss_box->sync_sample_count);
    stss_box->sync_sample_number = (uint32_t *)malloc(sizeof(uint32_t) * stss_box->sync_sample_count);
    for (i = 0; i < stss_box->sync_sample_count; i++)
    {
        stss_box->sync_sample_number[i] = read_uint32_big(mp4_file);
    }
    stss_end_pos = ftell(mp4_file);
    if (stss_end_pos - stss_start_pos > 0)
        fseek(mp4_file, stss_box->box_basic.box_length - (stss_end_pos - stss_start_pos), SEEK_CUR);
    stbl_box->stss_box = stss_box;
    return True;
}

Boolean mp4_read_ctts_box(void *arg, FILE *mp4_file)
{
    Stbl_Box *stbl_box = (Stbl_Box *)arg;
    Ctts_Box *ctts_box = (Ctts_Box *)malloc(sizeof(Ctts_Box));
    mp4_read_basic_box(&(ctts_box->box_basic), mp4_file);
    printf("box name: %s, %d\n", ctts_box->box_basic.box_name, ctts_box->box_basic.box_length);
    fseek(mp4_file, ctts_box->box_basic.box_length-8, SEEK_CUR);

    stbl_box->ctts_box = ctts_box;
    return True;
}

Boolean mp4_read_stsc_box(void *arg, FILE *mp4_file)
{
    Stbl_Box *stbl_box = (Stbl_Box *)arg;
    Stsc_Box *stsc_box = (Stsc_Box *)malloc(sizeof(Stsc_Box));
    int i;

    mp4_read_basic_box(&(stsc_box->box_basic), mp4_file);
    printf("box name: %s, %d\n", stsc_box->box_basic.box_name, stsc_box->box_basic.box_length);
    fread(&stsc_box->version, 1, 1, mp4_file);
    fread(&stsc_box->flags, 1, 3, mp4_file);

    stsc_box->sample_to_chunk_numbers = read_uint32_big(mp4_file);
    stsc_box->sample_to_chunk_entrys = (Sample_To_Chunk_Entry *)malloc(sizeof(Sample_To_Chunk_Entry) * stsc_box->sample_to_chunk_numbers);

    for (i = 0; i < stsc_box->sample_to_chunk_numbers; i++)
    {
        stsc_box->sample_to_chunk_entrys[i].first_chunk = read_uint32_big(mp4_file);
        stsc_box->sample_to_chunk_entrys[i].samples_per_chunk = read_uint32_big(mp4_file);
        stsc_box->sample_to_chunk_entrys[i].sample_description_id = read_uint32_big(mp4_file);
    }
    stbl_box->stsc_box = stsc_box;

    return True;
}

Boolean mp4_read_stsz_box(void *arg, FILE *mp4_file)
{
    Stbl_Box *stbl_box = (Stbl_Box *)arg;
    Stsz_Box *stsz_box = (Stsz_Box *)malloc(sizeof(Stsz_Box));
    int i;

    mp4_read_basic_box(&(stsz_box->box_basic), mp4_file);
    printf("box name: %s, %d\n", stsz_box->box_basic.box_name, stsz_box->box_basic.box_length);
    fread(&stsz_box->version, 1, 1, mp4_file);
    fread(&stsz_box->flags, 1, 3, mp4_file);

    stsz_box->sample_size = read_uint32_big(mp4_file);
    if (stsz_box->sample_size == 0)
    {
        stsz_box->sample_size_entry_number = read_uint32_big(mp4_file);
        stsz_box->sample_size_tables = (uint32_t *)malloc(sizeof(uint32_t) * stsz_box->sample_size_entry_number);
        for (i = 0; i < stsz_box->sample_size_entry_number; i++)
        {
            stsz_box->sample_size_tables[i] = read_uint32_big(mp4_file);
        }
    }
    stbl_box->stsz_box = stsz_box;
    return True;
}

Boolean mp4_read_stco_box(void *arg, FILE *mp4_file)
{
    Stbl_Box *stbl_box = (Stbl_Box *)arg;
    Stco_Box *stco_box = (Stco_Box *)malloc(sizeof(Stco_Box));
    int i;

    mp4_read_basic_box(&(stco_box->box_basic), mp4_file);
    printf("box name: %s, %d\n", stco_box->box_basic.box_name, stco_box->box_basic.box_length);
    fread(&stco_box->version, 1, 1, mp4_file);
    fread(&stco_box->flags, 1, 3, mp4_file);

    stco_box->chunk_numbers = read_uint32_big(mp4_file);
    stco_box->chunk_offset_tables = (uint32_t *)malloc(sizeof(uint32_t) * stco_box->chunk_numbers);

    for (i = 0; i < stco_box->chunk_numbers; i ++)
    {
        stco_box->chunk_offset_tables[i] = read_uint32_big(mp4_file);
    }
    stbl_box->stco_box = stco_box;
    return True;
}

long get_mp4_file_size(FILE *mp4_file)
{
    long mp4_file_size;
    fseek(mp4_file, 0, SEEK_END);
    mp4_file_size = ftell(mp4_file);
    fseek(mp4_file, 0, SEEK_SET);
    return mp4_file_size;
}

uint32_t get_trak_samples(Stts_Box *stts_box)
{
    int i;
    uint32_t all_samples_count = 0;
    for (i = 0; i < stts_box->number_of_entries; i++)
    {
        all_samples_count += stts_box->time_to_sample_entrys[i].sample_count;
    }
    return all_samples_count;
}

Boolean get_chunk_index(Stsc_Box *stsc_box, uint32_t sample_index, uint32_t *chunk_index, uint32_t *offset_current_chunk)
{
    Sample_To_Chunk_Entry *sample_to_chunk_entrys = stsc_box->sample_to_chunk_entrys;
    uint32_t sample_to_chunk_numbers = stsc_box->sample_to_chunk_numbers;
    uint32_t i;
    uint32_t start_sample_index_chunk;
    uint32_t end_sample_index_chunk = 0;

    if (sample_to_chunk_numbers == 1)
    {
        *chunk_index = sample_index / sample_to_chunk_entrys->samples_per_chunk + 1;
        *offset_current_chunk = sample_index % sample_to_chunk_entrys->samples_per_chunk;
        return True;
    }
    else 
    {
        for (i = 0; i < sample_to_chunk_numbers-1; i++)
        {
            start_sample_index_chunk = end_sample_index_chunk;
            end_sample_index_chunk = start_sample_index_chunk + 
                (sample_to_chunk_entrys[i+1].first_chunk - sample_to_chunk_entrys[i].first_chunk)*sample_to_chunk_entrys[i].samples_per_chunk;
            //printf("sample_index = %d, end_sample_index_chunk = %d, start_sample_index_chunk = %d\n", sample_index, 
            //    end_sample_index_chunk, start_sample_index_chunk);
            if (sample_index < end_sample_index_chunk)
            {
                uint32_t samples = sample_index - start_sample_index_chunk;
                *chunk_index = samples / sample_to_chunk_entrys[i].samples_per_chunk + sample_to_chunk_entrys[i].first_chunk;
                *offset_current_chunk = samples % sample_to_chunk_entrys[i].samples_per_chunk;
                return True;
            }
        }

        uint32_t samples = sample_index - end_sample_index_chunk;
        *chunk_index = samples / sample_to_chunk_entrys[i].samples_per_chunk + sample_to_chunk_entrys[i].first_chunk;
        *offset_current_chunk = samples % sample_to_chunk_entrys[i].samples_per_chunk;
        return True;
    }
}

uint32_t get_sample_pos_in_mp4_file(Stbl_Box *stbl_box, uint32_t chunk_index, uint32_t offset_sample_chunk, uint32_t sample_index)
{
    Stco_Box *stco_box = stbl_box->stco_box;
    Stsz_Box *stsz_box = stbl_box->stsz_box;
    uint32_t i;

    uint32_t current_chunk_pos = stco_box->chunk_offset_tables[chunk_index-1];
    for(i = offset_sample_chunk; i > 0; i--)
    {
        
        current_chunk_pos += stsz_box->sample_size_tables[sample_index - i];
        
    }
    return current_chunk_pos;
}

void mp4_make_video_file(Moov_Box *moov_box, FILE *mp4_file, const char *video_file_name) 
{
    Trak_Box *video_trak;
    FILE *video_file;
    uint32_t i;
    Mp4_avcC_Box *avcC_box;
    uint32_t samples_count;
    uint32_t chunk_index;
    uint32_t offset_sample_chunk;
    uint32_t sample_pos;
    uint32_t sample_size;
    uint8_t *buffer;
    uint32_t sync_bytes_header = 1 << 24;
    uint32_t nalu_size;
    uint32_t nalu_total = 0;

    for (i = 0; i < moov_box->trak_index; i++)
    {
        video_trak = &(moov_box->track_box[i]);
        if (MP4_TAG('v', 'i', 'd', 'e') == video_trak->mdia_box->hdlr_box->component_subtype)
            break;
    }
    if ((video_file = fopen(video_file_name, "wb+")) == NULL)
        err_exit("fopen");
    avcC_box = video_trak->mdia_box->minf_box->stbl_box->stsd_box->sample_descriptions[0].avc1->avcC;
    if (!avcC_box)
        err_exit("avcc_box is null");
    for (i = 0; i < avcC_box->numOfSequenceParameterSet; i++)
    {
        fwrite(&sync_bytes_header, 4, 1, video_file);
        fwrite(avcC_box->sps[i].sequenceParameterSetNALUnit, 1, avcC_box->sps[i].sequenceParameterSetLength, video_file);
    }
    for (i = 0; i < avcC_box->numOfPictureParameterSets; i++)
    {
        fwrite(&sync_bytes_header, 4, 1, video_file);
        fwrite(avcC_box->pps[i].pictureParameterSetNALUnit, 1, avcC_box->pps[i].pictureParameterSetLength, video_file);
    }
    samples_count = get_trak_samples(video_trak->mdia_box->minf_box->stbl_box->stts_box);
    printf("samples_count = %d\n", samples_count);

    for (i = 0;i < samples_count; i++)
    {
        get_chunk_index(video_trak->mdia_box->minf_box->stbl_box->stsc_box, i, &chunk_index, &offset_sample_chunk);
        sample_pos = get_sample_pos_in_mp4_file(video_trak->mdia_box->minf_box->stbl_box, chunk_index, offset_sample_chunk, i);
        sample_size = video_trak->mdia_box->minf_box->stbl_box->stsz_box->sample_size_tables[i];
        fseek(mp4_file, 0, SEEK_SET);
        fseek(mp4_file, sample_pos, SEEK_SET);
        nalu_size = 0;
        nalu_total = 0;
        while (nalu_total < sample_size) 
        {
            nalu_size = read_uint32_big(mp4_file);
            nalu_total += nalu_size + 4;
            buffer = (uint8_t *)malloc(sizeof(uint8_t) * nalu_size);
            fread(buffer, 1, nalu_size, mp4_file);
            fwrite(&sync_bytes_header, 4, 1, video_file);
            fwrite(buffer, 1, nalu_size, video_file);
            free(buffer);
        }
    }
    fclose(video_file);
}

int get_frequency_index(int freq) {
	int result = 0;

	switch (freq)
	{
	case 96000 : result = 0; break;
	case 88200 : result = 1; break;
	case 64000 : result = 2; break;
	case 48000 : result = 3; break;
	case 44100 : result = 4; break;
	case 32000 : result = 5; break;
	case 24000 : result = 6; break;
	case 22050 : result = 7; break;
	case 16000 : result = 8; break;
	case 12200 : result = 9; break;
	case 11025 : result = 10; break;
	case 8000 : result = 11; break;
	default : result = 4; break; 
	}
	return result;
}

void make_adts_header(Mp4_MP4A_Box *mp4a_box, uint32_t sample_len, uint8_t *packet)
{
    int profile = 1; 
    int freqIdx = get_frequency_index(mp4a_box->sample_rate);  //44100HZ
    int chanCfg = mp4a_box->channels;  
    int private_bit = 0;
    int original_copy = 0;
    int home = 0;
    int copyright_identification_bit = 0;
    int coptright_indetification_start = 0;
    int number_of_raw_blocks_in_frame = 0;

    packet[0] = (unsigned char)0xFF;
    packet[1] = (unsigned char)0xF1;
    packet[2] = (unsigned char)(((profile)<<6) + (freqIdx<<2) +(private_bit << 1) + (chanCfg >> 2));
    packet[3] = (unsigned char)(((chanCfg & 3)<<6) +(original_copy << 5) +(home << 4)+(copyright_identification_bit << 3) +(coptright_indetification_start << 2)+ (sample_len >> 11));
    packet[4] = (unsigned char)((sample_len & 0x7FF) >> 3);
    packet[5] = (unsigned char)(((sample_len& 7) << 5) + (0x7ff >> 6));
    packet[6] = (unsigned char)(((0x7ff & 0x3f) << 2) + number_of_raw_blocks_in_frame);
}

void mp4_make_audio_file(Moov_Box *moov_box, FILE *mp4_file, const char *audio_file_name) 
{
    Trak_Box *audio_track;
    FILE *audio_file;
    uint32_t i;
    uint32_t samples_count;
    uint32_t chunk_index;
    uint32_t offset_sample_chunk;
    uint32_t sample_pos;
    uint32_t sample_size;
    uint8_t *buffer;
    uint8_t *adts_header = (uint8_t *)malloc(sizeof(uint8_t) * 7);

    for (i = 0; i < moov_box->trak_index; i++)
    {
        audio_track = &(moov_box->track_box[i]);
        if (MP4_TAG('s', 'o', 'u', 'n') == audio_track->mdia_box->hdlr_box->component_subtype)
            break;
    }
    if ((audio_file = fopen(audio_file_name, "wb+")) == NULL)
        err_exit("fopen");
    
    samples_count = get_trak_samples(audio_track->mdia_box->minf_box->stbl_box->stts_box);
    for (i = 0;i < samples_count; i++)
    {
        get_chunk_index(audio_track->mdia_box->minf_box->stbl_box->stsc_box, i, &chunk_index, &offset_sample_chunk);
        sample_pos = get_sample_pos_in_mp4_file(audio_track->mdia_box->minf_box->stbl_box, chunk_index, offset_sample_chunk, i);
        sample_size = audio_track->mdia_box->minf_box->stbl_box->stsz_box->sample_size_tables[i];

        fseek(mp4_file, 0, SEEK_SET);
        fseek(mp4_file, sample_pos, SEEK_SET);
        make_adts_header(audio_track->mdia_box->minf_box->stbl_box->stsd_box->sample_descriptions[0].mp4a, sample_size+7, adts_header);
        buffer = (uint8_t *)malloc(sizeof(uint8_t) * sample_size);
        fread(buffer, 1, sample_size, mp4_file);
        fwrite(adts_header, 1, 7, audio_file);
        fwrite(buffer, 1, sample_size, audio_file);
        free(buffer);
    }
    fclose(audio_file);
}

int main(int argc, char* argv[])
{
    // Forrest_Gump_IMAX.mp4
    FILE *mp4_file = NULL;
    long mp4_file_size;
    Mp4_File_Root_Box *mp4_file_root_box = NULL;

    long mp4_cur_pos = 0;
    uint32_t box_size;
    uint32_t box_temp_size;
    uint32_t box_type;
    int i;
    mp4_info.MAX_BOX_SIZE = sizeof(mp4_parse_table)/sizeof(Mp4_Parse_Enrty);
    mp4_file_root_box = (Mp4_File_Root_Box *)malloc(sizeof(Mp4_File_Root_Box));
    printf("MAX_BOX_SIZE = %d\n", mp4_info.MAX_BOX_SIZE);
    if ((mp4_file = fopen(argv[1], "rb+")) == NULL)
        err_exit("fopen");

    mp4_file_size = get_mp4_file_size(mp4_file);

    while (mp4_cur_pos < mp4_file_size)
    {
        fread(&box_temp_size, 4, 1, mp4_file);
        box_size = get_big_endian32(box_temp_size);
        fread(&box_type, 4, 1, mp4_file);
        if (box_size == 0)
            break;
        fseek(mp4_file, -8, SEEK_CUR);
        for (i = 0; i < mp4_info.MAX_BOX_SIZE; i++)
        {
            if (box_type == mp4_parse_table[i].type)
            {
                mp4_parse_table[i].mp4_read_box(mp4_file_root_box, mp4_file);
                break;
            }
        }
        mp4_cur_pos += box_size;
        //fseek(mp4_file, box_size, SEEK_CUR);
        memset(&box_temp_size, 0, 4);
        memset(&box_size, 0, 4);
        memset(&box_type, 0, 4);
    }
    mp4_make_video_file(mp4_file_root_box->moov_box, mp4_file, "video.h264");
    mp4_make_audio_file(mp4_file_root_box->moov_box, mp4_file, "audio.aac");
    fclose(mp4_file);
    
    return 0;
}