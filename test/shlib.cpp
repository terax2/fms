#include "shlib.h"

/*
    Mutex Class
*/

CMutex::CMutex()
{
    m_mutex = SDL_CreateMutex();
}

CMutex::~CMutex()
{
	if ( m_mutex != NULL )
	{
		SDL_DestroyMutex ( m_mutex ) ;
		m_mutex = NULL ;
	}
}

void CMutex::lock()
{
    SDL_LockMutex(m_mutex);
}


void CMutex::unlock()
{
    SDL_UnlockMutex(m_mutex);
}



/*
    Thread Class
*/

CThread::CThread( int (*fn)(void *), void * data )
{
	m_thread = NULL    ;
	m_thread = SDL_CreateThread ( fn, data ) ;
}

CThread::~CThread()
{
	if ( m_thread )
	{
		SDL_WaitThread ( m_thread, NULL ) ;
		m_thread = NULL ;
	}
}









//////////////////////////////////////////////////////////////////////////////
CIDXFile::CIDXFile()
:   m_Count(0), isLast(false), fpidx(-1), idxPos(0)
{
    memset(&m_IDX, 0, sizeof(FILE_IDX));
//    if (!m_IDX.IDX)  {
//        m_IDX.IDX = new _INDEX_[MAX_IDX];
//        memset(m_IDX.IDX, 0, IDX_SZ * MAX_IDX);
//    }
}

CIDXFile::~CIDXFile()
{
//    if (m_IDX.IDX)  {
//        delete[] m_IDX.IDX ;
//        m_IDX.IDX = NULL   ;
//    }
    clear();
}

bool CIDXFile::clear()
{
    memset(&m_IDX, 0, sizeof(FILE_IDX));
    if (m_IDX.IDX)  {
        memset(m_IDX.IDX, 0, IDX_SZ * m_Count);
        m_Count = 0 ;
    }    
    if (fpidx >= 0) {
        close(fpidx);
        fpidx = -1  ;     
    }
}

bool CIDXFile::openFile(char * szFile)
{
	struct stat sbuf    ;
    clear();

	if ((fpidx = open(szFile, O_LARGEFILE, O_RDONLY)) < 0)    {
	    fpidx = -1  ;
		return false;
	}

	if (fstat(fpidx, &sbuf)< 0)
	{
		fprintf(stderr, "%s FILE STAT ERROR\n", szFile);
		close(fpidx);
		fpidx = -1  ;
		return false;
	}

    int fsize = sbuf.st_size ;
    idxPos    = sbuf.st_size ;
    
	if ( read(fpidx, (char*)&m_IDX.HEAD, IHD_SZ) != IHD_SZ )
	{
		fprintf(stderr, "%s index header read error\n", szFile);
		close(fpidx);
		fpidx = -1  ;
		return false;
	}
	fprintf(stderr, "origSize = [%d]\n", fsize);
	
	fsize    -= IHD_SZ ;
	isLast    = (m_IDX.HEAD.Next[0] == '\0' ? true : false);

    m_Count   = (int)(fsize/IDX_SZ);
    
    fprintf(stderr, "prev = [%s]\n", m_IDX.HEAD.Prev);
    fprintf(stderr, "next = [%s]\n", m_IDX.HEAD.Next);
    
    fprintf(stderr, "fileSize = [%d][%d]\n", fsize, IDX_SZ);
    fprintf(stderr, "idxCount = [%d]\n", m_Count);

	
	int rsize = read(fpidx, (char*)&m_IDX.IDX, fsize);
	if (rsize != fsize) {
		fprintf(stderr, "%s index data read error [%s]\n", szFile, strerror(errno));
		close(fpidx);
		fpidx = -1  ;
		return false;
    }

    if (!isLast)    {
        idxPos = 0  ;
    	close(fpidx);
    	fpidx = -1  ;
    }
    
	snprintf(m_idxfile, sizeof(m_idxfile), "%s", szFile);
	return true ;
}

bool CIDXFile::AddedFile(char* szFile)
{
	if (fpidx < 0) return false;
    int olPos = idxPos  ;
    int olCnt = m_Count ;
    
    lseek (fpidx, 0, SEEK_SET);
	if ( read(fpidx, (char*)&m_IDX.HEAD, IHD_SZ) != IHD_SZ ) {
		fprintf(stderr, "%s index header read error", szFile);
		close(fpidx);
		fpidx = -1  ;
		return false;
	}
	isLast = (m_IDX.HEAD.Next[0] == '\0' ? true : false);

    int fsize = lseek (fpidx, 0, SEEK_END);
    int Asize = fsize - idxPos;
    idxPos    = fsize   ;

	int Acnt  = (int)(Asize/IDX_SZ) ;
	if (Acnt < 1)   {
		close(fpidx);
		fpidx = -1  ;
	    return false;
	}
	
	m_Count  += Acnt                ;
	lseek (fpidx, olPos, SEEK_SET)  ;
	int rsize = read(fpidx, &m_IDX.IDX[olCnt], Asize);
	if (rsize != Asize) {
		fprintf(stderr, "%s index data Added read error", szFile);
		close(fpidx);
		fpidx = -1  ;
		return false;
    }

    if (!isLast)    {
        idxPos = 0  ;
    	close(fpidx);
    	fpidx = -1  ;
    }
    return true ;
}

loff_t CIDXFile::findposition(char * pTime)
{
  	int16_t high = m_Count - 1, low = -1, probe ;
    char tmp[_DATE_+1];
    memset(tmp, 0, sizeof(tmp));
  	while ( high - low > 1 )	{
  		probe = (high + low) / 2 ;
  		memcpy(tmp, m_IDX.IDX[probe].time, _DATE_);
  		fprintf(stderr, "time = [%s]\n", tmp);
  		if ( memcmp(m_IDX.IDX[probe].time, pTime, _DATE_) >= 0 )
  			high = probe	;
  		else
  			low	 = probe	;
  	}
	//kde : 인접한 상위값을 찾는로직//////
	m_IPos = high   ;
  	return	m_IDX.IDX[m_IPos].fpos ;
  	//////////////////////////////////////
  	/*
	//kde : 정확한 키 값을 찾는 로직//////
    if (low == -1 || IDX[low].pos != CURTPOS)
    	return -1 ;
   	else
    	return low;
	//////////////////////////////////////  	
	*/
}

loff_t CIDXFile::findposition(loff_t offset)
{
  	int16_t high = m_Count - 1, low = -1, probe ;

  	while ( high - low > 1 )	{
  		probe = (high + low) / 2 ;
  		if ( m_IDX.IDX[probe].fpos >= offset )
  			high = probe	;
  		else
  			low	 = probe	;
  	}
	//kde : 인접한 상위값을 찾는로직//////
	m_IPos = high   ;
  	return	m_IDX.IDX[m_IPos].fpos ;
  	//////////////////////////////////////
  	/*
	//kde : 정확한 키 값을 찾는 로직//////
    if (low == -1 || IDX[low].pos != CURTPOS)
    	return -1 ;
   	else
    	return low;
	//////////////////////////////////////  	
	*/
}

loff_t CIDXFile::seekToAbsolute (int Scale)
{
    if (m_Count <= (m_IPos + Scale) || 0 > (m_IPos + Scale)) {
        return -1   ;
    }
    m_IPos += Scale ;
    return  m_IDX.IDX[m_IPos].fpos;
}

loff_t CIDXFile::seekToRelative (int Scale)
{
    if (m_Count <= (m_IPos + Scale) || 0 > (m_IPos + Scale)) {
        if ( Scale > 0 && isLast)   {
            if (!AddedFile (m_idxfile))
                return -1;
        }   else    {
            return -1   ;
        }
    }
    
    loff_t bfPos = m_IDX.IDX[m_IPos].fpos;
    m_IPos += Scale ;
    loff_t afPos = m_IDX.IDX[m_IPos].fpos;
    
    return (afPos - bfPos) ;
}

loff_t CIDXFile::getCurtPos ()
{
    return  m_IDX.IDX[m_IPos].fpos ;
}

void CIDXFile::setRawFile (char * indexName)
{
    int i;
    memset(m_rawfile, 0, sizeof(m_rawfile));
    for (i = 0; i < strlen(indexName); i++)
    {
        if (indexName[i] == '.') break;
        m_rawfile[i] = indexName[i] ;
    }
    memcpy(&m_rawfile[i], ".raw", 4);
    fprintf(stderr, "%s() Raw filename = [%s]", m_rawfile);
}

bool CIDXFile::getNextFile (bool DIR)
{
    memset(m_idxfile, 0, sizeof(m_idxfile));
    if ( DIR )  {
        if (m_IDX.HEAD.Next[0] == '\0') return false;
        snprintf(m_idxfile, sizeof(m_idxfile), "%s", m_IDX.HEAD.Next);
    }   else    {
        if (m_IDX.HEAD.Prev[0] == '\0') return false;
        snprintf(m_idxfile, sizeof(m_idxfile), "%s", m_IDX.HEAD.Prev);
    }
    
    setRawFile(m_idxfile)   ;
    openFile  (m_idxfile)   ;
    m_IPos = (DIR ? 0: m_Count-1);
    return true ;
}
