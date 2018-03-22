#include "tsdbhttp.h"
extern  CServerLog  pLog;

CTSDBHTTP::CTSDBHTTP()
:   m_cURL(NULL )
{
    memset(&m_stime, 0, sizeof(m_stime));
    memset(&m_ntime, 0, sizeof(m_ntime));
    pLog.Print(LOG_DEBUG, 0, NULL, 0, "%s() Starting ...", __FUNCTION__);
}

CTSDBHTTP::~CTSDBHTTP()
{
    Close();
    pLog.Print(LOG_DEBUG, 0, NULL, 0, "%s() Destroy  ...", __FUNCTION__);
}


void CTSDBHTTP::SetSvr (char * ip, uint16_t port)
{
    snprintf(m_cSTR, sizeof(m_cSTR), "%s:%d", ip, port);
    pLog.Print(LOG_DEBUG, 0, NULL, 0, "%s()[%d] Server Info = (%s)", __FUNCTION__, __LINE__, m_cSTR);
}

void CTSDBHTTP::Init()
{
    if (m_cURL == NULL)
    {
        m_cURL = curl_easy_init();
        while(m_cURL == NULL)
        {
            pLog.Print(LOG_ERROR, 0, NULL, 0, "%s()[%d] Unable to initialize cURL interface...", __FUNCTION__, __LINE__);
            SDL_Delay(3000);
            m_cURL = curl_easy_init();
        }
        snprintf(m_tUrl, sizeof(m_tUrl), "http://%s/api/put", m_cSTR);
        curl_easy_setopt(m_cURL, CURLOPT_NOSIGNAL, 1);
        curl_easy_setopt(m_cURL, CURLOPT_URL, m_tUrl);
        ////////////////////////////////////////////////////
        //curl_easy_setopt(m_cURL, CURLOPT_TCP_KEEPALIVE, 1L); //curl 7.25 이상.
        //curl_easy_setopt(m_cURL, CURLOPT_TCP_KEEPIDLE , 120L);
        //curl_easy_setopt(m_cURL, CURLOPT_TCP_KEEPINTVL, 60L);
        ////////////////////////////////////////////////////
        curl_easy_setopt(m_cURL, CURLOPT_POST, 1);
        curl_easy_setopt(m_cURL, CURLOPT_TIMEOUT, 3L);
    }
}

void CTSDBHTTP::Close()
{
    if (m_cURL)
    {
        curl_easy_cleanup(m_cURL);
        m_cURL = NULL;
    }
}

bool CTSDBHTTP::Send (char * pData, int pLen)
{
//    if(m_cURL)
//    {
//        curl_easy_setopt(m_cURL, CURLOPT_POSTFIELDS, pData);
//        CURLcode res = curl_easy_perform(m_cURL) ;
//        if(res != CURLE_OK)
//        {
//            pLog.Print(LOG_ERROR, 0, NULL, 0, "%s()[%d] (%s)(%s)\npData = (%s)", __FUNCTION__, __LINE__, m_cSTR, curl_easy_strerror(res), pData);
//            Close();
//            return false;
//        }   else
//            return true;
//    }   else
//    {
//        pLog.Print(LOG_ERROR, 0, NULL, 0, "%s()[%d] Unable to initialize cURL interface...", __FUNCTION__, __LINE__);
//        return false;
//    }

    if(m_cURL)
    {
        curl_easy_setopt (m_cURL, CURLOPT_POSTFIELDS, pData);
        CURLcode res = curl_easy_perform(m_cURL);
        if(res == CURLE_OK)
        {
            long status = 0;
            res = curl_easy_getinfo(m_cURL, CURLINFO_RESPONSE_CODE, &status);
            if (res != CURLE_OK || status != 204)
            {
                pLog.Print(LOG_ERROR, 0, NULL, 0, "%s()[%d] (%s)(%u)(%s), pData = (%s)", __FUNCTION__, __LINE__, m_cSTR, status, curl_easy_strerror(res), pData);
                if (status == 400 || status == 500)
                {
                    time(&m_ntime);
                    if ((int)difftime(m_ntime, m_stime) >= 300)
                    {
                        SendMail(pData, m_tUrl, status);
                        time(&m_stime);
                    }
                    if (status == 400)
                        return true;
                }
                return false;
            }   else
                return true;
        }   else
        {
            //time(&m_ntime);
            //if ((int)difftime(m_ntime, m_stime) >= 300)
            //{
            //    SendMail(pData, m_tUrl, -1);
            //    time(&m_stime);
            //}
            pLog.Print(LOG_ERROR, 0, NULL, 0, "%s()[%d] (%s)(%s), pData = (%s)", __FUNCTION__, __LINE__, m_cSTR, curl_easy_strerror(res), pData);
            return false;
        }
    }   else
    {
        pLog.Print(LOG_ERROR, 0, NULL, 0, "%s()[%d] Unable to initialize cURL interface...", __FUNCTION__, __LINE__);
        return false;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CTSDBHTTP::SendMail (char * pData, char * pUrl, int status)
{
    try
    {
        char mBody[SMI_BUF] = {0,};
        char Title[MIN_BUF] = {0,};
        snprintf(Title, sizeof(Title), "OpenTSDB Insert Error Mail : 점검요망 !!!");
        snprintf(mBody, sizeof(mBody), "Code : %d\r\n"
                                       "URL  : %s\r\n"
                                       "Data : %s\r\n",
                                       status, pUrl, pData);

        CSmtp mail;

        mail.SetSMTPServer("inmail.gabia.com",2525);
        //mail.SetLogin("ykjo@gabia.com");
        //mail.SetPassword("whdydrb11");  지우면 SMTP 인증없이 통화처리 됨.

        mail.SetSenderName("TMS Monitoring");
        mail.SetSenderMail("tlbagent@gabia.com");
        mail.SetReplyTo("tlbagent@gabia.com");
        mail.SetSubject(Title);

        mail.AddRecipient("kimde@gabia.com");
        mail.AddRecipient("ksk@gabia.com");
        mail.AddRecipient("ykjo@gabia.com");

        mail.SetXPriority(XPRIORITY_NORMAL);
        mail.SetXMailer("The Bat! (v3.02) Professional");

        mail.AddMsgLine(mBody);
        mail.Send();
    }
    catch(ECSmtp e)
    {
        pLog.Print(LOG_ERROR, 0, NULL, 0, "%s()[%d] Error: %s", __FUNCTION__, __LINE__, e.GetErrorText().c_str());
    }
    return;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
