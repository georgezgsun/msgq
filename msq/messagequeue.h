#ifndef MESSAGEQUEUE_H
#define MESSAGEQUEUE_H

#include <string>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>

#define PERMS 0644
#define DEAFULTMSGQ "/tmp/msgq.txt"
using namespace std;

struct MsgBuf
{
   long mType;
   char mText[200];
};

static string Ids[40] = {"SERVER", "LOG", "DATABASE", "RADAR", "GPS", "TRIGGER", "WIFI", "USER", \
                  "UPLOADER", "DOWNLOADER", "FRONTCAM", "REARCAM", "LEFTCAM", "RIGHTCAM", \
                 "CAM1", "CAM2", "CAM3", "CAM4", "CAM5", "CAM6", "BLUETOOTH", "ETH", \
                 "MIC1", "MIC2", "AUDIO1", "AUDIO2", "AUDIO3", "AUDIO4", \
                 "RECORDER", "MEDIACENTER", "BWC", "BWC1", "BWC2", "BWC3", "BWC4", \
                 "WATCHDOG", "", "", "", ""};

class MessageQueue
{

public:
    MessageQueue(string name); // Specify the module name, like "GPS", "RADAR", "TRIGGER"
    MessageQueue(); // Reserved for server use

    bool SndMsg(string msg, long type);
    bool SndMsg(string msg);

    string RcvMsg(string id); // receive a message by id, like "GPS", "RADAR", "TRIGGER"
    string RcvMsg(); // receive a message by last id
    ~MessageQueue();

    int err; // 0 for no error

private:
    struct MsgBuf buf;
    int mId;
    long mType; // sender Type, my Type
    long rType; // receiver Type, peer Type
};

#endif // MESSAGEQUEUE_H
