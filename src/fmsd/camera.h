#ifndef __CAMERA_H__
#define __CAMERA_H__

#include <net/if.h>
#include <netdb.h>
#include <math.h>
#include <assert.h>
#include <SDL.h>
#include <SDL_thread.h>
//#include <ext/hash_map>

#include <iconv.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/types.h>
#include <sys/time.h>
//#include <pcrecpp.h>
//extern "C" {
//    #include<curl/curl.h>
//}

#include "Epoll.h"
#include "mutex.h"
#include "groupsock.h"
#include "lst_queue.h"
#include "ServerLog.h"
#include "ParseConfig.h"
#include "Utility.h"
#include "Network.h"
//#include "media_time.h"
#include "datamap.h"

//#include <net-snmp/net-snmp-config.h>
//#include <net-snmp/net-snmp-includes.h>

using namespace std;
//using namespace __gnu_cxx;
#define _FMS_DAEMON_V_ "FMSd v1.1"

////// queue type ///////
#define _MSINK_         11
#define _EMOFF_         999


///////////////////////// SDL_Delay(1) ////////////////////////////////////
#define _TIME2WAIT_     980
#define _SLEEP_WAIT_    250                                 // millisecondes
#define _TICK_WAIT_    (uint64_t)(_TIME2WAIT_ * 1.0)        // 1.0 seconds
///////// *********************************************************** /////
#define _DELAY_WAIT_    100                                 // millisecondes
#define _TIME2DELAY_    800
///////////////////////////////////////////////////////////////////////////
#define _SLEEP_TIME_    5                                   // millisecondes
#define _TIME2SLEEP_    775
#define _SEND_COUNT_   (uint64_t)(_TIME2SLEEP_ * 2.0)       // 2 seconds
#define _RECV_TIME_    (uint64_t)(_TIME2SLEEP_ * 8.0)       // 8 seconds
#define _LORA_TIME_    (uint64_t)(_TIME2SLEEP_ * 4.0)       // 4 seconds
#define _NJS_SND_CNT_  (uint64_t)(_TIME2SLEEP_ * 2.0)       // 2 seconds
#define _MICROSLEEP_    999000

#define _UDP_TIMEOUT_   5                                   // 5 seconds
///////////////////////////////////////////////////////////////////////////


#define _FRAME_QUEUE_  (1024 * 5)
#define _RECV_QUEUE_   (1024 * 2)
#define _SQL_SNDQUE_   (1024 * 1024)
#define _NJS_SNDQUE_   (10)
#define _EVENT_QUEUE_  (1024 * 256)
#define _FIRMWARE_SZ_  (1024 * 100)

#define _LORA_BUFF_    (1024 * 10)

#define SMI_BUF         1200
#define BIG_BUF         512
#define MDL_BUF         320
#define MID_BUF         256
#define MIN_BUF         128
#define FUL_FLE         81
#define SML_BUF         64
#define TMP_BUF         32
#define HEX_BUF         16
#define LVL_BUF         8

//#define randomize()     srand((unsigned)time(NULL)+(unsigned)getpid())
//#define random(num)     (rand()%(num))

#define _MAX_TRANSMIT_      1

////////////////////////////////
#define _DELIMETER_         0x0A

////////////////////////////////
#define _CLOSE_             0
#define _ISOK_              100
#define _NONE_              101
#define _SUCCESS_           110
#define _FAIL_              200

#define _KEEP_ALIVE_        20
#define _DEV_CUR_CLEAR_     10
#define _EVENT_OCCURED_     1
#define _EVENT_CLEARED_     2

#define _NET_STOP_START_    10010
#define _NET_STOP_ING_      10020

#define BAT_TEMP_CNT        32
#define UPS_3DATA_CNT       6

#define EVT_CHECK_CNT       14
#define BAT_CHECK_CNT       11
#define UPS_CHECK_CNT       8


#define SZ_U64              sizeof(uint64_t)
#define SZ_U32              sizeof(uint32_t)

///////////////////////////////////////
#define _PROTOCOL_SMP_      0xA1
#define _PROTOCOL_SNMP_     0xB1
#define _PROTOCOL_MODBUS_   0xC1
#define _PROTOCOL_BYPASS_   0xD1

/////////// for SysUp defined //////////
#define _SMP_PKT_SOP_       0x02
#define _SMP_PKT_EOP_       0x03

#define CALL_OID_NUMBER     7
#define CALL_OID_NUMBER2    13

#define _DEVICE_TYPE_       1
#define _SNMP_TYPE_         2
#define _DUAL_TYPE_         3

#define _UPS_MAX_DEV_       1024


#pragma pack(push, 1)

////* EVENT Check ITEM  *////
typedef enum {
    // THR_DEF_CONF
    UPS_IN_VOL_         = 0,
    UPS_IN_FREQ_        = 1,
    UPS_OUT_VOL_        = 2,
    UPS_OUT_CUR_        = 3,
    UPS_BAT_VOL_        = 4,
    UPS_BAT_TMP_        = 5,
    ////////////////////////
    BAT_CHG_VOL_        = 6,
    BAT_CHG_CUR_        = 7,
    BAT_TEMP_           = 8,
    ////////////////////////
    // THR_DEV_DEF_CONF
    BAT_LMT_SMOKE_      = 9,
    BAT_LMT_HEAT_       = 10,
    ////////////////////////
    BAT_CHG_VOL_CUTOFF  = 11,
    BAT_CHG_CUR_CUTOFF  = 12,
    BAT_TEMP_CUTOFF     = 13,

    /// UPS Status UsedYN ///
    E_UPS_UTIL_FAIL     = 14,
    E_UPS_BAT_LOW       = 15,
    E_UPS_BYPASS_ON     = 16,
    E_UPS_UPS_FAIL      = 17,
    E_UPS_STANDBY_TYPE  = 18,
    E_UPS_TESTING       = 19,
    E_UPS_SHUTDOWN      = 20,
    E_UPS_BEEP_ON       = 21
    /////////////////////////
}   EVT_CODE;

typedef enum {
    EVT_HEAT            = 0,
    EVT_SMOKE           = 1,
    EVT_POWER           = 2,
    EVT_NETWORK         = 3,
    EVT_GATEWAY         = 4,
    EVT_DEVICE          = 5,
    EVT_MODEM           = 6,
    ////////////////////////
    EVT_BAT_VOL         = 7,
    EVT_BAT_CUR         = 8,
    EVT_BAT_TEMP        = 9,
    ////////////////////////
    EVT_BAT_FIRE        = 10
}   BAT_CODE;

typedef enum {
    UPS_UTIL_FAIL       = 0,
    UPS_BAT_LOW         = 1,
    UPS_BYPASS_ON       = 2,
    UPS_UPS_FAIL        = 3,
    UPS_STANDBY_TYPE    = 4,
    UPS_TESTING         = 5,
    UPS_SHUTDOWN        = 6,
    UPS_BEEP_ON         = 7
}   UPS_CODE;
////////////////////////////////

////* NJS Function Code (Command) *////
typedef enum {
    NJS_FCODE_SET_LIMIT     = 0x03,
    NJS_FCODE_GET_DATA      = 0x04,
    NJS_FCODE_DEV_ACTIVE    = 0x05,
    NJS_FCODE_CHG_ALLCFG    = 0x06,
    ///////////////////////////////
    NJS_FCODE_UPS_RESET     = 0x10,
    NJS_FCODE_UPS_INVERT    = 0x11,
    NJS_FCODE_UPS_BUZZER    = 0x12,
    NJS_FCODE_UPS_WAKEUP    = 0x13,
    NJS_FCODE_SET_HEAT_RESET= 0x18,
    NJS_FCODE_SET_SWITCH    = 0x21,
    NJS_FCODE_SET_BAT_BUZ   = 0x22,
    ///////////////////////////////
    NJS_FCODE_GET_NETWORK   = 0x31,
    NJS_FCODE_SET_NETWORK   = 0x32,
    NJS_FCODE_GET_CONFIG    = 0x33,
    NJS_FCODE_SET_CONFIG    = 0x34,
    NJS_FCODE_AFW_UPDATE    = 0x35
}   NJS_FCODE;

////* Dev Function Code (Command) *////
typedef enum {
    DEV_FCODE_KEEP_ALIVE    = 0x01,
    DEV_FCODE_GET_DEV_INFO  = 0x02,
    ///////////////////////////////
    DEV_FCODE_SET_LIMIT     = 0x03,
    DEV_FCODE_GET_DATA      = 0x04,
    ///////////////////////////////
    DEV_FCODE_UPS_RESET     = 0x10,
    DEV_FCODE_UPS_INVERT    = 0x11,
    DEV_FCODE_UPS_BUZZER    = 0x12,
    DEV_FCODE_UPS_WAKEUP    = 0x13,
    DEV_FCODE_SET_HEAT_RESET= 0x18,
    DEV_FCODE_SET_SWITCH    = 0x21,
    DEV_FCODE_SET_BAT_BUZ   = 0x22,

    DEV_CONNECT_UPDATE      = 0xFF
}   DEV_FCODE;


////* UDP Function Code (Command) *////
typedef enum {
    UDP_FCODE_GET_NETWORK   = 0x05,
    UDP_FCODE_SET_NETWORK   = 0x06,
    UDP_FCODE_GET_CONFIG    = 0x07,
    UDP_FCODE_SET_CONFIG    = 0x08,

    UDP_FCODE_FW_UPDATE     = 0x11,
    UDP_FCODE_FW_PACKET     = 0x12,
    UDP_FCODE_FW_VERIFY     = 0x13,
    UDP_FCODE_FW_FINISH     = 0x14,

    UDP_FCODE_RES_ERROR     = 0x80
}   UDP_FCODE;


/***   UpsStatus Bits  ***/
#define APP_MBUS_SBIT_UTIL_FAIL         (0x80)
#define APP_MBUS_SBIT_BAT_LOW           (0x40)
#define APP_MBUS_SBIT_BYPASS_ON         (0x20)
#define APP_MBUS_SBIT_UPS_FAIL          (0x10)
#define APP_MBUS_SBIT_STANDBY_TYPE      (0x08)
#define APP_MBUS_SBIT_TESTING           (0x04)
#define APP_MBUS_SBIT_SHUTDOWN          (0x02)
#define APP_MBUS_SBIT_BEEP_ON           (0x01)

/***    BatStatus Bits  ***/
#define APP_MBUS_BBIT_HEAT              (0x01)
#define APP_MBUS_BBIT_SMOKE             (0x02)
#define APP_MBUS_BBIT_BAT_SW            (0x04)  // 1=on
#define APP_MBUS_BBIT_AID_RUN           (0x08)
#define APP_MBUS_BBIT_BUZ_ON            (0x10)
#define APP_MBUS_BBIT_BAT_SW_MODE       (0x20)  // 1=auto mode

/***    SysEventBits Bits   ***/
#define APP_MBUS_EVT_POWER              (0x0001)
#define APP_MBUS_EVT_NETWORK            (0x0002)
#define APP_MBUS_EVT_GATEWAY            (0x0004)
#define APP_MBUS_EVT_DEVICE             (0x0008)
#define APP_MBUS_EVT_MODEM              (0x0010)
#define APP_MBUS_EVT_HEAT               (0x0020)
#define APP_MBUS_EVT_SMOKE              (0x0040)
#define APP_MBUS_EVT_BAT_TEMP           (0x0080)
#define APP_MBUS_EVT_BAT_VOL            (0x0100)
#define APP_MBUS_EVT_BAT_CUR            (0x0200)
#define APP_MBUS_EVT_FIRE               (0x0400)

/***    Limit EventBits Bits   ***/
#define APP_MBUS_EVT_UPS_INVOL_MAX2     (0x0000000000000001)
#define APP_MBUS_EVT_UPS_INVOL_MAX1     (0x0000000000000002)
#define APP_MBUS_EVT_UPS_INVOL_MIN1     (0x0000000000000004)
#define APP_MBUS_EVT_UPS_INVOL_MIN2     (0x0000000000000008)

#define APP_MBUS_EVT_UPS_INFREQ_MAX2    (0x0000000000000010)
#define APP_MBUS_EVT_UPS_INFREQ_MAX1    (0x0000000000000020)
#define APP_MBUS_EVT_UPS_INFREQ_MIN1    (0x0000000000000040)
#define APP_MBUS_EVT_UPS_INFREQ_MIN2    (0x0000000000000080)

#define APP_MBUS_EVT_UPS_OUTVOL_MAX2    (0x0000000000000100)
#define APP_MBUS_EVT_UPS_OUTVOL_MAX1    (0x0000000000000200)
#define APP_MBUS_EVT_UPS_OUTVOL_MIN1    (0x0000000000000400)
#define APP_MBUS_EVT_UPS_OUTVOL_MIN2    (0x0000000000000800)

#define APP_MBUS_EVT_UPS_OUTCUR_MAX2    (0x0000000000001000)
#define APP_MBUS_EVT_UPS_OUTCUR_MAX1    (0x0000000000002000)
#define APP_MBUS_EVT_UPS_OUTCUR_MIN1    (0x0000000000004000)
#define APP_MBUS_EVT_UPS_OUTCUR_MIN2    (0x0000000000008000)

#define APP_MBUS_EVT_UPS_BATVOL_MAX2    (0x0000000000010000)
#define APP_MBUS_EVT_UPS_BATVOL_MAX1    (0x0000000000020000)
#define APP_MBUS_EVT_UPS_BATVOL_MIN1    (0x0000000000040000)
#define APP_MBUS_EVT_UPS_BATVOL_MIN2    (0x0000000000080000)

#define APP_MBUS_EVT_UPS_BATTEMP_MAX2   (0x0000000000100000)
#define APP_MBUS_EVT_UPS_BATTEMP_MAX1   (0x0000000000200000)
#define APP_MBUS_EVT_UPS_BATTEMP_MIN1   (0x0000000000400000)
#define APP_MBUS_EVT_UPS_BATTEMP_MIN2   (0x0000000000800000)

#define APP_MBUS_EVT_BAT_TEMP_MAX2      (0x0000000001000000)
#define APP_MBUS_EVT_BAT_TEMP_MAX1      (0x0000000002000000)
#define APP_MBUS_EVT_BAT_TEMP_MIN1      (0x0000000004000000)
#define APP_MBUS_EVT_BAT_TEMP_MIN2      (0x0000000008000000)

#define APP_MBUS_EVT_BAT_VOL_MAX2       (0x0000000010000000)
#define APP_MBUS_EVT_BAT_VOL_MAX1       (0x0000000020000000)
#define APP_MBUS_EVT_BAT_VOL_MIN1       (0x0000000040000000)
#define APP_MBUS_EVT_BAT_VOL_MIN2       (0x0000000080000000)

#define APP_MBUS_EVT_BAT_CUR_MAX2       (0x0000000100000000)
#define APP_MBUS_EVT_BAT_CUR_MAX1       (0x0000000200000000)
#define APP_MBUS_EVT_BAT_CUR_MIN1       (0x0000000400000000)
#define APP_MBUS_EVT_BAT_CUR_MIN2       (0x0000000800000000)

/* APP_MBUS_REG_LIMIT / Limit Use Bit */
#define APP_MBUS_USEBIT_HEAT            (0x0001)
#define APP_MBUS_USEBIT_SMOKE           (0x0002)
#define APP_MBUS_USEBIT_HEAT_SMOKE      (0x0004)
#define APP_MBUS_USEBIT_LMT_VOL         (0x0008)
#define APP_MBUS_USEBIT_LMT_CUR         (0x0010)
#define APP_MBUS_USEBIT_LMT_TEMP        (0x0020)
//////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////


typedef struct _address
{
    char        ip      [TMP_BUF];
    int         port    ;
}   Address;

typedef struct _tcheck_
{
    time_t      last    ;
    time_t      curt    ;
}   TCHECK;


////////////////////// DB info ///////////////////////////////
typedef struct  _dbinfo_
{
    char        host    [MIN_BUF];
    int         port    ;
    char        user    [TMP_BUF];
    char        psword  [SML_BUF];
    char        dbname  [TMP_BUF];
    int         timeout ;
    int         retry   ;
}   DBINFO;
#define SZ_DBINFO sizeof(DBINFO)


////////////////// Server <-> Controller : Common format /////////////////
typedef struct  _hmsg_
{
    uint8_t     sop     ;
    uint8_t     devid   ;
    uint16_t    length  ;           //  Functon Code(1) + Payload(N) = 1 + N 사용, BigEndian
    uint8_t     fcode   ;           //  Fail 일 경우 : Function Code + 0x80 더해서 응답.
}   HMSG;
#define SZ_HMSG sizeof(HMSG)

typedef struct  _tmsg_
{
    uint8_t     eop     ;
}   TMSG;
#define SZ_TMSG sizeof(TMSG)

////////////////////// Server <-> Controller : Payload ///////////////////////////////

typedef struct  _devinfo_
{
    uint32_t    serial   ;
    uint8_t     macaddr  [6];
    uint8_t     localip  [4];
    uint16_t    modelcode;
    uint8_t     modelname[16];
    uint8_t     localname[16];
    uint8_t     localcode[8];
    uint8_t     umsserial[12];
    uint8_t     upsserial[12];
    uint8_t     batserial[12];
    uint8_t     batboxserial[12];
    uint8_t     loraserial[20];
}   DEVINFO;
#define SZ_DEVINFO  sizeof(DEVINFO)


//////////////// ALARM GET/SET Limit Info ////////////////
typedef struct
{
    uint16_t    Max2;
    uint16_t    Min2;
    uint16_t    Time;
    uint16_t    Max1;
    uint16_t    Min1;
}   _ULIMIT_;

typedef struct
{
    _ULIMIT_    Lmt ;
    uint8_t     Sms ;
    uint8_t     Used;
}   _UPS_LMT_;

typedef struct
{
    int16_t     Max ;
    int16_t     Min ;
    uint16_t    Time;
}   _BLIMIT_;

typedef struct
{
    _BLIMIT_    ChargeVoltage;
    _BLIMIT_    ChargeCurrent;
    _BLIMIT_    Temp;
    uint16_t    LmtHeatFlag  ;
}   BAT_LIMIT;
#define SZ_BAT_LIMIT sizeof(BAT_LIMIT)


typedef struct  _devdef_
{
    CDataMap    evtLimit    ;
    uint16_t    pollingtime ;
    uint16_t    dbsavetime  ;
    uint16_t    nodejstime  ;
    uint8_t     bcode   [16];
    uint8_t     bname   [21];
    uint8_t     aflag   ;
    char        snmpip  [16];
    uint8_t     phase       ;
}   DEVDEF;
#define SZ_DEVDEF sizeof(DEVDEF)


typedef struct  _devsock_
{
    ClntInfo    sock        ;
    DEVINFO     devinfo     ;
    uint32_t    seqno       ;
}   DEVSOCK;
#define SZ_DEVSOCK sizeof(DEVSOCK)


typedef struct  _data_
{
    uint16_t    UpsInVol;           /* UPS Input Voltage      [R]*/
    uint16_t    UpsInFreq;          /* UPS Input Frequency      */
    uint16_t    UpsOutVol;          /* UPS Output Voltage     [R]*/
    uint16_t    UpsOutCur;          /* UPS Output Current     [R]*/
    uint16_t    UpsBatVol;          /* UPS Battery Voltage      */
    uint16_t    UpsBatTemp;         /* UPS Battery Temperature  */
    uint16_t    UpsStatus;          /* UPS Status               */

    uint16_t    BatVol;             /* Battery Voltage [V]      */
    int16_t     BatCur;             /* Battery Current [mA]     */
    int16_t     BatEnvTemp;         /* Battery Environment Temperature [0.1C]   */
    uint16_t    BatEnvHumi;         /* Battery Environment Humidity [%]         */
    int16_t     BatTemp[BAT_TEMP_CNT];  /* Battery Temperatures [0.1C]    */
    uint16_t    BatStatus;          /* Battery Status               */

    uint16_t    EventBits;          /* Event Bits                   */
}   DATA;
#define SZ_DATA sizeof(DATA)

typedef struct  _data2_
{
    uint16_t    UpsData[UPS_3DATA_CNT]; /* 3상 UPS Input Voltage/Output Voltage/Output Current [S/T]*/
}   DATA2;
#define SZ_DATA2 sizeof(DATA2)

typedef struct  _getdata_
{
    uint32_t    serial          ;
    DATA        Data            ;
    uint64_t    LmtEvtBits      ;   /* Limit Event Bits         */
    DATA2       Data2           ;
    time_t      timestamp       ;
}   GETDATA;
#define SZ_GETDATA sizeof(GETDATA)

typedef struct  _ostat_
{
    uint16_t    UpsStatus;          /* UPS Status               */
    uint16_t    BatStatus;          /* Battery Status           */
    uint16_t    EventBits;          /* Event Bits               */
    uint64_t    LmtEvtBits;         /* Limit Event Bits         */
}   OSTAT;
#define SZ_OSTAT sizeof(OSTAT)


typedef struct  _upssnmp_
{
    uint32_t    Serial  ;
    uint16_t    UpsInVol;           /* UPS Input Voltage      [R]*/
    uint16_t    UpsInFreq;          /* UPS Input Frequency      */
    uint16_t    UpsOutVol;          /* UPS Output Voltage     [R]*/
    uint16_t    UpsOutCur;          /* UPS Output Current     [R]*/
    uint16_t    UpsBatVol;          /* UPS Battery Voltage      */
    uint16_t    UpsBatTemp;         /* UPS Battery Temperature  */
    uint16_t    UpsStatus;          /* UPS Status               */
    uint16_t    UpsData[UPS_3DATA_CNT];
}   UPSSNMP;
#define SZ_UPSSNMP sizeof(UPSSNMP)

typedef struct  _upssnmplst_
{
    uint16_t    Count    ;
    UPSSNMP     upsSnmp  [_UPS_MAX_DEV_];
}   UPSSNMPLST;
#define SZ_UPSSNMPLST sizeof(UPSSNMPLST)


typedef struct  _lora_
{
    uint16_t    UpsInVol;           /* UPS Input Voltage        */
    uint16_t    UpsInFreq;          /* UPS Input Frequency      */
    uint16_t    UpsOutVol;          /* UPS Output Voltage       */
    uint16_t    UpsOutCur;          /* UPS Output Current       */
    uint16_t    UpsBatVol;          /* UPS Battery Voltage      */
    uint16_t    UpsBatTemp;         /* UPS Battery Temperature  */
    uint16_t    UpsStatus;          /* UPS Status               */

    uint16_t    BatVol;             /* Battery Voltage [V]      */
    int16_t     BatCur;             /* Battery Current [mA]     */
    int16_t     BatEnvTemp;         /* Battery Environment Temperature [0.1C]   */
    uint16_t    BatEnvHumi;         /* Battery Environment Humidity [%]         */
    int16_t     BatTemp;            /* Battery Max Temperatures [0.1C]    */
    uint16_t    BatStatus;          /* Battery Status               */

    uint16_t    EventBits;          /* Event Bits                   */
}   LORA;
#define SZ_LORA  sizeof(LORA)
#define SZ_LORA2 (SZ_LORA* 2)

typedef struct  _getlora_
{
    char        loraserial  [24];
    uint32_t    loratime    ;
    uint32_t    refreshtime ;
    char        Data        [SZ_LORA2];
}   GETLORA;
#define SZ_GETLORA sizeof(GETLORA)


typedef struct  _check_
{
    time_t      alarmtime       ;
    time_t      curttime        ;
    uint8_t     alarmStart      ;
    uint8_t     DBInsert        ;
    uint64_t    flag            ;
}   CHECK;
#define SZ_CHECK sizeof(CHECK)


typedef struct  _evtchk_
{
    CHECK       Max2            ;
    CHECK       Max1            ;
    CHECK       Min1            ;
    CHECK       Min2            ;
}   EVTCHK;
#define SZ_EVTCHK sizeof(EVTCHK)


typedef struct  _batchk_
{
    time_t      alarmtime       ;
    uint8_t     DBInsert        ;
}   BATCHK;
#define SZ_BATCHK sizeof(BATCHK)


typedef struct  _event_
{
    uint32_t    serial      ;
    time_t      starttime   ;
    time_t      endtime     ;
    uint8_t     level       ;
    uint8_t     itsm_level  ;
    uint16_t    evtno       ;
    uint16_t    evtval      ;
    uint16_t    limitval    ;
    char        message     [MID_BUF];
    uint8_t     smsflag     ;
    uint8_t     bcode       [16];
    uint8_t     bname       [21];
}   EVENTS;
#define SZ_EVENTS sizeof(EVENTS)
/////////////////////////////////////////////////////////////


////////////////////// Server --> NodeJS(Web Server): Payload ///////////////////////////////
typedef struct  _njsmsg_
{
    uint32_t    serial      ;
    DATA        Data        ;
    uint64_t    LmtEvtBits  ;   /* Limit Event Bits     */
    DATA2       Data2       ;   /* 3상 데이타           */
}   NJSMSG;
#define SZ_NJSMSG sizeof(NJSMSG)

////////////////////// Server <-- NodeJS: need data ///////////////////////////////
typedef struct _did_
{
    uint32_t    serial  ;
}   DID;
#define SZ_DID sizeof(DID)

typedef struct _network_
{
    uint8_t     dhcp           ;
    uint8_t     localip     [4];
    uint8_t     subnetmask  [4];
    uint8_t     gateway     [4];
    uint8_t     dns         [4];
}   NETWORK;
#define SZ_NETWORK sizeof(NETWORK)

typedef struct _devnet_
{
    uint32_t    serial         ;
    uint8_t     dhcp           ;
    uint8_t     localip     [4];
    uint8_t     subnetmask  [4];
    uint8_t     gateway     [4];
    uint8_t     dns         [4];
}   DEVNET;
#define SZ_DEVNET sizeof(DEVNET)

typedef struct _cfg_
{
    uint8_t     serverip    [4] ;
    uint16_t    serverport      ;
    uint8_t     localcode   [8] ;
    uint8_t     umsserial   [12];
    uint8_t     upsserial   [12];
    uint8_t     batserial   [12];
    uint8_t     batboxserial[12];
    uint8_t     loraserial  [20];
}   CFG;
#define SZ_CFG sizeof(CFG)


typedef struct _devcfg_
{
    uint32_t    serial          ;
    uint8_t     serverip    [4] ;
    uint16_t    serverport      ;
    uint32_t    localcode       ;
    uint32_t    umsserial       ;
    uint32_t    upsserial       ;
    uint32_t    batserial       ;
    uint32_t    batboxserial    ;
    uint8_t     loraserial  [20];
}   DEVCFG;
#define SZ_DEVCFG sizeof(DEVCFG)

typedef struct _info_
{
    uint32_t    serial  ;
    uint8_t     devip   [4];
}   INFO;
#define SZ_INFO sizeof(INFO)

typedef struct _scfg_
{
    uint32_t    serial  ;
    CFG         cfg     ;
}   SCFG;
#define SZ_SCFG sizeof(SCFG)

typedef struct _snet_
{
    uint32_t    serial  ;
    NETWORK     net     ;
}   SNET;
#define SZ_SNET sizeof(SNET)

typedef struct _fwupt_
{
    uint32_t    serial  ;
    uint8_t     devip   [4];
    uint8_t     filepath[MIN_BUF];
}   FWUPT;
#define SZ_FWUPT sizeof(FWUPT)

typedef struct _onoff_
{
    uint8_t     onoff   ;
}   ONOFF;
#define SZ_ONOFF sizeof(ONOFF)

typedef struct _retmsg_
{
    uint16_t    elen    ;
    char        emsg    [MID_BUF];
}   RETMSG;
#define SZ_RETMSG sizeof(RETMSG)

typedef struct  _njscmd_
{
    DID         did     ;
    INFO        info    ;
    SNET        snet    ;
    SCFG        scfg    ;
    FWUPT       fwu     ;
    uint8_t     fcode   ;
    uint8_t     value   ;
    RETMSG      eMsg    ;
}   NJSCMD;
#define SZ_NJSCMD sizeof(NJSCMD)

typedef struct {
    uint8_t     MagicCode[4];
    uint16_t    ModelCode;
    uint16_t    FwVersion;
}   FW_VER_TABLE;
#define SZ_VTABLE sizeof(FW_VER_TABLE)

typedef struct _fwupdate_
{
    uint32_t    serial          ;
    uint16_t    modelcode       ;
    uint16_t    fwversion       ;
    uint32_t    fwsize          ;
    uint16_t    packetsize      ;
    uint16_t    checksum        ;
}   FWUPDATE;
#define SZ_FWUPDATE sizeof(FWUPDATE)

typedef struct _pkt_
{
    uint32_t    serial          ;
    uint16_t    pNum            ;
}   PKT;
#define SZ_PKT sizeof(PKT)

typedef struct _fwpacket_
{
    PKT         pkt             ;
    uint8_t     pData[MID_BUF]  ;
}   FWPACKET;
#define SZ_FWPACKET sizeof(FWPACKET)


typedef struct _snmp_
{
    uint32_t    serial          ;
}   SNMP;
#define SZ_SNMP sizeof(SNMP)


typedef struct _snmplst_
{
    uint16_t    Count           ;
    SNMP        snmp    [1024]  ;
}   SNMPLST;
#define SZ_SNMPLST sizeof(SNMPLST)



///////////////////////////// Common Structure ///////////////////////////////
typedef struct  _umsg_
{
    ClntInfo    sock    ;
    char        msg     [MDL_BUF];
}   UMSG;
#define SZ_UMSG sizeof(UMSG)


///////////////////////////// Event Sms Sending /////////////////////////////
typedef struct  _smsdata_
{
    char        keyword     [HEX_BUF];
    char        BizGugun    [LVL_BUF];
    char        date        [HEX_BUF];
    char        severity    [LVL_BUF];
    char        AppCode     [LVL_BUF];
    char        bCode       [TMP_BUF];
    char        bName       [TMP_BUF];
    char        Machine     [TMP_BUF];
    char        message     [MID_BUF];
}   KBSMS;
#define SZ_KBSMS sizeof(KBSMS)


#pragma pack(pop)
////////////////////////////////////////////////////////////

static const inline uint8_t IsNumber (char * String, int Size)
{
    if (!Size) return 0;
    for (int nByte = 0; nByte < Size; nByte++ )
    {
        if (String[nByte] < '0' || String[nByte] > '9')
            return  0 ;
    }
    return  1 ;
}

static inline const void delayTimestamp(uint32_t seconds, uint32_t microseconds)
{
    struct timeval timeout = {seconds,microseconds};
    if (select(0, NULL, NULL, NULL, &timeout) <= 0) {
        return ;
    }
}

static inline void sleepWait(uint32_t mseconds, bool * IsStop)
{
    uint32_t TickCount = 0;
    while(TickCount < mseconds)
    {
        SDL_Delay(_SLEEP_WAIT_)  ;
        TickCount += _SLEEP_WAIT_;
        if (!*IsStop) return;
    }
}

static inline void sleepWait(uint32_t seqno, uint16_t seconds)
{
    struct  timeval tv      ;
    gettimeofday(&tv, NULL) ;
    int8_t T = (tv.tv_sec % seconds);
    int8_t S = (seqno     % seconds);
    if (T > S)
        sleep((seconds-T+S));
    else
        sleep((S-T));
}

static inline void sleepWait(uint32_t seqno, uint16_t seconds, bool * IsStop)
{
    struct  timeval tv      ;
    gettimeofday(&tv, NULL) ;
    int8_t T = (tv.tv_sec % seconds);
    int8_t S = (seqno     % seconds);
    if (T > S)
        sleepWait((seconds-T+S) * 1000, IsStop);
    else
        sleepWait((S-T) * 1000, IsStop);
}

static inline char const * getdate()
{
    static char buf[32];
    time_t tt = time(NULL);
    strftime(buf, sizeof buf, "%Y-%m-%d %H:%M:%S", localtime(&tt));
    return buf;
}

static inline char const * convdate(time_t tt)
{
    static char buf[16];
    strftime(buf, sizeof buf, "%Y%m%d%H%M%S", localtime(&tt));
    return buf;
}

static inline const char * convDateTime (uint32_t seconds)
{
    static  char    buf2[TMP_BUF]       ;
    struct  tm      stTime2             ;
    struct  timeval tv2 = {seconds,0}   ;
    localtime_r(&tv2.tv_sec, &stTime2)  ;
    snprintf(buf2, sizeof(buf2), "%04d-%02d-%02dT%02d:%02d:%02d",
                                 stTime2.tm_year + 1900,
                                 stTime2.tm_mon  + 1   ,
                                 stTime2.tm_mday       ,
                                 stTime2.tm_hour       ,
                                 stTime2.tm_min        ,
                                 stTime2.tm_sec        );
    return buf2;
}

static inline const time_t ToTimestamp(const char * pStr)
{
    struct tm timeDate;
    memset(&timeDate, 0, sizeof(struct tm));
    strptime(pStr,"%Y-%m-%d %H:%M:%S", &timeDate);
    return mktime(&timeDate);
}

//static const inline uint32_t getTimestamp(uint16_t div)
//{
//    struct timeval tv;
//    gettimeofday(&tv, NULL);
//    return tv.tv_sec - (tv.tv_sec % div);
//}

static const inline uint32_t getTimestamp()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec;
}

//static const inline string getKeyValue(const char * pBuf, const char * pkey, const char * defval)
//{
//    string ret(defval);
//    if (pBuf == NULL) return ret;
//    try
//    {
//        string value;
//        char   mkey[TMP_BUF];
//        snprintf(mkey, sizeof(mkey), "%s=(\\w+)", pkey);
//        if (pcrecpp::RE(mkey).PartialMatch(pBuf, &value))
//            return value;
//        else
//            return ret;
//    }
//    catch (...)
//    {
//        printf("\x1b[1;35m%s()[%d] parsing failed !!!(%s)\x1b[0m\n", __FUNCTION__, __LINE__, pBuf);
//    }
//    return ret;
//}

static inline const uint16_t HexToU16( char * buff, int Len )
{
    uint16_t value = 0;
    for (int i = 0; i < Len; i++)
        value += ((uint8_t)buff[i] << (8 * (Len - i - 1)));
    return value;
}

static inline const uint32_t HexToU32( char * buff, int Len )
{
    uint32_t value = 0;
    for (int i = 0; i < Len; i++)
        value += ((uint8_t)buff[i] << (8 * (Len - i - 1)));
    return value;
}

static const inline uint16_t MakePacket(UMSG * pMsg, uint8_t fcode, void * pData = NULL, uint16_t pLen = 0)
{
    memset(pMsg->msg, 0, sizeof(pMsg->msg));
    HMSG  * head = (HMSG*)pMsg->msg ;
    char  * body = (char*)&pMsg->msg[SZ_HMSG];

    head->sop    = _SMP_PKT_SOP_;
    head->devid  = 0;
    head->length = htons(pLen+1);
    head->fcode  = fcode;

    if (pData) {
        memcpy(body, pData, pLen);
    }
    TMSG * tail  = (TMSG*)&pMsg->msg[SZ_HMSG+pLen];
    tail->eop    = _SMP_PKT_EOP_;

    return (uint16_t)(SZ_HMSG + pLen + SZ_TMSG);
}


//  UTF-8 To EUC-KR
static inline const void Utf8ToEuckr(char * in_str, int insize, char * outstr, int outsize)
{
    size_t in_size  = insize ;
    size_t out_size = outsize;
    iconv_t it  = iconv_open("EUC-KR", "UTF-8");
    int16_t ret = iconv(it, &in_str, &in_size, &outstr, &out_size);
    iconv_close(it);
    if (ret < 0)
        snprintf(outstr, outsize, "%s", in_str);
}

#endif  /* __CAMERA_H__  */
