/*
 * LogOutput.cpp
 *
 *  Created on: Sep 14, 2017
 *      Author: mark
 */
#define LOG_OUT_XXX  1
#if LOG_OUT_XXX
#include "LogOutput.h"
#include <log4cplus/logger.h>
#include <log4cplus/configurator.h>
#include <iomanip>
#include <log4cplus/logger.h>
#include <log4cplus/fileappender.h>
#include <log4cplus/consoleappender.h>
#include <log4cplus/layout.h>
#include <time.h>
#include <memory>
#include <string.h>
//#include "hwCommonUtil.h"

using namespace log4cplus;
Logger GZyVSecureLogger;
#endif
int null_size_g= 0;
void ZyWritelogByVSecure(char* leval,char* info ,bool outputr)
{
#if LOG_OUT_XXX
	struct tm *p;
	time_t lt=time(NULL);
	p=localtime(&lt);
	char* timetemp=ctime(&lt);
	*(timetemp+strlen(timetemp)-1)='\0';
	char temp[10000];
	sprintf(temp,"[%s] %s",timetemp,info);
	if (!outputr)
	{
		printf("%s\n",temp);
	}
	if (outputr && null_size_g%10000==0)
	{
		printf("%s\n",temp);
	}
	null_size_g++;
	if(memcmp(leval,"TRACE",5)==0)
	LOG4CPLUS_TRACE(GZyVSecureLogger,temp);
	if(memcmp(leval,"DEBUG",5)==0)
	LOG4CPLUS_DEBUG(GZyVSecureLogger,temp);
	if(memcmp(leval,"INFO",4)==0)
	LOG4CPLUS_INFO(GZyVSecureLogger,temp);
	if(memcmp(leval,"WARN",4)==0)
	LOG4CPLUS_WARN(GZyVSecureLogger,temp);
	if(memcmp(leval,"ERROR",5)==0)
	LOG4CPLUS_ERROR(GZyVSecureLogger,temp);
	if(memcmp(leval,"FATAL",5)==0)
	LOG4CPLUS_FATAL(GZyVSecureLogger,temp);
#endif
}

void InitLogSystem()
{
#if LOG_OUT_XXX
    char filename[512];
    struct tm *p;
    time_t lt=time(NULL);
    p=localtime(&lt);
    sprintf(filename,"%s/%d-%d-%d.txt",LOG_PATH_HW_ANTI,(1900+p->tm_year), (1+p->tm_mon),p->tm_mday);
    //SharedAppenderPtr pFileAppender(new FileAppender((filename)));
    SharedAppenderPtr pFileAppender(new DailyRollingFileAppender((filename)));
    GZyVSecureLogger = Logger::getInstance(("LoggerName"));
    GZyVSecureLogger.addAppender(pFileAppender);
#endif
}



