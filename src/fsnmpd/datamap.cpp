#include "datamap.h"

////////////////////////////////////////////////////////////////////////////////////////////
CDataMap::CDataMap()
:   m_sdata    (NULL),
    m_idata    (NULL),
    m_data_free(NULL)
{
    if (!m_sdata)
    {
        m_sdata = new map < string  , void *, less<string> > ();
        m_sdata->clear();
    }
    if (!m_idata)
    {
        m_idata = new map < u_int32_t, void *, less<u_int32_t> > ();
        m_idata->clear();
    }
}

CDataMap::~CDataMap()
{
    allremoveStr();
    if (m_sdata)
    {
        delete m_sdata;
        m_sdata = NULL;
    }
    allremoveInt();
    if (m_idata)
    {
        delete m_idata;
        m_idata = NULL;
    }
}

void CDataMap::allremoveStr()
{
    map < string, void *, less<string> >::iterator iter;
    while(!m_sdata->empty())
    {
        iter = m_sdata->begin();
        if ((*iter).second)
        {
            if (m_data_free)
                (m_data_free)((*iter).second);
            else
                free ((*iter).second);
        }
        m_sdata->erase(iter);
    }
    m_sdata->clear();
}

void CDataMap::allremoveInt()
{
    map < u_int32_t, void *, less<u_int32_t> >::iterator iter;
    while(!m_idata->empty())
    {
        iter = m_idata->begin();
        if ((*iter).second)
        {
            if (m_data_free)
                (m_data_free)((*iter).second);
            else
                free ((*iter).second);
        }
        m_idata->erase(iter);
    }
    m_idata->clear();
}

void CDataMap::removeAll ()
{
    allremoveStr();
    allremoveInt();
}

int  CDataMap::remove ( string Key )
{
    map < string, void *, less<string> >::iterator iter;
    iter = m_sdata->find(Key);
    if( iter == m_sdata->end() )
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
    m_sdata->erase(iter) ;
    return  0;
}

void CDataMap::insert ( string Key, void * pdata )
{
    if (!m_sdata)
    {
        m_sdata = new map < string  , void *, less<string> > ();
        m_sdata->clear();
    }
    m_sdata->insert (map<string, void *, less<string> >::value_type(Key, pdata));
}

u_int32_t CDataMap::SCount()
{
    return (u_int32_t)m_sdata->size();
}

u_int32_t CDataMap::ICount()
{
    return (u_int32_t)m_idata->size();
}

void * CDataMap::getdata ( string Key )
{
    map < string, void *, less<string> >::iterator iter;
    iter = m_sdata->find(Key);
    if ( iter != m_sdata->end() )
        return (*iter).second;
    else
        return NULL;
}

int  CDataMap::remove ( u_int32_t Key )
{
    map < u_int32_t, void *, less<u_int32_t> >::iterator iter;
    iter = m_idata->find(Key);
    if( iter == m_idata->end() )
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
    m_idata->erase(iter) ;
    return  0;
}

void CDataMap::insert ( u_int32_t Key, void * pdata )
{
    if (!m_idata)
    {
        m_idata = new map < u_int32_t, void *, less<u_int32_t> > ();
        m_idata->clear();
    }
    m_idata->insert (map<u_int32_t, void *, less<u_int32_t> >::value_type(Key, pdata));
}

void * CDataMap::getdata ( u_int32_t Key )
{
    map < u_int32_t, void *, less<u_int32_t> >::iterator iter;
    iter = m_idata->find(Key);
    if ( iter != m_idata->end() )
        return (*iter).second;
    else
        return NULL;
}
///////////////////////////////////////////////////////////////////////////////////////////////////
