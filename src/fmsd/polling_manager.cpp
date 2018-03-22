#include "polling_manager.h"
#include "fmsd_config.h"

extern CServerLog   pLog ;
extern CConfig      pcfg ;
extern bool         IsALive ;
extern CLstQueue    EVQList;

CPollingManager::CPollingManager(CSendMsg ** sndmsg)
:   m_SndMsg (sndmsg),
    m_DevLst (NULL  ),
    m_LoraLst(NULL  ),
    m_Thread (NULL  ),
    m_Nhread (NULL  ),
    m_Lhread (NULL  ),
    m_LTLoop (NULL  ),
    m_SeqNo  (0     )
{
    m_DevLst  = new CDataMap();
    m_DevLst->setfree(freeData);
    m_LoraLst = new CDataMap();
    m_LoraLst->setfree(freeData);

    m_DQList.set_maxqueue(_RECV_QUEUE_);
    m_Thread = SDL_CreateThread(DoDQList, this);
    m_Nhread = SDL_CreateThread(DoNQList, this);
    m_Lhread = SDL_CreateThread(DoLQList, this);
    m_LTLoop = SDL_CreateThread(DoLTLoop, this);
    EVQList.send_message(_DEV_CUR_CLEAR_, NULL, 0);

    //init_snmp("fmsd_snmp");
    SNMPInit();
    pLog.Print(LOG_WARNING, 0, NULL, 0, "%s() Starting ...", __FUNCTION__) ;
}

CPollingManager::~CPollingManager()
{
    if (m_Thread)
    {
        m_DQList.send_message(_CLOSE_, NULL, 0);

        SDL_WaitThread(m_Thread, NULL);
        m_Thread = NULL;

        CLst * pLst = NULL ;
        while ((pLst = m_DQList.get_message()) != NULL)
            delete pLst;
    }
    if (m_Nhread)
    {
        m_NQList.send_message(_CLOSE_, NULL, 0);

        SDL_WaitThread(m_Nhread, NULL);
        m_Nhread = NULL;

        CLst * pLst = NULL ;
        while ((pLst = m_NQList.get_message()) != NULL)
            delete pLst;
    }
    if (m_LTLoop)
    {
        SDL_WaitThread(m_LTLoop, NULL);
        m_LTLoop = NULL;
    }
    if (m_Lhread)
    {
        m_LQList.send_message(_CLOSE_, NULL, 0);

        SDL_WaitThread(m_Lhread, NULL);
        m_Lhread = NULL;

        CLst * pLst = NULL ;
        while ((pLst = m_LQList.get_message()) != NULL)
            delete pLst;
    }

    if (m_DevLst)
    {
        delete m_DevLst;
        m_DevLst = NULL;
    }
    if (m_LoraLst)
    {
        delete m_LoraLst;
        m_LoraLst = NULL;
    }
    pLog.Print(LOG_WARNING, 0, NULL, 0, "%s() Destroy ...", __FUNCTION__);
}

void CPollingManager::freeData (void * f)
{
    if (f)
    {
        CPollingSMP * dev = (CPollingSMP*)f;
        dev->Stop();
        dev->StopThread();
        delete dev;
        dev = NULL;
    }
}


void CPollingManager::SNMPInit ()
{
    //////////////////////////////////////
    CDBInterface    DBInf(pcfg.getMyDB());
    SNMPLST         snmpLst   ;
    DEVINFO         DevInfo   ;
    //////////////////////////////////////

    while (!DBInf.DBConnect())
        sleep(3);

    if(DBInf.getSNMPLst(&snmpLst))
    {
        for (uint32_t i = 0; i < snmpLst.Count; i++)
        {
            memset(&DevInfo, 0, SZ_DEVINFO);
            DevInfo.serial = snmpLst.snmp[i].serial;
            pLog.Print(LOG_INFO, 0, NULL, 0, "%s()[%d] SNMP serial : [%010u]", __FUNCTION__, __LINE__, DevInfo.serial);
            
            CPollingSMP * Dev = (CPollingSMP*)m_DevLst->getdata(DevInfo.serial);
            if (Dev == NULL)
            {
                Dev = new CPollingSMP(m_SndMsg);
                m_DevLst->insert(DevInfo.serial, Dev);
                Dev->StartThread();
            }
            Dev->Dev_SnmpUpdate(DevInfo, i % pcfg.getPoolCount());
            SDL_Delay(2);
        }
    }
    
    DBInf.DBClose();
    return;
}

uint8_t CPollingManager::getDevInfo(ClntInfo * sock)
{
    UMSG uMsg;
    //// Recv Sync Message /////////////////////////////////////////////////////////////////////////////////
    int readBytes = getDataExact(sock->fd, uMsg.msg, 5, _RECV_TIME_);
    if (readBytes != 5)
    {
        pLog.Print(LOG_ERROR, 0, NULL, 0, "%s()[%d] recv sync err = (%s)", __FUNCTION__, __LINE__, getError(readBytes, _RECV_TIME_));
        return 200;
    }
    ////////////////////////////////////////////////////////////////////////////////////////////////////////

    //// Send Message //////////////////////////////////////////////////////////////////////////////////////
    uint16_t tlen = MakePacket(&uMsg, DEV_FCODE_GET_DEV_INFO);
    pLog.Print(LOG_DEBUG, 1, uMsg.msg, tlen, "%s()[%d] To Device Send message !!!", __FUNCTION__, __LINE__);
    //// Send Message //////////////////////////////////////////////////////////////////////////////////////
    int sendbytes = SendSocketExact (sock->fd, uMsg.msg, tlen);
    if (sendbytes != tlen)
    {
        pLog.Print(LOG_ERROR, 0, NULL, 0, "%s()[%d] to Device Send error (%d)(%s) !!!", __FUNCTION__, __LINE__, errno, strerror(errno));
        return 201;
    }

    //// Recv Message //////////////////////////////////////////////////////////////////////////////////////
    memset(uMsg.msg, 0, sizeof(uMsg.msg));
    HMSG  * head = (HMSG*)uMsg.msg ;
    char  * body = (char*)&uMsg.msg[SZ_HMSG];

    pLog.Print(LOG_DEBUG, 0, NULL, 0, "%s()[%d] from Device Recv message !!!", __FUNCTION__, __LINE__);
    readBytes = getDataExact(sock->fd, uMsg.msg, SZ_HMSG, _RECV_TIME_);
    if (readBytes != SZ_HMSG)
    {
        pLog.Print(LOG_ERROR, 0, NULL, 0, "%s()[%d] recv head err = (%s)", __FUNCTION__, __LINE__, getError(readBytes, _RECV_TIME_));
        return 202;
    }

    if (head->sop == _SMP_PKT_SOP_)
    {
        if (head->fcode == UDP_FCODE_RES_ERROR)
        {
            pLog.Print(LOG_ERROR, 0, NULL, 0, "%s[%d] head->fcode = [%02x] !!!!!!!!!!!!!! ERROR !!!!!!!!!!!!!!", __FUNCTION__, __LINE__, head->fcode);
            return _FAIL_;
        }
        uint16_t bodySize = htons(head->length);
        readBytes = getDataExact(sock->fd, body, bodySize, _RECV_TIME_);
        if (readBytes != bodySize)
        {
            pLog.Print(LOG_ERROR, 0, NULL, 0, "%s()[%d] recv body err = (%s)", __FUNCTION__, __LINE__, getError(readBytes, _RECV_TIME_));
            return 203;
        }

        bodySize -= 1;
        TMSG * tail = (TMSG*)&body[bodySize];
        if (tail->eop == _SMP_PKT_EOP_)
        {
            m_DevInfo = *(DEVINFO*)body;
            snprintf((char*)m_DevInfo.localcode, sizeof(m_DevInfo.localcode), "%04u", atoi((char*)m_DevInfo.localcode));
            m_DevInfo.loraserial[16] = '\0';

            pLog.Print(LOG_INFO , 0, NULL, 0, "devinfo.serial       = [%010u]", m_DevInfo.serial);
            pLog.Print(LOG_DEBUG, 0, NULL, 0, "devinfo.macaddr      = [%02X:%02X:%02X:%02X:%02X:%02X]", m_DevInfo.macaddr[0], m_DevInfo.macaddr[1], m_DevInfo.macaddr[2]
                                                                                                      , m_DevInfo.macaddr[3], m_DevInfo.macaddr[4], m_DevInfo.macaddr[5]);
            pLog.Print(LOG_DEBUG, 0, NULL, 0, "devinfo.localip      = [%u.%u.%u.%u]", m_DevInfo.localip[0], m_DevInfo.localip[1], m_DevInfo.localip[2], m_DevInfo.localip[3]);
            pLog.Print(LOG_DEBUG, 0, NULL, 0, "devinfo.modelcode    = [%u]", m_DevInfo.modelcode);
            pLog.Print(LOG_DEBUG, 0, NULL, 0, "devinfo.modelname    = [%s]", (char*)m_DevInfo.modelname);
            pLog.Print(LOG_DEBUG, 0, NULL, 0, "devinfo.localname    = [%s]", (char*)m_DevInfo.localname);
            pLog.Print(LOG_DEBUG, 0, NULL, 0, "devinfo.localcode    = [%s]", (char*)m_DevInfo.localcode);
            pLog.Print(LOG_DEBUG, 0, NULL, 0, "devinfo.umsserial    = [%s]", (char*)m_DevInfo.umsserial);
            pLog.Print(LOG_DEBUG, 0, NULL, 0, "devinfo.upsserial    = [%s]", (char*)m_DevInfo.upsserial);
            pLog.Print(LOG_DEBUG, 0, NULL, 0, "devinfo.batserial    = [%s]", (char*)m_DevInfo.batserial);
            pLog.Print(LOG_DEBUG, 0, NULL, 0, "devinfo.batboxserial = [%s]", (char*)m_DevInfo.batboxserial);
            pLog.Print(LOG_DEBUG, 0, NULL, 0, "devinfo.loraserial   = [%s]", (char*)m_DevInfo.loraserial);
        }   else
        {
            pLog.Print(LOG_ERROR, 0, NULL, 0, "%s()[%d] End of Packet failed :: EOP = [%02X]", __FUNCTION__, __LINE__, tail->eop);
            return 205;
        }
    }   else
    {
        pLog.Print(LOG_ERROR, 0, NULL, 0, "%s()[%d] Start of Packet failed :: SOP = [%02X]", __FUNCTION__, __LINE__, head->sop);
        return 206;
    }

    return _ISOK_;
}

int CPollingManager::DoDQList (void * param)
{
    ///////////////////////////////////////////////////
    CPollingManager * self  = (CPollingManager*)param ;
    CLst            * pLst  = NULL  ;
    ClntInfo        * sock  = NULL  ;
    uint32_t          size  = 0     ;
    uint32_t          seqno = 0     ;
    ///////////////////////////////////////////////////

    pLog.Print(LOG_DEBUG, 0, NULL, 0, "%s() Starting ...", __FUNCTION__);
    while (SDL_SemWait(self->m_DQList.get_semaphore()) == 0)
    {
        while ((pLst = self->m_DQList.get_message()) != NULL)
        {
            if (!pLst->get_type())
            {
                delete pLst ;
                pLst = NULL ;
                return 0    ;
            }

            seqno = pLst->get_type();
            sock  = (ClntInfo*)pLst->get_message (size);
            if (sock)
            {
                if (self->getDevInfo(sock) == _ISOK_)
                {
                    CPollingSMP * Dev = (CPollingSMP*)self->m_DevLst->getdata(self->m_DevInfo.serial);
                    if (Dev == NULL)
                    {
                        Dev = new CPollingSMP(self->m_SndMsg);
                        self->m_DevLst->insert(self->m_DevInfo.serial, Dev);
                        self->m_DevInfo.loraserial[16] = '\0';
                        self->m_LoraLst->insert((char*)self->m_DevInfo.loraserial, Dev);
                        Dev->StartThread();
                    }
                    Dev->Dev_Update(*sock, self->m_DevInfo, seqno % pcfg.getPoolCount());
                    SDL_Delay(2);
                }   else
                {
                    pLog.Print(LOG_ERROR, 0, NULL, 0, "%s()[%d] getDevInfo Error !!!!", __FUNCTION__, __LINE__);
                    epCloseN(sock, true);
                }
            }

            delete pLst  ;
            pLst = NULL  ;
        }
    }
    return 0;
}

uint8_t CPollingManager::Njs_Command(ClntInfo * sock)
{
    UMSG uMsg;
    memset(&m_NjsCmd, 0, SZ_NJSCMD);

    // Recv Message ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
    memset(uMsg.msg, 0, sizeof(uMsg.msg));
    HMSG  * head = (HMSG*)uMsg.msg ;
    char  * body = (char*)&uMsg.msg[SZ_HMSG];

    int readBytes = getDataExact(sock->fd, uMsg.msg, SZ_HMSG, _RECV_TIME_);
    if (readBytes != SZ_HMSG)
    {
        pLog.Print(LOG_ERROR, 0, NULL, 0, "%s()[%d] recv head err = (%s)", __FUNCTION__, __LINE__, getError(readBytes, _RECV_TIME_));
        return 202;
    }

    pLog.Print(LOG_DEBUG, 1, uMsg.msg, SZ_HMSG, "%s()[%d] recv message", __FUNCTION__, __LINE__);
    if (head->sop == _SMP_PKT_SOP_)
    {
        uint16_t bodySize = htons(head->length);
        readBytes = getDataExact(sock->fd, body, bodySize, _RECV_TIME_);
        if (readBytes != bodySize)
        {
            pLog.Print(LOG_ERROR, 0, NULL, 0, "%s()[%d] recv body err = (%s)", __FUNCTION__, __LINE__, getError(readBytes, _RECV_TIME_));
            return 203;
        }

        bodySize -= 1;
        TMSG * tail = (TMSG*)&body[bodySize];

        pLog.Print(LOG_DEBUG, 1, body, bodySize+1, "%s()[%d] recv message", __FUNCTION__, __LINE__);
        if (tail->eop == _SMP_PKT_EOP_)
        {
            m_NjsCmd.fcode = head->fcode;
            if (m_NjsCmd.fcode == NJS_FCODE_GET_CONFIG || m_NjsCmd.fcode == NJS_FCODE_GET_NETWORK)
            {
                m_NjsCmd.info = *(INFO*)body;
                m_NjsCmd.info.serial = htonl(m_NjsCmd.info.serial);
                pLog.Print(LOG_DEBUG, 0, NULL, 0, "m_NjsCmd.fcode                 = [%02X]" , m_NjsCmd.fcode      );
                pLog.Print(LOG_INFO , 0, NULL, 0, "m_NjsCmd.info.serial           = [%010u]", m_NjsCmd.info.serial);
                pLog.Print(LOG_DEBUG, 0, NULL, 0, "m_NjsCmd.info.devip            = [%u.%u.%u.%u]", m_NjsCmd.info.devip[0], m_NjsCmd.info.devip[1], m_NjsCmd.info.devip[2], m_NjsCmd.info.devip[3]);
            }   else
            if (m_NjsCmd.fcode == NJS_FCODE_SET_CONFIG)
            {
                m_NjsCmd.info = *(INFO*)body;
                m_NjsCmd.info.serial = htonl(m_NjsCmd.info.serial);
                pLog.Print(LOG_DEBUG, 0, NULL, 0, "m_NjsCmd.fcode                 = [%02X]" , m_NjsCmd.fcode      );
                pLog.Print(LOG_INFO , 0, NULL, 0, "m_NjsCmd.info.serial           = [%010u]", m_NjsCmd.info.serial);
                pLog.Print(LOG_DEBUG, 0, NULL, 0, "m_NjsCmd.info.devip            = [%u.%u.%u.%u]", m_NjsCmd.info.devip[0], m_NjsCmd.info.devip[1], m_NjsCmd.info.devip[2], m_NjsCmd.info.devip[3]);

                m_NjsCmd.scfg.cfg = *(CFG*)&body[SZ_INFO];
                m_NjsCmd.scfg.cfg.serverport = htons(m_NjsCmd.scfg.cfg.serverport);
                pLog.Print(LOG_DEBUG, 0, NULL, 0, "m_NjsCmd.scfg.cfg.serverip     = [%u.%u.%u.%u]", m_NjsCmd.scfg.cfg.serverip[0]  , m_NjsCmd.scfg.cfg.serverip[1]  , m_NjsCmd.scfg.cfg.serverip[2]  , m_NjsCmd.scfg.cfg.serverip[3]  );
                pLog.Print(LOG_DEBUG, 0, NULL, 0, "m_NjsCmd.scfg.cfg.serverport   = [%u]", m_NjsCmd.scfg.cfg.serverport);
                pLog.Print(LOG_DEBUG, 0, NULL, 0, "m_NjsCmd.scfg.cfg.localcode    = [%s]", (char*)m_NjsCmd.scfg.cfg.localcode   );
                pLog.Print(LOG_DEBUG, 0, NULL, 0, "m_NjsCmd.scfg.cfg.umsserial    = [%s]", (char*)m_NjsCmd.scfg.cfg.umsserial   );
                pLog.Print(LOG_DEBUG, 0, NULL, 0, "m_NjsCmd.scfg.cfg.upsserial    = [%s]", (char*)m_NjsCmd.scfg.cfg.upsserial   );
                pLog.Print(LOG_DEBUG, 0, NULL, 0, "m_NjsCmd.scfg.cfg.batserial    = [%s]", (char*)m_NjsCmd.scfg.cfg.batserial   );
                pLog.Print(LOG_DEBUG, 0, NULL, 0, "m_NjsCmd.scfg.cfg.batboxserial = [%s]", (char*)m_NjsCmd.scfg.cfg.batboxserial);
                pLog.Print(LOG_DEBUG, 0, NULL, 0, "m_NjsCmd.scfg.cfg.loraserial   = [%s]", (char*)m_NjsCmd.scfg.cfg.loraserial  );
            }   else
            if (m_NjsCmd.fcode == NJS_FCODE_SET_NETWORK)
            {
                m_NjsCmd.info = *(INFO*)body;
                m_NjsCmd.info.serial = htonl(m_NjsCmd.info.serial);
                pLog.Print(LOG_DEBUG, 0, NULL, 0, "m_NjsCmd.fcode                 = [%02X]" , m_NjsCmd.fcode      );
                pLog.Print(LOG_INFO , 0, NULL, 0, "m_NjsCmd.info.serial           = [%010u]", m_NjsCmd.info.serial);
                pLog.Print(LOG_DEBUG, 0, NULL, 0, "m_NjsCmd.info.devip            = [%u.%u.%u.%u]", m_NjsCmd.info.devip[0], m_NjsCmd.info.devip[1], m_NjsCmd.info.devip[2], m_NjsCmd.info.devip[3]);

                m_NjsCmd.snet.net = *(NETWORK*)&body[SZ_INFO];
                pLog.Print(LOG_DEBUG, 0, NULL, 0, "m_NjsCmd.snet.net.dhcp         = [%u]", m_NjsCmd.snet.net.dhcp );
                pLog.Print(LOG_DEBUG, 0, NULL, 0, "m_NjsCmd.snet.net.localip      = [%u.%u.%u.%u]", m_NjsCmd.snet.net.localip[0]   , m_NjsCmd.snet.net.localip[1]   , m_NjsCmd.snet.net.localip[2]   , m_NjsCmd.snet.net.localip[3]   );
                pLog.Print(LOG_DEBUG, 0, NULL, 0, "m_NjsCmd.snet.net.subnetmask   = [%u.%u.%u.%u]", m_NjsCmd.snet.net.subnetmask[0], m_NjsCmd.snet.net.subnetmask[1], m_NjsCmd.snet.net.subnetmask[2], m_NjsCmd.snet.net.subnetmask[3]);
                pLog.Print(LOG_DEBUG, 0, NULL, 0, "m_NjsCmd.snet.net.gateway      = [%u.%u.%u.%u]", m_NjsCmd.snet.net.gateway[0]   , m_NjsCmd.snet.net.gateway[1]   , m_NjsCmd.snet.net.gateway[2]   , m_NjsCmd.snet.net.gateway[3]   );
                pLog.Print(LOG_DEBUG, 0, NULL, 0, "m_NjsCmd.snet.net.dns          = [%u.%u.%u.%u]", m_NjsCmd.snet.net.dns[0]       , m_NjsCmd.snet.net.dns[1]       , m_NjsCmd.snet.net.dns[2]       , m_NjsCmd.snet.net.dns[3]       );
            }   else
            if (m_NjsCmd.fcode == NJS_FCODE_AFW_UPDATE)
            {
                m_NjsCmd.fwu   = *(FWUPT*)body;
                m_NjsCmd.fwu.serial  = htonl(m_NjsCmd.fwu.serial);
                m_NjsCmd.info.serial = m_NjsCmd.fwu.serial;
                pLog.Print(LOG_DEBUG, 0, NULL, 0, "m_NjsCmd.fcode                 = [%02X]" , m_NjsCmd.fcode      );
                pLog.Print(LOG_INFO , 0, NULL, 0, "m_NjsCmd.fwu.serial            = [%010u]", m_NjsCmd.fwu.serial );
                pLog.Print(LOG_DEBUG, 0, NULL, 0, "m_NjsCmd.fwu.devip             = [%u.%u.%u.%u]", m_NjsCmd.fwu.devip[0], m_NjsCmd.fwu.devip[1], m_NjsCmd.fwu.devip[2], m_NjsCmd.fwu.devip[3]);
                pLog.Print(LOG_DEBUG, 0, NULL, 0, "m_NjsCmd.fwu.filepath          = [%s]"   , m_NjsCmd.fwu.filepath);
            }   else
            {
                m_NjsCmd.did   = *(DID*)body;
                m_NjsCmd.did.serial = htonl(m_NjsCmd.did.serial);
                pLog.Print(LOG_DEBUG, 0, NULL, 0, "m_NjsCmd.fcode                 = [%02X]" , m_NjsCmd.fcode      );
                pLog.Print(LOG_INFO , 0, NULL, 0, "m_NjsCmd.did.serial            = [%010u]", m_NjsCmd.did.serial );

                if (m_NjsCmd.fcode != NJS_FCODE_UPS_RESET      && m_NjsCmd.fcode != NJS_FCODE_SET_LIMIT &&
                    m_NjsCmd.fcode != NJS_FCODE_SET_HEAT_RESET && m_NjsCmd.fcode != NJS_FCODE_UPS_WAKEUP)
                {
                    m_NjsCmd.value = (uint8_t)body[SZ_DID];
                    pLog.Print(LOG_DEBUG, 0, NULL, 0, "m_NjsCmd.value                 = [%u]", m_NjsCmd.value);
                }
            }
        }   else
        {
            pLog.Print(LOG_ERROR, 0, NULL, 0, "%s()[%d] End of Packet failed :: EOP = [%02X]", __FUNCTION__, __LINE__, tail->eop);
            return 205;
        }
    }   else
    {
        pLog.Print(LOG_ERROR, 0, NULL, 0, "%s()[%d] Start of Packet failed :: SOP = [%02X]", __FUNCTION__, __LINE__, head->sop);
        return 206;
    }

    return _ISOK_;
}


uint8_t CPollingManager::Njs_Response(ClntInfo * sock, uint8_t fcode, char * emsg, uint16_t len)
{
    UMSG uMsg;
    uMsg.sock = *sock;

    uint16_t tlen = MakePacket(&uMsg, fcode, emsg, len);
    pLog.Print(LOG_DEBUG, 1, uMsg.msg, tlen, "%s()[%d] before send message !!!", __FUNCTION__, __LINE__);

    // Send Message ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
    int sendbytes = SendSocketExact (uMsg.sock.fd, uMsg.msg, tlen);
    if (sendbytes != tlen)
    {
        pLog.Print(LOG_ERROR, 0, NULL, 0, "%s()[%d] send message error (%d)(%s) !!", __FUNCTION__, __LINE__, errno, strerror(errno));
        return 201;
    }

    return _ISOK_;
}

int CPollingManager::DoNQList (void * param)
{
    ///////////////////////////////////////////////////
    CPollingManager * self  = (CPollingManager*)param ;
    CLst            * pLst  = NULL                    ;
    ClntInfo        * sock  = NULL                    ;
    uint32_t          size  = 0                       ;
    ///////////////////////////////////////////////////

    pLog.Print(LOG_DEBUG, 0, NULL, 0, "%s() Starting ...", __FUNCTION__);
    while (SDL_SemWait(self->m_NQList.get_semaphore()) == 0)
    {
        while ((pLst = self->m_NQList.get_message()) != NULL)
        {
            if (!pLst->get_type())
            {
                delete pLst ;
                pLst = NULL ;
                return 0    ;
            }

            sock = (ClntInfo*)pLst->get_message (size);
            if (sock)
            {
                if (self->Njs_Command(sock) == _ISOK_)
                {
                    uint32_t serial = 0;
                    if (self->m_NjsCmd.fcode > 0x30)
                        serial = self->m_NjsCmd.info.serial ;
                    else
                        serial = self->m_NjsCmd.did.serial;

                    if (serial == 0xFFFFFFFF)
                    {
                        map < uint32_t, void *, less<uint32_t> > * idata = self->m_DevLst->getIlist();
                        map < uint32_t, void *, less<uint32_t> >::iterator iter;
                        for ( iter = idata->begin(); iter != idata->end(); )
                        {
                            CPollingSMP * Dev = (CPollingSMP*)(*iter).second;
                            Dev->Njs_Command(&self->m_NjsCmd);
                            ++iter  ;
                        }
                        self->Njs_Response(sock, self->m_NjsCmd.fcode);
                    }   else
                    {
                        CPollingSMP * Dev = (CPollingSMP*)self->m_DevLst->getdata(serial);
                        if (Dev)
                        {
                            if (Dev->Njs_Command(&self->m_NjsCmd) != _ISOK_)
                            {
                                pLog.Print(LOG_ERROR, 0, NULL, 0, "%s()[%d] NodeJS Command Fail !!! [%010u][%02X]", __FUNCTION__, __LINE__, serial, self->m_NjsCmd.fcode);
                                self->Njs_Response(sock, UDP_FCODE_RES_ERROR, self->m_NjsCmd.eMsg.emsg, self->m_NjsCmd.eMsg.elen);
                            }   else
                                self->Njs_Response(sock, self->m_NjsCmd.fcode);
                        }   else
                        {
                            /////////////////////////////////////////////////////////////
                            DEVINFO     DevInfo   ;
                            memset(&DevInfo, 0, SZ_DEVINFO);
                            DevInfo.serial = serial;

                            Dev = new CPollingSMP(self->m_SndMsg);
                            self->m_DevLst->insert(DevInfo.serial, Dev);
                            Dev->StartThread();
                            Dev->Dev_SnmpUpdate(DevInfo, DevInfo.serial % pcfg.getPoolCount());
                            pLog.Print(LOG_INFO, 0, NULL, 0, "%s()[%d] SNMP serial : [%010u]", __FUNCTION__, __LINE__, serial);

                            while (!Dev->IsStarting()) SDL_Delay(10);
                            if ( Dev->getDevAflag() == _SNMP_TYPE_)
                            {
                                self->Njs_Response(sock, self->m_NjsCmd.fcode);
                            }   else
                            {
                                pLog.Print(LOG_ERROR, 0, NULL, 0, "%s()[%d] NodeJS Cmmand Fail, Device Not Connected [%010u] !!!!", __FUNCTION__, __LINE__, serial);
                                self->Njs_Response(sock, UDP_FCODE_RES_ERROR);
                            }
                            /////////////////////////////////////////////////////////////
                        }
                    }
                }
                epCloseN(sock, true);
            }

            delete pLst  ;
            pLst = NULL  ;
        }
    }
    return 0;
}

int CPollingManager::DoLQList (void * param)
{
    ///////////////////////////////////////////////////
    CPollingManager * self  = (CPollingManager*)param ;
    CLst            * pLst  = NULL                    ;
    CTCPSock          Lora (pcfg.getLoraInfo())       ;
    char            * pBuf  = new char [_LORA_BUFF_]  ;
    int               pLen  = 0                       ;
    int               lCnt  = 0                       ;
    ///////////////////////////////////////////////////

    pLog.Print(LOG_DEBUG, 0, NULL, 0, "%s() Starting ...", __FUNCTION__);
    while (SDL_SemWait(self->m_LQList.get_semaphore()) == 0)
    {
        while ((pLst = self->m_LQList.get_message()) != NULL)
        {
            if (!pLst->get_type())
            {
                if (pBuf)
                {
                    delete[] pBuf;
                    pBuf = NULL  ;
                }
                delete pLst ;
                pLst = NULL ;
                return 0    ;
            }

            if (Lora.Connect((char*)"Lora Connect"))
            {
                while(true)
                {
                    int readBytes = getDataExact(Lora.Socket(), pBuf, 4, _LORA_TIME_);
                    if (readBytes != 4)
                    {
                        if (readBytes == -1)
                            pLog.Print(LOG_DEBUG, 0, NULL, 0, "%s()[%d] (%s)", __FUNCTION__, __LINE__, getError(readBytes, _LORA_TIME_));
                        else
                            pLog.Print(LOG_ERROR, 0, NULL, 0, "%s()[%d] Recv Error = (%s)", __FUNCTION__, __LINE__, getError(readBytes, _LORA_TIME_));
                        break;
                    }   else
                    {
                        pLen = htonl(HexToU32(pBuf, 4));
                        pLog.Print(LOG_DEBUG, 0, NULL, 0, "%s()[%d] Lora Body length = [%u]", __FUNCTION__, __LINE__, pLen);

                        readBytes = getDataExact(Lora.Socket(), pBuf, pLen, _LORA_TIME_);
                        if (readBytes != pLen)
                        {
                            pLog.Print(LOG_ERROR, 0, NULL, 0, "%s()[%d] Lora Body err = (%s)", __FUNCTION__, __LINE__, getError(readBytes, _LORA_TIME_));
                            break;
                        }   else
                        {
                            lCnt = pLen / SZ_GETLORA;
                            pLog.Print(LOG_WARNING, 0, NULL, 0, "%s()[%d] Lora Data Total Count = [%d]", __FUNCTION__, __LINE__, lCnt);

                            for ( int i = 0; i < lCnt; i++ )
                            {
                                GETLORA * GetLora = (GETLORA*)&pBuf[i * SZ_GETLORA];

                                memmove (GetLora->loraserial, &GetLora->loraserial[8], 16);
                                GetLora->loraserial[16] = '\0';

                                CPollingSMP * Dev = (CPollingSMP*)self->m_LoraLst->getdata(GetLora->loraserial);
                                if (Dev)
                                {
                                    Dev->Lora_Message(GetLora);
                                }   else
                                {
                                    pLog.Print(LOG_WARNING, 0, NULL, 0, "%s()[%d] Controller not connected (Lora passing fail), Lora Serial = [%s] !!!!", __FUNCTION__, __LINE__, GetLora->loraserial);
                                }
                            }
                        }
                    }
                }
                Lora.Close();
            }

            delete pLst  ;
            pLst = NULL  ;
        }
    }
    if (pBuf)
    {
        delete[] pBuf;
        pBuf = NULL  ;
    }
    return 0;
}

int CPollingManager::DoLTLoop (void * param)
{
    ///////////////////////////////////////////////////
    CPollingManager * self  = (CPollingManager*)param ;
    ///////////////////////////////////////////////////

    pLog.Print(LOG_DEBUG, 0, NULL, 0, "%s() Starting ...", __FUNCTION__);
    while (IsALive)
    {
        sleep(pcfg.getRefreshTime());
        self->m_LQList.send_message(1, NULL, 0);
    }
    return 0;
}

void CPollingManager::ConnectDevice(uint32_t seqno, ClntInfo sock)
{
    m_DQList.send_message(seqno, (const void*)&sock, sizeof(ClntInfo));
}

void CPollingManager::CommandDevice(ClntInfo sock)
{
    m_NQList.send_message(1, (const void*)&sock, sizeof(ClntInfo));
}

