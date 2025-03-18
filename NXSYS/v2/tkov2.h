#ifndef __RELAY_COMPILER_OBJECT_MODULE__
#define __RELAY_COMPILER_OBJECT_MODULE__

#define TKO_VERSION_2_MAGIC 0xA751
#define TKO_VERSION_2_STRING "RLYCOMP"
#define TKO_VERSION_2 2

#ifdef _MSC_VER
#pragma pack(push,tkov2)
#pragma pack(2)
/* need for EVERYTHING */
#endif

struct _TKO_VERSION_2_HEADER {
    short unsigned magic;
    char magic_string[8];
    short version;
    short header_size;
    short unsigned compat_code_len;
    short unsigned compat_static_len;
    long time;
    char user[32];
    char arch[32];
    short    bits;
    short    archindex;
    unsigned int code_len;
    unsigned int static_len;
    int compiler_version;
};

enum _TKO_VERSION_2_COMPID
  {TKOI_NULL, TKOI_SOURCEID, TKOI_ISD, TKOI_ESD, TKOI_RLD, TKOI_TXT,
   TKOI_DPD, TKOI_RTD, TKOI_RTT, TKOI_EOF, TKOI_TMR, TKOI_ATS, TKOI_FRM,
   TKOI_CID, TKOI_LAST};

#define TKOV2_COMPID_STRINGS {"NUL", "SRC", "ISD", "ESD", "RLD", "TXT", \
   "DPD", "RTD", "RTT", "EOF", "TMR", "ATS", "FRM", "CID"}
 

struct _TKO_VERSION_2_COMPONENT_HEADER {
    enum _TKO_VERSION_2_COMPID compid;
    int number_of_items;
    int unsigned length_of_item;
    int unsigned length_of_block;
};

struct _TKO_VERSION_2_DEP_TABLE {
    int esize;
    int esdx;
    int n_entries;
};

struct TKO_DEFBLOCK {
    long n;
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

#ifdef _MSC_VER

#pragma pack(pop,tkov2)
/* need for EVERYTHING */
#endif


#endif
