#include<iostream>
#include<netinet/in.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<sys/select.h>
#include<sys/time.h>
#include<unistd.h>
#include<errno.h>
#include<string.h>
#include<arpa/inet.h>
#include<map>
#include<list>
#include<cstdio>
#include<stdlib.h>
#include<stdint.h>
using namespace std;

const int MAX_CLIENT = 200;
const int MAX_RECV_SIZE = 1000;
const int MAX_BUFFER_SIZE = 900;
const int SEND_DATA = 1;
const int SEND_CMD = 2;
const int INITIAL = 0;
const int SENDING = 1;
const int FINISH = 2;

struct header
{
	int32_t cmd;
	int32_t state;
	int32_t length;
	char name[40];
	char data[MAX_BUFFER_SIZE];
};

struct header periodCommand;

struct User
{
	User(){}
	User(int sock){fd = sock;commandData.clear();}
	//User(const struct User &other){fd = other.fd;commandData = other.commandData;}
	struct User& operator=(const struct User &other){fd = other.fd;commandData = other.commandData;}
	~User(){
		for (list<struct header*>::iterator Iter = commandData.begin(); Iter != commandData.end(); Iter++)
			free(*Iter);
		commandData.clear();	
	}
	int fd;	
	list<struct header*>commandData;
};

std::map<int,User>userMap;

void pushDataToPeer(struct header command)
{
	std::map<int,User>::iterator Iter;
	for(Iter = userMap.begin(); Iter != userMap.end(); Iter++)
	{
		struct header *copy = (struct header*)malloc(sizeof(struct header));
		memcpy(copy,&command,sizeof(struct header));
		Iter -> second.commandData.push_back(copy);	
	}
}
int period = 0,totalPeriodNumber = 0;

void pushPeriodCommand(char *command)
{
	char userCommand[50] = {0},userAction[50] = {0};
	sscanf(command,"%s%s%d%d",userCommand,userAction,&totalPeriodNumber,&period);
	cout<<"pushPeriodCommand,command: "<<userCommand<<",userAction: "<<userAction<<",totalPeriodNumber:"<<totalPeriodNumber<<",period: "<<period<<endl;
	memset(&periodCommand,0,sizeof(struct header));
	periodCommand.cmd = SEND_CMD;
	sprintf(periodCommand.data,"%s %s",userCommand,userAction);
}

void sendCommandPeriodly()
{
	static unsigned int previousSendTime = 0;
	struct timeval tv;
	gettimeofday(&tv,NULL);
	if( tv.tv_sec - previousSendTime >= 1)
		previousSendTime = tv.tv_sec;
	else 
		return;

	if(totalPeriodNumber <= 0)
		return;

	if(userMap.empty())
		return;

	cout<<"sendCommandPeriodly"<<endl;

	static int previousSendFd = -1;
	std::map<int,User>::iterator Iter;

	while( ( (Iter = userMap.find(previousSendFd)) == userMap.end() ))
	{
		if( (previousSendFd <= (userMap.end()--) -> first) && (previousSendFd >= userMap.begin() -> first))
		{
			cout<<"1"<<endl;
			previousSendFd++;
		}
		else if( previousSendFd < userMap.begin() -> first)
		{
			cout<<"2"<<endl;
			previousSendFd = userMap.begin() -> first;
			break;
		}
		else
		{
			cout<<"3"<<endl;
			previousSendFd = userMap.begin() -> first;
			break;
		}
	}
	cout<<"previousSendFd:"<<previousSendFd<<endl;
	Iter = userMap.find(previousSendFd);
	for(int index = 0 ; index < period ; index++,Iter++)
	{
		if(Iter == userMap.end())
			Iter = userMap.begin();
		struct header *copy = (struct header*)malloc(sizeof(struct header));
		memcpy(copy,&periodCommand,sizeof(struct header));
		Iter -> second.commandData.push_back(copy);	
		cout<<"sendCommandPeriodly,fd: "<<Iter -> first<<endl;
		if(--totalPeriodNumber <= 0)
		{
			cout<<"periodCommand send finish"<<endl;
			break;
		}
	}
	if(Iter == userMap.end())
		Iter = userMap.begin();
	previousSendFd = Iter -> first;
}

void sendFile(char *fileName)
{
	char name[50] = {0};
	sscanf(fileName,"%s",name);
	FILE *fp = fopen(name,"rb");	
	if(fp == NULL)
	{
		cout<<"No such file exists:"<<name<<",length:"<<strlen(name)<<endl;
		return;
	}
	fseek(fp,0,SEEK_END);
	long fileSize = ftell(fp);
	fseek(fp,0,SEEK_SET);

	int position = 0;
	struct header reply;
	memset(&reply,0,sizeof(struct header));
	reply.cmd = SEND_DATA;
	sprintf(reply.name,"%s",name);

	while(true)
	{
		memset(reply.data,0,MAX_BUFFER_SIZE);
		if(position == 0)
		{
			reply.state = INITIAL;
			if( MAX_BUFFER_SIZE <= fileSize )
				reply.length = MAX_BUFFER_SIZE;
			else
				reply.length = fileSize;
			position += reply.length;
		}
		else
		{
			if( position + MAX_BUFFER_SIZE <= fileSize )
			{
				reply.state = SENDING;
				reply.length = MAX_BUFFER_SIZE;
			}
			else
				break;
			position += reply.length;
		}
		fread(reply.data,1,reply.length,fp);
		pushDataToPeer(reply);
	}

	reply.state = FINISH;
	reply.length = fileSize - position;
	memset(reply.data,0,MAX_BUFFER_SIZE);
	fread(reply.data,1,reply.length,fp);
	pushDataToPeer(reply);
	fclose(fp);
}

void pushCommandToPeer(char *command)
{
	cout<<"command: "<<command<<endl;
	struct header reply;
	memset(&reply,0,sizeof(struct header));
	reply.cmd = SEND_CMD;
	memcpy(reply.data,command,strlen(command));	
	pushDataToPeer(reply);
}

void parseUserCommand()
{
	char command[200] = {0};
	fgets(command,200,stdin);

	char *needle = NULL;	
	if((needle = strstr(command,"TIME") ) != NULL)
		pushPeriodCommand(needle + 4);
	else if( (needle = strstr(command,"EXE") ) != NULL) 	
		pushCommandToPeer(command);
	else if((needle = strstr(command,"DEL") ) != NULL)
		pushCommandToPeer(command);
	else if((needle = strstr(command,"FIN") ) != NULL)
		pushCommandToPeer(command);
	else if((needle = strstr(command,"DOWN") ) != NULL)
		pushCommandToPeer(command);
	else if((needle = strstr(command,"DOWN") ) != NULL)
		pushCommandToPeer(command);
	else if((needle = strstr(command,"SEND") ) != NULL)
		sendFile(needle +4);
	else if((needle = strstr(command,"SIZE") ) != NULL)
		cout<<"Total client number: "<<userMap.size()<<endl;
	else if((needle = strstr(command,"SYS") ) != NULL)
		pushCommandToPeer(command);
	else
		cout<<"Error command: "<<command<<endl;
}

int main()
{
	struct sockaddr_in server;
	memset(&server,0,sizeof(struct sockaddr_in));
	server.sin_family = AF_INET;
	server.sin_port = htons(5555);
	inet_aton("140.114.71.174",&(server.sin_addr));

	int serverFd = socket(AF_INET,SOCK_STREAM,0);
	int optval = 1;
 	if( setsockopt(serverFd, SOL_SOCKET, SO_REUSEADDR, (const void *)&optval , sizeof(optval)) == -1)
		cout<<strerror(errno)<<endl;

	if(bind(serverFd,(struct sockaddr*)(&server),sizeof(server)) < 0 )
	{
		cout<<strerror(errno)<<endl;
		return 0;
	}
		
	if(listen(serverFd,10) != 0)
	{
		cout<<strerror(errno)<<endl;
		return 0;
	}	

	struct sockaddr client;
	socklen_t clientLength = 0;
	list<int> peerFds;

	fd_set allSet,readSet,writeSet;
	FD_ZERO(&allSet);
	readSet = allSet;
	cout<<"Server starts, waiting for clients"<<endl;
	while(true)
	{
		readSet = allSet;
		writeSet = allSet;
		FD_SET(serverFd,&readSet);
		FD_SET(fileno(stdin),&readSet);

		int totalWorkable = select(MAX_CLIENT,&readSet,&writeSet,NULL,NULL);
		int connectFd = 0;

		if(FD_ISSET(serverFd,&readSet))
		{
			if( (connectFd = accept(serverFd,&client,&clientLength)) > 0)
			{
				peerFds.push_back(connectFd);
				struct User temp(connectFd);
				userMap[connectFd] = temp;
				FD_SET(connectFd,&allSet);
				cout<<"accept a new client: "<<connectFd<<endl;
			}
			else
			{
				cout<<strerror(errno)<<endl;
				char c =fgetc(stdin);
			}

			if(--totalWorkable <= 0)
				continue;
		}

		if(FD_ISSET(fileno(stdin),&readSet))	
			parseUserCommand();

		struct header recvData;
		int receiveByte = 0;	
		for(list<int>::iterator Iter = peerFds.begin(); Iter != peerFds.end(); Iter++)
		{
			if(FD_ISSET(*Iter,&readSet))
			{
				if( receiveByte = recv(*Iter,&recvData,MAX_RECV_SIZE,0) < 0)		
				{
					FD_CLR(*Iter,&allSet);
					cout<<"Peer leaves "<<endl;	
					userMap.erase(*Iter);
					Iter = peerFds.erase(Iter);
				}
				else
				{
					cout<<(recvData.data)<<endl;
				}

				if(--totalWorkable <= 0)
					continue;
			}
		}

		int sentByte = 0;
		struct header *sendData;
		for(list<int>::iterator Iter = peerFds.begin(); Iter != peerFds.end(); Iter++)
		{
			if(FD_ISSET(*Iter,&writeSet))
			{
				if( !userMap[*Iter].commandData.empty())
				{	
					sendData = userMap[*Iter].commandData.front();
					if( (sentByte = send(*Iter,(void*)(sendData),MAX_RECV_SIZE,0)) < 0)		
					{
						FD_CLR(*Iter,&allSet);
						cout<<"Peer leaves "<<endl;	
						userMap.erase(*Iter);
						Iter = peerFds.erase(Iter);
					}
					else
					{
						//cout<<"Sending: "<<sendData->state<<","<<sentByte<<","<<sendData->length<<endl;
						userMap[*Iter].commandData.pop_front();
						free(sendData);
						sendData = NULL;
					}
				}	
				if(--totalWorkable <= 0)
					continue;
			}
		}
		sendCommandPeriodly();
	}
	return 0;
}
