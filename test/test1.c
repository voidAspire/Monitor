#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h> 
#include <fcntl.h>

#include "../moniter.h"



static int pipe_fd = -1;

int main(int argc, char const *argv[])
{
	    const char *fifo_name = FIFO_PATH;  
    const int open_mode = O_WRONLY; 
    pipe_fd = open(fifo_name, open_mode);
    printf("Process %d opening FIFO O_WRONLY succed\n", getpid()); 
	while(1)
	{
		printf("test exe is running...\n");
		sleep(5);
		MT_SendMessageToMoniter(NULL);
	}
	return 0;
}



void MT_SendMessageToMoniter(char *newExepath)
{
    uint16_t res = 0;

    char buff[MAX_CMD_LINE];
    if (newExepath == NULL)
    {
        sprintf(buff, TICK_MESSAGE);
    }
    else
    {
        sprintf(buff, "%s", newExepath);
    }

    if(pipe_fd != -1)  
    {
            res = write(pipe_fd, buff, MAX_CMD_LINE);  
            if(res == -1)  
            {  
                fprintf(stderr, "Write error on pipe\n");  
                return;
            } 
            else
            {
                printf("Write succed on pipe\n");
                if (newExepath != NULL)
                {
                    close(pipe_fd);
                    printf("old is deaded!\n");
                    //exit(0);

                }
            } 
    }  
    else
    {
        printf("open fifo FALSE\n");
        return;
    }

}