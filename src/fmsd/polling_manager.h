/////////////////////////////////////////////////////////////////////////
//
//  제어하는 클래스..
//
/////////////////////////////////////////////////////////////////////////

#ifndef ___POLLING_MAN_H___
#define ___POLLING_MAN_H___

#include "fms_interface.h"
#include "polling_smp.h"

class CPollingManager : virtual public CFMSInterface
{
    public :
        CPollingManager(CSendMsg ** sndmsg);
        virtual ~CPollingManager();

        virtual void    ConnectDevice(uint32_t seqno, ClntInfo sock);
        virtual void    CommandDevice(ClntInfo sock);

    private:
        void            SNMPInit    ();
        uint8_t         getDevInfo  (ClntInfo * sock);
        uint8_t         Njs_Command (ClntInfo * sock);
        uint8_t         Njs_Response(ClntInfo * sock, uint8_t fcode, char * emsg = NULL, uint16_t len = 0);
        static void     freeData    (void * f    );
        static int      DoDQList    (void * param);
        static int      DoNQList    (void * param);
        static int      DoLQList    (void * param);
        static int      DoLTLoop    (void * param);

    private:
        CSendMsg     ** m_SndMsg    ;
        CDataMap      * m_DevLst    ;
        CDataMap      * m_LoraLst   ;
        SDL_Thread    * m_Thread    ;
        SDL_Thread    * m_Nhread    ;
        SDL_Thread    * m_Lhread    ;
        SDL_Thread    * m_LTLoop    ;
        DEVINFO         m_DevInfo   ;
        NJSCMD          m_NjsCmd    ;
        CLstQueue       m_DQList    ;
        CLstQueue       m_NQList    ;
        CLstQueue       m_LQList    ;
        uint32_t        m_SeqNo     ;
};

#endif //___POLLING_MAN_H___
