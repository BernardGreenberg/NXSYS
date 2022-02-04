#ifndef __RELAY_COMPILER_OBJECT_MODULE__
#define __RELAY_COMPILER_OBJECT_MODULE__

#define TKO_VERSION_1_MAGIC 0xA751
#define TKO_VERSION_1_STRING "RLYCOMP"
#define TKO_VERSION_1 1

struct _TKO_VERSION_1_HEADER {
    short unsigned magic;
    char magic_string[8];
    short version;
    short header_size;
    short unsigned code_len;
    short unsigned static_len;
    long time;
    char user[32];
};

enum _TKO_VERSION_1_COMPID
  {TKOI_NULL, TKOI_SOURCEID, TKOI_ISD, TKOI_ESD, TKOI_RLD, TKOI_TXT,
   TKOI_DPD, TKOI_RTD, TKOI_RTT, TKOI_EOF, TKOI_TMR, TKOI_ATS, TKOI_FRM,
   TKOI_LAST};

#define TKOV1_COMPID_STRINGS {"NUL", "SRC", "ISD", "ESD", "RLD", "TXT", \
   "DPD", "RTD", "RTT", "EOF", "TMR", "ATS", "FRM"}
 

struct _TKO_VERSION_1_COMPONENT_HEADER {
#ifdef _WIN32
    short compid;
#else
    enum _TKO_VERSION_1_COMPID compid;
#endif
    short number_of_items;
    short unsigned length_of_item;
    short unsigned length_of_block;
};

struct _TKO_VERSION_1_DEP_TABLE {
    short esize;
    short esdx;
    short n_entries;
};

struct TKO_DEFBLOCK {
    long n;
    short type;
    short data;
};

struct TKO_DPTE_HEADER {
    short affector;
    short count;
};

struct TKO_TIMER_DEF {
    short rlyisdid;
    short time;
};


#endif