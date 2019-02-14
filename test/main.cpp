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
    MsgPkt pkt;
    MessageQueue *mq;

    clock_t t = clock();
    clock_t nextSend = 0;
    if (argc == 1)
    {
        cout << "Running in server mode, waiting for client to connect and then response." << endl;
        mq = new MessageQueue("", 1);
        if (mq->err < 0)
        {
            cerr << "Cannot connect to required message queue\n";
            return -1;
        }

        while (true)
        {
            pkt = mq->RcvPkt(1);
            if ( pkt.sType != 0)
            {
                cout << "Received at " << current_time() << " : (" << pkt.sType << ") " \
                     << pkt.sec << "." << pkt.usec << " " << pkt.mText << endl;
                mq->SndMsg("Here you are at " + current_time(), pkt.sType);
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

        mq = new MessageQueue("", sType);
        if (mq->err < 0)
        {
            cerr << "Cannot open message queue with Id " << sType << ", error= " << mq->err << endl;
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

        while (true)
        {
            pkt = mq->RcvPkt(sType);
            if (pkt.sType != 0)
                cout << "Received at " << current_time() << " : " << pkt.sType << " " \
                     << pkt.sec << "." << pkt.usec << " " << pkt.mText << endl;

            t = clock();
            if (t > nextSend)
            {
                cout << "Sent query request at " << current_time() << endl;
                mq->SndMsg("Query data at " + current_time(), 1);
                nextSend = t + CLOCKS_PER_SEC;
            }
        }
    }

  //  return a.exec();
}
