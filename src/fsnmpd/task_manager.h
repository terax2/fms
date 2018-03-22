#ifndef ___TASK_MAN_H___
#define ___TASK_MAN_H___

#include "camera.h"
#include "DBInterface.h"

class CTaskManager
{
    public :
        CTaskManager()      ;
        ~CTaskManager(void) ;

    private:
        void            Initialize      ();
        void            SNMPInit        ();
        bool            StartTaskManager();
        void            StopTaskManager ();

        static int      DoDBRetry       (void * param);
        static int      DoDBWrite       (void * param);
        static int      DoSnmpGet       (void * param);
        static void     freeData        (void * f    );
        ///////////////////////////////////////////////

    private:
        SDL_Thread    * m_DBWrite       ;
};

#endif //___TASK_MAN_H___
