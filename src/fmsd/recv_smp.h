#ifndef __SMP_RECEIVER_H__
#define __SMP_RECEIVER_H__

#include "media_sink.h"
#include "media_feeder.h"
#include "sendmsg.h"
//#include "tcpsock.h"

class CRecvSMP : public CMediaSink, public CMediaFeeder
{
    public:
        CRecvSMP(CSendMsg ** sndmsg);
        virtual ~CRecvSMP();
        bool                getActive       () {return m_Source;};
        void                setUpdate       (ClntInfo sock);
        void                setSeqNo        (uint32_t seqno) { m_seqno = seqno; };

    protected:
        virtual int         ThreadMain      ();
        void                DoStartCapture  ();
        void                DoStopCapture   ();

    private:
        void                DeviceRequest   (UMSG * uMsg );
        static int          DoQMList        (void * param);
        //static void         RespFunc        (void * param);

    protected:
        bool                m_Source        ;

    private:
        UMSG              * m_uMsg          ;
        //ClntInfo            m_sock          ;
        //char              * m_Buff          ;
        //int16_t             m_Size          ;
        /////////////////////////////////////
        SDL_Thread        * m_Thread        ;
        CLstQueue           m_MQList        ;
        CSendMsg         ** m_SndMsg        ;
        //CTCPSock          * m_NodeJS        ;
        uint32_t            m_seqno         ;
        
};


#endif /* __SMP_RECEIVER_H__ */

