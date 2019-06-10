#include<sys/types.h>
#include<sys/ipc.h>
#include<sys/msg.h>
#include<string.h>
#include<stdlib.h>
#include<stdio.h>
#include<sys/types.h>
#include<ctype.h>
#include<signal.h>
#include<sys/ipc.h>
#include<sys/wait.h>
#include<unistd.h>
#include<time.h>
#include <list>
#include<iostream>
using namespace std;

struct msgbuff
{
    long mtype;
    char mtext[64];
};

list<int> pids;

int systemTime=0;

int diskID;

int currentProcID;

void handler(int signum);

int procUpKey=95005;
int procDownKey=95006;
int diskUpKey=96003;
int diskDownKey=96004;
key_t procUpQ,procDownQ,diskUpQ,diskDownQ;
int main()
{

    
    while(1)
    {
        
        
    procUpQ = msgget(procUpKey, IPC_CREAT|0644);    
    
    if(procUpQ==-1)
    {
        perror("Error in creation of the process up queue...\n");
        exit(-1);
    }

    else 
    {
        cout<<"Creation of the process up queue successful..\n";
    }

    printf("Process Up queue ID: %d\n",procUpQ);

    diskUpQ = msgget(diskUpKey, IPC_CREAT|0644);

    if(diskUpQ==-1)
    {
        perror("Error in creation of the disk up queue...\n");
        exit(-1);
    }

    else 
    {
        cout<<"Creation of the disk up queue successful..\n";
    }

    printf("Process Up queue ID: %d\n",diskUpQ);

    procDownQ = msgget(procDownKey, IPC_CREAT|0644);    

    if(procDownQ==-1)
    {
        perror("Error in creation of the process down queue...\n");
        exit(-1);
    }

    else 
    {
        cout<<"Creation of the process down queue successful..\n";
    }
    
    printf("Process Down queue ID: %d\n",procDownQ);
    

    diskDownQ = msgget(diskDownKey, IPC_CREAT|0644);    

    if(diskDownQ==-1)
    {
        perror("Error in creation of the disk down queue...\n");
        exit(-1);
    }

    else 
    {
        cout<<"Creation of the disk down queue successful..\n";
    }
    
    printf("Disk Down queue ID: %d\n",diskDownQ);

    signal(SIGINT,handler);

    int rec_val=0;
    
    struct msgbuff message;

    rec_val=msgrcv(diskUpQ,&message,sizeof(message.mtext),0,!IPC_NOWAIT);

    if (rec_val==-1)
    {
        perror("Error in receiving the message at the kernel side ....");
        exit(-1);
    }
    else {
        cout<<"Disk ID received at kernel: "<<to_string(message.mtype)<<"\n";
        diskID=message.mtype;
    }

    while(true)
    {
        struct msgbuff msg;

        
        rec_val=msgrcv(procUpQ,&msg,sizeof(msg.mtext),0,!IPC_NOWAIT);

        if (rec_val==-1)
        {
            perror("Error in receiving the message at the kernel side ....");
            exit(-1);
        }
        else {
            cout<<"\nProcess received at kernel with ID: "<<to_string(msg.mtype)<<endl;
            pids.push_back(msg.mtype);
            if (msg.mtext[0]=='F') break;
            else cout<<"Awaiting other processes...\n";
        }
        
    }
    list<int>::iterator it;

    cout<<"Sending the initializing signal to all processes and to disk, now clk=0...\n";

    for (it=pids.begin();it!=pids.end();it++)
        kill(*it,SIGUSR2);
    kill(diskID,SIGUSR2);

    time_t timer;
    time(&timer);
    long long prev=timer;

    while (1)
    {

        time(&timer);
        if(prev!=(long long)timer)
        {
            systemTime++;
            cout<<"Incrementing system clock....\nSystem time: "+to_string(systemTime)+"..\n";
            prev=timer;
            list<int>::iterator it;
            for (it=pids.begin();it!=pids.end();it++)
                kill(*it,SIGUSR2);
            kill(diskID,SIGUSR2);
        }

        //Here will do the kernel operations regularly....
        
        //Process==>Kernel  procUpQ             Kernel==>Process     procDownQ

        //Disk===>Kernel    diskUpQ             Kernel==>Disk        diskDownQ

        bool add=true;
        bool msgSent=false;
        struct msgbuff procMsg;
        int rec_val; 
        rec_val=msgrcv(procUpQ,&procMsg,sizeof(procMsg.mtext),0,IPC_NOWAIT);

        if (rec_val!=-1)   //If a message was received from the process side...
        {

            kill(diskID,SIGUSR1);

            struct msgbuff diskMsg;

        
            rec_val=msgrcv(diskUpQ,&diskMsg,sizeof(diskMsg.mtext),112233,IPC_NOWAIT);

 		while (rec_val==-1)
            	{
                time(&timer);
                if(prev!=(long long)timer)
                {
                    systemTime++;
                    cout<<"Incrementing system clock while waiting for disk reply about spaces....\nSystem time: "+to_string(systemTime)+"..\n";
                    prev=timer;
                    list<int>::iterator it;
                    for (it=pids.begin();it!=pids.end();it++)
                      kill(*it,SIGUSR2);
                    kill(diskID,SIGUSR2);
                }
                rec_val=msgrcv(diskUpQ,&diskMsg,sizeof(diskMsg.mtext),112233,IPC_NOWAIT);

            }


            if (rec_val!=-1)
                cout<<"Message Empty disk spaces: "<<to_string(diskMsg.mtext[0])<<endl;

            if (procMsg.mtext[0]=='D')
                add=false;

            if (diskMsg.mtext[0]=='0'&&add)
            {
                procMsg.mtext[0]='2';
                int send_val=msgsnd(procDownQ,&procMsg,sizeof(procMsg.mtext),IPC_NOWAIT);
                cout<<"Kernel will not send add request for process "+to_string(procMsg.mtype)+" ,because disk has no space...\n";
            }
            
            else if (diskMsg.mtext[0]=='10'&&!add)
            {
                procMsg.mtext[0]='3';
                int send_val=msgsnd(procDownQ,&procMsg,sizeof(procMsg.mtext),IPC_NOWAIT);
                cout<<"Kernel will not send delete request for process "+to_string(procMsg.mtype)+" ,because disk is empty...\n";
            }

            else if (diskMsg.mtext[0]!='0'&&add)
            {
                cout<<"Sending add request for process "<<to_string(procMsg.mtype)<<" to the disk now to add message:\n"<<procMsg.mtext<<"\n";
                int send_val=msgsnd(diskDownQ,&procMsg,sizeof(procMsg.mtext),IPC_NOWAIT);
                msgSent=true;
            }
            
            else if (diskMsg.mtext[0]!='10'&&!add)
            {
                cout<<"Sending delete request for process "<<to_string(procMsg.mtype)<<" to the disk now...\n";
                int send_val=msgsnd(diskDownQ,&procMsg,sizeof(procMsg.mtext),IPC_NOWAIT);
                msgSent=true;
            }
            while (msgSent)
            {
                
                time(&timer);
                if(prev!=(long long)timer)
                {
                    systemTime++;
                    cout<<"Incrementing system clock while waiting for disk reply....\nSystem time: "+to_string(systemTime)+"..\n";
                    prev=timer;
                    list<int>::iterator it;
                    for (it=pids.begin();it!=pids.end();it++)
                      kill(*it,SIGUSR2);
                    kill(diskID,SIGUSR2);
                }

                struct msgbuff fromDisk;

                rec_val=msgrcv(diskUpQ,&fromDisk,sizeof(fromDisk.mtext),diskID,IPC_NOWAIT);

                if (rec_val!=-1)
                {
                    if (fromDisk.mtext[0]=='0')
                    cout<<"Disk successfully added the message for process "<< to_string(procMsg.mtype) << "...\n";
                    else if (fromDisk.mtext[0]=='1')
                    cout<<"Disk successfully deleted the message for process "<<to_string(procMsg.mtype)<<"...\n";        
                     else if (fromDisk.mtext[0]=='2')
                    cout<<"Disk unsuccessfully added the message for process because no free slots "<<to_string(procMsg.mtype)<<"...\n";        
                    else if (fromDisk.mtext[0]=='3')
                    cout<<"Disk unsuccessful in deleting the message for process "+to_string(procMsg.mtype)<<" because the requested slot was already empty...\n";
                    fromDisk.mtype=procMsg.mtype;
                    int send_val=msgsnd(procDownQ,&fromDisk,sizeof(fromDisk.mtext),IPC_NOWAIT);
                    msgSent=false;
                }


            }
            
        }

    }

    }
}


void handler(int signum)
{

 
	int x= msgctl(procUpQ, IPC_RMID, (struct msqid_ds *) 0);
    cout<<x<<endl;
    x= msgctl(procDownQ, IPC_RMID, (struct msqid_ds *) 0);
    cout<<x<<endl;
    x= msgctl(diskUpQ, IPC_RMID, (struct msqid_ds *) 0);
    cout<<x<<endl;
    x= msgctl(diskDownQ, IPC_RMID, (struct msqid_ds *) 0);
    cout<<x<<endl;
	exit(signum);

}