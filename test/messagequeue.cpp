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
#include <sys/time.h>

// key = 0 indicate to use default key based on "msgq.txt";
MessageQueue::MessageQueue(string keyFilename, long type)
{
    mType = type;
    err = -1; // 0 indicates no error;
    if (mType <= 0) return;
    if(keyFilename == "") keyFilename = DEAFULTMSGQ;

    // Get the key for the message queue
    err = -2; // perror("ftok");
    key_t mKey = ftok(keyFilename.c_str(), 'B');
    if (mKey == -1) return;

    err = -3; //perror("msgget");
    mId = msgget(mKey, PERMS | IPC_CREAT);
    if (mId == -1) return;

    err = 0;
};

MessageQueue::~MessageQueue()
{
    // if it is the the server , delete the message queue
    if(1 == mType)
        msgctl(mId, IPC_RMID, nullptr);
};
bool MessageQueue::SndMsg(string msg, long SrvType)
{
    err = -1;
    if (SrvType <= 0) return -1;

    struct timeval tv;
    gettimeofday(&tv, nullptr);
    buf.rType = SrvType;
    buf.pkt.sType = mType;
    buf.pkt.sec = tv.tv_sec;
    buf.pkt.usec = tv.tv_usec;
    size_t len = msg.length();
    if (len > sizeof(buf.pkt.mText))
        len = sizeof(buf.pkt.mText);

    strcpy(buf.pkt.mText, msg.c_str());
    return msgsnd(mId, &buf, len + 3*sizeof (long), IPC_NOWAIT) >= 0;
};

bool MessageQueue::AskForData(long SrvType)
{
    return SndMsg("Query", SrvType);
};

string MessageQueue::RcvMsg(long SrvType)
{
    ssize_t headerLen = 3 * sizeof(long);
    buf.pkt.sType = 0;
    buf.pkt.sec = 0;
    buf.pkt.usec = 0;
    err = -1;
    if (SrvType <= 0) return "";

    err = -2;
    ssize_t len = msgrcv(mId, &buf, sizeof(buf.pkt), SrvType, IPC_NOWAIT) - headerLen;
    if (len <= 0) return "";

    err = 0;
    buf.pkt.mText[len] = '\0';
    string msg;
    msg.assign(buf.pkt.mText, static_cast<unsigned long>(len));
    return msg;
};

MsgPkt MessageQueue::RcvPkt(long SrvType)
{
    ssize_t headerLen = 3 * sizeof(long);
    err = -1;
    buf.pkt.sType = 0;
    buf.pkt.sec = 0;
    buf.pkt.usec = 0;
    if (SrvType <= 0) return buf.pkt;

    err = -2;
    ssize_t len = msgrcv(mId, &buf, sizeof(buf.pkt), SrvType, IPC_NOWAIT) - headerLen;
    if (len <= 0) return buf.pkt;

    err = 0;
    return buf.pkt;
}
