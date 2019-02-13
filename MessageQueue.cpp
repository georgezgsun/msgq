#ifndef MESSAGEQUEUE_H
#include "MessageQueue.h"
#define MESSAGEQUEUE_H
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>

// key = 0 indicate to use default key based on "msgq.txt";
MessageQueue::MessageQueue(key_t key, bool server)
{
	err = 0; // 0 indicates no error;
	serverType = 1; // 1 reserved for server to read;
	
	int flag = PERMS;
	string txt = DEAFULTMSGQ;
	
	if (server)
	{
		flag |= IPC_CREAT;
		mType = 1; // 1 reserved for server;
	}
	else
		mType = (long) getpid(); // use the pid to identify the client;

	// Get the key for the message queue
	if (key == 0) key = ftok(txt.c_str(), 'B');
	if (key == -1) 
	{
		err = -2; // perror("ftok");
		return;
	}
	
	mId = msgget(key, flag);
	if (mId == -1)
	{
		err = -1; //perror("msgget");
		return;
	}
	mKey = key;
};

MessageQueue::~MessageQueue()
{
	// if it is the the server , delete the message queue
	if(serverType == mType)
		msgctl(mId, IPC_RMID, NULL);
};

int MessageQueue::SendMsg(string msg, long type)
{
	if (type <= 0) return -1;

	size_t len = msg.length();
	if (len > sizeof(buf.mText))
		len = sizeof(buf.mText);
	
	buf.mType = type;
	strcpy(buf.mText, msg.c_str());
	return msgsnd(mId, &buf, len, IPC_NOWAIT);
};

ssize_t MessageQueue::ReadMsg(string *msg, long type)
{
	if (type <= 0) return -1;

	ssize_t len = msgrcv(mId, &buf, sizeof(buf.mText), type, IPC_NOWAIT);
	buf.mText[len] = '\0';
	msg->assign(buf.mText, len);
	return len;
};
