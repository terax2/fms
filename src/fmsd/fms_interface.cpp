#include "fms_interface.h"
extern CServerLog   pLog ;

CFMSInterface::CFMSInterface()
{
    pLog.Print(LOG_WARNING, 0, NULL, 0, "%s() Starting ...", __FUNCTION__) ;
}

CFMSInterface::~CFMSInterface()
{
    pLog.Print(LOG_WARNING, 0, NULL, 0, "%s() Destroy ...", __FUNCTION__);
}



