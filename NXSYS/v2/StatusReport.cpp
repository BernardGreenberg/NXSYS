//
//  StatusReport.cpp
//  NXSYSMac
//
//  Created by Bernard Greenberg on 10/1/14.
//  Copyright (c) 2014 BernardGreenberg. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <string>
#include "STLExtensions.h"
#include "MapperThunker.h"

#include "StatusReport.h"
#include "CompiledReportInfo.h"
#if NXSYSMac
#define CRLF "\n"
#else
#define CRLF "\n\r"
#endif

extern std::string InterlockingName;
extern std::string GlobalFilePathname;
int meterNTurnouts();
int CountRelaySyms();

#include "windows.h"
#include "nxgo.h"
#include "typeid.h"
#include "signal.h"
#include "xtgtrack.h"
#include <time.h>

void getFileTime(const char* filename, std::string& result) {
     struct stat st;
    /* if this doesn't work on Windows (needs _stat), make it do so. */
    if (stat(filename, &st) == 0) {
        time_t ftime = st.st_mtime;

        result = ctime(&ftime);
    }
    else {
        result = "Cannot determine file modification time.";
    }
}

int InterlockingFileStatus::contemplari (GraphicObject* g) {
    switch (g->TypeID()) {
        case TypeId::SIGNAL:
        {
            PanelSignal * psp = (PanelSignal*)g;
            Signal * sp = psp->Sig;
            nSignals += 1;
            if (sp->XlkgNo > 0) {
                nControlledSignals += 1;
            }
            break;
        }
        case TypeId::EXITLIGHT:
            nExitLights += 1;
            break;
        case TypeId::TURNOUT: // not in nxgo table in real NXSYS
            break;
        case TypeId::JOINT:
            nJoints += 1;
            break;
        case TypeId::TRACKSEG:
        {
            const TrackSeg* tsp = (const TrackSeg*) g;
            const TrackCircuit * tcp = (const TrackCircuit*) tsp->Circuit;
            if (tcp != NULL) {
                TrackCircuits.insert((int)tcp->StationNo);
            }
            nTrackSegments += 1;
        }
            break;
        default:
            break;
    }
    return 0;
}

InterlockingFileStatus::InterlockingFileStatus() {
    InterlockingName = ::InterlockingName;
    Pathname = GlobalFilePathname;
    getFileTime(Pathname.c_str(), ctimeModified);

    nSwitches = meterNTurnouts();
    nCrossovers = 0;
    nSignals = 0;
    nControlledSignals = 0;
    nTrackSegments = 0;
    nJoints = 0;
    nInsulatedJoints = 0;
    nExitLights = 0;
    nSymbols = 0;
    nRelays = CountRelaySyms();
    TrackCircuits.clear();
    /* See mapper_thunker.h */
    auto fnarg = [this](GraphicObject* g, void*) -> int {return contemplari(g);};
    MapAllGraphicObjects(value_thunker<GraphicObject*, int>(fnarg), &fnarg);
};


std::string InterlockingFileStatus::Report () {
    std::string report;
    report += InterlockingName + CRLF;
    report += "Top level file: ";
    report += Pathname + CRLF;
    report += "Modified (on disk): ";
    report += ctimeModified + CRLF;
    
    struct items {
        std::string label;
        int count;
    } table[] =
    {   {"switches (switch levers)", nSwitches},
        {"signals (total)", nSignals},
        {"controlled signals", nControlledSignals},
        {"track segments", nTrackSegments},
        {"track circuits", (int)TrackCircuits.size()},
        {"exit lights", nExitLights},
        {"total graphic objects", GraphicObjectCount()},
        {"\nLogic substrate", 0},
        {"relays (non-stub)", nRelays},
//        {"atomic symbols", nSymbols}
    };
    
    for (auto& e : table){
        if (e.label[0] == '\n')
            report += e.label + CRLF + CRLF;
        else
            report += FormatString("%5d  ", e.count) + e.label + CRLF;
    }
    const CompiledReportInfo* Cp = GetCompiledReportInfo();
    if (Cp != nullptr) {
        std::vector<std::string> A;
        const auto& C = *Cp;
        A.push_back("\n\n");
        A.push_back("Compiled file compilation info:");
        A.push_back(FormatString("Header version: %d, compiler version %d", C.HeaderVersion, C.CompilerVersion));
        A.push_back(FormatString("Compiled for %s (%d bits) by %s", C.Architecture.c_str(), C.Bits, C.User.c_str()));
        A.push_back("  at " + std::string(ctime(&C.CompilationTime)));
        if (!C.Compiler.empty())
            A.push_back("Compiler: " + C.Compiler);
        A.push_back(FormatString("Relays defined %d, referenced %d.", C.ISDCount, C.ESDCount));
        A.push_back(FormatString("Code length %d = 0x%X bytes", C.CodeLen, C.CodeLen));
        A.push_back(FormatString("Required static \"linkage section\" length %d = 0x%X bytes", C.StaticLen, C.StaticLen));

        for (auto line : A) {
            if (line.back() == '\n')
                line.pop_back();
            report += line + CRLF;
        }
    }
    return report;
}
