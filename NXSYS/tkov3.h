#pragma once

/* These structures have been tested to produce the same offsets on clang++ on Mac and MSVC
   on Windows 11.  The ones without _3 in the name are v2 compatible, for what it's worth
   The version 2 structures lay out differently on Windows and Mac, because sizeof(long)
   is 64 on Mac and 32 on Windows.  There are no "long"s in V3, only uintXX_t's when not int.
    */

#define TKO_VERSION_3_MAGIC 0xA753
#define TKO_VERSION_3_STRING "RLYCOMP"
#define TKO_VERSION_3 3
static constexpr int BITS_PER_BYTE = 8;

#include <cstdint>  /* needed on Windows to get uintXX... defined... */

enum _TKO_VERSION_3_COMPID
  {TKOI_NULL,         /* not used */
   TKOI_SOURCEID,     /* not used currently */
   TKOI_ISD,          /* internal symbol dict - ISDENTRY's for each relay defined here */
   TKOI_ESD,          /* external symbol dict - ESDENTRY's for each relay referenced OR defined */
   TKOI_RLD,          /* not used currently */
   TKOI_TXT,          /* machine instruction code */
   TKOI_DPD,          /* dependent pairs - in groups of DPTE_HEADER 1 per affector */
   TKOI_RTD,          /* relay type data- a heap of strings */
   TKOI_RTT,          /* relay type table- indices into RTT */
   TKOI_EOF,          /* end of file entry -no data */
   TKOI_TMR,          /* timer relay definitions */
   TKOI_ATS,          /* atomic symbols not relays */
   TKOI_FRM,          /* encoded pseudoLisp forms to be evaluated at load time */
   TKOI_CID,          /* compiler ID - offset in file of C string */
   TKOI_LAST};        /* limit on ID's */
   
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
#define TKO_VERSION_3_EXPECTED_HEADER_SIZE 96
};
static_assert(sizeof(_TKO_VERSION_3_HEADER) == TKO_VERSION_3_EXPECTED_HEADER_SIZE);
static_assert(sizeof(enum _TKO_VERSION_3_COMPID) * BITS_PER_BYTE == 32);



struct _TKO_VERSION_3_COMPONENT_HEADER {
    enum _TKO_VERSION_3_COMPID compid;
    int32_t  number_of_items;
    int32_t  length_of_item;
    uint32_t length_of_block;
};

struct TKO_ISDENTRY {
    uint32_t n;  /* lever or track sec # -- leaves a lot to be desired */
    int32_t  type_index;  /* index in RTT of relay nomenclature */
    uint32_t code_offset; /* location in TXT where code begins */
};

struct TKO_ESDENTRY {
    uint32_t n;  /* lever or track sec # -- leaves a lot to be desired */
    int32_t type_index;  /* index in RTT of relay nomenclature */
};

struct TKO_DPTE_HEADER {
    int32_t affector;    // index into ESD
    int32_t count;       //followed by that many 32-bit indices into ISD.
};

struct TKO_TIMER_DEF {
    int32_t rlyisdid;
    uint32_t time;
};
