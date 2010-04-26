#ifndef _LOGMACROS_H_
#define _LOGMACROS_H_

#include <time.h>

enum
{
    INF,
    WAN,
    ERR
};

#define DEBUG_LOG(type, fmt, ...) \
    do{\
        if(isWriteLog() || ERR == type)\
        {\
            writeLog(type, "[%s:%d]>> "fmt, getName(__FILE__), __LINE__, ##__VA_ARGS__);\
        }\
    }while(0);

#define DEBUG_HEX(start, length) \
    do{\
        if(isWriteLog())\
        {\
            printHex(start, length, getName(__FILE__), __LINE__);\
        }\
    }while(0);

//�ж��Ƿ���Ҫ��ӡ��־
bool isWriteLog();
//����־�ļ�
void initDebugLog(const char* filename);
//д��־
void writeLog(int type, const char *fmt,...);
//��ȫ·���л�ȡ�ļ���
const char* getName(const char* path);
//��ӡ����������
void printHex(const unsigned char* start, int len, const char* filename, int linenumber);

const char* timeToStr(const time_t& time);

const char* binToHex(const unsigned char* start, int len);

#endif

