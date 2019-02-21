//#include <QCoreApplication>
#include <ctime>
#include <chrono>
#include <iostream>
#include <unistd.h>
#include <sys/time.h>
#include <string.h>
#include "messagequeue.h"

#define CMD_ONBOARD 1
#define CMD_LIST 1
#define CMD_DATABASEINIT 2
#define CMD_DATABASEUPDATE 2
#define CMD_DATABASEQUERY 3
#define CMD_LOG 4
#define CMD_WATCHDOG 5
#define CMD_DOWN 5
#define CMD_STOP 6
#define CMD_SUBSCRIBE 7
#define CMD_QUERY 8

using namespace std;

struct Onboard
{
    char cmd;
    char type;
    char title[10];
    pid_t pid;
    pid_t ppid;
};

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
    int PreEvent = 120;
    int Chunk = 60;
    int TotalReceived = 0;

    string nameCam1 = "FrontCam";

    Property *mProperty =new Property [3]
    {
        {"PreEvent", -1, &PreEvent},
        {"CHUNK", -1, &Chunk},
        {"CAMNAME", sizeof(nameCam1), &nameCam1}
    };

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

            if (strncmp(txt, data, dataLen) != 0)
            {
                strcpy(data, txt);
                mq->BroadcastUpdate(&data, dataLen);
            }

            len = mq->RcvMsg(static_cast<void *>(&txt));
            if ( len <= 0)
                continue;

            if (txt[0] >= 32) // a normal text
            {
                msg.assign(txt, static_cast<size_t>(len));
                cout << "Received " << msg << " at " << current_time() << " : (" << mq->msgType << ") " \
                     << mq->msgTS_sec << "." << mq->msgTS_usec << endl;
                continue;
            }

            if (txt[0] == CMD_ONBOARD)
            {
                Onboard report;
                memcpy(&report, &txt, sizeof(report));
                cout << report.title << " is now onboard with type " << report.type << ", PID " << report.pid;
                cout << ", parent PID " << report.ppid << endl;

                // <List><n>[title1][t1][title2][t2] ... [titlen][tn] ; title is of const length 9
                strcpy(txt,"--Radar+++++GPS+++++++Trigger+++User++++++Networks++FrontCam++RearCam+++Recorder++Uploader++Dnloader++");
                txt[0] = CMD_LIST;
                txt[1] = 10;
                for (int i = 0; i < 10; i++)
                {
                    txt[i*10 + 10] = 0;  // put a /0 at the end of each title
                    txt[i*10 + 11] = 11 + i; // give the index of each title,
                }
                tmp.assign(txt, 10 * txt[1] + 2);
                cout << "Send back services list as:" << txt << endl;
                for (int i = 2; i < tmp.length(); i++)
                    if (txt[i] == '+')
                        txt[i] = 0;
                mq->SndMsg(&txt, 10*txt[1]+2, mq->msgType);

                // Send back database keys
                // <db><total_n>[key1][key2] ... [keyn] ; key is of const length 9
                strcpy(txt,"--PreEvent+FrontCam+Chunk++++User     Networks FrontCam RearCam  Recorder Uploader Dnloader");
                txt[0] = CMD_DATABASEINIT;
                txt[1] = 3; // n=3
                tmp.assign(txt, 2+txt[1]*9);
                cout << "Send back the keys as: " << tmp << endl;
                for (int i = 2; i < tmp.length(); i++)
                    if (txt[i] == '+')
                        txt[i] = 0;
                mq->SndMsg(&txt, 9*txt[1] + 2, mq->msgType);

                // Send back values
                // <db><n>[index_1][type_1][value_1][index_2][type_2][value_2] ... [index_n][type_n][value_n] ; type represent type and length
                int index = 0;
                txt[index++] = CMD_DATABASEQUERY;
                txt[index++] = 3;  // total 3 properties

                txt[index++] = 0;  // index of the first property
                txt[index++] = sizeof (int); // type of an integer
                memcpy(txt + index, &index, sizeof (int));
                index += sizeof(int);

                tmp = "sample string";
                txt[index++] = 1;
                txt[index++] = tmp.length() | 0x80;
                memcpy(txt + index, tmp.c_str(), tmp.length());
                index += tmp.length();

                txt[index++] = 2;  // index of the first property
                txt[index++] = sizeof (int); // type of an integer
                memcpy(txt + index, &index, sizeof (int));
                index += sizeof(int);

                tmp.assign(txt, index);
                mq->SndMsg(tmp, mq->msgType);
            }

            if (msg == "end")
            {
                mq->~MessageQueue();
                break;
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
        cout << "Sent the subscription to server\n";

        int chunk;
        int trigger;
        string frontCam;
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

                cout << "Database query FrontCam=" << frontCam << ", PreEvent=" << PreEvent << ", chunk=" << chunk << endl;

                TotalReceived ++;
                if (TotalReceived == 3)
                {
                    mq->dbAssign("PreEvent", &PreEvent);
                    mq->dbAssign("FrontCam", &frontCam);
                    mq->dbAssign("Chunk", &chunk);
                }
            }
        }
    }

    cout << "End of the demo. \n";

  //  return a.exec();
}
