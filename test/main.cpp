#include <QCoreApplication>
#include <ctime>
#include <chrono>
#include <iostream>
#include <unistd.h>
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
string current_time()
{
    time_t now = time(nullptr);
    struct tm tstruct;
    char buf[40];
    tstruct = *localtime(&now);
    //format: HH:mm:ss
    strftime(buf, sizeof(buf), "%X", &tstruct);
    return buf;
}

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    string msg;
    long rType = 10;
    MessageQueue *mq;
    bool isServer = argc > 1;
    if (isServer)
    {
        cout << "Running in server mode, waiting for client to connect." << endl;
        mq = new MessageQueue(0, true);
        if (mq->err < 0)
        {
            cerr << "Cannot connect to required message queue\n";
            return -1;
        }
        while (mq->ReadMsg(&msg, 1) < 1)
        {
            cout << ".";
            sleep(1);
        }
        cout << current_date() << " " << current_time() << " : " << msg << endl;

        while (mq->SendMsg("Server sent at " + current_time(), rType) > 0)
        {
            sleep(1);
            cout << ":";
        }

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
        while (mq->SendMsg("Alert from client", 1) < 1)
        {
            sleep(1);
        };

        while (true)
        {
            if (mq->ReadMsg(&msg,rType) > 0)
                cout << "Received at " << current_time() << " : " << msg << endl;
            else
                sleep(1);
        }
    }

    return a.exec();
}
