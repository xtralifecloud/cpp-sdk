//
//  XtraLife_thread.h
//  XtraLife
//
//  Created by Roland Van Leeuwen on 23/09/11.
//  Copyright 2011 Clan of the Cloud. All rights reserved.
//

#ifndef XtraLife_thread_h
#define XtraLife_thread_h

#include "include/XtraLifeHelpers.h"

namespace XtraLife {
    namespace Helpers {

        /**
         * Simple thread. Just provide an implementation for the Run() method and start it by calling Start() on the resulting instance.
         * This is a CRefClass, meaning that you need to call Release() on it when you are done using it. Do not delete it directly (if the thread is being run, your program will crash).
         * To start a task and forget about it, you may do as follows: \code
            struct MyTask: public CThread {
                virtual void Run() {
                    printf("Starting\n");
                    usleep(3 * 1000 * 1000);
                    printf("Done\n");
                }
            };
            MyTask *t = new MyTask;
            t->Start();
            t->Release(); \endcode
         */
        class CThread : public CRefClass {
            enum state {READY, RUNNING, COMPLETED} mState;
            struct CThreadVars *mVars;

            static void *startupRoutine(void * class_xlThread);
            void RunForInternalUse();

        protected:
            /**
             * Override and implement your threaded work in this method.
             * Never call this directly, use RunForInternalUse instead.
             */
            virtual void Run() = 0;

        public:
            CThread();
            virtual ~CThread();
            /**
             * Starts the execution of the thread (instantiating is not enough). Note
             * that starting a thread automatically Retains it until execution finishes.
             * @return whether the thread could be started. Note that you cannot start a
             * thread twice, even if it has finished.
             */
            bool Start();
            /**
             * Joins with the thread, waiting for it to end and returning only once done.
             * @return whether everything went well (but regardless of the return value,
             * the thread shouldn't be running anymore upon return)
             */
            bool Join();
            /**
             * @return whether the thread has completed its work.
             */
            bool HasFinished() { return mState == COMPLETED; }
        };

        /**
         * Represents a mutex. Locks content for mutually exclusive access on two threads.
         * Just call Lock when you want to access data, and Unlock when you are done.
         */
        class CMutex {
            struct CMutexVars *mVars;
            friend class CConditionVariable;

        public:
            CMutex();
            ~CMutex();

            void Lock();
            void Unlock();
            
            /**
             * Use this to lock a mutex in a closed scope. { CMutex::ScopedLock(myMutex); ... }
             * The mutex is locked and automatically unlocked at the end of the current block (function).
             */
            struct ScopedLock {
                CMutex &mutex;
                ScopedLock(CMutex &mutex) : mutex(mutex) { mutex.Lock(); }
                ~ScopedLock() { mutex.Unlock(); }
            };
            /**
             * This one does just the same but only if a boolean variable passed in the constructor is true,
             * else the lock is untouched.
             */
            struct ConditionallyScopedLock {
                CMutex *mMutex;
                ConditionallyScopedLock(CMutex &mutex, bool lockItOrNot) : mMutex(lockItOrNot? &mutex : NULL) { if (mMutex) { mMutex->Lock(); } }
                ~ConditionallyScopedLock() { if (mMutex) { mMutex->Unlock(); } }
            };
        };

        /**
         * Basic condition variable. Prefer CProtectedVariable to protect content.
         */
        class CConditionVariable {
            CMutex mMutex;
            struct CConditionVariableVars *mVars;

        public:
            CConditionVariable();
            ~CConditionVariable();
            void LockVar();
            void UnlockVar();
            void SignalAll();
            void SignalOne();
            bool Wait(int timeoutMilliseconds = 0);
        };

        /**
         * Represents a locked variable. It is used when a content has to be accessed mutually exclusively on multiple threads, and you want one of the threads to wait for a modification. \code
            CProtectedVariable<int> countGuard(0);

            void takeOneAndWaitUntilAvailable() {
                int *count = countGuard.LockVar();
                while (*count == 0) {
                    countGuard.Wait();
                }
                (*count) -= 1;
                count = countGuard.UnlockVar();
            }

            void giveOne() {
                int *count = countGuard.LockVar();
                if (*count == 0) {
                    countGuard.SignalAll();
                }
                (*count) += 1;
                count = countGuard.UnlockVar();
            } \endcode
         */
        template <class T>
        class CProtectedVariable {
            T mProtectedData;
            CConditionVariable mCondVar;

        public:
            CProtectedVariable() {}
            CProtectedVariable(T value) : mProtectedData(value) {}

            /**
             * Request access to the variable (once you get it, you will be the only thread at the time).
             * @return a pointer to the protected variable for raw access
             */
            T *LockVar() { mCondVar.LockVar(); return &mProtectedData; }
            /**
             * Unlock the variable, allowing it to be modified by other threads.
             * @return a null pointer that we recommend assigning to prevent further modification
             */
            T *UnlockVar() { mCondVar.UnlockVar(); return NULL; }

            /**
             * Must be called within a LockVar/UnlockVar block.
             */
            void SignalOne() { mCondVar.SignalOne(); }
            /**
             * Must be called within a LockVar/UnlockVar block.
             */
            void SignalAll() { mCondVar.SignalAll(); }
            /**
             * Waits for a call to Signal*() from another thread. Must be called within a LockVar/UnlockVar block.
             * @return whether the condition has been signaled (false means a timeout or error)
             */
            bool Wait(int timeoutMilliseconds = 0) { return mCondVar.Wait(timeoutMilliseconds); }
        };

    }
}

#endif
