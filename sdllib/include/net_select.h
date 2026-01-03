#ifndef CNC_RED_ALERT_SDLLIB_NET_SELECT_H_
#define CNC_RED_ALERT_SDLLIB_NET_SELECT_H_

enum SocketEvent {
  SOCKEV_READ,
  SOCKEV_WRITE,
  SOCKEV_ERROR,
};

typedef void (*SocketCallback)(int, SocketEvent, void *);

bool Socket_Register_Select(int socket, SocketCallback callback, void *data);
void Socket_Unregister_Select(int socket);

void Socket_Check_Write(int socket, bool check);

void Socket_Select();

#endif  // CNC_RED_ALERT_SDLLIB_NET_SELECT_H_
