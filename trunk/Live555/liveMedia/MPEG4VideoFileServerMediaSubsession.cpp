/**********
This library is free software; you can redistribute it and/or modify it under
the terms of the GNU Lesser General Public License as published by the
Free Software Foundation; either version 2.1 of the License, or (at your
option) any later version. (See <http://www.gnu.org/copyleft/lesser.html>.)

This library is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
more details.

You should have received a copy of the GNU Lesser General Public License
along with this library; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
**********/
// "liveMedia"
// Copyright (c) 1996-2009 Live Networks, Inc.  All rights reserved.
// A 'ServerMediaSubsession' object that creates new, unicast, "RTPSink"s
// on demand, from a MPEG-4 video file.
// Implementation

#include "MPEG4VideoFileServerMediaSubsession.hh"
#include "MPEG4ESVideoRTPSink.hh"
#include "ByteStreamFileSource.hh"
#include "MPEG4VideoStreamFramer.hh"
#include "LogMacros.hh"

MPEG4VideoFileServerMediaSubsession*
MPEG4VideoFileServerMediaSubsession::createNew(UsageEnvironment& env,
					       char const* fileName,
					       Boolean reuseFirstSource) {
  return new MPEG4VideoFileServerMediaSubsession(env, fileName, reuseFirstSource);
}

MPEG4VideoFileServerMediaSubsession
::MPEG4VideoFileServerMediaSubsession(UsageEnvironment& env,
                                      char const* fileName, Boolean reuseFirstSource)
  : FileServerMediaSubsession(env, fileName, reuseFirstSource),
    fDoneFlag(0) {
}

MPEG4VideoFileServerMediaSubsession
::~MPEG4VideoFileServerMediaSubsession() {
}

static void afterPlayingDummy(void* clientData) {
  MPEG4VideoFileServerMediaSubsession* subsess
    = (MPEG4VideoFileServerMediaSubsession*)clientData;
  subsess->afterPlayingDummy1();
}

void MPEG4VideoFileServerMediaSubsession::afterPlayingDummy1() {
  DEBUG_LOG(INF, "MPEG4VideoFileServerMediaSubsession::afterPlayingDummy1");
  // Unschedule any pending 'checking' task:
  envir().taskScheduler().unscheduleDelayedTask(nextTask());
  // Signal the event loop that we're done:
  setDoneFlag();
}

static void checkForAuxSDPLine(void* clientData) {
  MPEG4VideoFileServerMediaSubsession* subsess
    = (MPEG4VideoFileServerMediaSubsession*)clientData;
  subsess->checkForAuxSDPLine1();
}

void MPEG4VideoFileServerMediaSubsession::checkForAuxSDPLine1() {
  //如果已经获取到auxSDP，则让任务调度器直接返回，否则延迟100ms再重新checkForAuxSDPLine1
  if (fDummyRTPSink->auxSDPLine() != NULL) {
    // Signal the event loop that we're done:
    DEBUG_LOG(INF, "Get AuxSDP success");
    setDoneFlag();
  } else {
    // try again after a brief delay:
    DEBUG_LOG(INF, "Delay 100ms to check AuxSDP");
    int uSecsToDelay = 100000; // 100 ms
    nextTask() = envir().taskScheduler().scheduleDelayedTask(uSecsToDelay,
			      (TaskFunc*)checkForAuxSDPLine, this);
  }
}

//Aux 辅助的，额外的；SDP 会话描述信息
char const* MPEG4VideoFileServerMediaSubsession
::getAuxSDPLine(RTPSink* rtpSink, FramedSource* inputSource) {
  DEBUG_LOG(INF, "Start playing to get AuxSDP");
  // Note: For MPEG-4 video files, the 'config' information isn't known
  // until we start reading the file.  This means that "rtpSink"s
  // "auxSDPLine()" will be NULL initially, and we need to start reading
  // data from our file until this changes.
  // 只有等播放后才能获取到AuxSDP
  fDummyRTPSink = rtpSink;

  // Start reading the file:
  fDummyRTPSink->startPlaying(*inputSource, afterPlayingDummy, this);

  // Check whether the sink's 'auxSDPLine()' is ready:
  checkForAuxSDPLine(this);

  DEBUG_LOG(INF, "doEventLoop, fDoneFlag=%d", fDoneFlag);
  envir().taskScheduler().doEventLoop(&fDoneFlag);

  char const* auxSDPLine = fDummyRTPSink->auxSDPLine();
  DEBUG_LOG(INF, "AuxSDPLine: %s", auxSDPLine);
  return auxSDPLine;
}

FramedSource* MPEG4VideoFileServerMediaSubsession
::createNewStreamSource(unsigned /*clientSessionId*/, unsigned& estBitrate) {
  DEBUG_LOG(INF, "MPEG4VideoFileServerMediaSubsession::createNewStreamSource, estBitrate=%d", estBitrate);
  estBitrate = 500; // kbps, estimate

  // Create the video source:
  ByteStreamFileSource* fileSource
    = ByteStreamFileSource::createNew(envir(), fFileName);
  if (fileSource == NULL) return NULL;
  fFileSize = fileSource->fileSize();

  // Create a framer for the Video Elementary Stream:
  return MPEG4VideoStreamFramer::createNew(envir(), fileSource);
}

RTPSink* MPEG4VideoFileServerMediaSubsession
::createNewRTPSink(Groupsock* rtpGroupsock,
		   unsigned char rtpPayloadTypeIfDynamic,
		   FramedSource* /*inputSource*/) {
  DEBUG_LOG(INF, "MPEG4VideoFileServerMediaSubsession::createNewRTPSink");
  return MPEG4ESVideoRTPSink::createNew(envir(), rtpGroupsock,
					rtpPayloadTypeIfDynamic);
}
