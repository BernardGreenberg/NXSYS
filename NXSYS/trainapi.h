#ifndef _NXSYS_TRAIN_API_H__
#define _NXSYS_TRAIN_API_H__

#define TRAIN_HALTCTL_GO 1
#define TRAIN_HALTCTL_HALTED 2

class GraphicObject;

int FilterTrainDialogMessages (MSG*);
void TrainDialog (GraphicObject * g, int haltflag);
void TrainMiscCtl (int);
GraphicObject* ChooseTrackNumber (int);
extern BOOL AutoEngageTrains;

#endif
