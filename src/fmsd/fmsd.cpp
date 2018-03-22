////////////// ====================================================================== ////////////////////
//
//                                      Unstable version !!
//
////////////// ====================================================================== ////////////////////
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>

#include "task_manager.h"
#include "fmsd_config.h"

CServerLog  pLog    ;
CConfig     pcfg    ;

bool        IsALive = true ;
pid_t       g_pid   ;

void sig_proc ( int signo )
{
    switch ( signo )
    {
        case SIGQUIT:
        case SIGTERM:
        case SIGABRT:
        case SIGPIPE:
        case SIGINT : // Ctrl + C
        {
            IsALive = false ;

            if (g_pid > 0)
            {
                kill(g_pid, SIGINT);
            }
            exit(0);
        }
        break ;

        default :
        {
            // do nothing ...
        }
        break ;
    }
}

int main(int argc, char** argv)
{
    if ( (argc == 2) && ( (strcmp(argv[1], "-v") == 0) || (strcmp(argv[1], "-V") == 0)) )
    {
        fprintf ( stderr, "%s (Release Version).\n", _FMS_DAEMON_V_     );
        fprintf ( stderr, "released by SysUp Inc. All Rights Reserved\n");
        fprintf ( stderr, "Copyright SysUp Inc. All Rights Reserved\n"  );
        return 0 ;
    }

#if DAEMON_VERSION
    g_pid = fork();
    if ( g_pid == -1 )
    {
        fprintf ( stderr, "fork() is failed.. \n" ) ;
        return -1 ;
    }
    else if ( g_pid != 0 )
    {
        return 0;
    }
    setpgrp();

    while (IsALive)
    {
        g_pid = fork();
        if ( g_pid == -1 )
        {
            fprintf ( stderr, "fork() is failed.. \n" ) ;
            return -1 ;
        }
        else if ( g_pid == 0 )
        {
            break;
        }
        else if ( g_pid != 0 )
        {
            signal ( SIGINT , sig_proc ) ;
            signal ( SIGQUIT, sig_proc ) ;
            signal ( SIGTERM, sig_proc ) ;
            signal ( SIGABRT, sig_proc ) ;
            signal ( SIGPIPE, sig_proc ) ;
            waitpid( g_pid  , NULL,  0 ) ;
            SDL_Delay(3000);
        }
    }
    setpgrp();
#endif

    char cfgName[MIN_BUF] = {0,};
    if ( argc >= 2 )
         memcpy(cfgName, argv[1], strlen(argv[1]));
    else memcpy(cfgName, DEFAULT_CONFIG_FILE, strlen(DEFAULT_CONFIG_FILE));

    pcfg.setLoadFile (cfgName);

    if ( pcfg.getLoadConfig() )
    {
        pLog.SetupLog (pcfg.getLogDir(), "fmsd", pcfg.getLogLevel(), pcfg.getLogSize());
        pLog.Print    (LOG_WARNING, 0, NULL, 0, "%s Starting ...", _FMS_DAEMON_V_);

        CTaskManager * pTaskM = new CTaskManager();
        while (IsALive)
        {
            sleep(1);
        }

        pLog.Print(LOG_WARNING, 0, NULL, 0, "** %s is shutdown. **", _FMS_DAEMON_V_);
        if (pTaskM) delete pTaskM   ;
        pLog.Print(LOG_WARNING, 0, NULL, 0, "%s Closing ...", _FMS_DAEMON_V_);
    }
    return 0;
}
