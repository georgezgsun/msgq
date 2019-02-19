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
	memset(&buf, 0, sizeof(buf));  // initialize the internal buffer
    memset(&myClients, 0, sizeof(myClients));
    memset(&mySubscriptions, 0, sizeof(mySubscriptions));
    memset(&services, 0, sizeof(services));

    totalSubscriptions = 0;
    totalClients = 0;
    totalMessageSent = 0;
    totalMessageReceived =0;
    totalServices = 0;
    nextDogFeedTime = 0;

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


    if (mType == 1)
    {// Read off all previous messages
        do
        {
        } while (msgrcv(mId, &buf, sizeof(buf), 0, IPC_NOWAIT) > 0);
    }
    else
    {// Register itself to the server
        string msg = CMD_ONBOARD;
        msg.append(" ");
        msg.append(to_string(getpid()));
        msg.append(" ");
        msg.append(to_string(getppid()));
        msg.append(" ");
        msg.append(mTitle);
        msg.append(" ");
        msg.append(to_string(mType)) ;
        msg.append(" ");
        SndMsg(msg, 1);
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
    return SndMsg(CMD_QUERY, SrvType);
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
        if (services[i][8] == (ServiceType & 0xFF))
        {
            str.assign(services[i], 8); // Right now the title is always 8 bytes
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
    string sub = CMD_SUBSCRIBE;
    string qry = CMD_QUERY;
    string lst = CMD_LIST;
    string dn = CMD_DOWN;
    struct timeval tv;
    ssize_t headerLen = 3 * sizeof(long) + 1;

	do
    {
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

        // no autoreply for server
        //if (mType == 1)
        //    return buf.len;

        // auto reply for new subscribe requests
        if (strncmp(sub.c_str(), buf.mText, sub.length()) == 0)
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
            continue; // continue to read next messages
        }

        // auto reply for service request
        if (strncmp(qry.c_str(), buf.mText, qry.length()) == 0)
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
            continue; // continue to read next message
        }

        // no autoreply for server
        if (mType == 1)
            return buf.len;

        // auto process the services list reply
        if ((msgType == 1) && (strncmp(lst.c_str(), buf.mText, lst.length()) == 0))
        { // List n title1 t1 title2 t2 ... titlen tn ; title is of const length 8
            i = lst.length();
            j = static_cast<size_t>(buf.mText[i]); // total items in this list
            if (j > 26)
            { // no more than 26 services can be transfered in one message
                err = static_cast<int>(j);
                return 0;
            }
            memcpy(services[totalServices], buf.mText+i+1, j*9);
            totalServices += j;
            continue;
        }

        // process watchdog notice of service provider down. Warning format is
        // Subscribe <Service type>
        if ((msgType == 1) && strncmp(dn.c_str(), buf.mText, dn.length()) == 0)
        {
            Subscribe(buf.mText[sub.length()+1]); // re-subscribe, waiting for the service provider been up again

            // stop broadcasting service data to that subscriber
            for ( i = 0; i < totalClients; i++)
                if (myClients[i] == buf.mText[sub.length()+1])
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
    string cmd = CMD_QUERY;

    return SndMsg(cmd, 1);
}; 

// Send a log to log server
bool MessageQueue::Log(string logContent, long logType)
{
    string cmd = CMD_LOG;
    return SndMsg(cmd + " " + to_string(logType) + " " +logContent, 1);
};
	
// Send a request to database server to query for the value of keyword. The result will be placed in the queue by database server.
bool MessageQueue::RequestDatabaseQuery(string keyword)
{
    string cmd = CMD_QUERY;
    return SndMsg(cmd +  " " + keyword, 1);
};  

// Send a request to database to update the value of keyword with newvalue. The database server will take care of the data type casting. 
bool MessageQueue::RequestDatabaseUpdate(string keyword, string newvalue)
{
    string cmd = CMD_UPDATE;
    return SndMsg(cmd + " " + keyword + " " + newvalue, 1);
};
