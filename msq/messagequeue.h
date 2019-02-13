#ifndef MESSAGEQUEUE_H
#define MESSAGEQUEUE_H

#include <string>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>

#define PERMS 0644
#define DEAFULTMSGQ "msgq.txt"
using namespace std;

struct MsgBuf
{
   long mType;
   char mText[200];
};

class MessageQueue
{

public:
    MessageQueue(key_t key, bool server);
    bool SendMsg(string msg, long type);
    ssize_t ReadMsg(string *msg, long type);
    int getMessgaeId();
    key_t getMessageKey();
    ~MessageQueue();

    int err;

private:
    struct MsgBuf buf;
    int mId;
    key_t mKey;
    long mType;
    bool isServer;
};

#endif // MESSAGEQUEUE_H
