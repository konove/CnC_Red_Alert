#ifndef CNC_RED_ALERT_RA_CCMPATH_H_
#define CNC_RED_ALERT_RA_CCMPATH_H_

int Init_MPATH(void);
void Shutdown_MPATH(void);
void Connect_MPATH(void);
void Destroy_MPATH_Connection(int id, int error);

#endif  // CNC_RED_ALERT_RA_CCMPATH_H_
