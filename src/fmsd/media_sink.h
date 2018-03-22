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

#ifndef __MEDIA_SINK_H__
#define __MEDIA_SINK_H__

#include "timestamp.h"
#include "media_node.h"

class CMediaSink : public CMediaNode
{
    public:
        CMediaSink() : CMediaNode() {
        }

        virtual ~CMediaSink() {
        }

        void EnqueueFrame(CMediaFrame* pFrame) {
            if (pFrame == NULL) {
                return;
            }
            m_Lock.lock();
            pFrame->AddReference()  ;
            m_myMsgQueue.send_message(MSG_SINK_FRAME, pFrame, 0, m_myMsgQueueSemaphore);
            m_Lock.unlock();
        }

        virtual const char* name() {
            return "CMediaSink";
        }

    protected:
        static const uint32_t MSG_SINK       = 4096;
        static const uint32_t MSG_SINK_FRAME = MSG_SINK + 1;
        Mutex       m_Lock;
};

#endif /* __MEDIA_SINK_H__ */

