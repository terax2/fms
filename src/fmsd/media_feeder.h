#ifndef __MEDIA_FEEDER_H__
#define __MEDIA_FEEDER_H__

#include "media_sink.h"

class CMediaFeeder {
    public:
        CMediaFeeder(void);
        virtual ~CMediaFeeder(void);

        bool        AddSink(CMediaSink *pSink);
        void        RemoveSink(CMediaSink *pSink);
        void        RemoveAllSinks(void);
        void        StartSinks(void);
        void        StopSinks(void);

    protected:
        void        ForwardFrame(CMediaFrame* pFrame);
        static const u_int16_t MAX_SINKS = _MAX_TRANSMIT_;
        CMediaSink *m_sinks[MAX_SINKS];
        Mutex       m_Lock;
};

#endif
