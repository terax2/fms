/////////////////////////////////////////////////////////////////////////
//
//  제어하는 클래스..
//
/////////////////////////////////////////////////////////////////////////

#ifndef ___FMS_INTF_H___
#define ___FMS_INTF_H___

#include "camera.h"

class CFMSInterface
{
    public :
        CFMSInterface();
        virtual ~CFMSInterface();

        virtual void   ConnectDevice(uint32_t seqno, ClntInfo sock) = 0;
        virtual void   CommandDevice(ClntInfo sock) = 0;
        //virtual void   LoraMessage  (char * pBuf, int len) = 0;

};

#endif //___FMS_INTF_H___
