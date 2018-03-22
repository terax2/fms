bin_PROGRAMS = fmsd

fmsd_SOURCES = fmsd.cpp            \
               fmsd.h              \
               fmsd_config.cpp     \
               fmsd_config.h       \
               groupsock.cpp       \
               groupsock.h         \
               msg_queue.cpp       \
               msg_queue.h         \
               lst_queue.cpp       \
               lst_queue.h         \
               media_time.h        \
               media_node.h        \
               media_frame.h       \
               media_feeder.cpp    \
               media_feeder.h      \
               media_sink.h        \
               mutex.cpp           \
               mutex.h             \
               camera.h            \
               timestamp.cpp       \
               timestamp.h         \
               task_manager.cpp    \
               task_manager.h      \
               fms_interface.cpp   \
               fms_interface.h     \
               recv_manager.cpp    \
               recv_manager.h      \
               recv_smp.cpp        \
               recv_smp.h          \
               polling_manager.cpp \
               polling_manager.h   \
               polling_smp.cpp     \
               polling_smp.h       \
               tcpsock.cpp         \
               tcpsock.h           \
               sendmsg.cpp         \
               sendmsg.h           \
               mysql_inf.cpp       \
               mysql_inf.h         \
               DBInterface.cpp     \
               DBInterface.h       \
               datamap.cpp         \
               datamap.h           \
               dualbuff.cpp        \
               dualbuff.h


fmsd_LDADD = -L/fsapp/FMS/lib -lhvaas -ltcmalloc -lmysqlclient
fmsd_CPPFLAGS = -I/fsapp/FMS/include -Wall -DDAEMON_VERSION=1 $(SDL_CFLAGS) -m64 -fno-strict-aliasing
fmsd_LDFLAGS =  

#-lpcrecpp
#               tsdbhttp.cpp        \
#               tsdbhttp.h          \
#               base64.cpp          \
#               base64.h            \
#               CSmtp.cpp           \
#               CSmtp.h             \
#               fmsd_load.cpp       \
#               fmsd_load.h
#               bypass_manager.cpp  \
#               bypass_manager.h    \
#               modebus_manager.cpp \
#               modebus_manager.h   \
#               snmp_manager.cpp    \
#               snmp_manager.h 
