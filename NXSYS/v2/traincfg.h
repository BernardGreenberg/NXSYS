#ifndef _NXSYS_TRAINCFG_H__
#define _NXSYS_TRAINCFG_H__

extern double
   CruisingSpeed, TrainLengthFeet, YellowFeetPerSecond;

extern void SetTrainKinematicsDefaults();

#ifdef NXV2
extern void SetUpLayoutTrainMetrics();
#endif

#endif
