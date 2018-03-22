#ifndef __HVAAS_PROTOCOL_H__
#define __HVAAS_PROTOCOL_H__

/**********************************************************************************************
* Filename   :  Hvaas_Protocol.h
* Developer  : sbs@gabia.co.kr
* Start Date : 2011/06/18
* Comment    :  HVaaS 프로토콜 관련 해더정의
***********************************************************************************************/

/*  INCLUDE  */
#include <stdint.h>

/*  DEFINE  */
#define   HEADER_MAGIC                      "GM"
//#define  HEADER_VERSION                    0x0001  //No use anymore.
#define   HEADER_VERSION_HVAAS              0x0002    // 2012. 4. Open Version
#define   HEADER_VERSION_HVAAS_NEW          0x0003   // 2012. 6. Open Version

/* Camera RTST Port */
#define   HVAAS_RTSP_PORT                   554


/* Service Type */
#define   SERVICETYPE_SETUPCONTROL          "00"
#define   SERVICETYPE_LIVECAST              "10"
#define   SERVICETYPE_ALARMBASE             "20"
#define   SERVICETYPE_HOLEPUNCHING          "30"
#define   SERVICETYPE_RELAYSTREAM           "33"
#define   SERVICETYPE_LIVE_HOLE             "40"
#define   SERVICETYPE_ALARM_HOLE            "50"
#define   SERVICETYPE_MUILTVIEW             "60"

/* Server Type */
#define   SERVERTYPE_SETUPCONTROL           "00"
#define   SERVERTYPE_LIVECAST               "10"
#define   SERVERTYPE_ALARM                  "20"
#define   SERVERTYPE_RENDEZVOUS             "30"
#define   SERVERTYPE_RELAYSTREAM            "33"
#define   SERVERTYPE_MUILTVIEW              "60"

#define  RELAYSTREAM_SERVERNAME             "relaystream"


/* Relay Stream Timeout when fail connect to ptop connect */
#define  RELAYSTREAM_TIMEOUT                8


/* Triple Des Key String */
#define  DES_KEY                            "GabiaVideoSolutionServiceTeam"


// Define Error Message
#define ERROR_SUCCESS                       "SUCCESS"
#define ERROR_MISMATCH_HEADER               "Mismatch Message Header"
#define ERROR_MISMATCH_MESSAGE_TYPE         "Unknown Message Type"
#define ERROR_MISMATCH_PAYLOAD_LEN          "Mismatch Payload Length"
#define ERROR_MISMATCH_PROTOCOL_VERSION     "Mismatch Protocol Version"
#define ERROR_MISMATCH_DEVICETYPE           "Mismatch Device Type Error"
#define ERROR_MESSAGE_PROCESS_ERROR         "Message Process Error"
#define ERROR_SERVER_CONNECTION_FAIL        "Server Connection Fail or Server Not Ready"
#define ERROR_SERVER_NOT_REGISTER           "Server Not Register, check DB ServerInfo"
#define ERROR_SERVER_SESSION_LIMIT          "Server Session Limit"
#define ERROR_DB_QUERY_FAIL                 "Database Query Fail check message elements"
#define ERROR_DB_FIELD_DUPLICATED           "DB Field has Duplicated it must have unique value"
#define ERROR_CAMERA_NOT_REGISTRATION       "Camera Not Registreation, cant not find MAC address"
#define ERROR_CAMERA_AUTH_FAIL              "Camera Authentication Fail"
#define ERROR_CAMERA_NOT_RECORD             "Camera Not Record, Can not find RecentRecord Information"
#define ERROR_CAMERA_FIRMWARE_NOTMATCH      "Camera firmware is not matching, it maybe old"
#define ERROR_USER_NOT_REGISTRATION         "User Not Registration"
#define ERROR_USER_PASSWORD_INCORRECT       "Incurrect UserPassword"
#define ERROR_USER_AUTH_FAIL                "User Authentication Fail"
#define ERROR_USER_MISMATCH_CAMERANUM       "User Mismatch Camera Number"
#define ERROR_CANT_FIND_VIDEO               "Can not find Video"
#define ERROR_OTP_MISMATCH                  "Find out OTPKey but Mismatch"
#define ERROR_OTP_TIMEOUT                   "Find out OTPKey but TimeOut"
#define ERROR_OTP_NOT_EXIST                 "Can not Find OTPKey"
#define ERROR_MEMORY_ALLOC_FAIL             "Memory Alloc fail or Buffer OverFlow, maybe request too many elements"
#define ERROR_INVALID_RESOLUTION_VALUE      "Invalid Resolution Value"
#define ERROR_SESSION_NOT_ALLOW             "Session Not Allow. check services period or session limit"
#define ERROR_STORAGE_NOT_ENOUGH            "Storage Not Enough"
#define ERROR_NOT_EXIST_DELETE_FILE         "Not Exist will be delete file"
#define ERROR_NO_HVAE_ANY_RESULT            "No have any result about request"
#define ERROR_IMAGE_TOO_LONG                "IFrame Image Size too long"
#define ERROR_SERVICE_END_CAMERA            "Camera Service End"
#define ERROR_INVALID_SAVE_TIME             "Invalid save time"
#define ERROR_CAMERA_NOT_CONNECTED          "Camera Not Connected on it's Service Server"
#define ERROR_MAKE_HOLE_SESSION_LIMIT       "CameraHolePunching Hole Number Limit Not Allowed"
#define ERROR_CAN_NOT_SUPPORT_NETWORK_TYPE  "Can not support network type, it must be share same network device or static network"
#define ERROR_NETORK_ADDRESS_PARSING_FAIL   "Can not parsing peer session network address, or message processing error"
#define ERROR_FAIL_TO_HOLE_CONNECTION       "Can not Connect to peer session, it can be camera or client "
#define ERROR_RELAY_CONNECT_TIMEOUT         "Relay Connect Timeout"
#define ERROR_USER_NOT_CONNECT              "User Not Connect"

//[SNS]+++++++++++++++++++++++++++++++++++
#define ERROR_ALARM_CAM_AUTH_FAIL           "Alarm Camera Authentication Fail"
#define ERROR_ALARM_SERVICE_EXPIRED         "Alarm Service Expired"
#define ERROR_ALARM_SAVECOUNT_OVERLIMIT     "Alarm Save Count Over Limit"
#define ERROR_ALARM_SAVEOFF_STATE           "Alarm Save OFF State"
#define ERROR_ALARM_SAVE_STATE              "Alarm Save State or Duplicate Event Info"
//++++++++++++++++++++++++++++++++++++++++

#define ERROR_UNKNOWN_ERROR                 "Unknown Error"




#pragma pack(push, 1)


/*  Type struct  */

//관리서버에서 사용, 프로토콜과 무관
typedef struct
{
        uint32_t          type;
        uint32_t          payload_Len;
        uint16_t          ver;
        uint8_t           encrypt;          // 0=non-encrypt, 1=des ecnrypt
        uint8_t           devicetype;       // 0=N/A, 10=ActiveX, 20=Android, 30=Iphone
        char              *payload;
}Hvaas_Payload;



//프로토콜 공통 헤더
typedef struct
{
        char             magic[2];          // fix "GM" (define)
        uint16_t         ver;               // fix 2 (define)
        uint16_t         type;              //enum MessageType
        uint32_t         bodylen;                 // payload Len
        uint8_t          encrypt;                // 0=non-encrypt, 1=des ecnrypt
        uint8_t          devicetype;            // 0=N/A, 10=ActiveX, 20=Android, 30=Iphone
        char             reserved[4];          // not usable
}Hvaas_Header;


typedef struct
{
        Hvaas_Header    *header;
        char            *payload;
}Hvaas_Packet;



//////////////////////////
// 이니셜티 카메라 헤더/////
//////////////////////////
typedef struct
{
        char             magic[2];         // fix "GM" (define)
        uint16_t         ver;                    // fix 1 (define)
        uint16_t         type;               //enum MessageType
        uint16_t         bodylen;             // payload Len
}Hvaas_Header_old;

typedef struct
{
        Hvaas_Header_old        *header;
        char                    *payload;
}Hvaas_Packet_old;



/*
-CameraSide
0x1000 ~ 0x1FFF

-Livestreamer <-> Gateway Side
0x2000 ~ 0x2FFF

-UserClient <-> Gateway Side
0x3000 ~ 0x3FFF

-DemonModule (Traffic & FileDelete) <-> Gateway Side
0x4000 ~ 0x4FFF

-Common ResultCode (Error?)
0x5000
*/

// 프로토콜 메시지 타입
enum MessageType
{
        //Camera Side
        CAMERA_DISCOVERY_REQUEST            = 0x1000,
        CAMERA_DISCOVERY_RESOPONSE          = 0x1001,
        CAMERA_INFORMATION_REQUEST          = 0x1010,
        CAMERA_INFORMATION_RESPONSE         = 0x1011,

        // Streamer Side
        CAMERA_AUTH_REQUEST                 = 0x2000,
        CAMERA_AUTH_RESPONSE                = 0x2001,
        CAMERA_CONNECTION_REQUEST           = 0x2010,
        CAMERA_CONNECTION_RESPONSE          = 0x2011,
        CAMERA_DISCONNECTION_REQUEST        = 0x2020,
        CAMERA_DISCONNECTION_RESPONSE       = 0x2021,
        CAMERA_EVENT_REQUEST                = 0x2030,
        CAMERA_EVENT_RESPONSE               = 0x2031,
        START_STREAM_REQUEST                = 0x2040,
        START_STREAM_RESPONSE               = 0x2041,
        STOP_STREAM_REQUEST                 = 0x2050,
        STOP_STREAM_RESPONSE                = 0x2051,
        SEND_STORAGE_INFO_REQUEST           = 0x2060,
        SEND_STORAGE_INFO_RESPONSE          = 0x2061,
        SEND_IMAGE_REQUEST                  = 0x2070,
        SEND_IMAGE_RESPONSE                 = 0x2071,
        KEEP_ALIVE_REQUEST                  = 0x2080,
        KEEP_ALIVE_RESPONSE                 = 0x2081,
        END_DATE_CAMERA_REQUEST             = 0x2090,
        END_DATE_CAMERA_RESPONSE            = 0x2091,
        LAST_FILE_INFO_REQUEST              = 0x2100,
        LAST_FILE_INFO_RESPONSE             = 0x2101,
        CAM_INFORMATION_CHANGE_REQUEST      = 0x2110,
        CAM_INFORMATION_CHANGE_RESPONSE     = 0x2111,
        STREAM_SERVER_REGISTER_REQUEST      = 0x2120,
        STREAM_SERVER_REGISTER_RESPONSE     = 0x2121,
        CLIENT_AUTH_REQUEST                 = 0x2130,
        CLIENT_AUTH_RESPONSE                = 0x2131,
        CAMERA_EVENT_AUTH_REQUEST           = 0x2140,
        CAMERA_EVENT_AUTH_RESPONSE          = 0x2141,

        SESSION_KEEP_ALIVE_REQUEST          = 0x2FF0,
        SESSION_KEEP_ALIVE_RESPONSE         = 0x2FF1,

         // User Side
        CONNECTION_REQUEST                  = 0x3000,
        CONNECTION_RESPONSE                 = 0x3001,
        GET_OTPKEY_REQUEST                  = 0x3010,
        GET_OTPKEY_RESPONSE                 = 0x3011,
        TIMELINE_INFORMATION_REQUEST        = 0x3020,
        TIMELINE_INFORMATION_RESPONSE       = 0x3021,
        PLAYBACK_INFORMATION_REQUEST        = 0x3030,
        PLAYBACK_INFORMATION_RESPONSE       = 0x3031,
        LIVE_IMAGE_REQUEST                  = 0x3040,
        LIVE_IMAGE_RESPONSE                 = 0x3041,
        FILE_IMAGE_REQUEST                  = 0x3050,
        FILE_IMAGE_RESPONSE                 = 0x3051,
        START_RECORD_REQUEST                = 0x3060,
        START_RECORD_RESPONSE               = 0x3061,
        SEND_MOBILE_TOKEN_REQUEST           = 0x3070,
        SEND_MOBILE_TOKEN_RESPONSE          = 0x3071,
        EVENT_LIST_REQUEST                  = 0x3080,
        EVENT_LIST_RESPONSE                 = 0x3081,
        CAMERA_STATUS_CHECK_REQUEST         = 0x3090,
        CAMERA_STATUS_CHECK_RESPONSE        = 0x3091,
        EVENT_DAYCOUNT_LIST_REQUEST         = 0x3100,
        EVENT_DAYCOUNT_LIST_RESPONSE        = 0x3101,

        EVENT_HOURCOUNT_REQUEST             = 0x3110,
        EVENT_HOURCOUNT_RESPONSE            = 0x3111,
        EVENT_RANGELIST_REQUEST             = 0x3120,
        EVENT_RANGELIST_RESPONSE            = 0x3121,

        CAMERA_CONNECTION_CHECK_REQUEST     = 0x3130,
        CAMERA_CONNECTION_CHECK_RESPONSE    = 0x3131,

        //Alarm List Request
        ALARM_EVENT_LIST_REQUEST               = 0x3140,
        ALARM_EVENT_LIST_RESPONSE              = 0x3141,
        //Alarm Range List Request
        ALARM_EVENT_RANGELIST_REQUEST          = 0x3150,
        ALARM_EVENT_RANGELIST_RESPONSE         = 0x3151,
        //Alarm List Enable/Disable Request
        ALARM_EVENT_LIST_CHANGE_STATE_REQUEST  = 0x3160,
        ALARM_EVENT_LIST_CHANGE_STATE_RESPONSE = 0x3161,
        //Alarm Usable List Request
        ALARM_EVENT_USABLELIST_REQUEST         = 0x3170,
        ALARM_EVENT_USABLELIST_RESPONSE        = 0x3171,
        //Alarm Event Day Count List Request
        ALARM_EVENT_DAYCOUNT_LIST_REQUEST      = 0x3180,
        ALARM_EVENT_DAYCOUNT_LIST_RESPONSE     = 0x3181,

        //Event List Extend
        EVENT_LIST_EXTEND_REQUEST              = 0x3190,
        EVENT_LIST_EXTEND_RESPONSE             = 0x3191,
        EVENT_RANGELIST_EXTEND_REQUEST         = 0x3200,
        EVENT_RANGELIST_EXTEND_RESPONSE        = 0x3201,

        // Error Response
        ERROR_RESPONSE                      = 0x5000,

        // ServerStatus Module Side
        SERVER_STATUS_REQUEST               = 0x4000,
        SERVER_STATUS_RESPONSE              = 0x4001,
        END_DATE_FILE_REQUEST               = 0x4010,
        END_DATE_FILE_RESPONSE              = 0x4011,
        FILE_DELETE_ERROR_REQUEST           = 0x4020,
        FILE_DELETE_ERROR_RESPONSE          = 0x4021,

        // image Sender Module Side
        GET_IMAGE_PATH_REQUEST              = 0x4030,
        GET_IMAGE_PATH_RESPONSE             = 0x4031,

        //Camera의 SeupControl 서버 접속정보요청
        CAMERA_CONNECTIONINFO_REQUEST       = 0x4100,
        CAMERA_CONNECTIONINFO_RESPONSE      = 0x4101,


        // Rendezvous server Side
        CAM_RANDEZVOUS_AUTH_REQUEST         = 0x1040,
        CAM_RANDEZVOUS_AUTH_RESPONSE        = 0x1041,
        CAM_MANAGER_AUTH_REQUEST            = 0x6010,     // to manager
        CAM_MANAGER_AUTH_RESPONSE           = 0x6011,   // from manager
        CAM_PEER_REQUEST                    = 0x6020,
        CAM_PEER_RESPONSE                   = 0x6021,
        CAM_SEND_PEERINFO_REQUEST           = 0x6030,
        CAM_SEND_PEERINFO_RESPONSE          = 0x6031,
        CAM_FAILCONNECT_REQUEST             = 0x6040,
        CAM_FAILCONNECT_RESPONSE            = 0x6041,
        CAM_RELAY_AUTH_REQUEST              = 0x6050,
        CAM_RELAY_AUTH_RESPONSE             = 0x6051,
        CAM_KEEP_ALIVE_REQUEST              = 0x6060,
        CAM_KEEP_ALIVE_RESPONSE             = 0x6061,

        USER_RANDEZVOUS_AUTH_REQUEST        = 0x6070,
        USER_RANDEZVOUS_AUTH_RESPONSE       = 0x6071,
        USER_RANDEZVOUS_AUTH_FOR_SNS_RESPONSE = 0x6171,
        USER_MANAGER_AUTH_REQUEST           = 0x6080,   // to manager
        USER_MANAGER_AUTH_RESPONSE          = 0x6081,   // from manager
        CAM_TRY_RELAY_REQUEST               = 0x6090,
        CAM_TRY_RELAY_RESPONSE              = 0x6091,

        USER_SEND_PEERINFO_REQUEST          = 0x6100,
        USER_SEND_PEERINFO_RESPONSE         = 0x6101,
        USER_CONNECT_PEER_REQUEST           = 0x6110,
        USER_CONNECT_PEER_RESPONSE          = 0x6111,
        USER_DISCONNECT_PEER_REQUEST        = 0x6120,
        USER_DISCONNECT_PEER_RESPONSE       = 0x6121,
        USER_KEEPALIVE_REQUEST              = 0x6130,
        USER_KEEPALIVE_RESPONSE             = 0x6131,

        USER_FAIL_CONNECT_REQUEST           = 0x6140,
        USER_FAIL_CONNECT_RESPONSE          = 0x6141,

        CAM_SESSION_CONNECT_REQUEST         = 0x6150,  // Camera<->Client
        CAM_SESSION_CONNECT_RESPONSE        = 0x6151,

        USER_TIMESTAMP_REQUEST              = 0x6160,
        USER_TIMESTAMP_RESPONSE             = 0x6161,

        USER_HOLE_SUCCESS_REQUEST           = 0x6190,  // Camera<->Client
        USER_HOLE_SUCCESS_RESPONSE          = 0x6191,

        USER_RELAY_CONNECTION_REQUEST       = 0x6200,
        USER_RELAY_CONNECTION_RESPONSE      = 0x6201,
        USER_TRY_RELAY_REQUEST              = 0x6210,
        USER_TRY_RELAY_RESPONSE             = 0x6211,
        SESSION_TEERDOWN_REQUEST            = 0x6220,
        SESSION_TEERDOWN_RESPONSE           = 0x6221,


        USER_RANDEZVOUS_AUTH_EXTEND_REQUEST  = 0x6270,
        USER_RANDEZVOUS_AUTH_EXTEND_RESPONSE = 0x6271,
        USER_MANAGER_AUTH_EXTEND_REQUEST     = 0x6280,
        USER_MANAGER_AUTH_EXTEND_RESPONSE    = 0x6281,



        // Rendezvous Server <-> Manager Server
        RANDEZVOUS_REGISTER_REQUEST         = 0x6500,
        RANDEZVOUS_REGISTER_RESPONSE        = 0x6501,
        RANDEZVOUS_KEEPALIVE_REQUEST        = 0x6510,
        RANDEZVOUS_KEEPALIVE_RESPONSE       = 0x6511,
        CLIENT_CONNECT_NOTI_REQUEST         = 0x6520,
        CLIENT_CONNECT_NOTI_RESPONSE        = 0x6521,
        CLIENT_DISCONNECT_NOTI_REQUEST      = 0x6530,
        CLIENT_DISCONNECT_NOTI_RESPONSE     = 0x6531,
        NOTI_CAM_CONNECT_REQUEST            = 0x6540,
        NOTI_CAM_CONNECT_RESPONSE           = 0x6541,
        NOTI_CAM_DISCONNECT_REQUEST         = 0x6550,
        NOTI_CAM_DISCONNECT_RESPONSE        = 0x6551,
        RANDEZVOUS_ERROR_NOTI_REQUEST       = 0x6560,
        RANDEZVOUS_ERROR_NOTI_RESPONSE      = 0x6561,
        RANDEZVOUS_SERVICE_ENDLIST_REQUEST  = 0x6570,
        RANDEZVOUS_SERVICE_ENDLIST_RESPONSE = 0x6571,
        RANDEZVOUS_SERVICE_CHANGELIST_REQUEST   = 0x6580,
        RANDEZVOUS_SERVICE_CHANGELIST_RESPONSE  = 0x6581,


        // Setup Control <-> Manager Server
        SETUP_REGISTER_REQUEST              = 0x7000,
        SETUP_REGISTER_RESPONSE             = 0x7001,
        SETUP_KEEPALIVE_REQUEST             = 0x7010,
        SETUP_KEEPALIVE_RESPONSE            = 0x7011,
        SETUP_CAM_AUTH_REQUEST              = 0x7020,
        SETUP_CAM_AUTH_RESPONSE             = 0x7021,
        SETUP_CAM_CONNECT_REQUEST           = 0x7030,
        SETUP_CAM_CONNECT_RESPONSE          = 0x7031,
        SETUP_CAM_DISCONNECT_REQUEST        = 0x7040,
        SETUP_CAM_DISCONNECT_RESPONSE       = 0x7041,
        SETUP_SERVICE_ENDLIST_REQUEST       = 0x7050,
        SETUP_SERVICE_ENDLIST_RESPONSE      = 0x7051,
        SETUP_SERVICE_CHANGELIST_REQUEST    = 0x7060,
        SETUP_SERVICE_CHANGELIST_RESPONSE   = 0x7061,
        SETUP_CAM_FIRMWARE_STATE_REQUEST    = 0x7070,
        SETUP_CAM_FIRMWARE_STATE_RESPONSE   = 0x7071,
        SETUP_CAM_NAT_SETINFO_REQUEST       = 0x7080,
        SETUP_CAM_NAT_SETINFO_RESPONSE      = 0x7081,

       //Eevent Sender Check Manager server
        EVENTSENDER_CHECK_MANAGER           = 0x9010,

        // AlarmCast Server Side
        // Manager Server <-> AlarmCast Server
        ALARM_REGISTER_REQUEST              = 0x8000,
        ALARM_REGISTER_RESPONSE             = 0x8001,
        // KeepAlive
        ALARM_KEEPALIVE_REQUEST             = 0x8010,
        ALARM_KEEPALIVE_RESPONSE            = 0x8011,
        // Authentication
        ALARM_CAMAUTH_REQUEST               = 0x8020,
        ALARM_CAMAUTH_RESPONSE              = 0x8021,
        //Camera Alarm Stop Stream
        ALARM2SETUP_STOP_STREAM_REQUEST     = 0x8030,
        ALARM2SETUP_STOP_STREAM_RESPONSE    = 0x8031,
//++++++++++++++++++++++++++++++++++++++++++
//[SNS_PROTOCOL] ischoi -20130527
//++++++++++++++++++++++++++++++++++++++++++
        //Manager Server <->  SNS Gateway Server
        //Camera Information
        SNS_GW_CAMSET_REQUEST               = 0xA010,
        SNS_GW_CAMSET_RESPONSE              = 0xA011,
        // Camera Alarm list count
        SNS_GW_EVENT_REQUEST                = 0xA020,
        SNS_GW_EVENT_REPONSE                = 0xA021,

        // SNS Gateway Server <-> SetupControl Server
        // Set Camera Event Set
        SNS_GW_EVENTSET_REQUEST             = 0xB010,
        SNS_GW_EVENTSET_RESPONSE            = 0xB011,

        //SNS Gateway Server <-> EventSender Server
        SNS_GW_EVENT_NOTI_REQUEST           = 0xC010,
        SNS_GW_EVENT_NOTI_RESPONSE          = 0xC011,
        // Camera On/Off Alarm
        SNS_GW_CAMERA_ONOFF_REQUEST         = 0xC020,
        SNS_GW_CAMERA_ONOFF_RESPONSE        = 0xC021,
        // Camera Event Count
        SNS_GW_EVENT_COUNT_REQUEST          = 0xC030,
        SNS_GW_EVENT_COUNT_RESPONSE         = 0xC031,

//++++++++++++++++++++++++++++++++++++++++++
//[Camera Service Status] kde -20140306
//++++++++++++++++++++++++++++++++++++++++++
        // Camera Service Status
        SVC_CAMERA_STATUS_REQUEST           = 0xFFFC,
        SVC_CAMERA_STATUS_RESPONSE          = 0xFFFD,
        SVC_SERVICE_STATUS_REQUEST          = 0xFFFE,
        SVC_SERVICE_STATUS_RESPONSE         = 0xFFFF
};




// 결과코드 enum
enum ResultCode
{
        SERVER_RESTART                      = 90,  // 서버 재시작 (only use keep alive message)
        SUCCESS                             = 100,
        MISMATCH_HEADER                     = 110,  // 메시지 parsing 관련 에러
        MISMATCH_MESSAGE_TYPE               = 111,
        MISMATCH_PAYLOAD_LEN                = 112,
        MISMATCH_PROTOCOL_VERSION           = 113,
        MISMATCH_DEVICETYPE                 = 114,
        MESSAGE_PROCESS_ERROR               = 120,  // 시스템관련에러
        SERVER_CONNECTION_FAIL              = 121,  // 서버연결이안되어있음 혹은 연결된서버가 하나도없음
        DB_QUERY_FAIL                       = 122,
        DB_FIELD_DUPLICATED                 = 123,  // 중복되지않아야할 필드가 중복되었음
        SERVER_NOT_REGISTER                 = 124,  // 디비에 서버 등록되어있지않음
        SERVER_SESSION_LIMIT                = 125,  // 서버 허용 세션 제한
        CAMERA_NOT_REGISTRATION             = 130,  // 카메라 관련 에러
        CAMERA_AUTH_FAIL                    = 131,  // 등록되지않은 카메라
        CAMERA_NOT_RECORD                   = 132,  // 녹화된데이터가 없음
        CAMERA_FIRMWARE_NOTMATCH            = 133,  // 카메라 펌웨어가 오래되거나 맞지않음
        USER_NOT_REGISTRATION               = 140,  // 등록되지않은 사용자ID
        USER_PASSWORD_INCORRECT             = 141,  // 사용자 Password 오류
        USER_AUTH_FAIL                      = 142,  // 사용자인증실패
        USER_MISMATCH_CAMERANUM             = 143,  // 등록된 카메라 숫자가 TB_MEM_MEMBER 와 다름
        CANT_FIND_VIDEO                     = 144,  // 해당시간에 영상이 없음
        OTP_MISMATCH                        = 145,  // OTP키가 있지만 맞지않음
        OTP_TIMEOUT                         = 146,  // OTP키 유효시간 초과
        OTP_NOT_EXIST                       = 147,  // OTP키가 없음
        MEMORY_ALLOC_FAIL                   = 148,  // new 실패
        INVALID_RESOLUTION_VALUE            = 149,  // 잘못된 플레그값 수신
        SESSION_NOT_ALLOW                   = 150,  // 세션 및 정책관련
        STORAGE_NOT_ENOUGH                  = 151,  // 스토리지 할당불가
        NOT_EXIST_DELETE_FILE               = 152,  // 지울파일이 없음
        SEND_CLOSEBY_VIDEO                  = 153,  // playback 에서 해당영상이 없어서 가장 가까운 지점의 위치를 알려줌
        NO_HVAE_ANY_RESULT                  = 154,  // 요청에 해당되는 리스트가없음.
        IMAGE_TOO_LONG                      = 155,  // 이미지가 너무 큼
        SERVICE_END_CAMERA                  = 156,  //서비스 만료


        INVALID_SAVE_TIME                   = 161,  // 지정된 저장시간이 아님
        CAMERA_NOT_CONNECTED                = 162,  // 카메라가 해당 서비스 서버에 연결되지 않음
        MAKE_HOLE_SESSION_LIMIT             = 163,  // 카메라 P2P 홀생성 제한 초과
        CAN_NOT_SUPPORT_NETWORK_TYPE        = 164,  // 홀펀칭 연결이 불가능한 네트워크 구조
        NETORK_ADDRESS_PARSING_FAIL         = 165,
        FAIL_TO_HOLE_CONNECTION             = 166,
        RELAY_CONNECT_TIMEOUT               = 167,  // 홀펀칭 연결 시도 실패 후 릴레이 가능한 시간을 초과
        USER_NOT_CONNECT                    = 168,

        USER_DUPLICATE_CONNECT              = 169,  // 중복된 사용자 접속 시도

        //[SNS]+++++++++++++++++++++++++++++++++++
        ALARM_CAM_AUTH_FAIL                 = 170,  // 알람카메라 인증 실패
        ALARM_SERVICE_EXPIRED               = 171,  // 알람 서비스 만료상태
        ALARM_SAVECOUNT_OVERLIMIT           = 172,  // 알람 저장카운트 초과
        ALARM_SAVEOFF_STATE                 = 173,  // 알람 저장 불가 상태
        ALARM_SAVE_STATE                    = 174,  // 알람 저장 진행 상태
        //++++++++++++++++++++++++++++++++++++++++

        UNKNOWN_ERROR                       = 210   // Default
};


// 서버 접속정보
typedef struct
{
        char            ServiceType[2];
        uint8_t         ServiceState;
        uint8_t         CameraType;
        uint8_t         CameraAudio;
        char            CamName[90];
        char            SerialNumber[16];
        uint8_t         CamSetupState;
        char            SetupIP[15];
        uint16_t        SetupPort;
        uint8_t         CamLiveCastState;
        char            StreamerIP[15];
        uint16_t        StreamerPort;
        char            StreamThumIP[15];
        uint16_t        StreamThumPort;
        uint8_t         CamAlarmState;
        char            AlarmIP[15];
        uint16_t        AlarmPort;
        char            AlarmThumIP[15];
        uint16_t        AlarmThumPort;
        uint8_t         CamRendezvousState;
        char            RendezvousIP[15];
        uint16_t        RendezvousPort;
        uint8_t         CamMuiltViewState;
        char            MuiltViewIP[15];
        uint16_t        MuiltViewPort;
        uint8_t         PlayPerHour;
        uint16_t        PlaySeconds;
}StreamConnectionInfo;



// 카메라 상태정보
typedef struct
{
        char           SerialNumber[16];
        char           ServiceType[2];
        uint8_t        SetupServerState;
        uint8_t        LiveCastServerState;
        uint8_t        AlarmState;
        uint8_t        RandezvousState;
        uint8_t        MuiltCastState;
}StreamStatusInfo;



// 카메라 정보
typedef struct
{
        uint8_t         CameraType;                 // 카메라종류 Axis=1
//      uint8_t         VideoEncryption;                // 영상암호화 적용여부
        uint8_t         CodecType;                  // 코덱  H.264=1
        uint8_t         FrameRate;                  // 초당프레임수
        uint32_t        BitRate;                    // 초당비트수
        char            Priority[10];               // none, framerate
        char            Resolution[10];             // 해상도   (640x480)
}VideoInfo;

typedef struct
{
    uint8_t     pretime ;
    uint16_t    posttime;
} AlarmInfo;

typedef struct
{
        VideoInfo       videoInfo;
        uint8_t         RecordType;
        uint8_t         AudioRecord;            //  Audio Save On/Off
        char            ServiceType[2];
        char            CamAddress[15];             // IP주소
        char            CamID[16];                   // default root:root
        char            CamPassword[16];
        char            StoragePath[80];
}CameraInfo;





typedef struct
{
    char               Memid[17];
    char               EventTitle[21];
    char               SendType[11];
    char               SendWeek[11];
    char               EventStartDate[7];
    char               EventEndDate[7];
    char               DetectCycle[11];
}EventRegister;


typedef struct
{
    char               Memid[17];
    char               EventTitle[21];
    char               SendType[11];
}OffEventRegister;




typedef struct
{
    uint8_t           Groupid;
    uint8_t           Serverid;
    char              ServerType[3];
}ServerIdentify;



/////////////////////////////////////////////
//////각 메시지 페이로드에 대한 정의////////////
/////////////////////////////////////////////


///////////// Global Error//////////////////////
typedef struct
{
        uint8_t      result;                    //enum ResultCode;
        char         errorReason[128];
}ErrorMsgResponse;



/////////////Camera Side//////////////////////
typedef struct
{
        char     MacAddress[12];
}CameraDiscoveryRequest;


typedef struct
{
        uint8_t          result;     //enum ResultCode;
        char             ServiceType[2];
        char             SetupIP[15];
        uint16_t         SetupPort;
        char             LiveCastIP[15];
        uint16_t         LiveCastPort;
        char             SaveTime[12];
        char             AlarmIP[15];
        uint16_t         AlarmPort;
        char             AlarmTime[12];
        char             RendezvousIP[15];
        uint16_t         RendezvousPort;
}CameraDiscoveryResponse;




typedef struct
{
        char     MacAddress[12];
        char     FirmwareVersion[32];
}CameraInformationRequest;


typedef struct
{
    uint8_t      result;
}CameraInformationResponse;


////////////////AlarmServer Side//////////////////
typedef struct
{
        uint8_t         result; //enum ResultCode;
        char            SerialNumber[16];
        CameraInfo      camInfo;
        AlarmInfo       almInfo;
}CameraAlarmAuthResponse;


////////////////LiveServer Side//////////////////
typedef struct
{
        char     MacAddress[12];
}CameraAuthRequest;

typedef struct
{
        uint8_t         result; //enum ResultCode;
        char            SerialNumber[16];
        char            ServiceType [2];
        CameraInfo      camInfo;
}CameraAuthResponse;

typedef struct
{
        uint64_t              SessionID;
        ServerIdentify        t_serverIdentify;
        char                  SerialNumber[16];
        char                  ServiceType[2];
}CameraConnectionRequest;

typedef struct
{
        uint8_t         result; //enum ResultCode;
}CameraConnectionResponse;


typedef struct
{
        uint64_t        SessionID;
      ServerIdentify    t_serverIdentify;
        char            SerialNumber[16];
        char            ServiceType[2];
        char            SaveTime[14];
        char            EndTime[14];
}CameraDisConnectionRequest;


typedef struct
{
        uint8_t          result;    //enum ResultCode;
}CameraDIsConnectionResponse;


typedef struct
{
        char                 SerialNumber[16];
        uint8_t              EventType;              // 1=동작감지
        char                 EventTime[14];
}CameraEventRequest;


typedef struct
{
        uint8_t           result;    //enum ResultCode;
}CameraEventResponse;

typedef struct
{
        char              SerialNumber[16];
        char              SessionID[10];
        char              OtpKey[8];                        //사용자로부터 받은 일회용세션키
}ClientAuthRequest;

typedef struct
{
        uint8_t           result;   //enum ResultCode;
        uint8_t           StreamLimit;
}ClientAuthResponse;


typedef struct
{
        uint64_t          SessionID;
        ServerIdentify    t_serverIdentify;
        char              SerialNumber[16];
        char              ClientAddress[15];
        char              ServiceType[2];
}StartStreamRequest;


typedef struct
{
        uint8_t           result;   //enum ResultCode;
        char              UserID[16];
        char              UserPassword[64];
}StartStreamResponse;


typedef struct
{
        uint64_t          SessionID;
        ServerIdentify    t_serverIdentify;
        char              SerialNumber[16];
        char              ServiceType[2];
}StopStreamRequest;


typedef struct
{
        uint8_t           result; //enum ResultCode;
}StopStreamResponse;



typedef struct
{
        char              SerialNumber[16];
        char              PrevSaveTime[14];
        char              PrevEndTime[14];
        char              SaveTime[14];
        char              RowFilePath[80];
        char              MetaFilePath[80];
}StorageInfoRequest;



typedef struct
{
        uint8_t          result;    //enum ResultCode;
}StorageInfoResponse;


typedef struct
{
        char              SerialNumber[16];
        uint32_t          ImageLen;
        VideoInfo         videoInfo;
        char              param[256];
}SendImageRequest;


typedef struct
{
        uint8_t           result;    //enum ResultCode;
}SendImageResponse;


typedef struct
{
        uint8_t           result;    //enum ResultCode;
        ServerIdentify    t_serverIdentify;
}SendKeepAliveRequest;


typedef struct
{
        uint8_t           result;    //enum ResultCode;
}SendKeepAliveResponse;

typedef struct
{
        uint8_t         groupid;
        uint8_t         serverid;
        char            ServerType[3];
}EndDateCameraRequest;

typedef struct
{
        uint8_t          result;    //enum ResultCode;
        uint8_t          endflag;
        uint32_t         EndCamNumber;
}EndDateCameraResponse;


typedef struct
{
        char             SerialNumber[16];
}LastFileInfoRequest;

typedef struct
{
        uint8_t          result;    //enum ResultCode;
        char             LastFilePath[80];
}LastFileInfoResponse;

typedef struct
{
        uint8_t         groupid;
        uint8_t         serverid;
    //  uint8_t         servertype;
        char              ServerType[3];
}CameraInfoChangeRequest;

typedef struct
{
        uint8_t         result;                     //enum ResultCode;
        uint8_t         endflag;
        uint32_t        ChangeCamNumber;
}CameraInfoChangeResponse;


typedef struct
{
        char     MacAddress[12];
}CameraEventAuthRequest;

typedef struct
{
        uint8_t         result; //enum ResultCode;
        char            SerialNumber[16];
        char            ServiceType[2];
}CameraEventAuthResponse;



/////////////////////////////User Side/////////////////////////////
typedef struct
{
        char              UserID[16];
        char              UserPassword[32];
}ConnectionRequest;

typedef struct _ConnectionResponse
{
        uint8_t           result;                        //enum ResultCode;
        uint8_t           UserCameraNumber;  //Counter
//      StreamConnectionInfo     *ConnInfo;                 // ConnInfo[UserCameraNumber]
}ConnectionResponse;



typedef struct _CameraStatusCheckRequest
{
    char                   UserID[16];
}CameraStatusCheckRequest;

typedef struct _CameraStatusCheckResponse
{
    uint8_t                  result;                        //enum ResultCode;
    uint8_t                  UserCameraNumber;  //Cam Counter
//  StreamStatusInfo         camStatus[UserCameraNumber];
}CameraStatusCheckResponse;


typedef struct _CameraConnectionCheckRequest
{
    char              SerialNumber[16];
}CameraConnectionCheckRequest;

typedef struct _CameraConnectionCheckResponse
{
    uint8_t           result;                        //enum ResultCode;
    char                            LiveCastSvrIP[15];
    uint16_t                    LiveCastUsePort;
    char                            RendezvousSvrIP[15];
    uint16_t                    RendezvousUsePort;

}CameraConnectionCheckResponse;


// Camera Connection SetupControl Server IP
//typedef struct _CameraConnectionInfoRequest
//{
//    char              SerialNumber[16];
//}CameraConnectionInfoRequest;

typedef struct _CameraConnectionInfoResponse
{
    uint8_t           result;
}CameraConnectionInfoResponse;


typedef struct _CameraConnectionInfo
{
    char              SerialNumber[16];
    char              SetupControlSvrIP[15];
    uint16_t          SetupControlUsePort;
    uint8_t           StateFlag;
}CameraConnectionInfo;


typedef struct _UserTimeStampRequest
{
    char              SerialNumber[16];
}UserTimeStampRequest;

typedef struct _GetOTPRequest
{
        char            UserID[16];
        char            UserPassword[64];
        char            SerialNumber[16];
}GetOTPRequest;


typedef struct _GetOTPResponse
{
        uint8_t         result;                 //enum ResultCode;
        char            SerialNumber[16];
        char            SessionID[10];
        char            OtpKey[8];
}GetOTPResponse;



typedef struct _TimeLineInfoRequest
{
        char            SerialNumber[16];
        uint8_t         RecordResolution;         // 1=sec, 2=hour, 3=day
        uint8_t         EventResolution;          // 1=sec, 2=30min, 3=hour, 4=day
        char            StartTime[14];
        char            EndTime[14];
}TimeLineInfoRequest;


typedef struct  _TimeLineInfoResponse
{
        uint8_t         result;                    //enum ResultCode;
        uint32_t        RecordCounter;
        uint32_t        EventCounter;
        char            RecordStartTime[14];
        char            RecordEndTime[14];
        char            EventTime[14];
}TimeLineInfoResponse;


typedef struct _PalyBackInfoRequest
{
        char              SerialNumber[16];
        char              PlayTime[14];
}PalyBackInfoRequest;


typedef struct  _PalyBackInfoResponse
{
        uint8_t          result;    //enum ResultCode;
        char             RecordFileName[32];
}PalyBackInfoResponse;



typedef struct
{
        char              UserID[16];
        char              SerialNumber[16];
}LiveImageRequest;

typedef struct
{
        uint8_t          result;
        uint32_t         ImageLen;
        VideoInfo        videoInfo;
        char             param[256];
}LiveImageResponse;


typedef struct
{
        char             UserID[16];
        char             SerialNumber[16];
        char             PlayTime[14];
}FileImageRequest;


typedef struct
{
        uint8_t          result;
        uint32_t         ImageLen;
        VideoInfo        videoInfo;
        char             param[256];
}FileImageResponse;


typedef struct _StartRecordRequest
{
        char             UserID[16];
        char             SerialNumber[16];
}StartRecordRequest;

typedef struct _StartRecordResponse
{
        uint8_t          result;
        char             RecordTime[14];
}StartRecordResponse;



typedef struct _SendMobileTokenRequest
{
        char             UserID[16];
        uint8_t          DeviceType;       // 0 = Andriod, 1 = IPhone
        char             DeviceToken[255];
}SendMobileTokenRequest;


typedef struct _SendMobileTokenResponse
{
        uint8_t          result;
}SendMobileTokenResponse;


typedef struct _EventList_t
{
        uint8_t          EventType;             // 0=Disconnect,  1=Detect Event
        char             EventTime[14];
}EventList_t;

typedef struct _EventListRequest
{
        char              SerialNumber[16];
        char              RequestTime[14];
        uint32_t          RequestCount;
}EventListRequest;

//Event Extend Data Structure
typedef struct _EventListExtend_t
{
   uint8_t      EventType;
   char         EventStartTime[14];
   char         EventEndTime[14];
   uint8_t      EventDisableState;  //  0 :  Enable state 1: Disable state
}EventListExtend_t;


typedef struct _EventListResponse
{
        uint8_t          result;
        char             SerialNumber[16];
        char             CameraName[90];
        uint32_t         EventListCount;
//      EventList_t or EventListExtend_t      EventList[EventListCount]
}EventListResponse;


typedef struct _EventDayCountList_t
{
        uint32_t         EventCount;
        char             RecordStartTime[14];
        char             RecordEndTime[14];
        char             FirstEvent[14];
        char             FirstRecord[14];
}EventDayCountList_t;


typedef struct _EventDayCountListRequest
{
        char              SerialNumber[16];
        char              RequestTime[14];
        uint32_t          RequestCount;
}EventDayCountListRequest;

typedef struct _EventDayCountListResponse
{
        uint8_t           result;
        char              SerialNumber[16];
        char              CameraName[90];
        uint32_t          EventListCount;
//      EventDayCountList_t      EventDayList[EventListCount]
}EventDayCountListResponse;


typedef struct
{
    char            SerialNumber[16];
    char            RequestTime[14];
}EventHourCountRequest;

typedef struct
{
    uint8_t         result;
    char            EventCountOnHour[24];
}EventHourCountResponse;



typedef struct
{
    char        SerialNumber[16];
    char        EventStartTime[14];
    char        EventEndTime[14];
}EventRangeListRequest;

typedef struct
{
    uint8_t    result;
    uint32_t   EventCount;
    //[EventCount];
    // EventInfo EventInfo[EventCount]
}EventRangeListResponse;

typedef struct _EventNotiInfo
{
    uint8_t     EventType;
    char        EventTime[14];
}EventNotiInfo;


//Alarm Event Data Structure
typedef struct _AlarmEventTimeInfo
{
   uint8_t      AlarmEventType;
   char         AlarmEventStartTime[14];
   char         AlarmEventEndTime[14];
   uint8_t      AlarmEventDisableState;  //  0 :  Enable state 1: Disable state
}AlarmEventTimeInfo;


//ALARM_EVENT_LIST
typedef struct _AlarmEventListRequest
{
    char         SerialNumber[16];
    char         RequestTime[14];
    uint32_t     RequestCount;
}AlarmEventListRequest;

typedef struct _AlarmEventListResponse
{
    uint8_t      result;
    char         SerialNumber[16];
    char         CameraName[90];
    uint32_t     AlarmEventListCount;
//  AlarmEventTimeInfo * AlarmEventListCount
}AlarmEventListResponse;

//ALARM_EVENT_RANGELIST
typedef struct _AlarmEventRangeListRequest
{
    char         SerialNumber[16];
    char         AlarmEventStartTime[14];
    char         AlarmEventEndTime[14];
    uint32_t     RequestCount;
}AlarmEventRangeListRequest;

typedef struct
{
    uint8_t      result;
    uint32_t     AlarmEventCount;
    // AlarmEventCount 만큼 아래 리스트가 연속적으로 추가
    // AlarmEventTimeInfo * AlarmEventCount;
}AlarmEventRangeListResponse;

//ALARM_EVENT_LIST_CHANGE_STATE
typedef struct _AlarmEventListChangeStateRequest
{
    char             SerialNumber[16];
    uint8_t          SetState;          // 0 : Enable, 1 : Disable state
    uint8_t          AlarmEventCount;
    // AlarmEventCount 만큼 아래 리스트가 연속적으로 추가
    //char           AlarmSaveTime[14];
}AlarmEventListChangeStateRequest;


typedef struct _AlarmEventListChangeStateResponse
{
    uint8_t         result;
}AlarmEventListChangeStateResponse;

//ALARM_EVENT_USABLELIST
typedef struct _AlarmEventUsableListRequest
{
    char         SerialNumber[16];
    char         AlarmSaveTime[14];
}AlarmEventUsableListRequest;

typedef struct _AlarmEventUsableListResponse
{
    uint8_t        result;
    uint8_t        forward_flag;
    char           F_AlarmSaveTime[14];
    char           F_AlarmEndTime[14];
    uint8_t        backward_flag;
    char           B_AlarmSaveTime[14];
    char           B_AlarmEndTime[14];
}AlarmEventUsableListResponse;

//ALARM_EVENT_DAYCOUNT_LIST

//////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////
//update server status
typedef struct _t_storageInfo
{
        char                mountName[50];
        uint64_t            nowUse;
        uint64_t            available;
        uint64_t            tot_available;
        char                usePersent[10];
}t_storageInfo;

typedef struct _t_netinfo
{
        char              interfaceName[20];
        uint64_t          in_traffic;
        uint64_t          out_traffic;
}t_netinfo;


typedef struct _StreamerStatusRequest
{
        ServerIdentify     t_serverIdentify;
        char               cpuUse[10];
        char               memUse[10];
        uint8_t            interfaceNum;
        uint8_t            storageNum;
}StreamerStatusRequest;


typedef struct _StreamerStatusResponse
{
        uint8_t          result;
}StreamerStatusResponse;


///////////////////////////
//end of date file delete
typedef struct _StorageDeleteFileRequest
{
    uint8_t         groupid;
    uint8_t         serverid;
    char            servertype[3];
}StorageDeleteFileRequest;


typedef struct _StorageDeleteFileResponse
{
        char                serialNumber[16];
        char                endTime[14];
        char                rowFilePath[80];
}StorageDeleteFileResponse;


typedef struct _DeleteFileErrorReturn
{
        uint8_t              errorNumber;
        char                 errorString[50];
        char                 rawFilePath[80];
}DeleteFileErrorReturn;



typedef struct _GetImageFilePathRequest
{
        char              UserID[16];
        char              SerialNumber[16];
        char              PlayTime[14];
}GetImageFilePathRequest;

typedef struct _GetImageFilePathResponse
{
        uint8_t           result;
        char              SerialNumber[16];
        char              PlayTime[14];
        char              MetaFilePath[80];
        char              RowFilePath[80];
}GetImageFilePathResponse;

//////////////////////////////////////////////////////
//////////////////////////////////////////////////////

typedef struct _StreamServerRegisterRequest
{
    char            ServerName[80];
    char            PrivateIP[15];
    char            StreamIP[15];
    char            CameraIP[15];
    char            ThumbnailIP[15];
    uint16_t        PrivatePort;
    uint16_t        StreamPort;
    uint16_t        CameraPort;
    uint16_t        ThumbnailPort;
    uint16_t        ServerMaxConnect;
    ServerIdentify  t_serverIdentify;
    char            GroupName[80];
    uint8_t        TotalStorageNum;
    char            StoragePath[80];
    uint32_t        MaxStorage;
}StreamServerRegisterRequest;


typedef struct _StreamServerRegisterResponse
{
        uint8_t          result;
}StreamServerRegisterResponse;



//AlarmCast Register Request
typedef struct _AlarmCastServerRegisterRequest
{
    char            ServerName[80];
    char            PrivateIP[15];
    char            StreamIP[15];
    char            CameraIP[15];
    char            ThumbnailIP[15];
    uint16_t        PrivatePort;
    uint16_t        StreamPort;
    uint16_t        CameraPort;
    uint16_t        ThumbnailPort;
    uint16_t        ServerMaxConnect;
    ServerIdentify  t_serverIdentify;
    char            GroupName[80];
    uint8_t        TotalStorageNum;
    char            StoragePath[80];
    uint32_t        MaxStorage;
}AlarmCastServerRegisterRequest;


typedef struct _AlarmCastServerRegisterResponse
{
        uint8_t          result;
}AlarmCastServerRegisterResponse;


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


////////////////////////////
/* ABOUT RANDEZVOUS SERVER */
////////////////////////////

typedef struct _CamRandezvousAuthRequest
{
        char             MacAddress[12];
        char             privateIP[15];
}CamRandezvousAuthRequest;


typedef struct _CamRandezvousAuthResponse
{
        uint8_t       result;
}CamRandezvousAuthResponse;




typedef struct _CamPeerRequest
{
        uint8_t      SessionID;
        uint8_t      NetworkType;
        char             publicIP[15];
        uint16_t         publicPort;
        char             privateIP[15];
        uint16_t         privatePort;
}CamPeerRequest;


typedef struct _CamPeerResponse
{
        uint8_t       result;
        uint8_t       SessionID;
}CamPeerResponse;



typedef struct _CamSendPeerInfoRequest
{
        char             MacAddress[12];
        uint8_t      SessionID;
        char             privateIP[15];
        uint16_t         privatePort;
}CamSendPeerInfoRequest;


typedef struct _CamSendPeerInfoResponse
{
        uint8_t       result;
}CamSendPeerInfoResponse;






typedef struct _CamKeepAliveRequest
{
    //  char             MacAddress[12];
}CamKeepAliveRequest;


typedef struct _CamKeepAliveResponse
{
    //  uint8_t       result;
}CamKeepAliveResponse;


typedef struct _UserTimeStampResponse
{
    uint8_t     result  ;
    uint32_t    sec     ;
    uint32_t    usec    ;
}UserTimeStampResponse;


typedef struct _CamFailConnectRequest
{
        char             MacAddress[12];
        uint8_t      SessionID;
}CamFailConnectRequest;


typedef struct _CamFailConnectResponse
{
        uint8_t       result;
}CamFailConnectResponse;



typedef struct _CamOpenConnectionRequest
{
        uint8_t          SessionID;
        uint8_t      NetworkType;
        char             publicIP[15];
        uint16_t         publicPort;
        char             privateIP[15];
        uint16_t         privatePort;
}CamOpenConnectionRequest;


typedef struct __CamOpenConnectionResponse
{
        uint8_t       result;
}_CamOpenConnectionResponse;






typedef struct _UserRandezvousAuthRequest
{
        char             SerialNumber[16];
        char             privateIP[15];
        uint16_t         privatePort;
        uint16_t         secondprivatePort;
}UserRandezvousAuthRequest;


typedef struct _UserRandezvousAuthExRequest
{
        char             SerialNumber[16];
        char             privateIP[15];
        uint16_t         privatePort;
        uint16_t         secondprivatePort;
        uint8_t          NAT_forward_yn;
        char             NAT_publicIP[15];
        uint16_t         NAT_publicPort;
}  UserRandezvousAuthExRequest;


//typedef struct _UserRandezvousAuthResponse
//{
//        uint8_t          result;
//        char             MacAddress[12];
//        char             CameraID[16];
//        char             CameraPassword[16];
//        uint8_t          CameraType;
//        uint8_t          CodecType;
//        uint8_t          FrameRate;
//        uint32_t         BitRate;
//        char             Priority[10];
//        char             Resolution[10];
//}UserRandezvousAuthResponse;
//
//typedef struct _UserRandezvousAuth4SNSResponse
//{
//        uint8_t          result;
//        char             MacAddress[12];
//        char             CameraID[16];
//        char             CameraPassword[16];
//        uint8_t          CameraType;
//        uint8_t          CodecType;
//        uint8_t          FrameRate;
//        uint32_t         BitRate;
//        char             Priority[10];
//        char             Resolution[10];
//        uint8_t          ConnctionState;  //0:No Previous Session, 1:Previous Session Disconnect
//}UserRandezvousAuth4SNSResponse;


/*
typedef struct _UserSendPeerInfoRequest
{
        uint8_t      SessionID;
        uint8_t          NetworkType;
        char             PublicIP[15];
        uint16_t         PublicPort;
        char             PrivateIP[15];
        uint16_t         PrivatePort;
}UserSendPeerInfoRequest;


typedef struct _UserSendPeerInfoResponse
{
        uint8_t       result;
}UserSendPeerInfoResponse;
*/


/* add it 2012.05.18 */
typedef struct _UserPeerInfoRequest
{
    char                SerialNumber[16];
}UserPeerInfoRequest;


typedef struct _UserPeerInfoResponse
{
        uint8_t      result;
    uint8_t      SessionID;
    uint8_t          NetworkType;
    char             PublicIP[15];
    uint16_t         PublicPort;
    char             PrivateIP[15];
    uint16_t         PrivatePort;
}UserPeerInfoResponse;




typedef struct _UserConnectPeerRequest
{
        uint8_t      SessionID;
          char                SerialNumber[16];
}UserConnectPeerRequest;


typedef struct _UserConnectPeerResponse
{
        uint8_t       result;
}UserConnectPeerResponse;


typedef struct _UserDisConnectPeerRequest
{
        uint8_t             SessionID;
        char                SerialNumber[16];
}UserDisConnectPeerRequest;


typedef struct _UserDisConnectPeerResponse
{
        uint8_t       result;
}UserDisConnectPeerResponse;


typedef struct _UserKeepAliveRequest
{
//      char          SerialNumber[12];
}UserKeepAliveRequest;


typedef struct _UserKeepAliveResponse
{
//      uint8_t       result;
}UserKeepAliveResponse;



typedef struct _UserFailConnectRequest
{
        char                SerialNumber[16];
        uint8_t             SessionID;
}UserFailConnectRequest;


typedef struct _UserFailConnectResponse
{
        uint8_t       result;
}UserFailConnectResponse;




typedef struct _RelayConnectionRequest
{
    char              SerialNumber[16];
}RelayConnectionRequest;

typedef struct _RelayConnectionResponse
{
    uint8_t           result;
    char              RelayServerAddress[15];
    uint16_t          RelayServerPort;
}RelayConnectionResponse;


typedef struct _UserTryRelayRequest
{
    char              SerialNumber[16];
}UserTryRelayRequest;

typedef struct _UserTryRelayResponse
{
    uint8_t           result;
}UserTryRelayResponse;


typedef struct _CamTryRelayRequest
{
    char              MacAddress[12];
}CamTryRelayRequest;

typedef struct _CamTryRelayResponse
{
    uint8_t           result;
}CamTryRelayResponse;


typedef struct _UserSessionTeerDownRequest
{
    uint8_t           result;
    char              SerialNumber[16];
}UserSessionTeerDownRequest;





/* RANDEZVOUS WITH MANAGER SERVER */
typedef struct _CamManagerAuthRequest
{
        char          MacAddress[12];
}CamManagerAuthRequest;

typedef struct _CamManagerAuthResponse
{
        uint8_t       result;
}CamManagerAuthResponse;

typedef struct _CamManagerAuthResponseEx
{
        uint8_t       result;
        char          SerialNumber[16];
        char          ServiceType [2];
        uint8_t       CameraKey;
}CamManagerAuthResponseEx;

/* RANDEZVOUS WITH MANAGER SERVER */
//typedef struct _CamManagerAuthExResponse
//{
//        uint8_t       result;
//        uint8_t       NAT_forward_yn;
//        char          NAT_publicIP[15];
//        uint16_t      NAT_publicPort;
//}CamManagerAuthExResponse;

typedef struct _UserManagerAuthRequest
{
        char             SerialNumber[16];
}UserManagerAuthRequest;


typedef struct _UserManagerAuthResponse
{
        uint8_t       result;
        char              ServiceType[2];
        char              MacAddress[12];
        char              CameraID[16];
        char              CameraPassword[16];
        uint8_t       CameraType;
        uint8_t       CodecType;
        uint8_t       FrameRate;
        uint32_t          BitRate;
        char              Priority[10];
        char              Resolution[10];
}UserManagerAuthResponse;


typedef struct _UserManagerAuthResponseEx
{
        uint8_t       result;
        char          ServiceType[2];
        char          MacAddress[12];
        char          CameraID[16];
        char          CameraPassword[16];
        uint8_t       CameraType;
        uint8_t       CodecType;
        uint8_t       FrameRate;
        uint32_t      BitRate;
        char          Priority[10];
        char          Resolution[10];
        //////////////////////////////
        uint8_t       NAT_forward_yn;
        char          NAT_publicIP[15];
        uint16_t      NAT_publicPort;
        //////////////////////////////
}UserManagerAuthResponseEx;


typedef struct _CamControlConnectRequest
{
    uint64_t            SessionID;
    ServerIdentify    t_serverIdentify;
    char                  MacAddress[12];
}CamControlConnectRequest;


typedef struct _CamControlConnectResponse
{
        uint8_t       result;
}CamControlConnectResponse;



typedef struct _CamControlDisConnectRequest
{
    uint64_t            SessionID;
    ServerIdentify    t_serverIdentify;
    char                MacAddress[12];
}CamControlDisConnectRequest;


typedef struct _CamControlDisConnectResponse
{
        uint8_t       result;
}CamControlDisConnectResponse;



typedef struct _ClientConnectNotiRequest
{
         uint8_t           devicetype;
         uint64_t         SessionID;
      ServerIdentify    t_serverIdentify;
        char               MacAddress[12];
        char               ClientAddress[15];
}ClientConnectNotiRequest;


typedef struct _ClientConnectNotiResponse
{
        uint8_t       result;
}ClientConnectNotiResponse;



typedef struct _ClientDisConnectNotiRequest
{
    uint8_t             devicetype;
    uint64_t            SessionID;
    ServerIdentify    t_serverIdentify;
    char                MacAddress[12];
}ClientDisConnectNotiRequest;


typedef struct _ClientDisConnectNotiResponse
{
        uint8_t       result;
}ClientDisConnectNotiResponse;





typedef struct _RendezvousErrorNotiRequest
{
    uint8_t         networkType;
    uint8_t         deviceType;
    uint8_t         errorCode;
    uint32_t            waitingTime;
    char                errstr[128];
    char                MacAddress[12];
    char                CamPublicAddress[15];
    char                CamPrivateAddress[15];
    char                UserPublicAddress[15];
    char                UserPrivateAddress[15];
}RendezvousErrorNotiRequest;


typedef struct _RendezvousErrorNotiResponse
{
    uint8_t       result;
}RendezvousErrorNotiResponse;




/* SETUP CONTROLLER SERVER WITH MANAGER SERVER */
// !!!! SETUP_REGISTER_REQUEST 메시지는 기존의 스트리머 등록과 동일한 구조를 취함
// !!!! 그외 기타 프로토콜은 유사하더라도 각기 구조체 선언

typedef struct _SetupKeepAliveRequest
{
      uint8_t           result;    //enum ResultCode;
      ServerIdentify    t_serverIdentify;
}SetupKeepAliveRequest;


typedef struct _SetupKeepAliveResponse
{
        uint8_t       result;
}SetupKeepAliveResponse;







typedef struct _SetupCamAuthVideoInfo
{
        uint8_t     ServiceType;  //10, 20, 30
        uint8_t     CodecType;
        uint32_t    Bitrate;
        char        Resolution[10];
        char        Priority[10];
        uint8_t     FrameRate;
        uint8_t     Gop;
}SetupCamAuthVideoInfo;


typedef struct _SetupCamAuthScheduleInfo
{
        uint8_t     ServiceScheduleStartDW;
        uint8_t     ServiceStartH;
        uint8_t     ServiceStartM;
        uint8_t     ServiceStopH;
        uint8_t     ServiceStopM;
}SetupCamAuthScheduleInfo;


typedef struct _SetupCamAuthRequest
{
        char        MacAddress[12];
        char        FirmwareVersion[32];
}SetupCamAuthRequest;


typedef struct _SetupCamAuthResponse
{
        uint8_t     result;
        uint8_t     CameraType;     // axis:1 hitron basic:33 hitron full:45
        char        ServiceType[2];       // 10, 20, 30, 40, 50, 60
        char        SerialNumber[16];
        char        EventServerIP[15];
        uint16_t    EventServerPort;
        uint8_t     AudioUse;
        uint8_t     AudioCodec;
        uint8_t     ArmState;         // On(1)/Off(0)
        uint8_t     EventUse;
        uint8_t     EventMotionUse;
        uint8_t     EventPIRUse;
        uint8_t     MotionSensitivity;
        uint8_t     LampUse;
        uint8_t     LampLuminance;
        uint8_t     LampActionUse;      // add it 2012. 07. 31
        uint8_t     LampActionLuminance;    // add it 2012. 07. 31
        uint32_t    PreTimeSecond;
        uint32_t    PostTimeSecond;
        uint8_t     PlayNo;
        uint32_t    PlayTime;
        char        Service_Start_Date[20]; // add it 2012. 07. 31
        char        Service_End_Date[20];   // add it 2012. 07. 31
        char        Service_Pause_Date[20]; // add it 2012. 07. 31
        char        Service_Pause_End_Date[20]; // add it 2012. 07. 31
        uint8_t     VideoInfoCount;
        uint8_t     ScheduleCount;
        //SetupCamAuthVideoInfo [VideoInfoCount];
        //SetupCamAuthScheduleInfo [ScheduleCount];
}SetupCamAuthResponse;



typedef struct _SetupCamConnectRequest
{
       uint64_t         SessionID;
     ServerIdentify   t_serverIdentify;
     char             SerialNumber[16];
}SetupCamConnectRequest;


typedef struct _SetupCamConnectResponse
{
     uint8_t          result;
}SetupCamConnectResponse;


typedef struct _SetupDisCamConnectRequest
{
     uint64_t         SessionID;
     ServerIdentify   t_serverIdentify;
     char             SerialNumber[16];
}SetupCamDisConnectRequest;


typedef struct _SetupCamDisConnectResponse
{
     uint8_t          result;
}SetupCamDisConnectResponse;



typedef struct _SetupServiceEndListRequest
{
    uint8_t         groupid;
    uint8_t         serverid;
    //uint8_t       servertype;
    char            ServerType[3];
}SetupServiceEndListRequest;


typedef struct _SetupServiceEndListResponse
{
    uint8_t          result;    //enum ResultCode;
    uint32_t         EndCamNumber;
    // char  SerialNumber[16] * EndcamNumber
}SetupServiceEndListResponse;


typedef struct _SetupServiceChangeListRequest
{
    uint8_t         groupid;
    uint8_t         serverid;
    //uint8_t       servertype;
    char              ServerType[3];
}SetupServiceChangeListRequest;


typedef struct _SetupServiceChangeListResponse
{
    uint8_t          result;    //enum ResultCode;
    uint32_t         EndCamNumber;
    // char  SerialNumber[16] * EndcamNumber
}SetupServiceChangeListResponse;


typedef struct _SetupCamFirmwareStateRequest
{
    char              MacAddress[12];
    uint8_t           Firmware_update_State;
}SetupCamFirmwareStateRequest ;


typedef struct _SetupCamFirmwareStateResponse
{
    uint8_t          result;    //enum ResultCode;
}SetupCamFirmwareStateResponse;

typedef struct _SetupCamNATSetInfoRequest
{
    char              MacAddress[12];
    uint8_t           NAT_SET_SATAE;
    char              NAT_PUBLIC_IP[15];
    uint16_t          NAT_PUBLIC_PORT;
}SetupCamNATSetInfoRequest ;


typedef struct _SetupCamNATSetInfoResponse
{
    uint8_t          result;    //enum ResultCode;
}SetupCamNATSetInfoResponse;

//[SETUP]---------------------------------------[END]


typedef struct _RandServiceEndListRequest
{
    uint8_t         groupid;
    uint8_t         serverid;
//  uint8_t         servertype;
    char              ServerType[3];
}RandServiceEndListRequest;


typedef struct _RandServiceEndListResponse
{
    uint8_t          result;    //enum ResultCode;
    uint32_t         EndCamNumber;
    // char  MacAddress[12] * EndcamNumber
}RandServiceEndListResponse;


typedef struct _RandServiceChangeListRequest
{
        uint8_t         groupid;
        uint8_t         serverid;
    //  uint8_t         servertype;
        char              ServerType[3];
}RandServiceChangeListRequest;


typedef struct _RandServiceChangeListResponse
{
    uint8_t          result;    //enum ResultCode;
    uint32_t         EndCamNumber;
    // char  MacAddress[12] * EndcamNumber
}RandServiceChangeListResponse;

//[AlarmCast] +++++++++++++++++++++++++++++++++++++++++
typedef struct _AlarmCastCameraSetRequest
{
        char         SerialNumber[16];
        uint8_t      bUsePIR ;
        uint8_t      bUseMotion ;
        uint8_t      bUseTrigger ;
        uint8_t      bUsePush ;

}AlarmCastCameraSetRequest;

typedef struct _AlarmCastCameraSetResponse
{
    uint8_t      result;    //enum ResultCode;
}AlarmCastCameraSetResponse;


typedef struct _AlarmCastEventRequest
{
        char         SerialNumber[16];
        uint8_t      eType ;
}AlarmCastEventRequest;

typedef struct _AlarmCastEventResponse
{
    uint8_t      result;      // enum ResultCode;
        uint16_t     savecount;   // save alarm count
        uint16_t     limitcount;    // limit alarm count
}AlarmCastEventResponse;


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//[SNS_PROTOCOL] ischoi -20130527
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
typedef struct _SNSGWCameraSetRequest
{
    uint8_t      ArmedState;  // Armed state ;
}SNSGWCameraSetRequest;

typedef struct _SNSGWCameraSetResponse
{
    uint8_t      result;      // enum ResultCode;
}SNSGWCameraSetResponse;

//typedef struct _SNSGWEventRequest
//{
//      //길이가 정해지지않은 Serial Number 길이 // Armed state ;
//}SNSGWEventRequest;

typedef struct _SNSGWEventResponse
{
    uint8_t      result;      // enum ResultCode;
}SNSGWEventResponse;

typedef struct _SNSGWEventInfo
{
    char        SerialNumber[16];
    char        IPAddress[15];
    uint16_t    Port;
}SNSGWEventInfo;


////////////////Camera Service Status //////////////////
typedef struct
{
    char        livecast        ;
    char        rendezvous      ;
    char        setup           ;
}   CamSt   ;

typedef struct
{
    char        SerialNumber[16];
    CamSt       st              ;
}   CamSvcSt    ;

typedef struct
{
    uint8_t     result          ;
    uint16_t    CamCount        ;
}   ServiceStatusResponse ;

typedef struct
{
    uint8_t     result          ;
    char        livecast        ;
    char        rendezvous      ;
    char        setup           ;
}   CameraStatusResponse ;

#define _MAX_SVCLIST_   1000
#define SZ_SVC          sizeof(CamSvcSt)
#define _SVCSTR_       (_MAX_SVCLIST_ * SZ_SVC)

#define _SVC_START_     120     // after seconds
#define _SVC_RETRY_     8       // retry seconds
///////////////////////////////////////////////////////

#pragma pack(pop)


#endif

