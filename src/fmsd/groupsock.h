#ifndef ___GROUPSOCK_H___
#define ___GROUPSOCK_H___

#include "camera.h"

int      getData              (int socket, char* buffer, int bufferSize);
int      getData              (int socket, char* buffer, int bufferSize, uint64_t seconds);
int      getData              (int socket, char* buffer, int bufferSize, uint64_t seconds, bool * Stop);
int      getDataWait          (int socket, char* buffer, int bufferSize, uint64_t seconds, bool * Stop);
int      getRead              (int socket, char* buffer, int bufferSize);
int      getDataExact         (int socket, char* buffer, int bufferSize);
int      getDataExact         (int socket, char* buffer, int bufferSize, uint64_t seconds);
int      getDataExact         (int socket, char* buffer, int bufferSize, uint64_t seconds, bool * Stop);

int      SendSocketExact      (int socket, char* buffer, int bufferSize, bool * IsClose);
int      SendSocketExact      (int socket, char* buffer, int bufferSize);
int      SendSocketExact      (int socket, char* buffer, int bufferSize, uint64_t SecCount);
int      SendStreamL          (int socket, char* buffer, int bufferSize, int8_t & ret, bool * IsClose);
int      SendStreamL          (int socket, char* buffer, int bufferSize, bool * IsClose);

unsigned zeroSendBufferTo     (int socket);
unsigned increaseSendBufferTo (int socket, unsigned requestedSize);
unsigned increaseRecvBufferTo (int socket, unsigned requestedSize);
unsigned getBufferSize        (int bufOptName, int socket);

int      setupSocket          (char * Addr, int port, int makeNonBlocking);
int      setupUDPSocket       ();

int      setNonBlock          (int sock);
int      setRcvTimeo          (int sock, int seconds);
int      setLinger            (int sock);
void     epClose              (ClntInfo * si, bool clear = false);
void     epCloseN             (ClntInfo * si, bool clear = false);
void     getRecvClear         (int sock, uint64_t seconds);
void     clearSocket          (int sock);
bool     socketclose          (int sock);
char  *  getError             (int Errno, int Timeout);
int      ConnectWithTimeout   (int fd, struct sockaddr * remote, int len, int secs, int * err);
int      RTrim                (char * buffer, int buflen);
void     TrimString           (char * szName);

#endif //___GROUPSOCK_H___
