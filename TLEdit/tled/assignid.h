#ifndef _NXSYS_ID_ASSIGN_H_
#define _NXSYS_ID_ASSIGN_H_

void InitAssignID();
int CheckID (int id);
int CheckIDAssign (int id);
int MarkIDAssign (int id);
int AssignID (int odd);
void DeAssignID (int id);
#define TLEDIT_AUTO_ID_BASE 10001

#endif
