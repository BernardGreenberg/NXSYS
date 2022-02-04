#ifndef _NXSYS_TRAIN_AUTOMATION_H__
#define _NXSYS_TRAIN_AUTOMATION_H__

#include "traindlg.h"

int  TrainAutoCreate   (int train_no, long start_place_id, long options);
BOOL TrainAutoCmd      (int train_no, WPARAM cmd);
BOOL TrainAutoSetSpeed (int train_no, double speed);
double TrainAutoGetSpeed (int train_no);
BOOL TrainAutoValidateTrainNo (int train_no);
BOOL TrainAutoLookupCommand (const char * scmd, int&icmd);


#define TRAIN_CTL_MINDLG   1
#define TRAIN_CTL_HIDEDLG  2
#define TRAIN_CTL_HALTED   4
#define TRAIN_CTL_FREEWILL 8

#endif
