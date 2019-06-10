#include <sys/types.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <cstring>
#include <signal.h>
#include <fstream>
#include <cstdlib>
#include <cstdio>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <unistd.h>
#include <sys/wait.h>
#include <iostream>
using namespace std;

int diskUpKey=96003;
int diskDownKey=96004;


struct msgbuff{
long mtype; /* type of message */
char mtext[64]; /* message text */
};

bool available[10];
string buffer[10];
int clk = -1;
int nextSend = -2;
struct msgbuff messagert;//to be received
struct msgbuff message;//to be sent
key_t msgqidUP,msgqidDOWN;

int checkFreeSlot()
{
  int av=-1;
  for ( int i = 0; i < 10; i++)
{
  if( available[i] == true)
  {
    av=i;
    break;
  }
}
return av;
}

void handler(int signum);
void handler1(int signum);

void Disk()
{

	for (int i=0;i<10;i++)
	{
		buffer[i]="";
	}


	struct msgbuff init_Msg;

	init_Msg.mtype=getpid();

	init_Msg.mtext[0]='/0';

	int temp = msgsnd(msgqidUP,&init_Msg, sizeof(init_Msg.mtext) ,  !IPC_NOWAIT);	

	cout<<"DiskID: "<<to_string(getpid())<<"\n";

	if (temp==-1)
		cout<<"Failed in sending ID to Kernel...\n";
	else cout<<"Succeeded in sending ID to Kernel...\n";

	while(clk==-1)
	{
		//Busy wait till clk arrives	
	}	

	cout<<"System time = 0 now...\n";

	while (1)
	{

		  int rec_val;
		  rec_val = msgrcv(msgqidDOWN, &messagert, sizeof(messagert.mtext), 0, IPC_NOWAIT);  
		  if(rec_val != -1) //message recieved
		  {
			    cout<<"Received request from kernel at CLK = "<<clk<<endl; 
		
			  if(messagert.mtext[0] == 'A')
			  {
				  int freeSlot=checkFreeSlot();//check if there is free slots to add the new data
				  if (freeSlot==-1)//disk is full ....... such a case won't happen
				  {
					  cout<<"No free slots"<<endl; 
					  message.mtext[0]='2';
					  message.mtype= getpid();
					   nextSend = clk + 3; //Latencies
				  }
				  else//avaliable places
  				  {
					  //Add the message data to the buffer
					  buffer[freeSlot] = string(messagert.mtext+2);
					  //set unavaliable
					  int k=2;
					  cout<<"Adding Message: '";
					  while (messagert.mtext[k]!='\0')
					  		cout<<messagert.mtext[k++];
					  cout<<"' to buffer slot: ";
					  available[freeSlot]= false;
					  //return notice with the operation success
					  message.mtext[0]='0';
					  cout<<message.mtext<<"\n";
					  //....
					  cout << "Adding to location " << freeSlot<<" ...." << endl;
					  nextSend = clk + 3; //Latencies
				  }
			  }
			  else //deletion
			  {
				   int target = (int )messagert.mtext[2] -48; //get loc to be deleted
				  if (!available[target])//if the target location is accuired 
				  {
					  available[target] = true;	
					  message.mtext[0]='1';
					  message.mtype= getpid();
					  buffer[target]="";	
					  cout << "Deleting ..." << endl;
           
				  }
				  else//if the target location is already free
            	  {
					  available[messagert.mtext[2]] = true;
					  message.mtext[0]='3';
					  cout << "Deletion Unsuccessful!" << endl;
				  }
				  nextSend = clk + 1; //Latencies   
			  }
		  }  
	  
	}
}



int main()
{


  pid_t pid;
  for (int i = 0; i < 10; i++)
  {
	  available[i] = true;
  }
  
  //getting message queues
  msgqidUP = msgget(diskUpKey, IPC_CREAT); //0644: explicit permissions
  msgqidDOWN = msgget(diskDownKey, IPC_CREAT); //0644: explicit permissions
  if(msgqidUP == -1 || msgqidDOWN == -1 )
  {	
    cout<<"Error in Queues getting "<<endl;
    exit(-1);
  }	

	signal(SIGUSR2, handler);  //Clock increment
    signal(SIGUSR1, handler1); //Sends av. spaces


  //operation 
  Disk();


  return 0;
}




void handler1(int signum) //returns avaliable slot count
{
	cout<<"Received Signal to check for disk capacity...\n";
    struct msgbuff msg;
    int c = 0;
    for (int i = 0; i < 10; i++)
    {
        if (available[i]==true)
        {
            c++;
        }
    }

    int send_val=-1;
  	string x;
  	x=c;
	char str[x.size()+1];
	strcpy(str, x.c_str());	// or pass &s[0]
  
  msg.mtype = 112233;   //special mtype for this operation only  	/* arbitrary value */
  msg.mtext[0]=char(c);
  while (send_val == -1)
  { 
	  send_val = msgsnd(msgqidUP, &msg, sizeof(msg.mtext), IPC_NOWAIT);
  }

}

void handler(int signum) //increments clock
{
    clk++;
	cout<<"System Time: "<<to_string(clk)<<"\n";
	//	cout << "CLK = "<<clk << endl;
	if (clk == nextSend)
	{
		message.mtype=getpid();
		int msgresultsend = msgsnd(msgqidUP,&message, sizeof(message.mtext), IPC_NOWAIT);
		if (msgresultsend==-1)
			cout<<"Failed to send result message to kernel...\n";
		else cout<<"Result message sent successfully to kernel...\n";
		cout << "Current Disk Memory Slots Status..." << endl;
		for(int i=0;i<10;i++)
		{
			if(available[i] == false ) cout << buffer[i]<<" "; else cout<<"NA ";
		} 
		cout<<endl;
	}
    
}
