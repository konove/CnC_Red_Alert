#ifndef CNC_RED_ALERT_RA_CCTEN_H_
#define CNC_RED_ALERT_RA_CCTEN_H_

int Init_TEN(void);
void Shutdown_TEN(void);
void Connect_TEN(void);
void Destroy_TEN_Connection(int id, int error);
void Send_TEN_Win_Packet(void);
void Send_TEN_Alliance(char* whom, int ally);
void Send_TEN_Out_Of_Sync(void);
void Send_TEN_Packet_Too_Late(void);

#endif  // CNC_RED_ALERT_RA_CCTEN_H_
