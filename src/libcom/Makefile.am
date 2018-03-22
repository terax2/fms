#lib_LIBRARIES = libhvaas.a
#libhvaas_a_SOURCES = main.cpp

lib_LTLIBRARIES = libhvaas.la
libhvaas_la_SOURCES = Utility.h       \
                      Utility.cpp     \
                      ServerLog.h     \
                      ServerLog.cpp   \
                      ParseConfig.h   \
                      ParseConfig.cpp \
                      Network.h       \
                      Network.cpp     \
                      Epoll.h         \
                      Epoll.cpp       \
                      tripleDes.h     \
                      tripleDes.cpp   \
                      mzdes.h         \
                      mzdes.cpp


libhvaas_la_LDFLAGS = -module -export-dynamic $(LDFLAGS) -m64

#g++ -shared -o libhvaas.so libhvaas.a
