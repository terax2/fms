#ifndef ___TSDBHTTP_H___
#define ___TSDBHTTP_H___

#include "camera.h"
#include "CSmtp.h"

class CTSDBHTTP
{
    public :
        CTSDBHTTP();
        ~CTSDBHTTP();

    public :
        void        SetSvr      (char * ip, uint16_t port);
        void        Init        ();
        void        Close       ();
        bool        Send        (char * pData, int pLen = 0);

    private:
        void        SendMail    (char * pData, char * pUrl, int status);

    private:
        CURL      * m_cURL      ;
        char        m_cSTR      [SML_BUF];
        char        m_tUrl      [SML_BUF];

        time_t      m_stime     ;
        time_t      m_ntime     ;
};

#endif //___TSDBHTTP_H___
