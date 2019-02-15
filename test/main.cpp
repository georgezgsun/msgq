//#include <QCoreApplication>
#include <ctime>
#include <chrono>
#include <iostream>
#include <unistd.h>
#include <sys/time.h>
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
    MessageQueue *mq = new MessageQueue();
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
        if (!mq->Open(1))
        {
            cerr << "Cannot connect to required message queue\n";
            return -1;
        }

        //mq->BroadcastUpdate(&data, dataLen);

        bool updated = true;
        while (true)
        {
            now = time(nullptr);
            tstruct = *localtime(&now);
            //format: HH:mm:ss
            dataLen = strftime(txt, 30, "%X", &tstruct) & 0xFF;

            updated = false;
            for (int i = 0; i < dataLen; i++)
            {
                if (txt[i] != data[i])
                {
                    updated = true;
                    data[i] = txt[i];
                }
            }
            if (updated)
                mq->BroadcastUpdate(&data, dataLen);

            len =mq->RcvMsg(static_cast<void *>(&txt), true);
            if ( len > 0)
            {
                msg.assign(txt, static_cast<size_t>(len));
                cout << "Received " << msg << " at " << current_time() << " : (" << mq->msgType << ") " \
                     << mq->msgTS_sec << "." << mq->msgTS_usec << endl;

                if ( msg == "end")
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
        int sType = atoi(argv[1]);
        if (!mq->Open(sType))
        {
            cerr << "Cannot connect to required message queue\n";
            return -1;
        }

        msg.assign(argv[1]);
        msg.append(" on board");
        if (!mq->SndMsg(msg, 1))
        {
            cerr << "Cannot send message " << msg << " to queue with error= " << mq->err << endl;
            return -2;
        }
        cout << "Sent " << msg << " at " << current_time() << endl;
        mq->Subscribe(1);

        while (true)
        {
            msg = mq->RcvMsg();
            if (msg.length() > 0)
            {
                cout << "Received " << msg << " at " << current_time() << " : (" << mq->msgType << ") " \
                     << mq->msgTS_sec << "." << mq->msgTS_usec << endl;
                if (msg == "end") break;
            }
        }
    }

    cout << "End of the demo. \n";

  //  return a.exec();
}
