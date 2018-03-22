#ifndef ___DATA_MAP_H___
#define ___DATA_MAP_H___

using namespace std;
#include <string>
#include <string.h>
#include <stdlib.h>
#include <map>

typedef void (*free_f)(void *);

class CDataMap
{
    public :
        CDataMap();
        ~CDataMap();
        u_int32_t SCount   ();
        u_int32_t ICount   ();
        //////////////////////////////////////////////
        void    insert   (string   Key, void * pdata);
        void  * getdata  (string   Key);
        int     remove   (string   Key);
        //////////////////////////////////////////////
        void    insert   (u_int32_t Key, void * pdata);
        void  * getdata  (u_int32_t Key);
        int     remove   (u_int32_t Key);
        //////////////////////////////////////////////
        void    setfree  (free_f m) {m_data_free = m;};
        //////////////////////////////////////////////
        void    removeAll();
        //////////////////////////////////////////////
        map < string  , void *, less<string   > > * getSlist() { return m_sdata; };
        map < u_int32_t, void *, less<u_int32_t > > * getIlist() { return m_idata; };
        //////////////////////////////////////////////

    private:
        void    allremoveInt();
        void    allremoveStr();

    private:
        map < string  , void *, less<string   > > * m_sdata ;
        map < u_int32_t, void *, less<u_int32_t > > * m_idata ;
        free_f  m_data_free;
};


#endif //___DATA_MAP_H___
