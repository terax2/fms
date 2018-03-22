#include <sys/time.h> 

#include <sys/types.h> 
#include <sys/socket.h> 
#include <sys/select.h> 
#include <sys/ioctl.h> 
#include <netdb.h> 
#include <net/if.h> 
#include <netinet/tcp.h> 
#include <arpa/inet.h>
#include <linux/sockios.h>
#include <sys/stat.h> 
#include <sys/wait.h> 
#include <sys/epoll.h> 
#include <pthread.h> 
#include <stdio.h> 
#include <string.h> 
#include <fcntl.h> 
#include <signal.h> 
#include <errno.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <math.h>
#include <SDL.h>
#include <SDL_thread.h>
#include <map>
#include <list>
#include <string>
//#include <asm/socket.h>

#include <netinet/in.h>
#include <netinet/ip.h>
#include <linux/netfilter.h>

using namespace std ;
 
#define MAX_CLIENT  101  
#define PORT        3355  
#define MAX_LINE    1024 
#define DEBUG  


class CMutex
{
    public:
	    CMutex();
	    virtual ~CMutex();

        void        lock();
        void        unlock();

    private:
	    SDL_mutex * m_mutex;
} ;

class CThread
{
    public:
	    CThread( int (*fn)(void *), void * data );
	    virtual ~CThread();

    private:
	    SDL_Thread *m_thread;
} ;

class CTest
{
    public :
	    CTest();
	    ~CTest();
};

///////////////////////////////////////////
#define MAX_IMG        (1024 * 20)
#define HAF_BUF         1024
#define MID_BUF         256
#define MIN_BUF         128
#define FUL_FLE         80
#define SML_BUF         64
#define TMP_BUF         32
#define HEX_BUF         16
                        
#define MSG_MIN         64
#define MSG_URI         MIN_BUF
#define PAY_LOAD        1388
                        
#define I_FRAME         0x7C
#define SI_FRAME        0x65
#define P_FRAME         0x5C
#define SP_FRAME        0x41
#define D_FRAME         0x06
#define D_LENGTH        23
                        
#define SZ_SER          16
#define USERID          16
#define PASSWD          32
#define OTPKEY          8
#define SESKEY          10
#define _DATE_          14

#define REALM           16
#define NONCE           46
#define _URI_           80
#define RESPS           32
#define _SDP_           540
#define SSINF           8
#define MAX_IDX         5000

typedef struct  _Index_ 
{
    uint8_t     time        [_DATE_];
    loff_t      fpos        ;
    uint16_t    size        ;
}   _INDEX_ ;

#define IDX_SZ  sizeof(_INDEX_)

typedef struct _Idxhd_  
{
	char        Prev        [FUL_FLE];
	char        Next        [FUL_FLE];
}  	_IHD_   ;

#define IHD_SZ  sizeof(_IHD_)

typedef struct _fIdx_   
{
	_IHD_       HEAD        ;
	_INDEX_     IDX         [MAX_IDX];
}  	FILE_IDX ;


class CIDXFile  
{
	public:
		CIDXFile();
		virtual ~CIDXFile();
		
		bool            openFile        (char * szFile  );
		
	    loff_t          findposition    (char * szTime  );
	    loff_t          findposition    (loff_t offset  );
	    loff_t          seekToAbsolute  (int    Scale   );
	    loff_t          seekToRelative  (int    Scale   );
	    loff_t          getCurtPos      ();
	    
	    bool            getNextFile     (bool   DIR     );
        char        *   getRawFile      ()  { return m_rawfile; };

	protected:
		FILE_IDX        m_IDX           ;
		int             m_Count         ;
		int             m_IPos          ;
		char            m_rawfile[FUL_FLE];
		char            m_idxfile[FUL_FLE];

    private:
        void            setRawFile      (char * indexName);
        bool            AddedFile       (char * szFile   );
        bool            clear           ();        
        
    private:
        int32_t         fpidx           ;
        bool            isLast          ;
        loff_t          idxPos          ;
                
};
