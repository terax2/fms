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
 *      Dave Mackie     dmackie@cisco.com
 *      Bill May        wmay@cisco.com
 */

#ifndef __MEDIA_FRAME_H__
#define __MEDIA_FRAME_H__

#include <sys/types.h>
#include <stdlib.h>
#include "media_time.h"

typedef u_int8_t MediaType;
typedef void (*media_free_f)(void *);

class CMediaFrame {
public:
    CMediaFrame(long type, void * pData = NULL, u_int32_t Length = 0)
    {
        m_refcnt     = 1     ;
        m_pData      = pData ;
        m_Length     = Length;
        m_media_free = NULL  ;
        m_Type       = type  ;
    }

    ~CMediaFrame() {
        if (m_refcnt != 0) abort();
        m_Lock.lock();
        if (m_pData)
        {
            if (m_media_free != NULL) {
                (m_media_free)(m_pData);
            }   else {
                free(m_pData);
            }
            m_pData = NULL;
        }
        m_Lock.unlock();
    }

    void SetMediaFreeFunction(media_free_f m) {
        m_media_free = m;
    };

    void AddReference(void) {
        m_Lock.lock();
        m_refcnt++;
        m_Lock.unlock();
    }

    bool RemoveReference(void) {
        u_int16_t ref;
        m_Lock.lock();
        m_refcnt--;
        ref = m_refcnt;
        m_Lock.unlock();
        return ref == 0;
    }

    // predefined types of frames
    // get methods for properties

    void * GetData(void) {
        return m_pData;
    }
    u_int32_t GetLength(void) {
        return m_Length;
    }
    long  GetType(void) {
        return m_Type;
    }

protected:
    Mutex           m_Lock      ;
    u_int16_t       m_refcnt    ;
    void *          m_pData     ;
    u_int32_t       m_Length    ;
    media_free_f    m_media_free;
    long            m_Type      ;
};

#endif /* __MEDIA_FRAME_H__ */

