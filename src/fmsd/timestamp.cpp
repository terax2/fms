/*
 *****************************************************************************

    Copyright (c) 2011, Gabia, Inc.
    All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are
    met:

        Redistributions of source code must retain the above copyright notice,
    this list of conditions and the following disclaimer.  Redistributions
    in binary form must reproduce the above copyright notice, this list of
    conditions and the following disclaimer in the documentation and/or
    other materials provided with the distribution.

    Neither the name of Blackwave, Inc. nor the names of its
    contributors may be used to endorse or promote products
    derived from this software without specific prior written
    permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND
    CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
    INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
    MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
    DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS
    BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
    EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
    TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
    DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
    ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
    TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
    THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
    SUCH DAMAGE.

 ******************************************************************************
*/

#include "timestamp.h"

CTimeStamp::CTimeStamp()
//:   m_Lock (NULL)
{
    struct timeval tv       ;
    gettimeofday(&tv, NULL) ;
    m_haveStartTimestamp = false            ;
    m_rtpTimestampOffset = tv.tv_sec        ;
    m_timeScale          = 90000            ;
    //m_Lock               = SDL_CreateMutex();
}

CTimeStamp::~CTimeStamp()
{
    //if (m_Lock)
    //{
    //    SDL_DestroyMutex(m_Lock);
    //    m_Lock = NULL;
    //}
}

uint32_t CTimeStamp::TimestampToRtp ( Timestamp t )
{
    if ( m_haveStartTimestamp )
    {
        return (uint32_t)(((t - m_startTimestamp) * m_timeScale) / TimestampTicks) + m_rtpTimestampOffset;
    }

    m_startTimestamp     = t    ;
    m_haveStartTimestamp = true ;
    return m_rtpTimestampOffset ;
}


uint32_t CTimeStamp::GetTimeStamp()
{
    return  TimestampToRtp (GetTimestamp());
}

uint64_t CTimeStamp::GetSessionKey()
{
    uint64_t SessionKey = 0     ;
    //SDL_LockMutex  (m_Lock)     ;
    m_Lock.lock()               ;
    SessionKey = GetTimestamp() ;
    //SDL_UnlockMutex(m_Lock)     ;
    m_Lock.unlock()             ;
    return  SessionKey          ;
}

