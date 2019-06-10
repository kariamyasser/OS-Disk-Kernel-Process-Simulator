#include <iostream>
#include <sys/wait.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include  <ctype.h>
#include <sys/msg.h>
#include <string>
#include <string.h>
#include <list>
#include <fstream>
#include <sstream>
#include <signal.h>

using namespace std;


int clk = -1;

	key_t Up = msgget(95005,IPC_CREAT);
    key_t Down = msgget(95006,IPC_CREAT);
 


void handlerUser(int signum);

struct msgbuff
{
   long mtype;
   char mtext[64];
};


struct Msgy
{
	int time ;
	 string operation;
	 string data;
};
list <Msgy> Queuemsg;


void handlerUser(int signum)
{
   
    clk ++;

	cout<<"System Time: "<<to_string(clk)<<"\n";

}



int main()
{   
	while(1)
	{
	char* FileName = new char [30];
	char Open = ' ';
	
	cout << "Enter file name : " << endl;
	cin.getline(FileName,30);


 cout << endl << "Do you want to open another file...(y/n)??" << endl;
 cin >> Open;

	struct msgbuff Process_Starter;
	int pid = getpid();
	Process_Starter.mtype = pid;

   if(Open == 'y' ) // this is last file 
   {    
	   char t = 't';
	   Process_Starter.mtext[0] = 'T';
	   Process_Starter.mtext[1] = '\0';

	  key_t temp = msgsnd(Up,&Process_Starter, sizeof(Process_Starter.mtext) ,  IPC_NOWAIT);
   }
   else if(Open == 'n') // another file will be read
   {
	   char f = 'f';
	   Process_Starter.mtext[0] = 'F';
	   Process_Starter.mtext[1] = '\0';
	 key_t temp = msgsnd(Up,&Process_Starter, sizeof(Process_Starter.mtext) ,  IPC_NOWAIT);
	
   }

 signal(SIGUSR2,handlerUser);

while(clk==-1)
{
	//Busy wait till clk arrives
}
cout<<"System clock now = 0...\n";

	int temp ; 
	string temp2;
	
	ifstream input(FileName,ios::in);

	 if(input.is_open())
	{
		cout<<"Data in file:"<<endl;
		int a;
		while( input>>a  )
		{
			struct Msgy flag;

			flag.time=0;

			flag.operation="";

			flag.data="";

			flag.time=a;
		
			cout<<to_string(flag.time)<<" ";

			input>> flag.operation;
			
			cout<<flag.operation<<" ";

			getline(input,temp2);

			cout<<temp2<<endl;

			flag.data= temp2;

			flag.data[flag.data.size()]='\0';

			Queuemsg.push_back(flag);

		}
	}



	input.close();

	while( Queuemsg.size() !=0 )
	{
		

	 	struct msgbuff Message_Sent;
		
	 	struct msgbuff Message_Rec;
		
		struct Msgy flag; 
		flag = Queuemsg.front();
		char Textato[64];

		for (int j=0;j<64;j++)
			{
				Textato[j]='\0';
			}


		if(flag.time <= clk)
		{   
			Message_Sent.mtype = pid;
			if( flag.operation == "ADD" )
				{   
					Textato[0] = 'A';

					int k=0;

					int l=1;

					while (flag.data[k]!='\0')
					{
						Textato[l++]=flag.data[k++];
					}
				}
				else if( flag.operation == "DEL" )
				{
				   
					Textato[0] = 'D';

					Textato[1]=' ';

					Textato[2]=flag.data[1];
					
				}

			strcpy(Message_Sent.mtext, Textato );


			printf("Message to be sent: %s\n",Message_Sent.mtext);

			 int tempSend = msgsnd(Up,&Message_Sent, sizeof(Message_Sent.mtext) , IPC_NOWAIT);
				Queuemsg.pop_front();
				 int tempRec = -1 ;
				 while( tempRec == -1)
				 {
					 tempRec = msgrcv(Down,&Message_Rec, sizeof(Message_Sent.mtext) , pid , IPC_NOWAIT);
				 }
				if(    strcmp(Message_Rec.mtext,"0") == 0  )
				cout << "Successful ADD " << endl;
				else if( strcmp(Message_Rec.mtext,"1") == 0)
				cout << "Successful DEL " << endl;
				else if( strcmp(Message_Rec.mtext,"2") == 0)
				cout << "unable to ADD " << endl;
				else if( strcmp(Message_Rec.mtext,"3") == 0)
				cout << "unable to DEL " << endl;
		
		}
	}

	cout<<"Process finished all instructions..\n Exiting now..\n";
	exit(0);
}
}
