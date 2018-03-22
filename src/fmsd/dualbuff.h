#ifndef __DUAL_BUFF_H__
#define __DUAL_BUFF_H__

#include <stdint.h>
#include "camera.h"

class CDualbuff {
    public:
        CDualbuff(uint8_t pMaxCnt = 2);
        ~CDualbuff(void);

        void        setClear    ();
        void        setActive   ();
        void        setUpdate   (string   skey, void * pObject);
        void        setUpdate   (uint32_t skey, void * pObject);
        uint32_t    getSCount   ();
        uint32_t    getICount   ();
        void     *  getData     (string   skey);
        void     *  getData     (uint32_t nkey);
        void     *  nextData    (string   skey);
        void     *  nextData    (uint32_t nkey);
        
        void        setFree     (free_f   m   );
        uint8_t     getActive   () { return m_Active; };

    private:
        CDataMap ** m_List      ;
        uint8_t     m_Active    ;
        uint8_t     m_MaxCnt    ;
        uint8_t     m_Next      ;
        Mutex       m_Mutex     ;
};

#endif  //  __DUAL_BUFF_H__
