/////////////////////////////////////////////////////////////////////////
//
//  제어하는 클래스..
//
/////////////////////////////////////////////////////////////////////////

#ifndef ___MODEBUS_MAN_H___
#define ___MODEBUS_MAN_H___

#include "log_interface.h"
#include "dev_receiver.h"

class CModebusManager : virtual public CLogInterface
{
    public :
        CModebusManager(CSendMsg ** sndmsg);
        virtual ~CModebusManager();

        virtual void    SessionAlloc (uint32_t seqno, ClntInfo sock, string mac);
        virtual void    CommandDevice(ClntInfo sock, string mac);

    private:
        static void     freeData    (void * f);

    private:
        CSendMsg     ** m_SndMsg    ;
        CDataMap      * m_DevLst    ;
};

#endif //___MODEBUS_MAN_H___
