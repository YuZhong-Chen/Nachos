// scheduler.cc
//	Routines to choose the next thread to run, and to dispatch to
//	that thread.
//
// 	These routines assume that interrupts are already disabled.
//	If interrupts are disabled, we can assume mutual exclusion
//	(since we are on a uniprocessor).
//
// 	NOTE: We can't use Locks to provide mutual exclusion here, since
// 	if we needed to wait for a lock, and the lock was busy, we would
//	end up calling FindNextToRun(), and that would put us in an
//	infinite loop.
//
// 	Very simple implementation -- no priorities, straight FIFO.
//	Might need to be improved in later assignments.
//
// Copyright (c) 1992-1996 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "debug.h"
//
#include "scheduler.h"
//
#include <queue>

#include "main.h"

//----------------------------------------------------------------------
// Scheduler::Scheduler
// 	Initialize the list of ready but not running threads.
//	Initially, no ready threads.
//----------------------------------------------------------------------
Scheduler::Scheduler() {
    ReadyList_L1 = new SortedList<Thread *>(Thread::ComparePriority);
    ReadyList_L2 = new SortedList<Thread *>(Thread::ComparePriority);
    ReadyList_L3 = new List<Thread *>;

    toBeDestroyed = NULL;
}

//----------------------------------------------------------------------
// Scheduler::~Scheduler
// 	De-allocate the list of ready threads.
//----------------------------------------------------------------------

Scheduler::~Scheduler() {
    delete ReadyList_L1;
    delete ReadyList_L2;
    delete ReadyList_L3;
}

//----------------------------------------------------------------------
// Scheduler::ReadyToRun
// 	Mark a thread as ready, but not running.
//	Put it on the ready list, for later scheduling onto the CPU.
//
//	"thread" is the thread to be put on the ready list.
//----------------------------------------------------------------------
void Scheduler::ReadyToRun(Thread *thread) {
    ASSERT(kernel->interrupt->getLevel() == IntOff);
    DEBUG(dbgThread, "Putting thread on ready list: " << thread->getName());

    thread->setStatus(READY);
    int Priority = thread->GetPriority();

    if (Priority >= 100) {
        // DEBUG(dbgTick, "Tick " << kernel->stats->totalTicks << ": Thread " << thread->getID() << " is inserted into queue L1");
        ReadyList_L1->Insert(thread);

        // if (kernel->currentThread->GetPriority() <= 99 || thread->GetApproximatedBurstTime() - thread->GetRunningBurstTime() < kernel->currentThread->GetApproximatedBurstTime() - kernel->currentThread->GetRunningBurstTime()) {
        //     kernel->currentThread->Yield();
        // }
    } else if (Priority >= 50) {
        // DEBUG(dbgTick, "Tick " << kernel->stats->totalTicks << ": Thread " << thread->getID() << " is inserted into queue L2");
        ReadyList_L2->Insert(thread);

        // if (kernel->currentThread->GetPriority() <= 49) {
        //     kernel->currentThread->Yield();
        // }
    } else {
        // DEBUG(dbgTick, "Tick " << kernel->stats->totalTicks << ": Thread " << thread->getID() << " is inserted into queue L3");
        ReadyList_L3->Append(thread);
    }
}

//----------------------------------------------------------------------
// Scheduler::FindNextToRun
// 	Return the next thread to be scheduled onto the CPU.
//	If there are no ready threads, return NULL.
// Side effect:
//	Thread is removed from the ready list.
//----------------------------------------------------------------------
Thread *Scheduler::FindNextToRun() {
    ASSERT(kernel->interrupt->getLevel() == IntOff);

    Thread *RemovedThread = NULL;
    if (!ReadyList_L1->IsEmpty()) {
        RemovedThread = ReadyList_L1->RemoveFront();
        // DEBUG(dbgTick, "Tick " << kernel->stats->totalTicks << ": Thread " << RemovedThread->getID() << " is removed from queue L1");
    } else if (!ReadyList_L2->IsEmpty()) {
        RemovedThread = ReadyList_L2->RemoveFront();
        // DEBUG(dbgTick, "Tick " << kernel->stats->totalTicks << ": Thread " << RemovedThread->getID() << " is removed from queue L2");
    } else if (!ReadyList_L3->IsEmpty()) {
        RemovedThread = ReadyList_L3->RemoveFront();
        // DEBUG(dbgTick, "Tick " << kernel->stats->totalTicks << ": Thread " << RemovedThread->getID() << " is removed from queue L3");
    }

    // if (RemovedThread != NULL) {
    //     cout << "Ticks : " << kernel->stats->totalTicks << " Select : " << RemovedThread->getID() << '\n';
    // }

    return RemovedThread;
}

//----------------------------------------------------------------------
// Scheduler::Run
// 	Dispatch the CPU to nextThread.  Save the state of the old thread,
//	and load the state of the new thread, by calling the machine
//	dependent context switch routine, SWITCH.
//
// Note: we assume the state of the previously running thread has
//	already been changed from running to blocked or ready (depending).
//
// Side effect:
//	The global variable kernel->currentThread becomes nextThread.
//
//	"nextThread" is the thread to be put into the CPU.
//	"finishing" is set if the current thread is to be deleted
//		once we're no longer running on its stack
//		(when the next thread starts running)
//----------------------------------------------------------------------

void Scheduler::Run(Thread *nextThread, bool finishing) {
    Thread *oldThread = kernel->currentThread;

    ASSERT(kernel->interrupt->getLevel() == IntOff);

    if (finishing) {  // mark that we need to delete current thread
        ASSERT(toBeDestroyed == NULL);
        toBeDestroyed = oldThread;
    }

    if (oldThread->space != NULL) {  // if this thread is a user program,
        oldThread->SaveUserState();  // save the user's CPU registers
        oldThread->space->SaveState();
    }

    oldThread->CheckOverflow();  // check if the old thread
                                 // had an undetected stack overflow
    oldThread->SetRunningBurstTime(kernel->stats->totalTicks - oldThread->GetStartTime());

    kernel->currentThread = nextThread;  // switch to the next thread
    nextThread->setStatus(RUNNING);      // nextThread is now running
    nextThread->SetStartTime(kernel->stats->totalTicks);
    nextThread->ResetWaitingTime();

    if (oldThread != nextThread) {
        DEBUG(dbgThread, "Switching from: " << oldThread->getName() << " to: " << nextThread->getName());

        if (oldThread->getID() != 0)
            DEBUG(dbgTick, "Tick " << kernel->stats->totalTicks << ": Thread " << nextThread->getID() << " is now selected for execution, thread " << oldThread->getID() << " is replaced, and it has executed " << oldThread->GetRunningBurstTime() << " ticks");

        // This is a machine-dependent assembly language routine defined
        // in switch.s.  You may have to think
        // a bit to figure out what happens after this, both from the point
        // of view of the thread and from the perspective of the "outside world".
        SWITCH(oldThread, nextThread);

        // we're back, running oldThread

        // interrupts are off when we return from switch!
        ASSERT(kernel->interrupt->getLevel() == IntOff);

        DEBUG(dbgThread, "Now in thread: " << oldThread->getName());
    }

    CheckToBeDestroyed();  // check if thread we were running
                           // before this one has finished
                           // and needs to be cleaned up

    if (oldThread->space != NULL) {     // if there is an address space
        oldThread->RestoreUserState();  // to restore, do it.
        oldThread->space->RestoreState();
    }
}

//----------------------------------------------------------------------
// Scheduler::CheckToBeDestroyed
// 	If the old thread gave up the processor because it was finishing,
// 	we need to delete its carcass.  Note we cannot delete the thread
// 	before now (for example, in Thread::Finish()), because up to this
// 	point, we were still running on the old thread's stack!
//----------------------------------------------------------------------
void Scheduler::CheckToBeDestroyed() {
    if (toBeDestroyed != NULL) {
        delete toBeDestroyed;
        toBeDestroyed = NULL;
    }
}

void Scheduler::AgingThread(int Ticks) {
    std::queue<Thread *> CheckQueue;
    std::queue<Thread *> UpdateQueue;

    // Aging Thread in L3
    while (!ReadyList_L3->IsEmpty()) {
        Thread *temp = ReadyList_L3->RemoveFront();
        if (temp->IncreaseWaitingTime(Ticks) && temp->GetPriority() >= 50) {
            UpdateQueue.push(temp);
            DEBUG(dbgTick, "Tick " << kernel->stats->totalTicks << ": Thread " << temp->getID() << " is removed from queue L3");
            DEBUG(dbgTick, "Tick " << kernel->stats->totalTicks << ": Thread " << temp->getID() << " is inserted into queue L2");
        } else {
            CheckQueue.push(temp);
        }
    }
    while (!CheckQueue.empty()) {
        ReadyList_L3->Append(CheckQueue.front());
        CheckQueue.pop();
    }
    while (!UpdateQueue.empty()) {
        CheckQueue.push(UpdateQueue.front());
        UpdateQueue.pop();
    }

    // Aging Thread in L2
    while (!ReadyList_L2->IsEmpty()) {
        Thread *temp = ReadyList_L2->RemoveFront();
        if (temp->IncreaseWaitingTime(Ticks) && temp->GetPriority() >= 100) {
            UpdateQueue.push(temp);
            DEBUG(dbgTick, "Tick " << kernel->stats->totalTicks << ": Thread " << temp->getID() << " is removed from queue L2");
            DEBUG(dbgTick, "Tick " << kernel->stats->totalTicks << ": Thread " << temp->getID() << " is inserted into queue L1");
        } else {
            CheckQueue.push(temp);
        }
    }
    while (!CheckQueue.empty()) {
        ReadyList_L2->Insert(CheckQueue.front());
        CheckQueue.pop();
    }
    while (!UpdateQueue.empty()) {
        CheckQueue.push(UpdateQueue.front());
        UpdateQueue.pop();
    }

    // Aging Thread in L1
    while (!ReadyList_L1->IsEmpty()) {
        Thread *temp = ReadyList_L1->RemoveFront();
        temp->IncreaseWaitingTime(Ticks);
        CheckQueue.push(temp);
    }
    while (!CheckQueue.empty()) {
        ReadyList_L1->Insert(CheckQueue.front());
        CheckQueue.pop();
    }
}

//----------------------------------------------------------------------
// Scheduler::Print
// 	Print the scheduler state -- in other words, the contents of
//	the ready list.  For debugging.
//----------------------------------------------------------------------
void Scheduler::Print() {
    if (!ReadyList_L1->IsEmpty()) {
        cout << "ReadyList_L1 contents: ";
        ReadyList_L1->Apply(ThreadPrint);
        cout << '\n';
    }

    if (!ReadyList_L2->IsEmpty()) {
        cout << "ReadyList_L2 contents: ";
        ReadyList_L2->Apply(ThreadPrint);
        cout << '\n';
    }

    if (!ReadyList_L3->IsEmpty()) {
        cout << "ReadyList_L3 contents: ";
        ReadyList_L3->Apply(ThreadPrint);
        cout << '\n';
    }
}
