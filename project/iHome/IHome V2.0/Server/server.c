#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "socket_server.h"
#include "server.h"

int user_fd = -1;
int home_fd = -1;

int main()
{
    int socket_fd;
    int user_fd;
    int pid;

    struct sockaddr_in user_addr;
    /*socket bind listen*/
    socket_fd = init_server(AF_INET, SERVER_PORT, SERVER_ADDR);
    printf("IP: %s PORT: %d\n", SERVER_ADDR, SERVER_PORT);
    while(1)
    {
        int user_len = sizeof(user_addr);
        /*wait client's requst*/
        printf("accepting the customer!\n");
        user_fd = accept(socket_fd, (struct sockaddr *)&user_addr, &user_len);
        if(user_fd == -1)
        {
            fprintf(stderr, "accept error!%s", strerror(errno));
            exit(EXIT_FAILURE);
        }
        printf("accept the request of %s  fd:%d\n", inet_ntoa(user_addr.sin_addr), user_fd);

        /*
         *  provide services for every user
         */
        if( pthread_create(&pid, NULL, (void *)user_handler, (void *)&user_fd) != 0)
        {
            printf("%s offline", inet_ntoa(user_addr.sin_addr));
            close(user_fd);
        }
    }

    return 0;
}
/*
 * funtion: service every user
 *
 */
void * user_handler(void * arg)
{
    char buff[MESIZE];
    int fd = *((int *)arg);
    int rn;
    int userhome_flag = -1; //0为终端,1为客户

    while(1)
    {
        memset(buff, 0, sizeof(buff));
        rn = read(fd, buff, MESIZE);
        printf("%s\n", buff);
        if(rn <= 0)
        {
            printf("%d offline\n", userhome_flag);
            if(userhome_flag == 0)
            {
                home_fd = -1;
            }
            else if(userhome_flag == 1)
            {
                user_fd = -1;
            }
            break;
        }
        else
        {
            service(buff, fd, &userhome_flag);
        }
    }
    close(fd);
}

void service(char * rev_msg, int fd, int * userhome_flag)
{
    int type;
    int subtype;

    char account[ACCOUNT_MAX + 1]; //account's num max = 32
    char password[ACCOUNT_MAX + 1];
    char destination[37]; //des's num max = 36
    char content[MESIZE];

    char tempbuf[MESIZE];

    int i;
    int j;
    int cmd_start;
    int res;
    int rev_len = strlen(rev_msg);
    i = 0;
    while((rev_msg[i] != '\0')&&(i < rev_len))
    {
        cmd_start = i;
        /*get type*/
        if(rev_msg[i + 1] == COMMAND_SEPERATOR)
        {
            type = rev_msg[i];
            switch(type)
            {
                case COMMAND_MANAGE: printf("COMMAND_MANAGE\n");break;
                case COMMAND_CONTRL: printf("COMMAND_CONTRL\n");break;
                case COMMAND_RESULT: printf("COMMAND_RESULT\n");break;
                default: printf("unknown type:%d\n", type);
            }
            i+=2;
            /*为心跳*/
            if(type == COMMAND_PULSE)
            {
                sprintf(tempbuf, "%c%c%c",COMMAND_PULSE,COMMAND_SEPERATOR, COMMAND_END);
                res = write(fd, tempbuf, strlen(tempbuf));
                if(res <= 0)
                {
                    fprintf(stderr, "write result error! %s %d", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                while((i < rev_len) && (rev_msg[i] != COMMAND_END) )//msg[i]=END
                {
                    i++;
                }
                i++;
                continue;
            }
        }
        else
        {
            /*this cmd is invalid, to find next cmd*/
            while((i < rev_len) && (rev_msg[i] != COMMAND_END) )//msg[i]=END
            {
                i++;
            }
            i++;
            continue;
        }
        /*get account*/
        for(j = 0; (rev_msg[i] != COMMAND_SEPERATOR) && (rev_msg[i] != '\0') && j <= ACCOUNT_MAX; i++, j++)
        {
            account[j] = rev_msg[i];
        }
        printf("account:%s\n", account);
        i++;
        account[j] = '\0';
        /*get type*/
        if(rev_msg[i + 1] == COMMAND_SEPERATOR)
        {
            subtype = rev_msg[i];
            switch(subtype)
            {
                case MAN_LOGIN: printf("MAN_LOGIN\n");break;
                case CTL_LAMP: printf("CTL_LAMP\n");break;
                case CTL_GET: printf("CTL_GET\n");break;
                case RES_LOGIN: printf("RES_LOGIN\n");break;
                case RES_LAMP: printf("RES_LAMP\n");break;
                case RES_TEMP: printf("RES_TEMP\n");break;
                case RES_HUMI: printf("RES_HUMI\n");break;
                default: printf("unknown subtype:%d\n", type);
            }
            i+=2;
        }
        else
        {
            /*this cmd is invalid, to find next cmd*/
            while((i < rev_len) && (rev_msg[i] != COMMAND_END) )//msg[i]=END
            {
                i++;
            }
            i++;
            continue;
        }
        if(type == COMMAND_MANAGE)
        {
            printf("COMMAND_MANAGE\n");
            if(subtype == MAN_LOGIN)
            {
                printf("MAN_LOGIN\n");
                /*get password*/
                for(j = 0; (rev_msg[i] != COMMAND_SEPERATOR) && (rev_msg[i] != '\0') && j <= ACCOUNT_MAX; i++, j++)
                {
                    password[j] = rev_msg[i];
                }
                i++;
                password[j] = '\0';
                /*authentication*/
                authentication(account, password, fd, userhome_flag);
            }
        }
        else if(type == COMMAND_CONTRL)
        {
            /*it's user*/
            if(account[j-1] != 'h')
            {
                if(home_fd < 0)
                {
                    printf("not found home!\n");
                }
                else
                {
                    /*this cmd is invalid, to find next cmd*/
                    j = 0;
                    while((cmd_start < rev_len) && (rev_msg[cmd_start] != COMMAND_END) )//msg[i]=END
                    {
                        tempbuf[j++] = rev_msg[cmd_start++];
                        i++;
                    }
                    i++;
                    tempbuf[j] = '\0';
                    /*home fd*/
                    res = write(home_fd, tempbuf, strlen(tempbuf));
                    if(res <= 0)
                    {
                        fprintf(stderr, "write result error! %s %d", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                }
            }//end of account
        }
        else if(type == COMMAND_RESULT)
        {
            printf("----------------send to user-------------------");
            /*it's home*/
            if(account[j-1]='h')
            {
                if(user_fd < 0)
                {
                    printf("not found user!\n");
                }
                else
                {
                    /*this cmd is invalid, to find next cmd*/
                    j = 0;
                    while((cmd_start < rev_len) && (rev_msg[cmd_start] != COMMAND_END) )//msg[i]=END
                    {
                        tempbuf[j++] = rev_msg[cmd_start++];
                        i++;
                    }
                    i++;
                    tempbuf[j] = '\0';
                    /*user fd*/
                    res = write(user_fd, tempbuf, strlen(tempbuf));
                    if(res <= 0)
                    {
                        fprintf(stderr, "write result error! %s %d", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                }
            }

        }//end of command_result

    }
}
void authentication(char * account, char * password, int fd, int * userhome_flag)
{
    int res;
    char tempbuf[MESIZE];
    printf("AUTHENTICATION account:%s pssword:%s\n",account, password);
    if(strcmp(account, "975559549") == 0)
    {

        if(strcmp(password, "545538516") == 0)
        {
            user_fd = fd; //记录用户fd
            *userhome_flag = 1;
            sprintf(tempbuf, "%c%c975559549%c%c%c%c%c%c",COMMAND_RESULT,COMMAND_SEPERATOR,COMMAND_SEPERATOR,
                    RES_LOGIN,COMMAND_SEPERATOR,
                    LOGIN_SUCCESS,COMMAND_SEPERATOR, COMMAND_END);
            res = write(fd, tempbuf, strlen(tempbuf));
            if(res <= 0)
            {
                fprintf(stderr, "write result error! %s %d", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            printf("975559549 user login success!\n");
        }
        else
        {
            sprintf(tempbuf, "%c%c975559549%c%c%c%c%c%c",COMMAND_RESULT,COMMAND_SEPERATOR,COMMAND_SEPERATOR,RES_LOGIN,COMMAND_SEPERATOR, LOGIN_FAILED,COMMAND_SEPERATOR, COMMAND_END);
            res = write(fd, tempbuf, strlen(tempbuf));
            if(res <= 0)
            {
                fprintf(stderr, "write result error! %s %d", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            printf("975559549 user login falied!\n");
        }
    }
    else if(strcmp(account, "975559549h") == 0)
    {

        if(strcmp(password, "975559549") == 0)
        {
            home_fd = fd; //记录家庭fd
            *userhome_flag = 0;
            sprintf(tempbuf, "%c%c975559549h%c%c%c%c%c%c",COMMAND_RESULT,COMMAND_SEPERATOR,COMMAND_SEPERATOR,
                    RES_LOGIN,COMMAND_SEPERATOR,
                    LOGIN_SUCCESS,COMMAND_SEPERATOR, COMMAND_END);
            res = write(fd, tempbuf, strlen(tempbuf));
            if(res <= 0)
            {
                fprintf(stderr, "write result error! %s %d", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            printf("975559549 home login success!\n");

        }
        else//验证失败
       {
           sprintf(tempbuf, "%c%c975559549%c%c%c%c%c%c",COMMAND_RESULT,COMMAND_SEPERATOR,COMMAND_SEPERATOR,RES_LOGIN,COMMAND_SEPERATOR, LOGIN_FAILED,COMMAND_SEPERATOR, COMMAND_END);
           res = write(fd, tempbuf, strlen(tempbuf));
           if(res <= 0)
           {
              fprintf(stderr, "write result error! %s %d", __FILE__, __LINE__);
              exit(EXIT_FAILURE);
           }
           printf("975559549 home login failed!\n");
        }
    }
    else
    {
        sprintf(tempbuf, "%c%c975559549%c%c%c%c%c%c",COMMAND_RESULT,COMMAND_SEPERATOR,COMMAND_SEPERATOR,RES_LOGIN,COMMAND_SEPERATOR, LOGIN_FAILED,COMMAND_SEPERATOR, COMMAND_END);
        res = write(fd, tempbuf, strlen(tempbuf));
        if(res <= 0)
        {
            fprintf(stderr, "write result error! %s %d", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        printf("%s login falied!\n", account);

    }
}
