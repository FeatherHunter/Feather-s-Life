#ifndef _H_IHOMESERVER
#define _H_IHOMESERVER

#define MESIZE 2048
#define ACCOUNT_MAX 32

#define COMMAND_PULSE  '0'
#define COMMAND_MANAGE 1
#define COMMAND_CONTRL 2
#define COMMAND_RESULT 3
#define MAN_LOGIN 11
#define RES_LOGIN 31

#define LOGIN_SUCCESS 1
#define LOGIN_FAILED  2

#define COMMAND_END 30
#define COMMAND_SEPERATOR 31

void * user_handler(void * arg);//deal with ever user
void service(char * rev_msg, int fd, int * userhome_flag);//
void authentication(char * account, char * password, int fd, int * userhome_flag);
#endif // _H_IHOMESERVER
