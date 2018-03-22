#ifndef ___DATA_MAP_H___
#define ___DATA_MAP_H___

using namespace std;
#include <string>
#include <map>

typedef void (*free_f)(void *);

class CDataMap
{
    public :
        CDataMap();
        ~CDataMap();

        // key type is string
        void    insert   (string SerialNo, void * pdata);
        void  * getdata  (string SerialNo);
        int     remove   (string SerialNo);

        void    setfree  (free_f m) { m_data_free = m; };
        int     size     () { return m_datas->size(); };

    private:
        void    allremove();

        // key type is string
        map < string, void * , less<string > > * m_datas ;
    private:
        free_f  m_data_free;
};

#endif //___DATA_MAP_H___
