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
MessageQueue::MessageQueue()
{
    mType = 0;
	mId = 0;
	filename = DEAFULTMSGQ;
	err = 0;
};

MessageQueue::MessageQueue(string keyFilename)
{
    mType = 0;
	mId = 0;
	filename = keyFilename;
	err = 0;
};

bool MessageQueue::Open(long SrvType)
{
	memset(&buf, 0, sizeof(buf));  // initialize the internal buffer
    memset(&subscribers, 0, sizeof(subscribers));
	totalSubcribers = 0;
    mType = SrvType;
	
    err = -1; // 0 indicates no error;
    if (mType <= 0) return false;

    // Get the key for the message queue
    err = -2; // perror("ftok");
    key_t mKey = ftok(filename.c_str(), 'B');
    if (mKey == -1) return false;

    err = -3; //perror("msgget");
    mId = msgget(mKey, PERMS | IPC_CREAT);
    if (mId == -1) return false;

    err = 0;
	return true;
}

MessageQueue::~MessageQueue()
{
    // if it is the the server , delete the message queue
    if(1 == mType)
        msgctl(mId, IPC_RMID, nullptr);
};

bool MessageQueue::SndMsg(string msg, long SrvType)
{
    err = -1;
	if (mId == 0) return false;
	
	err = -2;
    if (SrvType <= 0) return false;

    struct timeval tv;
    gettimeofday(&tv, nullptr);
    buf.rType = SrvType;
    buf.sType = mType;
    buf.sec = tv.tv_sec;
    buf.usec = tv.tv_usec;
	
    size_t len = msg.length();
    if (len > sizeof(buf.mText))
        len = sizeof(buf.mText);
    buf.len = static_cast<unsigned char>(len);

    memcpy(buf.mText, msg.c_str(), len);
	
	int rst = msgsnd(mId, &buf, len + 3*sizeof (long) + 1, IPC_NOWAIT);
	err = errno;
    return rst == 0;
};

// Send a packet with given length to specified service provider
bool MessageQueue::SndMsg(void *p, size_t len, long SrvType)
{
    err = -1;
	if (mId == 0) return false;
	
	err = -2;
    if (SrvType <= 0) return false;

    struct timeval tv;
    gettimeofday(&tv, nullptr);
    buf.rType = SrvType;
    buf.sType = mType;
    buf.sec = tv.tv_sec;
    buf.usec = tv.tv_usec;
    if (len > sizeof(buf.mText))
        len = sizeof(buf.mText);
    buf.len = static_cast<unsigned char>(len);

    memcpy(buf.mText, p, len);
	int rst = msgsnd(mId, &buf, len + 3*sizeof (long) + 1, IPC_NOWAIT);
	err = errno;
    return rst == 0;
}; 


bool MessageQueue::AskForService(long SrvType)
{
    return SndMsg(CMD_QUERY, SrvType);
};

bool MessageQueue::Subscribe(long SrvType)
{
    return SndMsg(CMD_SUBSCRIBE, SrvType);
};

bool MessageQueue::BroadcastUpdate(void *p, unsigned char dataLength)
{
    struct timeval tv;
    gettimeofday(&tv, nullptr);

    buf.sType = mType;
    buf.sec = tv.tv_sec;
    buf.usec = tv.tv_usec;
    buf.len = dataLength;
    memcpy(buf.mText, p, buf.len);

    int rst;
    for (int i = 0; i < totalSubcribers; i++)
    {
        buf.rType = subscribers[i];
        rst = msgsnd(mId, &buf, buf.len + 3*sizeof (long) + 1, IPC_NOWAIT);
        err = errno;
        if (rst < 0) return false;
    }
    return true;
}; // Multicast the data stored at *p with dataLength and send it to every subscriber

// receive a message from sspecified ervice provider, like GPS, RADAR, TRIGGER. No AutoReply
string MessageQueue::RcvMsg()
{
    ssize_t headerLen = 3 * sizeof(long) + 1;
    memset(&buf, 0, sizeof(buf));  // initialize the internal buffer

    err = -1;
    ssize_t len = msgrcv(mId, &buf, static_cast<size_t>(headerLen) + sizeof(buf.mText), mType, IPC_NOWAIT) - headerLen;
    if (len < 0) return "";

    msgType = buf.sType; // the service type of last receiving message
    msgTS_sec = buf.sec; // the time stamp in seconds of latest receiving message
    msgTS_usec = buf.usec;

    err = 0;
//    buf.pkt.mText[len] = '\0';
    string msg;
    msg.assign(buf.mText, static_cast<size_t>(len));
    return msg;
};

 // receive a packet from specified service provider. Autoreply all requests when enabled.
ssize_t MessageQueue::RcvMsg(void *p,bool AutoReply)
{
    ssize_t headerLen = 3 * sizeof(long) + 1;
	ssize_t len = 0;
    string sub = CMD_SUBSCRIBE;
    string qry = CMD_QUERY;
	do
    {
        len = msgrcv(mId, &buf, sizeof(buf), mType, IPC_NOWAIT) - headerLen;
		err = errno;
		if (len <= 0) return len;
 		
        memcpy(p, buf.mText, static_cast<size_t>(len));
		msgType = buf.sType; // the service type of last receiving message 
		msgTS_sec = buf.sec; // the time stamp in seconds of latest receiving message
		msgTS_usec = buf.usec;
		if (!AutoReply) return len;

		// process subscription
        if (strncmp(sub.c_str(), buf.mText, sub.length()) == 0)
		{
            if (msgType > 0) // add new subscriber to the list
				subscribers[totalSubcribers++] = msgType; // increase totalSubcribers by 1
			else
			{  // delete the corresponding subscriber from the list
				for ( int i = 0; i < totalSubcribers; i++)
                    if (subscribers[i] + msgType == 0)
					{  // move later subscriber one step up
						for (int j = i; j < totalSubcribers - 1; j++)
                            subscribers[j] = subscribers[j+1];
						totalSubcribers--;  // decrease totalSubcribers by 1
					}
			}
			continue; // continue to read more messages 
		}
		
		// process request
        if (strncmp(qry.c_str(), buf.mText, qry.length()) == 0)
		{
			struct timeval tv;
			gettimeofday(&tv, nullptr);
            buf.rType = msgType;
			buf.sType = mType;
			buf.sec = tv.tv_sec;
			buf.usec = tv.tv_usec;
			buf.len = dataLength;
			
			memcpy(buf.mText, mData, dataLength);
            len = msgsnd(mId, &buf,  static_cast<size_t>(len) + 3*sizeof (long) + 1, IPC_NOWAIT);
			err = errno;
			continue;   // continue to read more messages
		}
		
		// any other receiving message will be processed outside
		return len;
    } while (len > 0);
	
    return len;
}

// Feed the dog at watchdog server
bool MessageQueue::FeedWatchDog()
{
    string cmd = CMD_QUERY;
    long SrvType = WATCHDOG;
    return SndMsg(cmd, SrvType);
}; 

// Send a log to log server
bool MessageQueue::Log(string logContent, long logType)
{
    string cmd = CMD_LOG;
    long SrvType = WATCHDOG;
    return SndMsg(cmd + " " + to_string(logType) + " " +logContent, SrvType);
};
	
// Send a request to database server to query for the value of keyword. The result will be placed in the queue by database server.
bool MessageQueue::RequestDatabaseQuery(string keyword)
{
    string cmd = CMD_QUERY;
    long SrvType = DATABASE;
    return SndMsg(cmd +  " " + keyword, SrvType);
};  

// Send a request to database to update the value of keyword with newvalue. The database server will take care of the data type casting. 
bool MessageQueue::RequestDatabaseUpdate(string keyword, string newvalue)
{
    string cmd = CMD_UPDATE;
    long SrvType = DATABASE;
    return SndMsg(cmd + " " + keyword + " " + newvalue, SrvType);
};
