#include <stdlib.h>
#include <memory.h>
#include "assignid.h"

#define MAX_OBJ_ID 32768
#define LOG_B_B 3
#define BITS_PER_BYTE (1 << LOG_B_B)
#define MAX_OBJ_ID_BYTES (MAX_OBJ_ID >> LOG_B_B)

static char unsigned IDMap [MAX_OBJ_ID_BYTES];

void InitAssignID() {
    memset (IDMap, 0, sizeof(IDMap));

}

int CheckID (int id) {			/* 1 = OK */
    if (id <= 0 || id >= MAX_OBJ_ID)
	return 0;
    int byteno = id >> LOG_B_B;
    int bit = 1 << (id % BITS_PER_BYTE);
    return ((IDMap[byteno] & bit) == 0);
}

int CheckIDAssign (int id) {
    if (id < 0 || id >= MAX_OBJ_ID)
	return 0;
    int byteno = id >> LOG_B_B;
    int bit = 1 << (id % BITS_PER_BYTE);
    if (IDMap[byteno] & bit)
	return 0;
    IDMap[byteno] |= bit;
    return 1;
}

int MarkIDAssign (int id) {
    int byteno = id >> LOG_B_B;
    int bit = 1 << (id % BITS_PER_BYTE);
    IDMap[byteno] |= bit;
    return id;
}

void DeAssignID (int id) {
    int byteno = id >> LOG_B_B;
    int bit = 1 << (id % BITS_PER_BYTE);
    IDMap[byteno] &= ~bit;

}

int AssignID (int odd) {
    odd = odd ? 1 : 0;			/* canonicalize */
    int wholemask =  0x55 << odd;
    int onemask = 1 << odd;
    int multbase
	    = ((TLEDIT_AUTO_ID_BASE + BITS_PER_BYTE-1)/BITS_PER_BYTE)*BITS_PER_BYTE;
    int firsti = TLEDIT_AUTO_ID_BASE + (1-odd);
    for (int i = firsti; i < multbase; i+=2)
	if (CheckIDAssign (i))
	    return i;
    for (int j = multbase/BITS_PER_BYTE; j < MAX_OBJ_ID_BYTES; j++) {
	unsigned char freebits = ~IDMap[j];
	if (freebits & wholemask) {
	    for (int k = odd; k < BITS_PER_BYTE; k+= 2) {
		if (onemask & freebits)
		    return MarkIDAssign (j*BITS_PER_BYTE + k);
		onemask <<= 2;
	    }
	}
    }
    return 0;
}
