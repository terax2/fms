/*
 * The contents of this file are subject to the Mozilla Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 *
 * The Original Code is MPEG4IP.
 *
 * The Initial Developer of the Original Code is Cisco Systems Inc.
 * Portions created by Cisco Systems Inc. are
 * Copyright (C) Cisco Systems Inc. 2000, 2001.  All Rights Reserved.
 *
 * Contributor(s):
 *              Bill May        wmay@cisco.com
 */
/*
 * dualbuff.cpp - generic class to send/receive messages.  Uses SDL mutexs
 * to protect queues
 */
#include "dualbuff.h"


/*****************************************************************************
 * CDualbuff class methods.  Defines information about a message queue
 *****************************************************************************/
CDualbuff::CDualbuff(uint8_t pMaxCnt)
:   m_List  (NULL   ),
    m_Active(1      ),
    m_MaxCnt(pMaxCnt),
    m_Next  (0      )
{
    m_Active = m_MaxCnt - 1;
    m_List   = new CDataMap*[m_MaxCnt];
    for (int i = 0; i < m_MaxCnt; i++)
        m_List[i] = new CDataMap();
}

CDualbuff::~CDualbuff (void)
{
    if (m_List)
    {
        for (int i = 0; i < m_MaxCnt; i++)
        {
            if (m_List[i])
            {
                delete m_List[i];
                m_List[i] = NULL;
            }
        }
        delete[] m_List;
        m_List = NULL ;
    }
}

void CDualbuff::setClear()
{
    m_Next = ((m_Active+1) % m_MaxCnt);
    m_List[m_Next]->removeAll();
}

void CDualbuff::setActive()
{
    //m_Active = m_Next;
    m_Mutex.lock();
    m_Active = m_Next;
    m_Mutex.unlock();
}

void CDualbuff::setUpdate(string skey, void * pObject)
{
    m_List[m_Next]->insert(skey,pObject);
}

void CDualbuff::setUpdate(uint32_t skey, void * pObject)
{
    m_List[m_Next]->insert(skey,pObject);
}

void * CDualbuff::getData (string skey)
{
    //return m_List[m_Active]->getdata(skey);
    m_Mutex.lock();
    void * pData = m_List[m_Active]->getdata(skey);
    m_Mutex.unlock();
    return pData;
}

void * CDualbuff::getData (uint32_t nkey)
{
    //return m_List[m_Active]->getdata(nkey);
    m_Mutex.lock();
    void * pData = m_List[m_Active]->getdata(nkey);
    m_Mutex.unlock();
    return pData;
}

uint32_t CDualbuff::getSCount()
{
    return m_List[m_Active]->SCount();
}

uint32_t CDualbuff::getICount()
{
    return m_List[m_Active]->ICount();
}

void * CDualbuff::nextData (string skey)
{
    return m_List[m_Next]->getdata(skey);
}

void * CDualbuff::nextData (uint32_t nkey)
{
    return m_List[m_Next]->getdata(nkey);
}

void CDualbuff::setFree (free_f m)
{
    for (int i = 0; i < m_MaxCnt; i++)
        m_List[i]->setfree(m);
}

/* end file dualbuff.cpp */
