#include "shell.h"
#include <stddef.h>
#include "clib.h"
#include <string.h>
#include "fio.h"
#include "filesystem.h"

#include "FreeRTOS.h"
#include "task.h"
#include "host.h"
#include "queue.h"
#include "timers.h"

volatile xTimerHandle xTimers;
volatile xQueueHandle xQueue = NULL;
volatile xQueueHandle xQueueFib = NULL;
extern xQueueHandle serial_printf_queue;

 struct FibMessage
 {
	xTaskHandle xHandle;
	int pos;
 };
 
typedef struct {
	const char *name;
	cmdfunc *fptr;
	const char *desc;
} cmdlist;

void ls_command(int, char **);
void man_command(int, char **);
void cat_command(int, char **);
void ps_command(int, char **);
void host_command(int, char **);
void help_command(int, char **);
void host_command(int, char **);
void mmtest_command(int, char **);
void test_command(int, char **);
void tic_command(int, char **);
void toc_command(int, char **);
void fib_command(int, char **);
void _command(int, char **);

#define MKCL(n, d) {.name=#n, .fptr=n ## _command, .desc=d}

cmdlist cl[]={
	MKCL(ls, "List directory"),
	MKCL(man, "Show the manual of the command"),
	MKCL(cat, "Concatenate files and print on the stdout"),
	MKCL(ps, "Report a snapshot of the current processes"),
	MKCL(host, "Run command on host"),
	MKCL(mmtest, "heap memory allocation test"),
	MKCL(help, "help"),
	MKCL(test, "test new function"),
	MKCL(tic, "start counter"),
	MKCL(toc, "return counter value"),
	MKCL(fib, "return fibonacci value"),
	MKCL(, ""),
};

int parse_command(char *str, char *argv[]){
	int b_quote=0, b_dbquote=0;
	int i;
	int count=0, p=0;
	for(i=0; str[i]; ++i){
		if(str[i]=='\'')
			++b_quote;
		if(str[i]=='"')
			++b_dbquote;
		if(str[i]==' '&&b_quote%2==0&&b_dbquote%2==0){
			str[i]='\0';
			argv[count++]=&str[p];
			p=i+1;
		}
	}
	/* last one */
	argv[count++]=&str[p];

	return count;
}

void ls_command(int n, char *argv[]){
    fio_printf(1,"\r\n"); 
    int dir;
    if(n == 0){
        dir = fs_opendir("");
    }else if(n == 1){
        dir = fs_opendir(argv[1]);
        //if(dir == )
    }else{
        fio_printf(1, "Too many argument!\r\n");
        return;
    }
(void)dir;   // Use dir
}

int filedump(const char *filename){
	char buf[128];

	int fd=fs_open(filename, 0, O_RDONLY);

	if( fd == -2 || fd == -1)
		return fd;

	fio_printf(1, "\r\n");

	int count;
	while((count=fio_read(fd, buf, sizeof(buf)))>0){
		fio_write(1, buf, count);
    }
	
    fio_printf(1, "\r");

	fio_close(fd);
	return 1;
}

void ps_command(int n, char *argv[]){
	signed char buf[1024];
	vTaskList(buf);
        fio_printf(1, "\n\rName          State   Priority  Stack  Num\n\r");
        fio_printf(1, "*******************************************\n\r");
	fio_printf(1, "%s\r\n", buf + 2);	
}

void cat_command(int n, char *argv[]){
	if(n==1){
		fio_printf(2, "\r\nUsage: cat <filename>\r\n");
		return;
	}

    int dump_status = filedump(argv[1]);
	if(dump_status == -1){
		fio_printf(2, "\r\n%s : no such file or directory.\r\n", argv[1]);
    }else if(dump_status == -2){
		fio_printf(2, "\r\nFile system not registered.\r\n", argv[1]);
    }
}

void man_command(int n, char *argv[]){
	if(n==1){
		fio_printf(2, "\r\nUsage: man <command>\r\n");
		return;
	}

	char buf[128]="/romfs/manual/";
	strcat(buf, argv[1]);

    int dump_status = filedump(buf);
	if(dump_status < 0)
		fio_printf(2, "\r\nManual not available.\r\n");
}

void host_command(int n, char *argv[]){
    int i, len = 0, rnt;
    char command[128] = {0};

    if(n>1){
        for(i = 1; i < n; i++) {
            memcpy(&command[len], argv[i], strlen(argv[i]));
            len += (strlen(argv[i]) + 1);
            command[len - 1] = ' ';
        }
        command[len - 1] = '\0';
        rnt=host_action(SYS_SYSTEM, command);
        fio_printf(1, "\r\nfinish with exit code %d.\r\n", rnt);
    } 
    else {
        fio_printf(2, "\r\nUsage: host 'command'\r\n");
    }
}

void vTimerCallback( xTimerHandle pxTimer ){
	int i;
	// Optionally do something if the pxTimer parameter is NULL.
	configASSERT( pxTimer );
	if( xQueueReceive( xQueue, &( i ), ( portTickType ) 10 ) )
	{
		i++;
		xQueueSend( xQueue, ( void * ) &i, ( portTickType ) 0 );
	}
}

void tic_command(int n, char*argv[])
{
	if (xQueue == NULL)
	{
		xQueue = xQueueCreate(1, sizeof(int));
	}
	if ( xTimers == NULL )
	{
		xTimers = xTimerCreate((const signed char *)"Timer",				// Just a text name, not used by the kernel.
								( 10 ),				// The timer period in ticks.
								pdTRUE,				// The timers will auto-reload themselves when they expire.
								( void * ) 0, 		// Assign each timer a unique id equal to its array index.
								vTimerCallback 		// Each timer calls the same callback when it expires.
								               );
	}
	if ( xTimerIsTimerActive( xTimers ) == pdFALSE )
	{
		if ( xTimerStart( xTimers, 0 ) != pdPASS )
		{
			// The timer could not be set into the Active state.
		}
		else
		{
			fio_printf(1, "\r\nTimer start\r\n");
		}
	}
	else
	{
		fio_printf(1, "\r\ntimer restart\r\n");
	}
	int i;
	xQueueReceive( xQueue, &( i ), ( portTickType ) 10 );
	i = 0;
	xQueueSend( xQueue, ( void * ) &i, ( portTickType ) 0 );
}

void toc_command(int n, char*argv[])
{
	if ( xTimerIsTimerActive( xTimers ) != pdFALSE ) // or more simply and equivalently "if( xTimerIsTimerActive( xTimer ) )"
	{
		int i;
		xQueueReceive( xQueue, &( i ), ( portTickType ) 10 );
		xQueueSend( xQueue, ( void * ) &i, ( portTickType ) 0 );
		fio_printf(1, "\r\nElapsed count:%d\r\n", i);
		xTimerStop( xTimers, 0 );
	}
	else
	{
		fio_printf(1, "\r\nTimer not start\r\n");
	}
}

long fibN(int pos)
{
	if (pos > 1)
		return fibN(pos - 2) + fibN(pos - 1);
	else if (pos == 1)
		return 1;
	else
		return 0;
}

void fib_task(void *pvParameters)
{
	struct SCIMessage buf;
	struct FibMessage fib;
	xQueueReceive( xQueueFib, &( fib ), ( portTickType ) 10 );
	int i = fibN(fib.pos);

	sprintf(buf.cData, "pos:%d reslut:%d\r\n", fib.pos, i);
	xQueueSend( serial_printf_queue, ( void * ) &buf, ( portTickType ) 0 );
	//fio_printf(2, "pos:%d reslut:%d\r\n",fib.pos, i);
	vTaskDelete(fib.xHandle);
}

void fib_command(int n, char*argv[])
{
	struct FibMessage fib;
	fib.pos = 0;
	if (n > 0)
	{
		char cPos[4] = {0};
		unsigned char i, len = strlen(argv[1]);
		if (len > 4)
			len = 4;
		memcpy(&cPos[0], argv[1], len);
		cPos[len] = 0;
		for (i = 0; i < len; i++)
		{
			if (cPos[i] >= '0' && cPos[i] <= '9')
			{
				fib.pos = fib.pos * 10 + (cPos[i] - '0');
			}
			else
			{
				break;
			}
		}
	}
	if (xQueueFib == NULL)
	{
		xQueueFib = xQueueCreate(1, sizeof(struct FibMessage));
	}
	xTaskCreate(fib_task,
	            (signed portCHAR *) "FIB",
	            512 /* stack size */, NULL, tskIDLE_PRIORITY + 1, &fib.xHandle);
	xQueueSend( xQueueFib, ( void * ) &fib, ( portTickType ) 0 );
	fio_printf(1, "\r\nstart cal\r\n");
}

void help_command(int n,char *argv[]){
	int i;
	fio_printf(1, "\r\n");
	for(i = 0;i < sizeof(cl)/sizeof(cl[0]) - 1; ++i){
		fio_printf(1, "%s - %s\r\n", cl[i].name, cl[i].desc);
	}
}

void test_command(int n, char *argv[]) {
    int handle;
    int error;

    fio_printf(1, "\r\n");
    
    handle = host_action(SYS_SYSTEM, "mkdir -p output");
    handle = host_action(SYS_SYSTEM, "touch output/syslog");

    handle = host_action(SYS_OPEN, "output/syslog", 8);
    if(handle == -1) {
        fio_printf(1, "Open file error!\n\r");
        return;
    }

    char *buffer = "Test host_write function which can write data to output/syslog\n";
    error = host_action(SYS_WRITE, handle, (void *)buffer, strlen(buffer));
    if(error != 0) {
        fio_printf(1, "Write file error! Remain %d bytes didn't write in the file.\n\r", error);
        host_action(SYS_CLOSE, handle);
        return;
    }

    host_action(SYS_CLOSE, handle);
}

void _command(int n, char *argv[]){
    (void)n; (void)argv;
    fio_printf(1, "\r\n");
}

cmdfunc *do_command(const char *cmd){

	int i;

	for(i=0; i<sizeof(cl)/sizeof(cl[0]); ++i){
		if(strcmp(cl[i].name, cmd)==0)
			return cl[i].fptr;
	}
	return NULL;	
}
