
/**********************************************************************************************************
 *  A. Standard Includes
*********************************************************************************************************/
#include <string.h>
#include <stdlib.h>
#include <memory.h>
#include <stdio.h>
#include <fcntl.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/prctl.h>
#include <pthread.h>
#include <sys/time.h>   
#include <sys/ioctl.h>   



/**********************************************************************************************************
*  sepecific Includes
*********************************************************************************************************/

#include "moniter.h"



/**********************************************************************************************************
 *  C. Object like Macro
*********************************************************************************************************/



/**********************************************************************************************************
 *  D. Function like Macro
*********************************************************************************************************/

/**********************************************************************************************************
 *  E. Local Function Declarations
*********************************************************************************************************/
static void mt_CreatMoniter();
static void mt_RunMoniter();
static void mt_StartProc();
static bool_t mt_RecvMessageFun();
static pid_t mt_GetProcPid(char *name);
static bool_t mt_ProcFind(char * procName, uint16_t procPid);
static void mt_MakeFifo();
static void mt_OpenFifo();
static uint16_t mt_ListenFifo();
static bool_t mt_IsDigit();
static bool_t mt_GetCurrentProcInfo();

/**********************************************************************************************************
 *  F. Local Object/Variable
*********************************************************************************************************/

static MT_zExeInfo_t MT_zShouldBeRunExe;
static MT_zExeInfo_t MT_zOldExe;
static MT_zExeInfo_t MT_zNowExe;

static uint16_t pipe_fd = -1; // Pipe file descriptor, conduit files in different ways in the semi-open process and management of the process.



/**********************************************************************************************************
 *  G. Exported Object/Variable
 *********************************************************************************************************/



/**********************************************************************************************************
 *  Local Function Implementations
**********************************************************************************************************/

 /*********************************************************************************************************
 * @brief   Open the fifofile read_only mode and cycle run the main program version management code.
 *
 * @usage
 *
 * @return   
 *         
********************************************************************************************************/
static void mt_CreatMoniter()
{

    mt_OpenFifo();
    while (1)
    {
        sleep(5);
        mt_RunMoniter();
    }

}

 /*********************************************************************************************************
 * @brief   Open the fifofile read_only mode.
 *
 * @usage
 *
 * @return   
 *       
********************************************************************************************************/
static void mt_OpenFifo()
{
    const char *fifo_name = FIFO_PATH;  
    uint16_t open_mode = O_RDONLY | O_NONBLOCK; 
    pipe_fd = open(fifo_name, open_mode);

    if (pipe_fd > 0)
    {
        printf("Process %d opening FIFO O_RDONLY succed\n", getpid()); 
    }
    else
    {
        printf("Process %d opening FIFO failed\n", getpid());
        exit(0);
    }
}

 /*********************************************************************************************************
 * @brief   Detecting whether the main program is running and the main program needs replacement.
 *                  If necessary, reboot the primary process or replacement main course.
 * @usage
 *
 * @return   
 *          
********************************************************************************************************/
static void mt_RunMoniter()
{

    if(mt_GetProcPid(NULL) == 0 && strcmp(MT_zNowExe.procName, MT_zShouldBeRunExe.procName) != 0 || (!mt_RecvMessageFun()) )
    {   
        mt_StartProc();
        if ( strcmp(MT_zOldExe.procName, MT_zShouldBeRunExe.procName) != 0 && mt_GetProcPid(NULL) == 0  )
        {
            memcpy(&MT_zShouldBeRunExe, &MT_zOldExe, sizeof(MT_zExeInfo_t));
        }
    }

    printf("Moniter_run...\n");

}

 /*********************************************************************************************************
 * @brief   Receive messages from the primary process, the heartbeat packet withheld or new program information.
 *
 * @usage
 *
 * @return   TRUE if moniter process recevie message.
 *          FALSE otherwise.
 *          
********************************************************************************************************/
static bool_t mt_RecvMessageFun()
{
    uint16_t res = 0;
    uint16_t len = 0;
    char *tmp; 
    char newExeCmd[MAX_CMD_LINE] = {0}; 
    uint16_t open_mode = O_RDONLY; 

   if(pipe_fd != -1)  
    { 
            res = mt_ListenFifo(); 
                  
            if (res > 0 )
            {
                len = read(pipe_fd, newExeCmd, MAX_CMD_LINE);
                if (strcmp(newExeCmd, TICK_MESSAGE) != 0 && len > 0)
                {
                    memset(&MT_zShouldBeRunExe, 0, sizeof(MT_zShouldBeRunExe));
                    memcpy(&MT_zShouldBeRunExe.cmdLine, newExeCmd, MAX_CMD_LINE);
                    tmp = strrchr(MT_zShouldBeRunExe.cmdLine, '/');
                    tmp += 1;
                    memcpy(&MT_zOldExe, &MT_zNowExe,sizeof(MT_zExeInfo_t));
                    memcpy(MT_zShouldBeRunExe.procName, tmp, strlen(tmp));
                }
                if (len > 0)
                {
                    return TRUE;
                }

            } 
            else 
            {
                if (res == 0)
                {
                    printf("Timeout...\n");
                    printf("Read FIFO with nothing or failed\n");
                }
            }
    }  
    else
    {
        printf("FIFO is not opened\n");
    }

    return FALSE; 
}

 /*********************************************************************************************************
 * @brief   Start of main process should run.
 *
 * @usage
 *
 * @return   
 *          
********************************************************************************************************/
static void mt_StartProc()
{
    uint16_t pid = fork();
    if(pid == 0)
        {
            pid = fork();
            if(pid == 0)
            {
                printf("It will run: %s\n", MT_zShouldBeRunExe.cmdLine);
                execl(MT_zShouldBeRunExe.cmdLine, MT_zShouldBeRunExe.procName, NULL);
                exit(0);
            }
            else
            {
               exit(0);
            }
        }
        else
        {
            wait(NULL);
        }
}

 /*********************************************************************************************************
 * @brief   Get process ID.
 *                  if the argument is NULL,  get the process ID of the main program should be run 
 *                  otherwise acquire the appropriate process name of the process ID
 *
 * @param   procName: the name of process want to be get
 * @usage
 *
 * @return   The pid if the process be found.
 *          0 otherwise.
 *          
********************************************************************************************************/
static pid_t mt_GetProcPid(char *procName)
{
     DIR * dp;
     char *dir = PROCES;
     char olddir[MAX_CMD_LINE];
     struct dirent * dirp;
     uint16_t procPid;
     getcwd(olddir, MAX_CMD_LINE);
     chdir(dir);
     if((dp = opendir(dir)) != NULL) 
     {
          while ((dirp = readdir(dp)) != NULL) 
         {
            char data[MAX_PROC_NAME];
            sprintf(data, "%s", dirp->d_name); 
            if((mt_IsDigit(data)))
            {
                procPid = atoi(dirp->d_name);
                if (mt_ProcFind(procName, procPid))
                {
                    return procPid;
                } 
            }
          }

     }
      closedir(dp);
      chdir(olddir);
      return 0;
}

 /*********************************************************************************************************
 * @brief   Determine whether a full numeric string.
 *
 * @param   name: string detected
 * @usage
 *
 * @return   TRUE if name is digit.
 *          FALSE otherwisw.
 *          
********************************************************************************************************/
static bool_t mt_IsDigit(char *name)
{
        uint16_t len, i;
        len = strlen(name);

        if (len > 0)
        {
            for (i = 0; i < len; i++)
            {
                if (name[i] < '0' || name[i] > '9')
                {
                    break;
                }
            }
            if (i == len)
            {
                return TRUE;
            }
        }
        return FALSE;
}

 /*********************************************************************************************************
 * @brief   Process name comparison to determine whether the same process.
 *
 * @param   procName: the name of process want to find
 * @param   procPid: the pid of process need to compare
 * @usage
 *
 * @return   TRUE if process be found.
 *          FALSE otherwise.
 *          
********************************************************************************************************/
static bool_t mt_ProcFind(char * procName, uint16_t procPid)
{
    char buffer[4096], *p, bfindprocname[MAX_PROC_NAME];
    uint16_t fd, len, bRet = 0;
    sprintf(buffer, "%d/stat", procPid);
    fd = open(buffer, O_RDONLY);
    if(fd != -1)
    {
        memset(buffer, '\0', sizeof(buffer));
        len = read(fd, buffer, sizeof(buffer)-1);
        close(fd);
        if(len >0)
        {
            p = buffer;
            p = strrchr(p, '(');
            {
                char *q = strrchr(p, ')');
                uint16_t len = q - p - 1;
                if (len >= sizeof(bfindprocname))
                {
                    len = sizeof(bfindprocname)-1;
                }
                memcpy(bfindprocname, p + 1, len);
                bfindprocname[len] = 0;
            }

            if (procName == NULL)
            {

                if (strcmp(bfindprocname, MT_zShouldBeRunExe.procName) == 0)
                {
                    MT_zNowExe.procPid = procPid;
                    memcpy(MT_zNowExe.procName, bfindprocname, sizeof(bfindprocname));
                    bRet = 1;
                }
            }
            else
            {
                if (strcmp(bfindprocname, procName) == 0)
                    bRet = 1;
            }
        }

    }
    if (bRet)
    {

        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

 /*********************************************************************************************************
 * @brief   Get information about the current primary process.
 *
 * @usage
 *
 * @return   TRUE if no error.
 *         FALSE otherwise.
 *          
********************************************************************************************************/
static bool_t mt_GetCurrentProcInfo()
{
    memset(&MT_zNowExe, 0, sizeof(MT_zNowExe));
    memset(&MT_zShouldBeRunExe, 0, sizeof(MT_zShouldBeRunExe));
    pid_t pid = getpid();
    char buffer[MAX_CMD_LINE] = {0};
    sprintf(buffer, "/proc/%d/stat", pid);
    uint16_t fd, len,size;
    char * p, *q;
    fd = open(buffer, O_RDONLY);
    if(fd != -1)
    {
        memset(buffer, '\0', sizeof(buffer));
        size = read(fd, buffer, sizeof(buffer));
        close(fd);
        if(size > 0)
        {
            p = buffer;
            p = strrchr(p, '(');
            {
                q = strrchr(p, ')');
                len = q - p - 1;
                if (len >= MAX_PROC_NAME)
                {
                    len--;
                }
                memcpy(MT_zNowExe.procName, p + 1, len);
                MT_zNowExe.procName[len] = 0;
                getcwd(buffer,MAX_CMD_LINE);
                sprintf(MT_zNowExe.cmdLine, "%s/%s", buffer ,MT_zNowExe.procName);
                memcpy(&MT_zShouldBeRunExe, &MT_zNowExe, sizeof(MT_zExeInfo_t));

                return TRUE;

            }
        }

    }
    return FALSE;
}

 /*********************************************************************************************************
 * @brief   Create a named pipe file.
 *
 * @usage
 *
 * @return   
 *          
********************************************************************************************************/
static void mt_MakeFifo()
{
    const char *fifo_name = FIFO_PATH;     
    uint16_t res = 0;  
      
        if(access(fifo_name, F_OK) == -1)  
        {  
            res = mkfifo(fifo_name, 0777);  
            if(res != 0)  
            {  
                fprintf(stderr, "Could not create fifo %s\n", fifo_name);   
            }
            else
            {
                printf("Create fifo succed\n");
            }
        }
        else
        {
            printf("Fifo exist ! \n");
        }      
}

 /*********************************************************************************************************
 * @brief   Named pipes monitor whether the file is readable.
 *
 * @usage
 *
 * @return   Number of file descriptors data can be read.
 *          
********************************************************************************************************/
static uint16_t mt_ListenFifo()
{
    uint16_t result;
    struct timeval timeout;
    fd_set recvfds;
    timeout.tv_sec = FIFO_LISTENTIME_SEC;
    timeout.tv_usec = FIFO_LISTENTIME_USEC;
    FD_ZERO(&recvfds);
    FD_SET(pipe_fd, &recvfds);

    result = select(FD_SETSIZE, &recvfds, NULL, NULL, &timeout);
    FD_ZERO(&recvfds);

    return result;
}


/**********************************************************************************************************
 *  Public Function Implementations
*********************************************************************************************************/

 /*********************************************************************************************************
 * @brief   Create a named pipe file and the main program version management process
 *
 * @usage
 *
 * @return   
 *          
********************************************************************************************************/
void MT_CodeMoniter()
{
    mt_MakeFifo();

    if (mt_GetProcPid(MONITER_NAME) == 0)
    {
        mt_GetCurrentProcInfo();
        #if 1
        int pid = fork();
        if(pid == 0)
        {
            pid = fork();
            if(pid == 0)
            {
                prctl(PR_SET_NAME, MONITER_NAME);
                mt_CreatMoniter();
            }
            else
            {
               exit(0);
            }
        }
        else
        {
            //wait(NULL);
        }
        #endif
    }
    const char *fifo_name = FIFO_PATH;  
    const int open_mode = O_WRONLY; 
    pipe_fd = open(fifo_name, open_mode);
    printf("Process %d opening FIFO O_WRONLY succed\n", getpid()); 
}

 /*********************************************************************************************************
 * @brief   Send information to program version management process
 *
 * @param   newExepath: the new version of the program full path
 * @usage
 *
 * @return   
 *          
********************************************************************************************************/
bool_t MT_SendAppUpgradeMessage( char *newExepath )
{
    bool_t     bRet = FALSE;
    uint32_t   pathLen = strlen( newExepath );
    
    if( pathLen )
    {
        bRet = MT_SendMessage( newExepath, pathLen );
    }
    
    return bRet;
}


bool_t  MT_SendAppHeatbeatMessage( void )
{
    uint16_t res = 0;
    char buff[MAX_CMD_LINE];  
    
    sprintf( buff, TICK_MESSAGE);

    return MT_SendMessage( buff, MAX_CMD_LINE );
}

bool_t MT_SendMessage( char* buff, uint32_t len )
{
    uint16_t res = 0;
    bool_t   bRet = FALSE;

    if( pipe_fd != -1 )  
    {
        res = write( pipe_fd, buff, len );  
        if(res == -1)  
        {
            fprintf(stderr, "Write error on pipe\n");  
        }
        else
        {
            printf("Write succed on pipe\n");
            bRet = TRUE;
        }
    }
    else
    {
        printf("open fifo FALSE\n");
    }
    
    return bRet;
}


