#ifndef MESSAGEQUEUE_H
#define MESSAGEQUEUE_H

#include <string>

using namespace std;

// structure that defines properties, used to retrieve properties from database
struct Property
{
    char key[9];// keyword of this property, shall match with those in database
    char type;  // type of this property, first bit 0 indicates string, 1 indicates interger. The rest bits return the length of the property.
    void * ptr; // pointer to the property
};

// structure that holds the whole packet of a message in message queue
struct MsgBuf
{
   long rType;  // Receiver type
   long sType; // Sender Type
   long sec;    // timestamp sec
   long usec;   // timestamp usec
   unsigned char len;   //length of message payload in mText
   char mText[255];  // message payload
};

class MessageQueue
{

public:
    MessageQueue(string keyFilename, string myServiceTitle, long myServiceType); // Define specified message queue
	
    bool Open(); // Open the message queue and specify the property of this module, the properties will be auto updated later
    long GetServiceType(string ServiceTitle); // Get the corresponding service type to ServiceTitle, return 0 for not found
    string GetServiceTitle(long ServiceType); // Get the corresponding service title to ServiceType, return "" for not found

    bool Subscribe(string SrvTitle); // Subscribe a service by its title
    bool Subscribe(long SrvType); // Subscribe a service by its type, unsubscribe with a negtive number
    bool AskForServiceOnce(long SrvType); // Ask for service data from specified service provider
    bool SndMsg(string msg, long SrvType); // Send a string to specified service provider
	bool SndMsg(void *p, size_t len, long SrvType); // Send a packet with given length to specified service provider
    bool BroadcastUpdate(void *p, unsigned char dataLength); // broadcast the data stored at *p with dataLength and send it to every subscriber

    string RcvMsg(); // receive a text message from sspecified ervice provider, like GPS, RADAR, TRIGGER. Not Autoreply.
    ssize_t RcvMsg(void *p); // receive a packet from specified service provider. Autoreply all requests when enabled.
	
	bool FeedWatchDog(); // Feed the dog at watchdog server
	bool Log(string logContent, long logType); // Send a log to log server

    bool dbAssign(string key, int * i); // assign *i to store the integer queried from the database
    bool dbAssign(string key, string * s); // assign *s to store the string queried from the database
    bool dbQuery(string key); // query for the value from database by the key. The result will be placed in variable assigned to the key before.
    bool dbUpdate(string key); // update the value in database by the key with new value saved in variable assigned to the key before.

    ~MessageQueue();

    int err; // 0 for no error
	long msgType; // the service type of last receiving message 
	long msgTS_sec; // the time stamp in seconds of latest receiving message
	long msgTS_usec; // the micro seconds part of the time stamp of latest receiving message

protected:
    struct MsgBuf buf;
    int mId;
    long mType; // my Type, service type of this module

    unsigned char totalSubscriptions;  // the number of my subscriptions
    long mySubscriptions[255];
    unsigned char totalClients; // the number of clients who subscribe my service
    long myClients[255];

    char mData[255]; // store the latest service data that will be sent out per request
    unsigned char dataLength; // store the length of service data

    unsigned char totalServices;
    char services[255][10];  // Services list got from server, 8 char + 1 stop + 1 index

    unsigned char totalProperties;
    long totalMessageSent;
    long totalMessageReceived;

    Property *pptr[255]; // Pointer to the Properties of theis module

    // return the index of the key in database
    char getIndex(string key);

private:
    string mTitle;
    key_t mKey;
};

#endif // MESSAGEQUEUE_H
