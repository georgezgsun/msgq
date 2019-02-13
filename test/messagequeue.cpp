#include "messagequeue.h"

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
    isServer = true; // 1 reserved for server to read;
    mKey = 0;

    int flag = PERMS;
    string txt = DEAFULTMSGQ;

    if (isServer)
    {
        flag |= IPC_CREAT;
        mType = 1; // 1 reserved for server;
    }
    else
        mType = static_cast<long>(getpid()); // use the pid to identify the client;

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
    if(isServer)
        msgctl(mId, IPC_RMID, nullptr);
};

bool MessageQueue::SendMsg(string msg, long type)
{
    if (type <= 0) return -1;

    size_t len = msg.length();
    if (len > sizeof(buf.mText))
        len = sizeof(buf.mText);

    buf.mType = type;
    strcpy(buf.mText, msg.c_str());
    return msgsnd(mId, &buf, len, IPC_NOWAIT) == 0;
};

ssize_t MessageQueue::ReadMsg(string *msg, long type)
{
    if (type <= 0) return -1;

    ssize_t len = msgrcv(mId, &buf, sizeof(buf.mText), type, IPC_NOWAIT);
    if (len < 0) return len;
    buf.mText[len] = '\0';
    msg->assign(buf.mText, static_cast<unsigned long>(len));
    return len;
};
