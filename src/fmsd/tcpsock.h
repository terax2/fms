/////////////////////////////////////////////////////////////////////////
//
//  제어하는 클래스..
//
/////////////////////////////////////////////////////////////////////////

#ifndef ___TCPSOCK_H___
#define ___TCPSOCK_H___

#include "camera.h"

class CTCPSock
{
    public :
        CTCPSock(Address pAddr);
        ~CTCPSock();

    public :
        void        SetSvr     (char * ip, uint16_t port);
        void        SetSvr     (Address  pAddr);
        void        SetTimeout (uint64_t timeout);
        bool        Connect    (char * desc = NULL, uint8_t isblock = 1);
        bool        Close      (bool Rcv = false, uint64_t seconds = 0);
        bool        IsConnected();
        int         Socket     ()  { return m_Socket; };
        int         Send       (char * pbuff, int pLen);

    private:
        Address     m_Addr      ;
        int         m_Socket    ;
        uint64_t    m_timeout   ;

};

#endif //___TCPSOCK_H___
