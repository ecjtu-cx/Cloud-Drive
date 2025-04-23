#ifndef __SQL_H__
#define __SQL_H__
#include "head.h"
#include "ser_cli.h"
#define STR_LEN 10

typedef struct node
{
    char path[30];
    struct node *next; 
}node;


int sql_connect(MYSQL **conn);
int find_name(MYSQL *conn,char *name,char*);
void get_salt(char *str);
void add_user(MYSQL *conn,char *name,char *salt,char *mima);
int math_user(MYSQL *conn,char *name,char *password,char *token);
int math_token(MYSQL *conn,char *name,char *token);
void ls_func(MYSQL *conn,char*name,int code,char *buf);
int operate_func(MYSQL *conn,Train_t *ptrain,QUR_msg *pqq_msg,char *name,int *pcode);
int find_pre_code(MYSQL *conn,char*path,int pcode);
int find_later_code(MYSQL *conn,int cur_code,char *filename,char *name);
int find_later_file(MYSQL *conn,int cur_code,char *filename,char *name);
int cd_func(MYSQL *conn,Train_t *ptrain,QUR_msg *pqq_msg,char *name,int *pcode);
int delete_file(MYSQL *conn,int code,char *name);
int find_file_info(MYSQL *conn,File_info*,char*name,int code);
int math_uoload(MYSQL *conn,File_info *pfile_info,char*name,int code);
void add_file(int code,char *name,File_info *pf);
void Llog(MYSQL *conn,const char *action,const char *name,const char *ip,const char *result);
void Olog(MYSQL *conn,const char *action,const char *name,const char *ip,const char *result);
int get_mqbuf(MYSQL *conn,MQ_buf *pf);


#endif
