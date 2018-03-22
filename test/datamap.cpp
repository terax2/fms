#include "datamap.h"

CDataMap::CDataMap()
:   m_datas(NULL), m_data_free(NULL)
{
    m_datas = new map < string, void *, less<string > > ();
}

CDataMap::~CDataMap()
{
    allremove();
    if (m_datas)
    {
        delete m_datas;
        m_datas = NULL;
    }
}

void CDataMap::allremove()
{
    map < string, void *, less<string> >::iterator iter;
    while(!m_datas->empty())
    {
        iter = m_datas->begin();
        if ((*iter).second)
        {
            if (m_data_free)
                (m_data_free)((*iter).second);
            else
                free ((*iter).second);
        }
        m_datas->erase(iter);
    }
}

int  CDataMap::remove ( string SerialNo )
{
    map < string, void *, less<string> >::iterator iter;
    iter = m_datas->find(SerialNo);
    if( iter == m_datas->end() )
    {
        return -1;
    }
    if ((*iter).second)
    {
        if (m_data_free)
            (m_data_free)((*iter).second);
        else
            free ((*iter).second);
    }
    m_datas->erase(iter) ;
    return  0;
}

void CDataMap::insert ( string SerialNo, void * pdata )
{
    m_datas->insert (map<string, void *, less<string> >::value_type(SerialNo, pdata));
}

void * CDataMap::getdata ( string SerialNo )
{
    map < string, void *, less<string> >::iterator iter;
    iter = m_datas->find(SerialNo);
    if ( iter != m_datas->end() )
        return (*iter).second;
    else
        return NULL;
}
