#ifndef _BFFP_ENCODING_H_INCLUDED
#define _BFFP_ENCODING_H_INCLUDED


typedef struct bfft_encoding_ {
    lz_tag_type tag;
    int length;
    int offset;
} bffp_encoding_t;


#endif // _BFFP_ENCODING_H_INCLUDED
