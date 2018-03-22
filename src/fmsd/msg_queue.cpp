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
 * msg_queue.cpp - generic class to send/receive messages.  Uses SDL mutexs
 * to protect queues
 */
#include "msg_queue.h"
#include "media_sink.h"

/*****************************************************************************
 * CMsg class methods.  Defines information about a single message
 *****************************************************************************/
CMsg::CMsg (uint32_t value, const void *msg, uint32_t msg_len, uint32_t param)
{
  m_value = value;
  m_msg_len = 0;
  m_has_param = (param != 0);
  m_param = param;
  m_next = NULL;

  if (msg_len) {
    void *temp = malloc(msg_len);
    if (temp) {
      memcpy(temp, msg, msg_len);
      m_msg_len = msg_len;
    }
    m_msg = temp;
  } else {
    m_msg = msg;
  }
}

CMsg::CMsg (uint32_t value, uint32_t param)
{
  m_value = value;
  m_msg_len = 0;
  m_has_param = 1;
  m_param = param;
  m_next = NULL;
}

CMsg::~CMsg (void)
{
  if (m_msg_len) {
    free((void *)m_msg);
    m_msg = NULL;
  }
}

const void *CMsg::get_message (uint32_t &len)
{
  len = m_msg_len;
  return (m_msg);
}

/*****************************************************************************
 * CMsgQueue class methods.  Defines information about a message queue
 *****************************************************************************/
CMsgQueue::CMsgQueue(void)
{
  m_msg_queue_start = NULL;
  m_msg_queue_end = NULL;
  //m_msg_queue_mutex = SDL_CreateMutex();
  m_queue_count = 0;
}

CMsgQueue::~CMsgQueue (void)
{
  //SDL_LockMutex(m_msg_queue_mutex);
  m_Lock.lock();
  CMsg *p = NULL;
  while (m_msg_queue_start != NULL) {
    p = m_msg_queue_start->get_next();
    delete m_msg_queue_start;
    m_msg_queue_start = p;
  }
  m_Lock.unlock();
  //SDL_UnlockMutex(m_msg_queue_mutex);
  //SDL_DestroyMutex(m_msg_queue_mutex);
  //m_msg_queue_mutex = NULL;
}

int CMsgQueue::send_message (uint32_t msgval,
                 const void *msg,
                 uint32_t msg_len,
                 SDL_sem *sem,
                 uint32_t param)
{
    CMsg *newmsg = new CMsg(msgval, msg, msg_len, param);
    if (newmsg == NULL) return (-1);
    return (send_message(newmsg, sem));
}

int CMsgQueue::send_message (uint32_t msgval, uint32_t param, SDL_sem *sem)
{
    CMsg *newmsg = new CMsg(msgval, param);
    if (newmsg == NULL) return -1;
    return (send_message(newmsg, sem));
}

int CMsgQueue::send_message(CMsg *newmsg, SDL_sem *sem)
{
  //SDL_LockMutex(m_msg_queue_mutex);
  m_Lock.lock();
  if (m_msg_queue_start == NULL) {
    m_msg_queue_start = newmsg;
    m_msg_queue_end = newmsg;
  } else {
    if (m_queue_count >= _FRAME_QUEUE_) {
        CMsg *pMsg = NULL;

        while (m_queue_count > 0) {
            pMsg = m_msg_queue_start;
            if (m_queue_count > 1)
                m_msg_queue_start = m_msg_queue_start->get_next();

            if (pMsg->get_value() == 4097u) {
                uint32_t dontcare;
                CMediaFrame *mf = (CMediaFrame*)pMsg->get_message(dontcare);
                if (mf->RemoveReference()) {
                    delete mf;
                }
            }
            delete pMsg ;
            pMsg = NULL ;
            m_queue_count--;
        }
        m_msg_queue_start = newmsg;
        m_msg_queue_end = newmsg;
    } else
    {
        m_msg_queue_end->set_next(newmsg);
        m_msg_queue_end = newmsg;
    }
  }
  m_queue_count ++;
  //SDL_UnlockMutex(m_msg_queue_mutex);
  m_Lock.unlock();
  if (sem != NULL) {
    SDL_SemPost(sem);
  }
  return (0);
}

CMsg *CMsgQueue::get_message (void)
{
  CMsg *ret = NULL;
  if (m_msg_queue_start == NULL)
    return(NULL);

  //SDL_LockMutex(m_msg_queue_mutex);
  m_Lock.lock();
  if (m_msg_queue_start == NULL)
    ret = NULL;
  else {
    ret = m_msg_queue_start;
    m_msg_queue_start = ret->get_next();
    m_queue_count--;
  }
  if (ret) {
    ret->set_next(NULL);
  }
  //SDL_UnlockMutex(m_msg_queue_mutex);
  m_Lock.unlock();
  return (ret);
}

/* end file msg_queue.cpp */
