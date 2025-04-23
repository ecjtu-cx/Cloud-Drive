#include "cfactory.h"
#include "child.h"
struct sockaddr_in ser;
Zhuce login_msg;
char path[200]={"/"};
//Train_t train;
//char code[50]={"0"};



void print(void)
{
//    system("clear");
//    printf("Hello %s\n",login_msg.name);
    printf("[%s@Netdisk ~%s]$ ",login_msg.name,path);
    fflush(stdout);
}

int main(int argc,char *argv[])
{
    ARGS_CHECK(argc,3);
    int cFd,ret,type,epfd;
    bzero(&ser,sizeof(ser));
    ser.sin_family=AF_INET;
    ser.sin_port=htons(atoi(argv[2]));
    ser.sin_addr.s_addr=inet_addr(argv[1]);//点分十进制转为32位的网络字节序
    time_t now;
    struct epoll_event evs[10];
    char buf[100],*passwd,*p,uppath[100];
    QUR_msg qq_msg;
    File_info file_info;
    Train_t train;
    int fds[2],vds[2];
    process_data normal_child,vip_child;



    epfd = epoll_create(1);
    epoll_func(epfd,0,EPOLL_CTL_ADD,EPOLLIN);

    socketpair(AF_LOCAL,SOCK_STREAM,0,fds);
    pthread_create(&normal_child.pid,NULL,normal_func,(void*)fds[1]);
    normal_child.fd = fds[0];
    //normal_child.busy_num = 0;
    epoll_func(epfd,normal_child.fd,EPOLL_CTL_ADD,EPOLLIN);


    socketpair(AF_LOCAL,SOCK_STREAM,0,vds);
    pthread_create(&vip_child.pid,NULL,vip_func,(void*)vds[1]);
    vip_child.fd = vds[0];




login_begin:
    system("clear");
    printf("请选择注册或者登陆：\n1:注册新账号\n2:已有账号直接登陆\n");
    scanf("%d",&type);
    if(type == 1)
    {
        printf("请输入要注册的用户名\n");
        scanf("%s",login_msg.name);
        passwd = getpass("请输入密码：");
        strcpy(buf,passwd);
        passwd = getpass("请再次输入密码确认：");
        if(strcmp(buf,passwd)!=0)
        {
            printf("两次输入不一致，即将回到主界面\n");
            sleep(1);
            goto login_begin;
        }else
        {
            strcpy(train.buf,login_msg.name);
            train.Len = strlen(train.buf)+1;
            train.ctl_code = LOGIN_PRE;
            cFd=socket(AF_INET,SOCK_STREAM,0);
            ERROR_CHECK(cFd,-1,"socket");
            ret=connect(cFd,(struct sockaddr*)&ser,sizeof(ser));
            ERROR_CHECK(ret,-1,"connect");
     //       printf("connect successful\n");
            ret = send(cFd,&train,train.Len+8,0);
            // ssend(cFd,epfd);
        }
    }else
    {
        printf("请输入要登录的用户名\n");
        scanf("%s",login_msg.name);
        passwd = getpass("请输入密码：");
        strcpy(buf,passwd);
        strcpy(train.buf,login_msg.name);
        train.Len = strlen(train.buf)+1;
        train.ctl_code = REGISTER_PRE;
        cFd=socket(AF_INET,SOCK_STREAM,0);
        ERROR_CHECK(cFd,-1,"socket");
        ret=connect(cFd,(struct sockaddr*)&ser,sizeof(ser));
        ERROR_CHECK(ret,-1,"connect");
//        printf("connect successful\n");
        ret = send(cFd,&train,train.Len+8,0);
        // ssend(cFd);
    }
    epoll_func(epfd,cFd,EPOLL_CTL_ADD,EPOLLIN);


    while(1)
    {
        int ready_num = epoll_wait(epfd,evs,10,-1);
        for(int i=0;i<ready_num;i++)
        {
           // if(evs[i].events==EPOLLIN && evs[i].data.fd == normal_child.fd)
           // {
           //     int z;
           //     read(normal_child.fd,&z,4);
           //     normal_child.busy_num--;
          //  }
            if(evs[i].events==EPOLLIN && evs[i].data.fd == cFd)
            {
                ret = one_recv(cFd,&train);
                if(ret == -1)
                {
                    close(cFd);
                    epoll_func(epfd,cFd,EPOLL_CTL_DEL,EPOLLIN);
                    cFd=socket(AF_INET,SOCK_STREAM,0);
                    ret=connect(cFd,(struct sockaddr*)&ser,sizeof(ser));
                    ERROR_CHECK(ret,-1,"connect");
             //       printf("重连 successful\n");
                    epoll_func(epfd,cFd,EPOLL_CTL_ADD,EPOLLIN);
                    train.Len = sizeof(Zhuce);
                    memcpy(train.buf,&login_msg,train.Len);
                    train.ctl_code = TOKEN_PLESE;
                    ret = send(cFd,&train,train.Len+8,0);
                    if(ret ==-1)
                        exit(0);
                    //重连，发token;重连要在服务器更新当前code值
                    //加入新的epoll
                    //此次SEND发name,tolen,当前code值；
                    continue;
                }
                switch(train.ctl_code)
                {
                case LOGIN_NO:
                    close(cFd);
                    epoll_func(epfd,cFd,EPOLL_CTL_DEL,EPOLLIN);
                    printf("账号已存在，请重新注册\n");
                    sleep(1);
                    goto login_begin;
                case LOGIN_POK:
                    strcpy(login_msg.token,train.buf);//此时token是颜值
                    strcpy(login_msg.passward,crypt(buf,login_msg.token));
                    //            printf("name = %s\nsalt = %s\npassward = %s\n",login_msg.name,
                    //                  login_msg.token,login_msg.passward);
                    train.Len = sizeof(Zhuce);
                    memcpy(train.buf,&login_msg,train.Len);
                    train.ctl_code = LOGIN_Q;
                    ret = send(cFd,&train,train.Len+8,0);
                    if(ret != -1)
                        printf("注册成功\n");
                    else
                        printf("网络繁忙，注册失败\n");
                    sleep(1);
                    close(cFd);
                    epoll_func(epfd,cFd,EPOLL_CTL_DEL,EPOLLIN);
                    goto login_begin;
                case REGISTER_NO:
                    printf("账号/密码错误，请重新登录\n");
                    close(cFd);
                    epoll_func(epfd,cFd,EPOLL_CTL_DEL,EPOLLIN);
                    sleep(1);
                    goto login_begin;
                case REGISTER_POK:
                    time(&now);
                    //           printf("salt = %s\n",train.buf);
                    strcpy(login_msg.passward,crypt(buf,train.buf));
                    sprintf(login_msg.token,"%s %s",login_msg.name,ctime(&now));
                    login_msg.token[strlen(login_msg.token)-1]=0;
                    //           printf("name = %s\ntoken = %s\npassward = %s\n",login_msg.name,
                    //                 login_msg.token,login_msg.passward);
                    train.Len = sizeof(Zhuce);
                    memcpy(train.buf,&login_msg,train.Len);
                    train.ctl_code = REGISTER_Q;
                    ret = send(cFd,&train,train.Len+8,0);
                    break;
                case REGISTER_OK:
                    system("clear");
                    printf("Hello: %s\n",login_msg.name);
                    printf("\n    登录成功\n\n");
                    printf("    'ps'----查看上传/下载进度\n\n    'rm'----删除文件或者文件夹（不能为空）\n");
                    printf("\n    'exit'----退出程序\n\n");
                    printf("    'mgets'----VIP多点下载\n\n");
                    strcpy(login_msg.passward,"0");//code值为0；
                    getchar();
                  //  sleep(2);
                    print();
                    break;
                case LS_OK:
                 //   print();
                    printf("%s",train.buf);
                    printf("[%s@Netdisk ~%s]$ ",login_msg.name,path);
                    fflush(stdout);
                    break;
                case OPERATE_NO:
                    printf("参数错误，操作失败\n");
                //   usleep(666666);
                    print();
                    break;
                case OPERATE_OK:
             //       printf("操作成功\n");
                    if(train.Len >0)
                    {
                        memcpy(&qq_msg,train.buf,train.Len);
                        //  strcpy(code,qq_msg.buf);
                        strcpy(login_msg.passward,qq_msg.buf);
                        strcpy(path,qq_msg.buf1);
                    }
              //      usleep(666666);
                    print();
                    break;
                case DOWNLOAD_POK:
                    memcpy(&file_info,train.buf,train.Len);
                //    if(normal_child.busy_num >=10)
               //     {
               //         printf("已经达到下载数量上限，请稍后再试\n");
                //        break;
                 //   }
                    write(normal_child.fd,train.buf,train.Len);
                    printf("文件:%s 已经开始下载\n",file_info.filename);
                    print();
              //      normal_child.busy_num++;
                    break;
                case UPLOAD_OK:
                    memcpy(&file_info,train.buf,train.Len);
                    printf("file : %s 上传完成\n",file_info.filename);
                    print();
                    break;
                case UPLOAD_POK:
                //    if(normal_child.busy_num >=10)
               //     {
               //         printf("已经达到上传数量上限，请稍后再试\n");
                //        break;
              //      }
               //     normal_child.busy_num++;
                    memcpy(&file_info,train.buf,train.Len);
                    file_info.filesize = -1;//-1表示下载;
                    write(normal_child.fd,&file_info,train.Len);
                    write(normal_child.fd,uppath,100);
                    int ccode = atoi(login_msg.passward);
                    write(normal_child.fd,&ccode,4);
                    printf("文件:%s 已经开始上传\n",file_info.filename);
                    print();
                    break;
                case DOWN_MORE_POK:
                    printf("shoudao train.len = %d\n",train.Len);
                    write(vip_child.fd,train.buf,train.Len);
                    break;
                }
            }
            if(evs[i].events==EPOLLIN && evs[i].data.fd == 0)
            {
                fgets(buf,100,stdin);
                buf[strlen(buf)-1]=0;
                p = buf;
                while(*p !=' '&& *p!=0)p++;
                if(*p!=0)
                    *(p++)=0;
                if(strcmp(buf,"ls")==0)
                {
                    train.Len =0;
                    train.ctl_code =LS_Q;
                    ssend(cFd,epfd,&train);
                }else
                if(strcmp(buf,"cd")==0||strcmp(buf,"mkdir")==0||strcmp(buf,"rm")==0)
                {
                    strcpy(qq_msg.buf,buf);
                    strcpy(qq_msg.buf1,p);
                    train.Len = sizeof(QUR_msg);
                    train.ctl_code = OPERATE_Q;
                    memcpy(train.buf,&qq_msg,train.Len);
                    ssend(cFd,epfd,&train);
                }else
                if(strcmp(buf,"gets")==0)
                {
                    strcpy(train.buf,p);
                    train.Len = strlen(train.buf)+1;
                    train.ctl_code = DOWNLOAD_PRE;
                    ssend(cFd,epfd,&train);
                }else
                if(strcmp(buf,"puts")==0)
                {
                    if(p[strlen(p)-1]=='/')
                    {
                        print();
                        break;
                    }
                   ret = get_file_info(p,&file_info);
                   if(ret == -1)
                       usleep(333333);
                   else
                   {
                        train.Len = sizeof(File_info);
                        train.ctl_code = UPLOAD_PRE;
                        memcpy(train.buf,&file_info,train.Len);
                        strcpy(uppath,p);
                        ssend(cFd,epfd,&train);
 //                       printf("send file suceful\n");
                   }
                }else
                if(strcmp(buf,"ps")==0)
                {
                    file_info.filesize =-2;
                    write(normal_child.fd,&file_info,sizeof(File_info));
                }else
                if(strcmp(buf,"exit")==0)
                {
                    exit(0);
                }else
                if(strcmp(buf,"mgets")==0)
                {
                    strcpy(train.buf,p);
                    train.Len = strlen(train.buf)+1;
                    train.ctl_code = DOWN_MORE_PRE;
                    ssend(cFd,epfd,&train);
                }else
                {
                    printf("ERORR!\n");
                    print();
                }
            }
        }
    }
    return 0;
}
