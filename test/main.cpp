//#include <QCoreApplication>
#include <ctime>
#include <chrono>
#include <iostream>
#include <unistd.h>
#include <sys/time.h>
#include <string.h>
#include "messagequeue.h"

using namespace std;
string current_date()
{
    time_t now = time(nullptr);
    struct tm tstruct;
    char buf[40];
    tstruct = *localtime(&now);
    //format: day DD-MM-YYYY
    strftime(buf, sizeof(buf), "%A %d/%m/%Y", &tstruct);
    return buf;
}
string current_time1()
{
    time_t now = time(nullptr);
    struct tm tstruct;
    char buf[40];
    tstruct = *localtime(&now);
    //format: HH:mm:ss
    strftime(buf, sizeof(buf), "%X", &tstruct);
    return buf;
}

string current_time()
{
	struct timeval tv;
	time_t nowtime;
	struct tm *nowtm;
	char tmbuf[64], buf[64];

    gettimeofday(&tv, nullptr);
	nowtime = tv.tv_sec;
	nowtm = localtime(&nowtime);
	strftime(tmbuf, sizeof tmbuf, "%Y-%m-%d %H:%M:%S", nowtm);
    snprintf(buf, sizeof buf, "%s.%06ld", tmbuf, tv.tv_usec);
    return buf;
}

int main(int argc, char *argv[])
{
//    QCoreApplication a(argc, argv);

    string msg;
    string tmp;
    char txt[255];
    char data[255];
    unsigned char dataLen = 0;

    time_t now;
    struct tm tstruct;
    clock_t nextSend = 0;
    ssize_t len;
    if (argc == 1)
    {
        cout << "Running in server mode, waiting for client to connect and then response." << endl;
        MessageQueue *mq = new MessageQueue("", "SERVER", 1);
        if (!mq->Open())
        {
            cerr << "Cannot connect to required message queue\n";
            return -1;
        }

        //bool updated = true;
        while (true)
        {
            now = time(nullptr);
            tstruct = *localtime(&now);
            //format: HH:mm:ss
            dataLen = strftime(txt, 30, "%X", &tstruct) & 0xFF;

            //updated = strncmp(txt, data, dataLen) == 0;
/*            for (int i = 0; i < dataLen; i++)
            {
                if (txt[i] != data[i])
                {
                    updated = true;
                    data[i] = txt[i];
                }
            }
            */
            if (strncmp(txt, data, dataLen) != 0)
            {
                strcpy(data, txt);
                mq->BroadcastUpdate(&data, dataLen);
            }

            len =mq->RcvMsg(static_cast<void *>(&txt));
            if ( len > 0)
            {
                msg.assign(txt, static_cast<size_t>(len));
                cout << "Received " << msg << " at " << current_time() << " : (" << mq->msgType << ") " \
                     << mq->msgTS_sec << "." << mq->msgTS_usec << endl;

                tmp = CMD_ONBOARD;
                if ( strncmp(txt, CMD_ONBOARD, tmp.length()) == 0)
                {
                    strcpy(txt,"List Radar   +GPS     +Trigger +User    +Networks+FrontCam+RearCam +Recorder+Uploader+Dnloader+");
                    txt[4] = 10;
                    for (int i = 1; i < 11; i++)
                        txt[i*9 + 4] = 10 + i;
                    tmp.assign(txt, 90);
                    mq->SndMsg(tmp, mq->msgType);
                }

                if (msg == "end")
                {
                    mq->~MessageQueue();
                    break;
                }
            }
        }
    }
    else
    {
        cout << "Running in client mode. Try to query from the server." << endl;

        int sType;
        int rst;
        msg = argv[1];

        sType = (argc == 2) ? 11 : atoi(argv[2]);

        if (sType > 255) sType = 255;
        if (sType < 2) sType = 2;

        MessageQueue *mq = new MessageQueue("", argv[1], sType);
        if (!mq->Open())
        {
            cerr << "Cannot connect to required message queue\n";
            delete mq;
            return -1;
        }

        if (sType == 1)
        {
            delete mq;
            return 0;
        }

        if (sType > 20)
        {
            rst = mq->SndMsg("end", 1);
            delete mq;
            return 0;
        }

        mq->Subscribe(1);

        int i = 11;
        while (true)
        {
            msg = mq->RcvMsg();
            if (msg.length() > 0)
            {
                cout << "Received " << msg << " at " << current_time() << " : (" << mq->msgType << ") " \
                     << mq->msgTS_sec << "." << mq->msgTS_usec << endl;

                if (i > 20) i = 11;
                cout << "Service list: " << mq->GetServiceTitle(i) << " " << i++ << endl;
                if (msg == "end") break;
            }
        }
    }

    cout << "End of the demo. \n";

  //  return a.exec();
}
