#ifndef CNC_RED_ALERT_RA_CCMPATH_H_
#define CNC_RED_ALERT_RA_CCMPATH_H_

int Init_MPATH();
void Shutdown_MPATH();
void Connect_MPATH();
void Destroy_MPATH_Connection(int id, int error);

#endif  // CNC_RED_ALERT_RA_CCMPATH_H_
