#ifndef MESSAGEQUEUE_H
#define MESSAGEQUEUE_H

#include <string>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>

#define PERMS 0644
#define DEAFULTMSGQ "/tmp/msgq.txt"
#define CMD_SUBSCRIBE "Subscribe"
#define CMD_QUERY "Query"
#define CMD_LOG "Log"
#define CMD_UPDATE "Update"
#define CMD_ONBOARD "Onboard"
#define CMD_LIST "List"
#define CMD_DOWN "Down"

#define SERVER 1
#define HEADER 1
#define BASE 1
#define CENTER 1
#define COMMANDER 1
#define CORE 1

using namespace std;

struct MsgBuf
{
   long rType;  // Receiver type
   long sType; // Sender Type
   long sec;
   long usec;
   unsigned char len;
   char mText[255];
};

class MessageQueue
{

public:
    MessageQueue(string keyFilename, string myServiceTitle, long myServiceType); // Define specified message queue
	
    bool Open(); // Open the message queue and name its service type, like GPS, RADAR, TRIGGER
    long GetServiceType(string ServiceTitle); // Get the corresponding service type to ServiceTitle, return 0 for not found
    string GetServiceTitle(long ServiceType); // Get the corresponding service title to ServiceType, return "" for not found

    bool Subscribe(string SrvTitle); // Subscribe a service by its title
    bool Subscribe(long SrvType); // Subscribe a service by its type, unsubscribe with a negtive number
    bool AskForServiceOnce(long SrvType); // Ask for service data from specified service provider
    bool SndMsg(string msg, long SrvType); // Send a string to specified service provider
	bool SndMsg(void *p, size_t len, long SrvType); // Send a packet with given length to specified service provider
    bool BroadcastUpdate(void *p, unsigned char dataLength); // Multicast the data stored at *p with dataLength and send it to every subscriber

    string RcvMsg(); // receive a text message from sspecified ervice provider, like GPS, RADAR, TRIGGER. Not Autoreply.
    ssize_t RcvMsg(void *p); // receive a packet from specified service provider. Autoreply all requests when enabled.
	
	bool FeedWatchDog(); // Feed the dog at watchdog server
	bool Log(string logContent, long logType); // Send a log to log server
	bool RequestDatabaseQuery(string keyword); // Send a request to database server to query for the value of keyword. The result will be placed in the queue by database server.
	bool RequestDatabaseUpdate(string keyword, string newvalue); // Send a request to database to update the value of keyword with newvalue. The database server will take care of the data type casting. 

    ~MessageQueue();

    int err; // 0 for no error
	long msgType; // the service type of last receiving message 
	long msgTS_sec; // the time stamp in seconds of latest receiving message
	long msgTS_usec; // the micro seconds part of the time stamp of latest receiving message


protected:
    struct MsgBuf buf;
    int mId;
    long PID;
    long mType; // my Type, ser type of this module

    unsigned char totalSubscriptions;  // the number of my subscriptions
    long mySubscriptions[255];
    unsigned char totalClients; // the number of clients that subscribe my service
    long myClients[255];

    char mData[255]; // store the latest serv data that will be sent out per request
    unsigned char dataLength; // store the length of service data

    unsigned char totalServices;
    char services[255][9];

    long totalMessageSent;
    long totalMessageReceived;

    bool AutoReply(bool isServer);

private:
    string mTitle;
    key_t mKey;
    long nextDogFeedTime;
};

#endif // MESSAGEQUEUE_H
