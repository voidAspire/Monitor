#ifndef _MONITER_H_
#define _MONITER_H_


/**********************************************************************************************************
 *  Standard Includes
 *********************************************************************************************************/
#include <stdio.h>
#include <stdint.h>


/**********************************************************************************************************
 *  Object like Macro
*********************************************************************************************************/
#define PROCES "/proc"
#define MONITER_NAME "moniter"
#define FIFO_PATH "/tmp/my_fifo"
#define TICK_MESSAGE "tick"
#define FIFO_LISTENTIME_SEC  30
#define FIFO_LISTENTIME_USEC 0
#define MAX_PROC_NAME 128
#define MAX_CMD_LINE 256
#define SIZE 1024


/**********************************************************************************************************
 *  Function like Macro
*********************************************************************************************************/
#if DEBUG
#define myprintf(fmt, args...)    printf("%s,%s(),%d:" fmt "\n", __FILE__,__FUNCTION__,__LINE__, ##args)
#else 
#define myprintf(fmt, args...)    
#endif

/**********************************************************************************************************
 *  Type defines
**********************************************************************************************************/


typedef struct  MT_zExeInfo_t
{

    char procName[MAX_PROC_NAME];
    char cmdLine[MAX_CMD_LINE];
    uint16_t procPid;

}MT_zExeInfo_t;



/**********************************************************************************************************
 *  Public Function 
*********************************************************************************************************/
void MT_CodeMoniter();

void MT_ExecMoniter(); //TO DO... , generate management process with executable files

bool_t  MT_SendAppHeatbeatMessage( void );
bool_t  MT_SendAppUpgradeMessage( char *newExepath );
bool_t  MT_SendMessage( char* buff, uint32_t len );



#endif //_MONITER_H_
