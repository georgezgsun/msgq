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
    mId = -1;
	filename = DEAFULTMSGQ;
	err = 0;
};

MessageQueue::MessageQueue(string keyFilename)
{
    mType = 0;
    mId = -1;
	filename = keyFilename;
	err = 0;
};

bool MessageQueue::Open(long SrvType)
{
	memset(&buf, 0, sizeof(buf));  // initialize the internal buffer
    memset(&myClients, 0, sizeof(myClients));
    memset(&mySubscriptions, 0, sizeof(mySubscriptions));
    totalSubscriptions = 0;
    totalClients = 0;
    totalMessageSent = 0;
    totalMessageReceived =0;
    autoReply = true;

    adoptWatchDog = false;
    nextDogFeedTime = 0;

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

bool MessageQueue::SetAutoReply(bool EnableAutoReply)
{
    autoReply = EnableAutoReply;
    return true;
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
    if (mId == -1) return false;
	
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
    if (rst == 0)
    {
        totalMessageSent++;
        return true;
    }
    return false;
};

// Send a packet with given length to specified service provider
bool MessageQueue::SndMsg(void *p, size_t len, long SrvType)
{
    err = -1;
//	if (mId == 0) return false;
	
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
    totalMessageSent++;
    return rst == 0;
}; 


bool MessageQueue::AskForService(long SrvType)
{
    return SndMsg(CMD_QUERY, SrvType);
};

bool MessageQueue::Subscribe(long SrvType)
{
    bool rst = SndMsg(CMD_SUBSCRIBE, SrvType);

    for (int i = 0; i < totalClients; i++)
        if (myClients[i] == SrvType)
            return rst;

    // add the new subscription to my list
    mySubscriptions[totalSubscriptions++] = SrvType;
    return rst;
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
    for (int i = 0; i < totalClients; i++)
    {
        buf.rType = myClients[i];
        rst = msgsnd(mId, &buf, buf.len + 3*sizeof (long) + 1, IPC_NOWAIT);
        err = errno;
        if (rst < 0) return false;
    }
    return true;
}; // Multicast the data stored at *p with dataLength and send it to every subscriber

// receive a message from sspecified ervice provider, like GPS, RADAR, TRIGGER. No AutoReply
string MessageQueue::RcvMsg()
{
    string msg = "";
    ssize_t len = RcvMsg(nullptr);
    if (len > 0)
        msg.assign(buf.mText, static_cast<size_t>(len));
    return msg;


}
/*
string RcvMsg1()
{
    ssize_t headerLen = 3 * sizeof(long) + 1;
    memset(&buf, 0, sizeof(buf));  // initialize the internal buffer

    err = -1;
    ssize_t len = msgrcv(mId, &buf, static_cast<size_t>(headerLen) + sizeof(buf.mText), mType, IPC_NOWAIT) - headerLen;
    totalMessageReceived++;
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
*/

 // receive a packet from specified service provider. Autoreply all requests when enabled.
ssize_t MessageQueue::RcvMsg(void *p)
{
    ssize_t headerLen = 3 * sizeof(long) + 1;
	ssize_t len = 0;
    string sub = CMD_SUBSCRIBE;
    string qry = CMD_QUERY;
    int i;
    int j;

    struct timeval tv;
	do
    {
        len = msgrcv(mId, &buf, sizeof(buf), mType, IPC_NOWAIT) - headerLen;
        err = len;
        totalMessageSent++;
        if (len <= 0)
            return 0;
 		
        if (p != nullptr)
            memcpy(p, buf.mText, static_cast<size_t>(len));
		msgType = buf.sType; // the service type of last receiving message 
		msgTS_sec = buf.sec; // the time stamp in seconds of latest receiving message
		msgTS_usec = buf.usec;
        if (!autoReply)
            return len;

        // Feed the watch dog
        gettimeofday(&tv, nullptr);
        if (adoptWatchDog && tv.tv_sec > nextDogFeedTime)
            FeedWatchDog();

        // process new subscribe requests
        if (strncmp(sub.c_str(), buf.mText, sub.length()) == 0)
		{
            if (msgType > 0) // add new subscriber to the list
                myClients[totalClients++] = msgType; // increase totalClients by 1
			else
			{  // delete the corresponding subscriber from the list
                for ( i = 0; i < totalClients; i++)
                    if (myClients[i] + msgType == 0)
					{  // move later subscriber one step up
                        for ( j = i; j < totalClients - 1; j++)
                            myClients[j] = myClients[j+1];
                        totalClients--;  // decrease totalClients by 1
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

        // process watch dog warning of service provider died. Warning format is
        // |Subscribe <Service type>
        long type = WATCHDOG;
        if ((msgType == type) && strncmp(sub.c_str(), buf.mText, sub.length()))
        {
            type = buf.mText[sub.length()+1];
            Subscribe(type); // re-subscribe, the service will be back when the service provider is up

            // stop broadcasting service data to that subscriber
            for ( i = 0; i < totalClients; i++)
                if (myClients[i] == msgType)
                {  // move later subscriber one step up
                    for ( j = i; j < totalClients - 1; j++)
                        myClients[j] = myClients[j+1];
                    totalClients--;  // decrease totalClients by 1
                }

            return 0;
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
