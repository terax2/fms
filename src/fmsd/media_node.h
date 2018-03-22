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
 * Copyright (C) Cisco Systems Inc. 2000-2002.  All Rights Reserved.
 *
 * Contributor(s):
 *      Dave Mackie     dmackie@cisco.com
 *      Bill May        wmay@cisco.com
 */

#ifndef __MEDIA_NODE_H__
#define __MEDIA_NODE_H__

#include "msg_queue.h"
#include "media_frame.h"

class CMediaNode
{
    public:
        CMediaNode() {
            m_myThread = NULL;
            m_myMsgQueueSemaphore = NULL;
        }

        static int StartThreadCallback(void* data) {
            return ((CMediaNode*)data)->ThreadMain();
        }

        void StartThread() {
            if (m_myThread == NULL) {
                m_myMsgQueueSemaphore = SDL_CreateSemaphore(0);
                m_myThread = SDL_CreateThread(StartThreadCallback, this);
            }
        }

        void StopThread() {
            if (m_myThread) {
                CMsg * pMsg;
                //debug_message("sending stop thread");

                //if (!Abort)
                m_myMsgQueue.send_message(MSG_NODE_STOP_THREAD, m_myMsgQueueSemaphore);
                //debug_message("waiting thread");
                SDL_WaitThread(m_myThread, NULL);
                //debug_message("done with thread");

                m_myThread = NULL;
                SDL_DestroySemaphore(m_myMsgQueueSemaphore);
                m_myMsgQueueSemaphore = NULL;

                while ((pMsg = m_myMsgQueue.get_message()) != NULL) {
                    delete pMsg;
                }
            }
        }

        void Start(void) {
            m_myMsgQueue.send_message(MSG_NODE_START, m_myMsgQueueSemaphore);
        }

        virtual void Stop(void) {
            m_myMsgQueue.send_message(MSG_NODE_STOP, m_myMsgQueueSemaphore);
        }

        virtual const char* name() {
            return "CMediaNode";
        }

        virtual ~CMediaNode() {
            Stop();
            StopThread();
        }

    protected:
        static const uint32_t MSG_NODE              = 1024;
        static const uint32_t MSG_NODE_START        = MSG_NODE + 1;
        static const uint32_t MSG_NODE_STOP         = MSG_NODE + 2;
        static const uint32_t MSG_NODE_STOP_THREAD  = MSG_NODE + 3;
        virtual int  ThreadMain(void) = 0;

     public:
        static const uint32_t MSG_NODE_LAST         = MSG_NODE + 4;


    protected:
        SDL_Thread *        m_myThread  ;
        CMsgQueue           m_myMsgQueue;
        SDL_sem    *        m_myMsgQueueSemaphore;
};

#endif /* __MEDIA_NODE_H__ */

