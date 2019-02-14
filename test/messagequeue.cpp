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
MessageQueue::MessageQueue(string name)
{
    for (int i = 0; i < static_cast<int>(sizeof(Ids)); i++)
    {
        if (strcasecmp(name.c_str(), Ids[i].c_str()))
        {
            mType = i+1;
            break;
        }
    }

    err = -1; // 0 indicates no error;
    if (mType == 0) return;

    int flag = PERMS;
    if (mType == 1) // This idicates a server
        flag |= IPC_CREAT;

    // Get the key for the message queue
    err = -2; // perror("ftok");
    string txt = DEAFULTMSGQ;
    key_t mKey = ftok(txt.c_str(), 'B');
    if (mKey == -1) return;

    err = -3; //perror("msgget");
    mId = msgget(mKey, flag);
    if (mId == -1) return;

    err = 0;
};

MessageQueue::MessageQueue()
{
    MessageQueue("SERVER");
}

MessageQueue::~MessageQueue()
{
    // if it is the the server , delete the message queue
    if(1 == mType)
        msgctl(mId, IPC_RMID, nullptr);
};

bool MessageQueue::SndMsg(string msg)
{
    err = -1;
    if (rType <= 0) return -1;

    unsigned long n = static_cast<unsigned long>( sprintf( buf.mText, "%d10", static_cast<int>(rType) ) );
    size_t len = msg.length() - n;
    if (len > sizeof(buf.mText))
        len = sizeof(buf.mText);

    buf.mType = rType;
    strcpy(buf.mText + n, msg.c_str());
    return msgsnd(mId, &buf, len + n, IPC_NOWAIT) >= 0;
};

bool MessageQueue::SndMsg(string msg, long type)
{
    rType = type;
    return SndMsg(msg);
};

string MessageQueue::RcvMsg(string name)
{
    for (int i = 0; i < static_cast<int>(sizeof(Ids)); i++)
    {
        if (strcasecmp(name.c_str(), Ids[i].c_str()))
        {
            rType = i+1;
            break;
        }
    }
    err = -1;
    if (rType <= 0) return "";

    return RcvMsg();
};

string MessageQueue::RcvMsg()
{
    err = -1;
    if (rType <= 0) return "";

    err = -2;
    ssize_t len = msgrcv(mId, &buf, sizeof(buf.mText), rType, IPC_NOWAIT);
    if (len < 0) return "";

    err = 0;
    buf.mText[len] = '\0';
    string msg;
    msg.assign(buf.mText, static_cast<unsigned long>(len));
    return msg;
}
