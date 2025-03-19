#pragma once

/* These structures have been tested to produce the same offsets on clang++ on Mac and MSVC
   on Windows 11.  The ones without _3 in the name are v2 compatible, for what it's worth
   The version 2 structures lay out differently on Windows and Mac, because sizeof(long)
   is 64 on Mac and 32 on Windows.  There are no "long"s in V3, only uintXX_t's when not int.
    */

#define TKO_VERSION_3_MAGIC 0xA753
#define TKO_VERSION_3_STRING "RLYCOMP"
#define TKO_VERSION_3 3
#define TKA_VERSION_3_EXPECTED_HEADER_SIZE 96

#include <cstdint>  /* needed on Windows to get uintXX... defined... */

enum _TKO_VERSION_3_COMPID
  {TKOI_NULL, TKOI_SOURCEID, TKOI_ISD, TKOI_ESD, TKOI_RLD, TKOI_TXT,
   TKOI_DPD, TKOI_RTD, TKOI_RTT, TKOI_EOF, TKOI_TMR, TKOI_ATS, TKOI_FRM,
   TKOI_CID, TKOI_LAST};
   
#define TKOV3_COMPID_STRINGS {"NUL", "SRC", "ISD", "ESD", "RLD", "TXT", \
   "DPD", "RTD", "RTT", "EOF", "TMR", "ATS", "FRM", "CID"}
 

struct _TKO_VERSION_3_HEADER {
 /*0 */   uint16_t magic;
 /*2 */   char     magic_string[8];
 /*10*/   uint16_t version;

 /*12*/   uint16_t header_size;
 /*14*/   uint8_t  bits;
 /*15*/   uint8_t  archindex;

 /*16*/   uint32_t time;
 /*20*/   char     user[32];
 /*52*/   char     arch[32];

 /*84*/   uint32_t code_len;
 /*88*/   uint32_t static_len;
 /*92*/   uint32_t compiler_version;
 /*96*/
};

struct _TKO_VERSION_3_COMPONENT_HEADER {
    enum _TKO_VERSION_3_COMPID compid;
    int number_of_items;
    int unsigned length_of_item;
    int unsigned length_of_block;
};

struct _TKO_VERSION_3_DEP_TABLE {
    int esize;
    int esdx;
    int n_entries;
};

struct TKO_DEFBLOCK_3 {
    int32_t n;
    int type;
    int data;
};

struct TKO_DPTE_HEADER {
    int affector;
    int count;
};

struct TKO_TIMER_DEF {
    int rlyisdid;
    int time;
};
