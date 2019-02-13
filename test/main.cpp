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

	gettimeofday(&tv, NULL);
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
    bool isServer = argc > 1;
    bool rst = false;
    clock_t t = clock();
    clock_t nextSend = 0;

    bool received = false;
    if (isServer)
    {
        cout << "Running in server mode, waiting for client to connect." << endl;
        mq = new MessageQueue(0, true);
        if (mq->err < 0)
        {
            cerr << "Cannot connect to required message queue\n";
            return -1;
        }

        rst = true;
        while (rst)
        {
            t = clock();
            if (mq->ReadMsg(&msg, 1) > 0)
            {
                cout << "Receive message (" << msg  << ") at " << current_time() << endl;
                received = true;
            }
/*            else
            {
                cout << t << " Waiting for clients ... " << nextSend << endl;
                sleep(1);
            }
*/
            if ((t > nextSend) && received)
            {
                //gettimeofday(&tv, nullptr);
                msg = "Hello from the Server at " + current_time();
                rst = mq->SendMsg(msg, rType);
                nextSend = t + CLOCKS_PER_SEC;
                cout << "and send back " <<msg << "-->" << rst << endl;
            }
        }
        cout << " Error. Returns " << rst << endl;
        mq->~MessageQueue();
    }
    else
    {
        cout << "Running in client mode. Try to cotact server." << endl;
        mq = new MessageQueue(0, false);
        if (mq->err < 0)
        {
            cerr << "Cannot connect to required message queue\n";
            return -1;
        }

        //rst = mq->SendMsg("First saluting from the client", 1);current_time()
		msg = "Salute from the client at " + current_time();
		rst = mq->SendMsg(msg, 1);
        while (rst)
        {
            t = clock();
            if (mq->ReadMsg(&msg,rType) >= 0)
            {
                cout << "Receive message (" << msg  << ") at " << current_time() << endl;
                received = true;
            }
/*            else
            {
                cout << t << " --> " << nextSend << endl;
                sleep(1);
            }
*/
            if ((t > nextSend ) && received)
            {
                //msg = "More salutings from the client";
                msg = "Salute from the client at " + current_time();
                rst = mq->SendMsg(msg, 1);
                nextSend = t + CLOCKS_PER_SEC;
                cout << msg << " sent. " << t << " --> " << nextSend << endl;
            }
        }
    }

  //  return a.exec();
}
