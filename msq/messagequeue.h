#ifndef MESSAGEQUEUE_H
#define MESSAGEQUEUE_H

#include <string>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>

#define PERMS 0644
#define DEAFULTMSGQ "/tmp/msgq.txt"

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
    long sType;  // sender type
    long sec;   // time stamp
    long usec;
    char mText[200];
};

struct MsgBuf
{
   long rType;  // receiver type
   MsgPkt pkt;
};


class MessageQueue
{

public:
    MessageQueue(string keyFilename, long SrvType); // Open specified message queue with given service name

    bool SndMsg(string msg, long SrvType); // Send a packet to specified service provider
    bool AskForData(long SrvType); // Send a specicial command to service provider asking for service data to be put on queue

    string RcvMsg(long SrvType); // receive a message from service provider, like GPS, RADAR, TRIGGER
    MsgPkt RcvPkt(long SrvType); // receive whole packet from service provider

    bool Subscript(long SrvType); // Subscript the Service
    bool UnSubspript(long SrvType); // Stop subscript the service

    ~MessageQueue();

    int err; // 0 for no error

private:
    struct MsgBuf buf;
    int mId;
    long mType; // sender Type, my Type
};

#endif // MESSAGEQUEUE_H
