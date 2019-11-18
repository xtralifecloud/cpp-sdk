//
//  XtraLife_thread.cpp
//  XtraLife
//
//  Created by Roland Van Leeuwen on 23/09/11.
//  Copyright 2011 __MyCompanyName__. All rights reserved.
//

#include <pthread.h>

#include "XtraLife_thread.h"
#include "include/XtraLifeHelpers.h"

namespace XtraLife {
namespace Helpers {

// On Windows some defines are missing
#ifdef _WINDOWS
#include <sys/timeb.h>
struct timeval {
	long tv_sec, tv_usec;		// seconds/microseconds
};

int gettimeofday (struct timeval *tp, void *tz) {
	struct _timeb timebuffer;
	_ftime(&timebuffer);
	tp->tv_sec = (long) timebuffer.time;
	tp->tv_usec = timebuffer.millitm * 1000;
	return 0;
}
#else
#include <sys/time.h>
#endif

//////////////////////////// OS-specific vars for the classes //////////////////////////////////////////////
struct CMutexVars {
	pthread_mutex_t mutex;
};
struct CConditionVariableVars {
	pthread_cond_t cond;
};
struct CThreadVars {
	pthread_t runningThread;
};

//////////////////////////// Mutex //////////////////////////////////////////////
CMutex::CMutex() {
	mVars = new CMutexVars;
	pthread_mutex_init(&mVars->mutex, NULL);
}

CMutex::~CMutex() {
	pthread_mutex_destroy(&mVars->mutex);
	delete mVars;
}

void CMutex::Lock() {
	pthread_mutex_lock(&mVars->mutex);
}

void CMutex::Unlock() {
	pthread_mutex_unlock(&mVars->mutex);
}

//////////////////////////// Thread //////////////////////////////////////////////
CThread::CThread() : mState(READY) {
	mVars = new CThreadVars;
}

CThread::~CThread() {
	if (mState != READY) {
		pthread_detach(mVars->runningThread);
	}
	delete mVars;
}

bool CThread::Start() {
	// Do not allow to start it twice
	if (mState != READY) {
		return false;
	}

	// Start the thread
	int rc = pthread_create(&mVars->runningThread, NULL, startupRoutine, (void*) this);
	if (rc == 0) {
		mState = RUNNING;
		this->Retain();
	}
	return rc == 0;
}

bool CThread::Join() {
	if (mState != RUNNING) {
		return false;
	}

	void *unusedThExitCode;
	int rc = pthread_join(mVars->runningThread, &unusedThExitCode);
	return rc == 0;
}

void CThread::RunForInternalUse() {
	Run();
	mState = COMPLETED;
	this->Release();
}

void *CThread::startupRoutine(void *class_xlThread) {
	CThread *ptr = (CThread*)class_xlThread;
	ptr->RunForInternalUse();
	return NULL;
}

//////////////////////////// Condition variable //////////////////////////////////////////////
CConditionVariable::CConditionVariable() {
	mVars = new CConditionVariableVars;
	pthread_cond_init(&mVars->cond, NULL);
}

CConditionVariable::~CConditionVariable() {
	pthread_cond_destroy(&mVars->cond);
	delete mVars;
}

void CConditionVariable::LockVar() {
	mMutex.Lock();
}

void CConditionVariable::UnlockVar() {
	mMutex.Unlock();
}

void CConditionVariable::SignalAll() {
	pthread_cond_broadcast(&mVars->cond);
}

void CConditionVariable::SignalOne() {
	pthread_cond_signal(&mVars->cond);
}

bool CConditionVariable::Wait(int timeoutMilliseconds) {
	if (timeoutMilliseconds == 0) {
		return pthread_cond_wait(&mVars->cond, &mMutex.mVars->mutex) == 0;
	} else {
		timeval tv;
		timespec ts;
		gettimeofday(&tv, NULL);
		ts.tv_sec = time(NULL) + timeoutMilliseconds / 1000;
		ts.tv_nsec = tv.tv_usec * 1000 + 1000 * 1000 * (timeoutMilliseconds % 1000);
		ts.tv_sec += ts.tv_nsec / (1000 * 1000 * 1000);
		ts.tv_nsec %= (1000 * 1000 * 1000);
		return pthread_cond_timedwait(&mVars->cond, &mMutex.mVars->mutex, &ts) == 0;
	}
}

}
}
