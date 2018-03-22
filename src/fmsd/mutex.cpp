/*
 *****************************************************************************

    Copyright (c) 2013, Everview, Inc.
    All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are
    met:

        Redistributions of source code must retain the above copyright notice,
    this list of conditions and the following disclaimer.  Redistributions
    in binary form must reproduce the above copyright notice, this list of
    conditions and the following disclaimer in the documentation and/or
    other materials provided with the distribution.

    Neither the name of Blackwave, Inc. nor the names of its
    contributors may be used to endorse or promote products
    derived from this software without specific prior written
    permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND
    CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
    INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
    MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
    DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS
    BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
    EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
    TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
    DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
    ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
    TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
    THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
    SUCH DAMAGE.

 ******************************************************************************
*/

#include "mutex.h"

// We encapsulate two items...Locks (pthread_mutex_t) and Semaphores (sem_t).
//
// Set Debug Level to 6 to get timing of waits dumped to the log.

Mutex::Mutex() {
    pthread_mutex_init(&theLock, NULL);
}

Mutex::~Mutex() {
    pthread_mutex_destroy(&theLock);
}

void Mutex::lock() {
    int status;
    status = pthread_mutex_lock(&theLock);
    if (status != 0) {
        perror( "pthread_mutex_lock error");
    }
}

void Mutex::unlock() {
    int status;
    status = pthread_mutex_unlock(&theLock);
    if (status != 0) {
        perror( "pthread_mutex_unlock error");
    }
}

//void Semaphore::initialize() {
//    id = NULL;
//    sem_init(&theSemaphore, 0, 0);
//}
//
//Semaphore::Semaphore() {
//    initialize();
//}
//
//Semaphore::~Semaphore() {
//    sem_destroy(&theSemaphore);
//}
//
//void Semaphore::P() {
//    int status;
//    do {
//        status = sem_wait(&theSemaphore);
//
//        if (status != 0 && errno != EINTR) {
//            perror( "sem_wait error");
//            break;
//        }
//
//    }   while (status != 0 && errno == EINTR);
//}
//
//void Semaphore::P(const char *i) {
//    id = i;
//    P();
//}
//
//void Semaphore::V() {
//    int status;
//    status = sem_post(&theSemaphore);
//    if (status != 0) {
//        perror( "sem_post error");
//    }
//}
//
//bool Semaphore::anyoneWaiting() {
//    int result;
//    int retcode = sem_getvalue(&theSemaphore, &result);
//
//    if (retcode != 0) {
//        perror( "sem_getvalue error");
//        return 0;
//    }
//    return result == 0;
//}
