bin_PROGRAMS = test

test_SOURCES = testfeng.cpp \
               our_md5.cpp  \
               our_md5.h    \
               shlib.cpp    \
               shlib.h      \
               Base64.cpp   \
               Base64.h     \
               datamap.cpp  \
               datamap.h    \
               mutex.cpp    \
               mutex.h

#main.cpp shlib.h

test_LDADD = -L/usr/local/lib -lpcrecpp -lcurl -lsnmp
test_CPPFLAGS = $(SDL_CFLAGS) -m64
#-lboost_regex
#-I/usr/include/boost
