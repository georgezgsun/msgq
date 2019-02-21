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

#define PERMS 0644
#define DEAFULTMSGQ "/tmp/msgq.txt"

#define CMD_ONBOARD 1
#define CMD_LIST 1
#define CMD_DATABASEINIT 2
#define CMD_DATABASEUPDATE 2
#define CMD_DATABASEQUERY 3
#define CMD_LOG 4
#define CMD_WATCHDOG 5
#define CMD_DOWN 5
#define CMD_STOP 6
#define CMD_SUBSCRIBE 7
#define CMD_QUERY 8

#define SERVER 1
#define HEADER 1
#define BASE 1
#define CENTER 1
#define COMMANDER 1
#define CORE 1

struct Onboard
{
    char cmd;
    char type;
    char title[9];
    pid_t pid;
    pid_t ppid;
};

MessageQueue::MessageQueue(string keyFilename, string myServiceTitle, long myServiceType)
{
    mType = myServiceType;
    mTitle = myServiceTitle;
    // Get the key for the message queue from given filename, if empty using default name
    if (keyFilename.length() == 0)
        keyFilename = DEAFULTMSGQ;

    // For server starts, make sure the keyFilename exists.
    if ((mType == 1) && access( keyFilename.c_str(), F_OK ) == -1)
    { //If not create one.
        fopen(keyFilename.c_str(),"w+");
    }
    mKey = ftok(keyFilename.c_str(), 'B');
    mId = -1;
	err = 0;
};

bool MessageQueue::Open()
{
    // initialize the internal buffers
    memset(&buf, 0, sizeof(buf));
    memset(&myClients, 0, sizeof(myClients));
    memset(&mySubscriptions, 0, sizeof(mySubscriptions));
    memset(&services, 0, sizeof(services));

    totalSubscriptions = 0;
    totalClients = 0;
    totalMessageSent = 0;
    totalMessageReceived =0;
    totalServices = 0;
    totalProperties = 0;

    Onboard report;

    err = -1; // 0 indicates no error;
    if (mType <= 0)
        return false;

    err = -2; // perror("ftok");
    if (mKey == -1)
        return false;

    err = -3; //perror("msgget");
    mId = msgget(mKey, PERMS | IPC_CREAT);
    if (mId == -1)
        return false;
    printf("MsgQue ID : %ld\n", mId);

    err = -4; // mTitle too long
    if (mTitle.length() > sizeof(report.title))
        return false;

    if (mType == 1)
    {// Read off all previous messages
        do
        {
        } while (msgrcv(mId, &buf, sizeof(buf), 0, IPC_NOWAIT) > 0);
    }
    else
    {// Register itself to the server
        report.cmd = CMD_ONBOARD;
        report.pid = getpid();
        report.ppid = getppid();
        report.type = static_cast<char>(mType & 0xFF);
        strncpy(report.title, mTitle.c_str(), mTitle.length());
        SndMsg(&report, sizeof(Onboard), 1);
    }

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

bool MessageQueue::AskForServiceOnce(long SrvType)
{
    char txt = CMD_QUERY;
    string msg;
    msg.assign(1, txt);
    return SndMsg(msg, SrvType);
};

long MessageQueue::GetServiceType(string ServiceTitle)
{
    for (int i = 0; i < totalServices; i++)
        if (strncmp(services[i], ServiceTitle.c_str(), ServiceTitle.length() ) == 0)
            return static_cast<long>(services[i][8]);
    return 0;
};

string MessageQueue::GetServiceTitle(long ServiceType)
{
    string str = "";
    if (ServiceType > 255 || ServiceType < 2)
        return str;

    for (size_t i = 0; i < totalServices; i++)
        if (services[i][9] == (ServiceType & 0xFF))
        {
            str.assign(services[i], 9); // Right now the title is always 8 bytes
            return str;
        }
    return "";
};

bool MessageQueue::Subscribe(string ServiceTitle)
{
    return Subscribe(GetServiceType(ServiceTitle));
};

bool MessageQueue::Subscribe(long SrvType)
{
    err = -1;
    if (SrvType <= 0)
        return false;

    char txt = CMD_SUBSCRIBE;
    string msg;
    msg.assign(1, txt);
    bool rst = SndMsg(msg, SrvType);

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
    char b[255] = "";
    string msg;
    ssize_t len = RcvMsg(b);
    if (len > 0)
        msg.assign(buf.mText, static_cast<size_t>(len));
    return msg;
}

 // receive a packet from specified service provider. Autoreply all requests when enabled.
ssize_t MessageQueue::RcvMsg(void *p)
{
    size_t i;
    size_t j;
    size_t n;
    struct timeval tv;
    ssize_t headerLen = 3 * sizeof(long) + 1;

	do
    {
        buf.rType = mType;
        ssize_t len = msgrcv(mId, &buf, sizeof(buf), mType, IPC_NOWAIT) - headerLen;
        err = len;
        if (len <= 0)
            return 0;
        totalMessageReceived++;
        err = 0;

        if (p != nullptr)
            memcpy(p, buf.mText, static_cast<size_t>(len));
		msgType = buf.sType; // the service type of last receiving message 
		msgTS_sec = buf.sec; // the time stamp in seconds of latest receiving message
		msgTS_usec = buf.usec;
        buf.len = static_cast<unsigned char>(len);

        // no auto reply forr those normal receiving
        if (buf.mText[0] > 31)
            return len;

        // auto reply for new subscription requests
        if (buf.mText[0] == CMD_SUBSCRIBE)
        {
            if (msgType > 0) // add new subscriber to the list when the type is positive
                myClients[totalClients++] = msgType; // increase totalClients by 1
            else
            {  // delete the corresponding subscriber from the list when the type is negative
                for ( i = 0; i < totalClients; i++)
                    if (myClients[i] + msgType == 0)
                    {  // move later subscriber one step up
                        for ( j = i; j < totalClients - 1; j++)
                            myClients[j] = myClients[j+1];
                        totalClients--;  // decrease totalClients by 1
                    }
            }
            // no continouse autoreply for server
            if (mType != 1)
                continue; // continue to read next messages
        }

        // auto reply for service request
        //if (strncmp(qry.c_str(), buf.mText, qry.length()) == 0)
        if (buf.mText[0] == CMD_QUERY)
        {
            gettimeofday(&tv, nullptr);
            buf.rType = msgType;
            buf.sType = mType;
            buf.sec = tv.tv_sec;
            buf.usec = tv.tv_usec;
            buf.len = dataLength;

            memcpy(buf.mText, mData, dataLength);
            len = msgsnd(mId, &buf,  static_cast<size_t>(buf.len) + 3*sizeof (long) + 1, IPC_NOWAIT);
            err = errno;

            // no continouse autoreply for server
            if (mType != 1)
                continue; // continue to read next message
        }

        // no other auto reply for server sending and reading
        if ((mType == 1) || (msgType != 1))
            return buf.len;

        // auto process the services list reply
        // <List><n>[title1][t1][title2][t2] ... [titlen][tn] ; title is of const length 9
        if (buf.mText[0] == CMD_LIST)
        {
            //i = lst.length();
            n = static_cast<size_t>(buf.mText[1]);
            if (n > 28)
                n = 28;
            for (i = 0; i < n; i++)
                memcpy(services[totalServices + i], buf.mText + 2 + i * 10, 10);
            totalServices += n;
            continue;
        }

        // auto process the database init reply
        // <db><n>[key_1][key_2] ... [key_n] ; key is of const length 9
        if (buf.mText[0] == CMD_DATABASEINIT)
        {
            n = buf.mText[1] & 0x7F;
            if (n > 28)
                n = 28;

            for (i = 0; i < n; i++)
            {
                if (!pptr[i])
                    pptr[i] = new Property;
                memcpy(pptr[i + totalProperties]->key, buf.mText + 2 + 9*i, 9);
            }
            totalProperties += n;
            continue;
        }

        // auto process the database query reply
        // <db><n>[index_1][type_1][value_1][index_2][type_2][value_2] ... [index_n][type_n][value_n] ; type represent type and length
        if (buf.mText[0] == CMD_DATABASEQUERY)
        {
            int offset = 1;
            char index;
            char type;
            n = buf.mText[offset++] & 0x7F;
            for (i = 0; i< (n & 0x7F); i++)
            {
                index = buf.mText[offset++];
                type = buf.mText[offset++];
                pptr[index]->type = type;

                // make sure the pointer is created before assign any value to it
                if (!pptr[index]->ptr)
                {
                    if ((type & 0x80) == 0)
                    {
                        pptr[index]->ptr = new int;
                    }
                    else
                    {
                        pptr[index]->ptr = new string;
                    }
                }

                if ((type & 0x80) == 0)
                    memcpy(pptr[index]->ptr, buf.mText+offset, type & 0x7F);
                else
                {
                    string tmp;
                    tmp.assign(buf.mText+offset, type & 0x7F);
                    static_cast<string *>(pptr[index]->ptr)->assign(buf.mText+offset, type & 0x7F);
                }
                offset += type&0x7F;
            }
            continue;
        }

        // process watchdog notice of service provider down. Warning format is
        // <Down>[Service type]
        if (buf.mText[0] == CMD_DOWN)
        {
            Subscribe(buf.mText[1]); // re-subscribe, waiting for the service provider been up again

            // stop broadcasting service data to that subscriber
            for ( i = 0; i < totalClients; i++)
                if (myClients[i] == buf.mText[1])
                {  // move later subscriber one step up
                    for ( j = i; j < totalClients - 1; j++)
                        myClients[j] = myClients[j+1];
                    totalClients--;  // decrease totalClients by 1
                }

            continue;
        }

        break;
    } while (buf.len > 0);
    return buf.len;
}

// Feed the dog at watchdog server
bool MessageQueue::FeedWatchDog()
{
    char txt = CMD_WATCHDOG;
    string msg;
    msg.assign(1, txt);
    return SndMsg(msg, 1);
}; 

// Send a log to log server
// Sending format is [Log][logType][logContent]
bool MessageQueue::Log(string logContent, long logType)
{
    char txt[255];
    txt[0] = CMD_LOG;
    memcpy(txt+1, &logType, sizeof (long));
    string msg;
    msg.assign(txt, sizeof (long) + 1);
    msg.append(logContent);
    return SndMsg(msg, 1);
};
	
// assign *i to store the integer queried from the database
bool MessageQueue::dbAssign(string key, int *i)
{
    char index = getIndex(key); // 0 for invalid, 1 for the first
    if (!index)
        return false;
    index--;  // adjust the index to the first one be 0

    if (!(pptr[index]->type & 0x80))
    {
        *i = *static_cast<int *>(pptr[index]->ptr);
        pptr[index]->ptr = i;
        return true;
    }
    return false;
};

// assign *s to store the string queried from the database
bool MessageQueue::dbAssign(string key, string *s)
{
    char index = getIndex(key); // 0 for invalid, 1 for the first
    if (!index)
        return false;
    index--;  // adjust the index to the first one be 0

    if (pptr[index]->type & 0x80)
    {
        *s = *static_cast<string *>(pptr[index]->ptr);
        pptr[index]->ptr = s;
        return true;
    }
    return false;
};

// Send a request to database server to query for the value of keyword. The result will be placed in the queue by database server.
bool MessageQueue::dbQuery(string key)
{
    char txt[2];
    txt[0] = CMD_DATABASEQUERY;
    txt[1] = getIndex(key);  // 0 for invalid, 1 for the first
    if (!txt[1])
        return false;
    string msg(txt);
    return SndMsg(msg, 1);
};  

// Send a request to database to update the value of keyword with newvalue. The database server will take care of the data type casting. 
bool MessageQueue::dbUpdate(string key)
{
    char txt[255];
    char index = getIndex(key); // 0 for invalid, 1 for the first
    if (!index)
        return false;

    char type = pptr[index-1]->type;  // adjust the index
    string msg;

    txt[0] = CMD_DATABASEUPDATE;
    txt[1] = index;
    if (type &0x80)
    {
        size_t len = static_cast<string *>(pptr[index]->ptr)->length() & 0x7F;
        txt[2] =  len | 0x80;
        msg.assign(txt, len + 3);
    }
    else
    {
        txt[2] = sizeof(int);
        memcpy(txt + 3, pptr[index]->ptr, sizeof(int));
        msg.assign(txt, sizeof(int) + 3);
    }

    return SndMsg(msg, 1);
};

char MessageQueue::getIndex(string key)
{
    for (int i = 0; i < totalProperties; i++)
        if (strncmp(pptr[i]->key, key.c_str(), key.length()) == 0)
            return i+1;
    return 0;
}
