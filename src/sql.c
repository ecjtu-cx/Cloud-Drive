#include "../include/sql.h"
//#include "head.h"

int sql_connect(MYSQL **conn)
{
    char server[]="localhost";
    char user[]="root";
    char password[]="950711";
    char database[]="Netdisk";//要访问的数据库名称
    *conn=mysql_init(NULL);
    if(!mysql_real_connect(*conn,server,user,password,database,0,NULL,0))
    {
        //    printf("Error connecting to database:%s\n",mysql_error(*conn));
        exit(0);
    }else{
        // printf("Connected...\n");
    }
    return 0;
}
int find_name(MYSQL *conn,char *name,char * salt)
{
    MYSQL_RES *res;
    MYSQL_ROW row;
    char query[300]="select salt from User where name='";
    sprintf(query,"%s%s%s",query, name,"'");
    puts(query);
    int t,ret =-1;
    t=mysql_query(conn,query);
    if(t)
    {
        printf("Error making query:%s\n",mysql_error(conn));
    }else{
        //printf("Query made...\n");
        res=mysql_use_result(conn);
        if(res)
        {
            if((row=mysql_fetch_row(res))!=NULL)
            {
                if(*row[0] == 0)
                    ret = -1;
                else
                {
                    if(salt !=NULL)
                        strcpy(salt,row[0]);
                    ret = 0;
                }
            }
            //   printf("res=NULL\n");
        }else{
            printf("查询出错\n");
            exit(0);
        }
        mysql_free_result(res);
        return ret;
    }
}
void get_salt(char *str)
{
    str[STR_LEN + 1] = 0;
    int i,flag;
    srand(time(NULL));
    for(i = 0; i < STR_LEN; i ++)
    {
        flag = rand()%3;
        switch(flag)
        {
        case 0:
            str[i] = rand()%26 + 'a';
            break;
        case 1:
            str[i] = rand()%26 + 'A';
            break;
        case 2:
            str[i] = rand()%10 + '0';
            break;
        }
    }
    *str='$';str[1]='5';str[2]='$';
    return;
}
void add_user(MYSQL *conn,char *name,char *salt,char *mima)
{
    char query[200]="insert into User (name,salt,ciphertext)values(";
    sprintf(query,"%s'%s','%s','%s')",query,name,salt,mima);
    //printf("query= %s\n",query);
    int t;
    t=mysql_query(conn,query);
    if(t)
    {
        printf("Error making query:%s\n",mysql_error(conn));
    }else{
        printf("insert success\n");
    }
}
int math_user(MYSQL *conn,char *name,char *password,char *token)
{
    int ret =-1;
    MYSQL_RES *res;
    MYSQL_ROW row;
    char query[300]="select ciphertext from User where name='";
    sprintf(query,"%s%s%s",query, name,"'");
    //puts(query);
    int t;
    t=mysql_query(conn,query);
    if(t)
    {
        printf("Error making query:%s\n",mysql_error(conn));
    }else
    {
        // printf("Query made...\n");
        res=mysql_use_result(conn);
        if(res)
        {
            if((row=mysql_fetch_row(res))!=NULL)
            {
                if(*row[0] == 0)
                    ret = -1;
                else if(strcmp(row[0],password)!=0)
                {
                    ret = -1;
                }
                else
                {
                    ret = 0;
                    mysql_free_result(res);
                    char pquery[200]="update User set token='";
                    strcpy(query,"where name ='");
                    sprintf(pquery,"%s%s' %s%s'",pquery,token,query,name);
                    //          puts(pquery);
                    t=mysql_query(conn,pquery);
                    if(t)
                    {
                        printf("Error making query:%s\n",mysql_error(conn));
                    }else{
                        printf("update success\n");
                    }
                    return ret;
                }
            }
        }else{
            printf("查询出错\n");
            exit(0);
        }
        mysql_free_result(res);
    }
    return ret;
}
//主函数更新的数据；
int math_token(MYSQL *conn,char *name,char *token)
{
    int ret =-1;
    MYSQL_RES *res;
    MYSQL_ROW row;
    char query[300]="select token from User where name='";
    sprintf(query,"%s%s%s",query, name,"'");
    //puts(query);
    int t;
    t=mysql_query(conn,query);
    if(t)
    {
        printf("Error making query:%s\n",mysql_error(conn));
    }else
    {
        // printf("Query made...\n");
        res=mysql_use_result(conn);
        if(res)
        {
            if((row=mysql_fetch_row(res))!=NULL)
            {
                if(*row[0] == 0)
                    ret = -1;
                else if(strcmp(row[0],token)==0)
                {
                    ret = 0;
                }
            }
        }else{
            printf("查询出错\n");
            exit(0);
        }
        mysql_free_result(res);
    }
    return ret;
}

void ls_func(MYSQL *conn,char*name,int code,char *buf)
{
    MYSQL_RES *res;
    MYSQL_ROW row;
//    *buf='\n';
//    buf[1]=0;
    *buf =0;
    char query[300]="select filetype ,filename ,filesize from Directory where belongID=";
    sprintf(query,"%s'%s' and procode =%d",query, name,code);
    //puts(query);
    int t;
    t=mysql_query(conn,query);
    if(t)
    {
        printf("Error making query:%s\n",mysql_error(conn));
    }else
    {
        // printf("Query made...\n");
        res=mysql_use_result(conn);
        if(res)
        {
            while((row=mysql_fetch_row(res))!=NULL)
            {
                sprintf(buf,"%s-%-10s%-20s %10s B\n",buf,row[0],row[1],row[2]);
            }
        }else{
            //       printf("Don't find data\n");
        }
        mysql_free_result(res);
    }
    return ;
}


//返回当前code（pcode）下的上级目录的code，并把当前目录名保存到path中，失败-1；
int find_pre_code(MYSQL *conn,char*path,int pcode)
{
    MYSQL_RES *res;
    MYSQL_ROW row;
    char query[300]="select procode,filename from Directory where code=";
    sprintf(query,"%s%d",query, pcode);
    //  puts(query);
    int t,ret=-1;
    t=mysql_query(conn,query);
    if(t)
    {
        printf("Error making query:%s\n",mysql_error(conn));
    }else
    {
        //      printf("Query made...\n");
        res=mysql_use_result(conn);
        if(res)
        {
            if((row=mysql_fetch_row(res))!=NULL)
            {
                //             printf("procode = %d\n",atoi(row[0]));
                strcpy(path,row[1]);
                //             printf("path =%s\n",path);
                ret = atoi(row[0]);
            }
        }else{
            printf("查询出错\n");
            exit(0);
        }
        mysql_free_result(res);
    }
    return ret;

}

//返回code下一级filename的code值（智能是文件夹）；
int find_later_code(MYSQL *conn,int cur_code,char *filename,char *name)
{
    MYSQL_RES *res;
    MYSQL_ROW row;
    char query[300]="select code from Directory where procode=";
    sprintf(query,"%s%d and belongID= '%s'and filetype = 'd' and filename ='%s'",query, cur_code,name,filename);
    //  puts(query);
    int t,ret=-1;
    t=mysql_query(conn,query);
    if(t)
    {
        printf("Error making query:%s\n",mysql_error(conn));
    }else
    {
        //     printf("Query made...\n");
        res=mysql_use_result(conn);
        if(res)
        {
            if((row=mysql_fetch_row(res))!=NULL)
            {
                //             printf("code = %d\n",atoi(row[0]));
                ret = atoi(row[0]);
            }
        }else{
            printf("查询出错\n");
            exit(0);
        }
        mysql_free_result(res);
    }
    return ret;
}

//返回code下一级filename的code值（文件和文件夹都可以）//可以搞个开关传参，懒得写了；
int find_later_file(MYSQL *conn,int cur_code,char *filename,char *name)
{
    MYSQL_RES *res;
    MYSQL_ROW row;
    char query[300]="select code from Directory where procode=";
    sprintf(query,"%s%d and belongID= '%s'and filename ='%s'",query, cur_code,name,filename);
    //  puts(query);
    int t,ret=-1;
    t=mysql_query(conn,query);
    if(t)
    {
        printf("Error making query:%s\n",mysql_error(conn));
    }else
    {
        //       printf("Query made...\n");
        res=mysql_use_result(conn);
        if(res)
        {
            if((row=mysql_fetch_row(res))!=NULL)
            {
                //             printf("code = %d\n",atoi(row[0]));
                ret = atoi(row[0]);
            }
        }else{
            printf("查询出错\n");
            exit(0);
        }
        mysql_free_result(res);
    }
    return ret;
}

//失败无操作，返回-1，成功会改变路径，火车上装上code，路径；
//pqq->buf 是操作，buf1是内容；
int cd_func(MYSQL *conn,Train_t *ptrain,QUR_msg *pqq_msg,char *name,int *pcode)//buf1是移动后的路径；
{
    char tranbuf[200];
    MYSQL_RES *res;
    MYSQL_ROW row;
    if(strcmp(pqq_msg->buf1,"..")==0)
    {
        if(*pcode==0)
            return -1;
        char query[300]="select procode from Directory where code=";
        sprintf(query,"%s%d and belongID= '%s'",query, *pcode,name);
        //     puts(query);
        int t;
        t=mysql_query(conn,query);
        if(t)
        {
            printf("Error making query:%s\n",mysql_error(conn));
        }else
        {
            //        printf("Query made...\n");
            res=mysql_use_result(conn);
            if(res)
            {
                if((row=mysql_fetch_row(res))!=NULL)
                {
                    *pcode = atoi(row[0]);
                    //                 printf("code = %d\n",*pcode);
                }
                else
                {
                    mysql_free_result(res);
                    return -1;
                }
            }else{
                printf("查询出错\n");
                exit(0);
            }
            mysql_free_result(res);
        }
    }
    else if(*pqq_msg->buf1 =='/')
    {
        int ret ,code =0;
        char *pcur,*pre;
        pcur=pre =pqq_msg->buf1+1;
        while(*pcur!=0)
        {
            while(*pcur!='/' && *pcur!=0)
                pcur++;
            if(*pcur != 0)
            {
                *pcur = 0;
                pcur++;
            }
            if(*pre !=0)
            {
                ret = find_later_code(conn,code,pre,name);
                if(ret == -1)
                    return -1;
                code = ret;
                //              printf("%s :   code = %d\n",pre,code);
                pre = pcur;
            }
        }
        *pcode = code;
        //     printf("*pcode = %d",*pcode);
    }
    else
    {
        int ret = find_later_code(conn,*pcode,pqq_msg->buf1,name);
        if(ret == -1)
            return -1;
        *pcode = ret;
        //       printf("curcode = %d",*pcode);
    }
    int ret;
    node head,*pnew = (node*)calloc(1,sizeof(node)),*q;
    head.next=0;
    ret = find_pre_code(conn,pnew->path,*pcode);
    while(ret !=-1)
    {
        pnew->next = head.next;
        head.next = pnew;
        pnew = (node*)calloc(1,sizeof(node));
        ret = find_pre_code(conn,pnew->path,ret);
    }
    strcpy(tranbuf,"/");
    pnew = head.next;
    if(pnew)
        *tranbuf =0;
    while(NULL !=pnew)
    {
        sprintf(tranbuf,"%s/%s",tranbuf,pnew->path);
        q=pnew;
        pnew = pnew->next;
        free(q);
    }
    bzero(pqq_msg,sizeof(QUR_msg));
    sprintf(pqq_msg->buf,"%d",*pcode);
    strcpy(pqq_msg->buf1,tranbuf);
    //   printf("qq code = %s\nqqbuf= %s\n",pqq_msg->buf,pqq_msg->buf1);
    ptrain->Len = sizeof(QUR_msg);
    ptrain->ctl_code = OPERATE_OK;
    memcpy(ptrain->buf,pqq_msg,ptrain->Len);
    return 0;
}

int delete_file(MYSQL *conn,int code,char *name)
{
    MYSQL_RES *res;
    MYSQL_ROW row;
    char query[300]="select * from Directory where procode=";
    sprintf(query,"%s%d and belongID= '%s'",query, code,name);
    //   puts(query);
    int t;
    t=mysql_query(conn,query);
    if(t)
    {
        printf("Error making query:%s\n",mysql_error(conn));
    }else
    {
        //      printf("Query made...\n");
        res=mysql_use_result(conn);
        if(res)
        {
            if((row=mysql_fetch_row(res))!=NULL)
            {
                //              printf("first filename = %s\n",row[0]);
                mysql_free_result(res);
                return -1;
            }
        }else{
            printf("查询出错\n");
            exit(0);
        }
        mysql_free_result(res);
    }
    strcpy(query,"delete from Directory where code=");
    sprintf(query,"%s%d",query,code);
    //   puts(query);
    t=mysql_query(conn,query);
    if(t)
    {
        printf("Error making query:%s\n",mysql_error(conn));
    }else{
        printf("delete success,delete row=%ld\n",(long)mysql_affected_rows(conn));
    }
    return 0;
}

//ptrain用来装车，cd成功车上有货，失败没装，其他都是len为零操作成功的小火车；
//qqmsg有操作和内容，name是client name，pcode是cilent上的code地址；
int operate_func(MYSQL *conn,Train_t *ptrain,QUR_msg *pqq_msg,char *name,int *pcode)
{
    MYSQL_RES *res;
    MYSQL_ROW row;
    if(strcmp(pqq_msg->buf,"mkdir")==0)
    {
        char qquery[300]="select * from Directory where procode=";
        sprintf(qquery,"%s%d and filename ='%s'and belongID='%s'",qquery,*pcode,
                pqq_msg->buf1,name);
        //        puts(qquery);
        int t;
        t=mysql_query(conn,qquery);
        if(t)
        {
            printf("Error making query:%s\n",mysql_error(conn));
        }else
        {
            //            printf("Query made...\n");
            res=mysql_use_result(conn);
            if(res)
            {
                if((row=mysql_fetch_row(res))!=NULL)
                {
                    mysql_free_result(res);
                    return -1;
                }
                else
                {
                    mysql_free_result(res);
                    char query[300]="insert into Directory (procode,filename,belongID,filetype)values(";
                    sprintf(query,"%s%d,'%s','%s','%s')",query,*pcode,pqq_msg->buf1,name,"d");
                    //                 printf("query= %s\n",query);
                    int t;
                    t=mysql_query(conn,query);
                    if(t)
                    {
                        printf("Error making query:%s\n",mysql_error(conn));
                    }else{
                        printf("insert success\n");
                    }
                    ptrain->Len = 0;
                    ptrain->ctl_code = OPERATE_OK;
                    return 0;
                }
            }else{
                printf("查询出错\n");
                exit(0);
            }
        }
    }else if(strcmp(pqq_msg->buf,"cd")==0)
    {
        int ret = cd_func(conn,ptrain,pqq_msg,name,pcode);
        if(ret == -1)
            return -1;
        else
            return 0;
    }
    else if(strcmp(pqq_msg->buf,"rm")==0)
    {
        //先返回想删的文件的code
        int ret = find_later_file(conn,*pcode,pqq_msg->buf1,name);
        if(ret == -1)
            return -1;
        ret = delete_file(conn,ret,name);
        if(ret == -1)
            return -1;
        ptrain->Len = 0;
        ptrain->ctl_code = OPERATE_OK;
        return 0;
    }
    ptrain->Len=0;
    ptrain->ctl_code = OPERATE_NO;
    return -1;
}

int find_file_info(MYSQL *conn,File_info *pfile_info,char*name,int code)
{
    MYSQL_RES *res;
    MYSQL_ROW row;
    char query[300]="select filesize, md5sum from Directory where procode=";
    sprintf(query,"%s%d and belongID= '%s'and filetype = 'f' and filename ='%s'",query, code,name,pfile_info->filename);
    //puts(query);
    int t,ret=-1;
    t=mysql_query(conn,query);
    if(t)
    {
        printf("Error making query:%s\n",mysql_error(conn));
    }else
    {
        res=mysql_use_result(conn);
        if(res)
        {
            if((row=mysql_fetch_row(res))!=NULL)
            {
                pfile_info->filesize = atoi(row[0]);
                strcpy(pfile_info->md5sum,row[1]);
                ret = 0;
            }
        }else{
            printf("查询出错\n");
            exit(0);
        }
        mysql_free_result(res);
    }
    return ret;

}
int math_uoload(MYSQL *conn,File_info *pfile_info,char*name,int code)
{
    MYSQL_RES *res;
    MYSQL_ROW row;
    int t,ret=-1;
    char query[300] ="select filesize from Directory where procode=";
    sprintf(query,"%s%d and belongID= '%s'and filetype = 'f' and filename ='%s'",query, code,name,pfile_info->filename);
    t=mysql_query(conn,query);
//    puts(query);
    if(t)
    {
        printf("Error making query:%s\n",mysql_error(conn));
    }else
    {
        res=mysql_use_result(conn);
        if(res)
        {
            if((row=mysql_fetch_row(res))!=NULL)
            {
                ret = -1;
            }else
                ret = 0;
        }else{
            printf("查询出错\n");
            exit(0);
        }
        mysql_free_result(res);
    }
//    printf("ret = %d\n",ret);
    if(ret == -1)
        return -1;
    strcpy(query,"select filesize from Directory where md5sum=");
    sprintf(query,"%s'%s'",query,pfile_info->md5sum);
//    puts(query);
    t=mysql_query(conn,query);
    if(t)
    {
        printf("Error making query:%s\n",mysql_error(conn));
    }else
    {
        res=mysql_use_result(conn);
        if(res)
        {
            if((row=mysql_fetch_row(res))!=NULL)
            {
                ret = 1;
            }
        }else{
            printf("查询出错\n");
            exit(0);
        }
        mysql_free_result(res);
    }
//    printf("ret = %d\n",ret);
    if(ret == 1)
    {
        strcpy(query,"insert into Directory (procode,filename,belongID,filetype,md5sum,filesize)values");
        sprintf(query,"%s(%d,'%s','%s','f','%s',%d)",query,code,pfile_info->filename,
                name,pfile_info->md5sum,pfile_info->filesize);
 //       puts(query);
        t=mysql_query(conn,query);
        if(t)
        {
            printf("Error making query:%s\n",mysql_error(conn));
        }else{
            printf("insert success\n");
        }
    }
    return ret;
}
void add_file(int code,char *name,File_info *pf)
{
    MYSQL *conn;
    sql_connect(&conn);
    char query[300]="insert into Directory(procode,filename,belongID,filetype,md5sum,filesize) values";
    sprintf(query,"%s(%d,'%s','%s','%s','%s',%d)",query,code,pf->filename,name,
            "f",pf->md5sum,pf->filesize);
//    puts(query);
    int t;
    t=mysql_query(conn,query);
//    if(!t)
//        printf("insert success\n");
    mysql_close(conn);
}
void Llog(MYSQL *conn,const char *action,const char *name,const char *ip,const char *result)
{
    char query[300]="insert into Login(action,name,ip_port,result) values";
    sprintf(query,"%s('%s','%s','%s','%s')",query,action,name,ip,result);
//    puts(query);
    int t;
    t=mysql_query(conn,query);
//    if(!t)
//        printf("insert success\n");
}
void Olog(MYSQL *conn,const char *name,const char *handel,const char *object,const char *result)
{
    char query[300]="insert into Operate(name,handle,object,result) values";
    sprintf(query,"%s('%s','%s','%s','%s')",query,name,handel,object,result);
//    puts(query);
    int t;
    t=mysql_query(conn,query);
//    if(!t)
//        printf("insert success\n");
}
int get_mqbuf(MYSQL *conn,MQ_buf *pf)
{
    MYSQL_RES *res;
    MYSQL_ROW row;
    char query[300]="select ip, port from Server_info where md5sum=";
    sprintf(query,"%s'%s'",query,pf->file.md5sum);
//    puts(query);
    int t,i=0;
    t=mysql_query(conn,query);
    if(t)
    {
        printf("Error making query:%s\n",mysql_error(conn));
    }else
    {
        res=mysql_use_result(conn);
        if(res)
        {
            while((row=mysql_fetch_row(res))!=NULL)
            {
//                printf("ip:%s,port:%s i= %d\n",row[0],row[1],i);
                pf->sfd_in[i].sin_family = AF_INET;
                pf->sfd_in[i].sin_port = htons(atoi(row[1]));
                pf->sfd_in[i].sin_addr.s_addr =inet_addr(row[0]);
                if(++i == SPOT_NUM)
                    break;
            }
        }else{
            printf("查询出错\n");
            exit(0);
        }
        mysql_free_result(res);
    }
    if(i == SPOT_NUM)
        return 0;
    else
        return -1;
}
