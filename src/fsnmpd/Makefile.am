bin_PROGRAMS = fsnmpd

fsnmpd_SOURCES = fsnmpd.cpp          \
                 fsnmpd.h            \
                 fsnmpd_config.cpp   \
                 fsnmpd_config.h     \
                 groupsock.cpp       \
                 groupsock.h         \
                 lst_queue.cpp       \
                 lst_queue.h         \
                 mutex.cpp           \
                 mutex.h             \
                 camera.h            \
                 task_manager.cpp    \
                 task_manager.h      \
                 datamap.cpp         \
                 datamap.h           \
                 mysql_inf.cpp       \
                 mysql_inf.h         \
                 DBInterface.cpp     \
                 DBInterface.h



fsnmpd_LDADD = -L/fsapp/FMS/lib -lhvaas -ltcmalloc -lmysqlclient
fsnmpd_CPPFLAGS = -I/fsapp/FMS/include -Wall -DDAEMON_VERSION=1 $(SDL_CFLAGS) -m64 -fno-strict-aliasing
fsnmpd_LDFLAGS =  

#-lpcrecpp
#               tsdbhttp.cpp        \
#                 timestamp.cpp       \
#                 timestamp.h         \
#                 msg_queue.cpp       \
#                 msg_queue.h         \
#                 media_time.h        \
#                 media_node.h        \
#                 media_frame.h       \
#                 media_sink.h        \
#                 tcpsock.cpp         \
#                 tcpsock.h           \
#                 media_feeder.cpp    \
#                 media_feeder.h      \
#               tsdbhttp.h          \
#               fmsd_load.cpp       \
#               fmsd_load.h
#               bypass_manager.cpp  \
#               bypass_manager.h    \
#               modebus_manager.cpp \
#               modebus_manager.h   \
#               snmp_manager.cpp    \
#               snmp_manager.h 
