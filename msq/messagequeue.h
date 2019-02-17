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

#define SERVER 1;
#define LOG 2;
#define DATABASE 3;
#define WATCHDOG 4;
#define RADAR 10
#define GPS 11;
#define TRIGGER 12;
#define WIFI 13;
#define BLUETOOTH 14;
#define USER 15
#define LOGIN 15; //LOGIN is the same as USER
#define UPLOADER 17;
#define DOWNLOADER 18;
#define FRONTCAM 20;
#define REARCAM 22;
#define RIGHTCAM 24;
#define LEFTTCAM 26;
#define CAM1 20;
#define CAM2 22;
#define CAM3 24;
#define CAM4 26;
#define CAM5 28;
#define CAM6 29;
#define ETH 30;
#define MIC 40;
#define MIC1 41;
#define MIC2 42;
#define MIC3 43;
#define MIC4 44;
#define BWC 50;
#define BWC1 51;
#define BWC2 52;
#define BWC3 53;
#define BWC4 54;
#define BWC 50;

using namespace std;

struct MsgPkt
{
	unsigned char lenOfText; // the actual length of mText
    long sType;  // sender type
    long sec;   // time stamp
    long usec;
    char mText[255];
};

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
    MessageQueue(string keyFilename); // Define specified message queue
	MessageQueue(); // Define a default message queue
	
	bool Open(long SrvType); // Open the message queue and name its service type, like GPS, RADAR, TRIGGER
    bool SetAutoReply(bool EnableAutoReply);
	
	bool Subscribe(long SrvType); // Subscribe a SrvType, unsubscribe for a negtive number 
    bool AskForService(long SrvType); // Ask for service data from specified service provider 
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

private:
    struct MsgBuf buf;
    int mId;
    long mType; // my Type, ser type of this module
    string filename;  // the file that the key is build from
    bool autoReply; // AutoReply is enabled when true

    unsigned char totalSubscriptions;  // the number of my subscriptions
    long mySubscriptions[255];

    unsigned char totalClients; // the number of clients that subscribe my service
    long myClients[255];

    char mData[255]; // store the latest serv data that will be sent out per request
    unsigned char dataLength; // store the length of service data

    long totalMessageSent;
    long totalMessageReceived;

    long nextDogFeedTime;
    bool adoptWatchDog;
};

#endif // MESSAGEQUEUE_H
