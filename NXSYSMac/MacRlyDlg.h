//
//  MacRlyDlg.h
//  NXSYSMac
//
//  Created by Bernard Greenberg on 8/30/14.
//  Copyright (c) 2014 BernardGreenberg. All rights reserved.
//

#ifndef NXSYSMac_MacRlyDlg_h
#define NXSYSMac_MacRlyDlg_h

#include <string>
#include <vector>

class Relay;

typedef void (*RelayListCallback)(Relay* theRelay, const char * tag);

Relay* RelayListDialog(int objNo, const char* typeName,
                     Relay** theRelays, int nRelays,
                    const char* op);

void DrawRelayAPI(Relay *);

std::string STLRelayName(Relay*);
void vectorizeDependents(Relay * r, std::vector<Relay*>& relays);
bool boolRelayState(Relay* r);
#endif
