/////////////////////////////////////////////////////////////////////////
//
//  제어하는 클래스..
//
/////////////////////////////////////////////////////////////////////////

#ifndef ___RECV_MAN_H___
#define ___RECV_MAN_H___

#include "fms_interface.h"
#include "recv_smp.h"

class CRecvManager : virtual public CFMSInterface
{
    public :
        CRecvManager(CSendMsg ** sndmsg);
        virtual ~CRecvManager();

        virtual void    ConnectDevice(uint32_t seqno, ClntInfo sock);
        virtual void    CommandDevice(ClntInfo sock);

    private:
        static void     freeData    (void * f);

    private:
        CSendMsg     ** m_SndMsg    ;
        CDataMap      * m_DevLst    ;
        uint32_t        m_SeqNo     ;
};

#endif //___RECV_MAN_H___
