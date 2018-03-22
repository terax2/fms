#ifndef __POLLING_SMP_H__
#define __POLLING_SMP_H__

#include "media_sink.h"
#include "media_feeder.h"
#include "sendmsg.h"


class CPollingSMP : public CMediaSink, public CMediaFeeder
{
    public:
        CPollingSMP(CSendMsg ** sndmsg);
        virtual ~CPollingSMP();
        void                Dev_Update      (ClntInfo sock, DEVINFO devInfo, uint32_t seqno);
        void                Dev_SnmpUpdate  (DEVINFO devInfo, uint32_t seqno);
        uint8_t             Njs_Command     (NJSCMD  * pNjsCmd = NULL);
        void                Lora_Message    (GETLORA * pLora);
        bool                IsStarting      ()  { return m_Source; };
        uint8_t             getDevAflag     ()  { return m_Devdef->aflag; };

    protected:
        virtual int         ThreadMain      ();
        void                DoStartCapture  ();
        void                DoStopCapture   ();
        void                NetworkAlarm    ();

    private:
        void                EvtchkRefresh   ();
        uint64_t            checkLimit      ();
        uint8_t             getDevData      (UMSG   * pMsg);
        void                getSnmpData     (DATA   * pData, DATA2  * pData2, uint8_t phase);
        //uint8_t             SnmpGetData     (DATA   * pData, DATA2  * pData2, char * pSnmpIp, uint8_t phase);
        //uint8_t             snmpGetEx       (char   * pOid , char   * pIp   , uint16_t * pv);
        //uint8_t             snmpGetEx2      (char   * pOid , char   * pIp   );
        //uint8_t             snmpGetLib      (char   * pOid , char   * pIp   );
        //uint8_t             SnmpGetDataEx   (DATA   * pData, DATA2  * pData2, char * pSnmpIp, uint8_t phase);
        uint8_t             getDevDataPost  ()                  ;
        uint8_t             setDevRequest   (UMSG   * pMsg , uint8_t fcode, void * pData = NULL, int len = 0);
        uint8_t             setKeepAlive    (UMSG   * pMsg )    ;
        static int          DoMQList        (void   * param)    ;
        static int          DoLQList        (void   * param)    ;
        static void         freeLimit       (void   * f    )    ;
        UMSG              * getMsg          () { return &m_Msg;};

        uint8_t             uptLoraData     (GETLORA * pLora);

        uint8_t             getUDPRequest   (uint8_t fcode, INFO * info, RETMSG * eMsg);
        uint8_t             setUDPRequest   (uint8_t fcode, INFO * info, CFG * cfg, RETMSG * eMsg);
        uint8_t             setUDPRequest   (uint8_t fcode, INFO * info, NETWORK * net, RETMSG * eMsg);
        uint8_t             firmwareUpdate  (uint8_t fcode, FWUPT * fwu, RETMSG * eMsg);

    protected:
        bool                m_Keep          ;
        bool                m_Init          ;
        bool                m_Source        ;
        bool                m_IsLora        ;
        bool                m_TryTo         ;
        bool                m_NetStop       ;

    private:
        UMSG                m_Msg           ;
        /////////////////////////////////////
        SDL_Thread        * m_Thread        ;
        SDL_Thread        * m_Lhread        ;
        CLstQueue           m_MQList        ;
        CLstQueue           m_LQList        ;
        CSendMsg         ** m_SndMsg        ;
        //CDBInterface      * m_DBInf         ;
        DEVDEF            * m_Devdef        ;
        GETDATA           * m_GetData       ;
        EVENTS            * m_Events        ;
        DEVSOCK           * m_Devsock       ;
        EVTCHK              m_EvtChk        [EVT_CHECK_CNT];
        BATCHK              m_BatChk        [BAT_CHECK_CNT];
        BATCHK              m_UpsChk        [UPS_CHECK_CNT];
        OSTAT               m_Status        ;
        /////////////////////////////////////
        DEVINFO             m_DevInfo       ;
        uint32_t            m_seqno         ;
        Mutex               m_Mutex         ;
        LORA                m_LData         ;
        uint8_t             m_FFlag         ;
        /////////////////////////////////////
        TCHECK              m_dev           ;
        TCHECK              m_njs           ;
        TCHECK              m_keep          ;
        TCHECK              m_dbs           ;
        TCHECK              m_Lora          ;
};


#endif /* __POLLING_SMP_H__ */

