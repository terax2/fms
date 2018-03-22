/////////////////////////////////////////////////////////////////////////
//
//  DB Pool Å¬·¡½º..
//
/////////////////////////////////////////////////////////////////////////

#ifndef ___SENDMSG_H___
#define ___SENDMSG_H___

#include "DBInterface.h"
#include "tcpsock.h"

//typedef void (*resp_f)(void *);

class CSendMsg
{
    public :
        CSendMsg();
        ~CSendMsg();

    public :
        void            SendDB      (char * pbuff, int pLen, int uid = 1);
        void            SendNJ      (char * pbuff, int pLen, int uid = 1);
        //void        setResp     (resp_f m) {m_resp_f = m;};

    private:
        static int      DoDBQList   (void * param);
        static int      DoNJQList   (void * param);

    private:
        SDL_Thread    * m_DBThread  ;
        SDL_Thread    * m_NJThread  ;
        CLstQueue       m_DBQList   ;
        CLstQueue       m_NJQList   ;
        //resp_f          m_resp_f    ;
};


#endif //___SENDMSG_H___
