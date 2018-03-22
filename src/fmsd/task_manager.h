#ifndef ___TASK_MAN_H___
#define ___TASK_MAN_H___

#include "fms_interface.h"
#include "recv_manager.h"
#include "polling_manager.h"


class CTaskManager
{
    public :
        CTaskManager()      ;
        ~CTaskManager(void) ;

    private:
        void            Initialize      ();
        bool            StartTaskManager();
        void            StopTaskManager ();

        static int      NJS_Receiver    (void * param);
        static int      DEV_Receiver    (void * param);
        //static int      LORA_Receiver   (void * param);
        static int      DoEVQList       (void * param);
        static int      DoSMSList       (void * param);
        static int      DoDEVInit       (void * param);
        ///////////////////////////////////////////////

    private:
        CSendMsg     ** m_SndMsg        ;
        CFMSInterface * m_fPollManager  ;

        SDL_Thread    * m_EVThread      ;
        SDL_Thread    * m_SMShread      ;
        SDL_Thread    * m_DEVInit       ;
        uint32_t        m_SeqNo         ;
};

#endif //___TASK_MAN_H___
