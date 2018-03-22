#include "media_feeder.h"

CMediaFeeder::CMediaFeeder (void)
{
    for (int i = 0; i < MAX_SINKS; i++) {
        m_sinks[i] = NULL;
    }
}

CMediaFeeder::~CMediaFeeder (void)
{
}

bool CMediaFeeder::AddSink(CMediaSink* pSink)
{
    bool rc = false;
    m_Lock.lock();
    for (int i = 0; i < MAX_SINKS; i++) {
        if (m_sinks[i] == pSink) {
            m_Lock.unlock();
            return true;
        }
    }

    for (int i = 0; i < MAX_SINKS; i++) {
        if (m_sinks[i] == NULL) {
            m_sinks[i] = pSink;
            rc = true;
            break;
        }
    }
    m_Lock.unlock();
    return rc;
}

void CMediaFeeder::StartSinks (void)
{
    m_Lock.unlock();
    for (int i = 0; i < MAX_SINKS; i++) {
        if (m_sinks[i] != NULL) {
            m_sinks[i]->Start();
        }
    }
    m_Lock.unlock();
}

void CMediaFeeder::StopSinks (void)
{
    m_Lock.lock();
    for (int i = 0; i < MAX_SINKS; i++) {
        if (m_sinks[i] != NULL) {
            m_sinks[i]->StopThread();
        }
    }
    m_Lock.unlock();
}
void CMediaFeeder::RemoveSink(CMediaSink* pSink)
{
    m_Lock.lock();
    for (int i = 0; i < MAX_SINKS; i++) {
        if (m_sinks[i] == pSink) {
            int j ;
            for (j = i; j < MAX_SINKS - 1; j++) {
                m_sinks[j] = m_sinks[j+1];
            }
            m_sinks[j] = NULL;
            break;
        }
    }
    m_Lock.unlock();
}

void CMediaFeeder::RemoveAllSinks(void)
{
    m_Lock.lock();
    for (int i = 0; i < MAX_SINKS; i++) {
        if (m_sinks[i] == NULL) {
            break;
        }
        m_sinks[i] = NULL;
    }
    m_Lock.unlock();
}

void CMediaFeeder::ForwardFrame(CMediaFrame* pFrame)
{
    m_Lock.lock();
    for (int i = 0; i < MAX_SINKS; i++) {
        if (m_sinks[i] == NULL) {
            break;
        }
        m_sinks[i]->EnqueueFrame(pFrame);
    }
    m_Lock.unlock();
    if (pFrame->RemoveReference()) delete pFrame;
    return;
}

