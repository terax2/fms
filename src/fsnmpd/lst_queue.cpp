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
 * lst_queue.cpp - generic class to send/receive messages.  Uses SDL mutexs
 * to protect queues
 */
#include "lst_queue.h"

/*****************************************************************************
 * CLst class methods.  Defines information about a single message
*****************************************************************************/
CLst::CLst ( uint32_t type, const void * msg, uint32_t len )
:   m_next  (NULL)  ,
    m_sid   (0   )  ,
    m_len   (0   )  ,
    m_type  (type)  ,
    m_msg   (NULL)
{
    if (len) {
        void * temp = malloc(len);
        if (temp) {
            memcpy(temp, msg, len);
            m_len = len;
        }
        m_msg = temp;
    }   else    {
        m_msg = msg;
    }
}

CLst::CLst ( uint32_t type, uint32_t sid )
:   m_next  (NULL)  ,
    m_sid   (sid )  ,
    m_len   (0   )  ,
    m_type  (type)  ,
    m_msg   (NULL)
{
}

CLst::CLst ( uint32_t type, string msg )
:   m_next  (NULL)  ,
    m_sid   (0   )  ,
    m_len   (0   )  ,
    m_type  (type)  ,
    m_msg   (NULL)
{
	m_str.clear();
	m_str = msg ;
}

CLst::~CLst (void)
{
    if (m_msg)  {
        free((void*)m_msg);
        m_msg = NULL;
    }
    m_str.clear()   ;
}

uint32_t CLst::get_sid ()
{
    return (m_sid);
}

const void * CLst::get_message (uint32_t & len)
{
    len = m_len   ;
    return (m_msg);
}

string CLst::get_string ()
{
    return (m_str);
}

/*****************************************************************************
 * CLstQueue class methods.  Defines information about a message queue
 *****************************************************************************/
CLstQueue::CLstQueue(void)
:   m_lst_queue_start (NULL),
    m_lst_queue_end   (NULL),
    m_queue_count     (0)   ,
    m_MaxQueue        (60)  ,
    //m_lst_queue_mutex (NULL),
    m_semaphore       (NULL)
{
    //m_lst_queue_mutex = SDL_CreateMutex()       ;
    m_semaphore       = SDL_CreateSemaphore(0)  ;
}

CLstQueue::~CLstQueue (void)
{
    //SDL_LockMutex(m_lst_queue_mutex);
    m_Lock.lock();
    CLst * p = NULL;
    while (m_lst_queue_start != NULL)
    {
        p = m_lst_queue_start->get_next();
        delete m_lst_queue_start;
        m_lst_queue_start = p;
    }
    //SDL_UnlockMutex(m_lst_queue_mutex);
    m_Lock.unlock();
    //SDL_DestroyMutex(m_lst_queue_mutex);
    //m_lst_queue_mutex = NULL;

    if (m_semaphore)
    {
        SDL_DestroySemaphore(m_semaphore);
        m_semaphore = NULL  ;
    }
}

int CLstQueue::send_message ( uint32_t type, const void * msg, uint32_t len )
{
    CLst * newlst = new CLst( type, msg, len ) ;
    if (newlst == NULL) return (-1) ;
    return (send_message(newlst))   ;
}

int CLstQueue::send_message ( uint32_t type, uint32_t sid )
{
    CLst * newlst = new CLst( type, sid ) ;
    if (newlst == NULL) return (-1) ;
    return (send_message(newlst))   ;
}

int CLstQueue::send_message ( uint32_t type, string msg )
{
    CLst * newlst = new CLst( type, msg ) ;
    if (newlst == NULL) return (-1) ;
    return (send_message(newlst))   ;
}

int CLstQueue::send_message(CLst * newlst)
{
    //SDL_LockMutex(m_lst_queue_mutex);
    m_Lock.lock();
    if (m_lst_queue_start == NULL)
    {
        m_lst_queue_start = newlst;
        m_lst_queue_end   = newlst;
    }   else
    {
        if (m_queue_count >= m_MaxQueue)
        {
            CLst * pLst = NULL  ;
            while (m_queue_count > 0)
            {
                pLst = m_lst_queue_start;
                if (!pLst->get_type())  break;
                if (m_queue_count > 1)
                    m_lst_queue_start = m_lst_queue_start->get_next();

                delete pLst     ;
                pLst = NULL     ;
                m_queue_count-- ;
            }

            if (m_queue_count > 0)
            {
                m_lst_queue_end->set_next(newlst);
                m_lst_queue_end = newlst;
            }   else
            {
                m_lst_queue_start = newlst;
                m_lst_queue_end = newlst;
            }
        }   else
        {
            m_lst_queue_end->set_next(newlst);
            m_lst_queue_end = newlst;
        }
    }
    m_queue_count ++;
    //SDL_UnlockMutex(m_lst_queue_mutex);
    m_Lock.unlock();
    if (m_semaphore != NULL)    {
        SDL_SemPost(m_semaphore);
    }
    return (0)  ;
}

void CLstQueue::set_maxqueue(int MaxQueue)
{
    m_MaxQueue = MaxQueue   ;
}

uint32_t CLstQueue::get_QCount()
{
	uint32_t Count = 0;
	m_Lock.lock();
	//SDL_LockMutex(m_lst_queue_mutex);
    Count = m_queue_count ;
    m_Lock.unlock();
    //SDL_UnlockMutex(m_lst_queue_mutex);
    return Count;
}

CLst * CLstQueue::get_message (void)
{
    CLst * ret = NULL;
    if (m_lst_queue_start == NULL)
        return(NULL);

    //SDL_LockMutex(m_lst_queue_mutex);
    m_Lock.lock();
    if (m_lst_queue_start == NULL)
        ret = NULL;
    else {
        ret = m_lst_queue_start;
        m_lst_queue_start = ret->get_next();
        m_queue_count--;
    }
    if (ret) {
        ret->set_next(NULL);
    }
    //SDL_UnlockMutex(m_lst_queue_mutex);
    m_Lock.unlock();
    return (ret);
}

/* end file lst_queue.cpp */
