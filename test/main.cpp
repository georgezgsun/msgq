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

    struct timeval tv;

    string msg;
    long rType = 10;
    MessageQueue *mq;

    clock_t t = clock();
    clock_t nextSend = 0;
    if (argc == 1)
    {
        cout << "Running in server mode, waiting for client to connect and then response." << endl;
        mq = new MessageQueue("SERVER");
        if (mq->err < 0)
        {
            cerr << "Cannot connect to required message queue\n";
            return -1;
        }

        while (true)
        {
            msg = mq->RcvMsg();
            if ( msg != "")
            {
                cout << current_date() << " " << current_time() << " : " << msg << endl;
                mq->SndMsg("Server received (" + msg + ") at " + current_time() );
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
        msg = argv[1];
        mq = new MessageQueue(msg);
        if (mq->err < 0)
        {
            cerr << "Cannot open message queue with Id " << msg << endl;
            return -1;
        }

        msg.append(" on board");
        if (!mq->SndMsg(msg)) return -2;
        cout << "Sent " << msg << " at " << current_time() << endl;

        t = clock();
        while (true)
        {
            msg = mq->RcvMsg("SERVER");
            if (msg != "")
                cout << "Received at " << current_time() << " : " << msg << endl;

            if ((t > nextSend) && mq->SndMsg("Query data at " + current_time()))
            {
                cout << "Sent query request at " << current_time() << endl;
                nextSend = t + CLOCKS_PER_SEC;
            }
        }
    }

  //  return a.exec();
}
