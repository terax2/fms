#ifndef __LST_QUEUE_H__
#define __LST_QUEUE_H__

#include <stdint.h>
#include "camera.h"

class CLst {
    public:
        CLst( uint32_t type, const void * msg, uint32_t len);
        CLst( uint32_t type, uint32_t sid ) ;
        CLst( uint32_t type, string msg )   ;
        ~CLst( void )           ;

        const void *get_message (uint32_t & len);
        string      get_string  ();
        uint32_t    get_sid     ();
        CLst       *get_next    (void)      { return m_next; };
        void        set_next    (CLst*next) { m_next = next; };
        uint32_t    get_type    (void)      { return m_type; };

    private:
        CLst       *m_next      ;
        uint32_t    m_sid       ;
        uint32_t    m_len       ;
        uint32_t    m_type      ;
        const void *m_msg       ;
        string      m_str       ;
};

class CLstQueue {
    public:
        CLstQueue(void) ;
        ~CLstQueue(void);
        int         send_message( uint32_t type, const void * msg, uint32_t len);
        int         send_message( uint32_t type, uint32_t sid );
        int         send_message( uint32_t type, string msg);
        void        set_maxqueue( int MaxQueue );
        CLst      * get_message ( void );
        SDL_sem   * get_semaphore() { return m_semaphore; };
        uint32_t    get_QCount();

    private:
        int         send_message(CLst *lst);
        CLst      * m_lst_queue_start   ;
        CLst      * m_lst_queue_end     ;
        int         m_queue_count       ;
        int         m_MaxQueue          ;
        //SDL_mutex * m_lst_queue_mutex   ;
        Mutex       m_Lock              ;
        SDL_sem   * m_semaphore         ;
};

#endif  //  __LST_QUEUE_H__
