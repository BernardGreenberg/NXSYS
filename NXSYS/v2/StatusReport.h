//
//  StatusReport.h
//  NXSYSMac
//
//  Created by Bernard Greenberg on 10/1/14.
//  Copyright (c) 2014 Bernard Greenberg. All rights reserved.
//

#ifndef NXSYSMac_StatusReport_h
#define NXSYSMac_StatusReport_h
#include <string>
#include <unordered_set>

struct InterlockingFileStatus {
    std::string InterlockingName;
    std::string Pathname;
    std::string ctimeModified;
    int nSwitches;
    int nCrossovers;
    int nSignals;
    int nControlledSignals;
    int nTrackSegments;
    int nJoints;
    int nInsulatedJoints;
    int nExitLights;
    int nSymbols;
    int nRelays;
    InterlockingFileStatus ();
    std::string Report();
    int contemplari(class GraphicObject*);
    std::unordered_set<int> TrackCircuits;
};

std::string InterlockingFileStatusReport();
void getStatusReportFileInfo(InterlockingFileStatus& ifs);
void getStatusReportObjectInfo(InterlockingFileStatus& ifs);

#endif
