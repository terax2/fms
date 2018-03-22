/////////////////////////////////////////////////////////////////////////
//
//  제어하는 클래스..
//
/////////////////////////////////////////////////////////////////////////

#ifndef ___BYPASS_MAN_H___
#define ___BYPASS_MAN_H___

#include "log_interface.h"
#include "dev_receiver.h"

class CBypassManager : virtual public CLogInterface
{
    public :
        CBypassManager(CSendMsg ** sndmsg);
        virtual ~CBypassManager();

        virtual void    SessionAlloc (uint32_t seqno, ClntInfo sock, string mac);
        virtual void    CommandDevice(ClntInfo sock, string mac);

    private:
        static void     freeData    (void * f);

    private:
        CSendMsg     ** m_SndMsg    ;
        CDataMap      * m_DevLst    ;
};

#endif //___BYPASS_MAN_H___
