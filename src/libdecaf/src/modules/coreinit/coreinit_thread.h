#pragma once
#include "coreinit_enum.h"
#include "coreinit_time.h"
#include "coreinit_internal_queue.h"
#include "common/be_val.h"
#include "common/structsize.h"
#include "virtual_ptr.h"
#include "ppcutils/wfunc_ptr.h"
#include "kernel/kernel.h"

struct Fiber;

namespace coreinit
{

/**
 * \defgroup coreinit_thread Thread
 * \ingroup coreinit
 *
 * The thread scheduler in the Wii U uses co-operative scheduling, this is different
 * to the usual pre-emptive scheduling that most operating systems use (such as
 * Windows, Linux, etc). In co-operative scheduling threads must voluntarily yield
 * execution to other threads. In pre-emptive threads are switched by the operating
 * system after an amount of time.
 *
 * With the Wii U's scheduling model the thread with the highest priority which
 * is in a non-waiting state will always be running (where 0 is the highest
 * priority and 31 is the lowest). Execution will only switch to other threads
 * once this thread has been forced to wait, such as when waiting to acquire a
 * mutex, or when the thread voluntarily yields execution to other threads which
 * have the same priority using OSYieldThread. OSYieldThread will never yield to
 * a thread with lower priority than the current thread.
 * @{
 */

#pragma pack(push, 1)

struct OSAlarm;
struct OSThread;

using OSThreadEntryPointFn = wfunc_ptr<uint32_t, uint32_t, void*>;
using OSThreadCleanupCallbackFn = wfunc_ptr<void, OSThread *, void *>;
using OSThreadDeallocatorFn = wfunc_ptr<void, OSThread *, void *>;

using be_OSThreadEntryPointFn = be_wfunc_ptr<uint32_t, uint32_t, void*>;
using be_OSThreadCleanupCallbackFn = be_wfunc_ptr<void, OSThread *, void *>;
using be_OSThreadDeallocatorFn = be_wfunc_ptr<void, OSThread *, void *>;

struct OSContext
{
   static const uint64_t Tag1 = 0x4F53436F6E747874ull;

   //! Should always be set to the value OSContext::Tag.
   be_val<uint64_t> tag;
   be_val<uint32_t> gpr[32];
   be_val<uint32_t> cr;
   be_val<uint32_t> lr;
   be_val<uint32_t> ctr;
   be_val<uint32_t> xer;

   // srr0 and srr1 would usually be here, however because these are used
   //  for operating system things and we are HLE, it should be safe to
   //  override them with our internal HLE linkup.
   be_val<uint32_t> nia;
   be_val<uint32_t> cia;

   UNKNOWN(0x14);
   be_val<uint32_t> fpscr;
   be_val<double> fpr[32];
   be_val<uint16_t> spinLockCount;
   be_val<uint16_t> state;
   be_val<uint32_t> gqr[8];
   UNKNOWN(4);
   be_val<double> psf[32];
   be_val<uint64_t> coretime[3];
   be_val<uint64_t> starttime;
   be_val<uint32_t> error;
   UNKNOWN(4);
   be_val<uint32_t> pmc1;
   be_val<uint32_t> pmc2;

   // pmc3 and pmc4 would usually be here, however because these are used
   //  for operating system things, it should be safe to use them.
   kernel::Fiber *fiber;

   be_val<uint32_t> mmcr0;
   be_val<uint32_t> mmcr1;
};
CHECK_OFFSET(OSContext, 0x00, tag);
CHECK_OFFSET(OSContext, 0x08, gpr);
CHECK_OFFSET(OSContext, 0x88, cr);
CHECK_OFFSET(OSContext, 0x8c, lr);
CHECK_OFFSET(OSContext, 0x90, ctr);
CHECK_OFFSET(OSContext, 0x94, xer);
//CHECK_OFFSET(OSContext, 0x98, srr0);
//CHECK_OFFSET(OSContext, 0x9c, srr1);
CHECK_OFFSET(OSContext, 0xb4, fpscr);
CHECK_OFFSET(OSContext, 0xb8, fpr);
CHECK_OFFSET(OSContext, 0x1b8, spinLockCount);
CHECK_OFFSET(OSContext, 0x1ba, state);
CHECK_OFFSET(OSContext, 0x1bc, gqr);
CHECK_OFFSET(OSContext, 0x1e0, psf);
CHECK_OFFSET(OSContext, 0x2e0, coretime);
CHECK_OFFSET(OSContext, 0x2f8, starttime);
CHECK_OFFSET(OSContext, 0x300, error);
CHECK_OFFSET(OSContext, 0x308, pmc1);
CHECK_OFFSET(OSContext, 0x30c, pmc2);
//CHECK_OFFSET(OSContext, 0x310, pmc3);
//CHECK_OFFSET(OSContext, 0x314, pmc4);
CHECK_OFFSET(OSContext, 0x318, mmcr0);
CHECK_OFFSET(OSContext, 0x31c, mmcr1);
CHECK_SIZE(OSContext, 0x320);

struct OSMutex;

struct OSMutexQueue
{
   be_ptr<OSMutex> head;
   be_ptr<OSMutex> tail;
   be_ptr<void> parent;
   UNKNOWN(4);
};
CHECK_OFFSET(OSMutexQueue, 0x0, head);
CHECK_OFFSET(OSMutexQueue, 0x4, tail);
CHECK_OFFSET(OSMutexQueue, 0x8, parent);
CHECK_SIZE(OSMutexQueue, 0x10);

struct OSFastMutex;

struct OSFastMutexQueue
{
   be_ptr<OSFastMutex> head;
   be_ptr<OSFastMutex> tail;
};
CHECK_OFFSET(OSFastMutexQueue, 0x00, head);
CHECK_OFFSET(OSFastMutexQueue, 0x04, tail);
CHECK_SIZE(OSFastMutexQueue, 0x08);

struct OSThreadLink
{
   be_ptr<OSThread> prev;
   be_ptr<OSThread> next;
};
CHECK_OFFSET(OSThreadLink, 0x00, prev);
CHECK_OFFSET(OSThreadLink, 0x04, next);
CHECK_SIZE(OSThreadLink, 0x8);

struct OSThreadQueue
{
   be_ptr<OSThread> head;
   be_ptr<OSThread> tail;
   be_ptr<void> parent;
   UNKNOWN(4);
};
CHECK_OFFSET(OSThreadQueue, 0x00, head);
CHECK_OFFSET(OSThreadQueue, 0x04, tail);
CHECK_OFFSET(OSThreadQueue, 0x08, parent);
CHECK_SIZE(OSThreadQueue, 0x10);

struct OSThreadSimpleQueue
{
   be_ptr<OSThread> head;
   be_ptr<OSThread> tail;
};
CHECK_OFFSET(OSThreadSimpleQueue, 0x00, head);
CHECK_OFFSET(OSThreadSimpleQueue, 0x04, tail);
CHECK_SIZE(OSThreadSimpleQueue, 0x08);

struct OSTLSSection
{
   be_ptr<void> data;
   UNKNOWN(4);
};
CHECK_OFFSET(OSTLSSection, 0x00, data);
CHECK_SIZE(OSTLSSection, 0x08);

struct OSThread
{
   static const uint32_t Tag = 0x74487244;

   OSContext context;

   //! Should always be set to the value OSThread::Tag.
   be_val<uint32_t> tag;

   //! Bitfield of OScpu::Core
   be_val<OSThreadState> state;

   //! Bitfield of OSThreadAttributes
   be_val<OSThreadAttributes> attr;

   //! Unique thread ID
   be_val<uint16_t> id;

   //! Suspend count (increased by OSSuspendThread).
   be_val<int32_t> suspendCounter;

   //! Actual priority of thread.
   be_val<int32_t> priority;

   //! Base priority of thread, 0 is highest priority, 31 is lowest priority.
   be_val<int32_t> basePriority;

   //! Exit value of the thread
   be_val<uint32_t> exitValue;

   //! Core run queue stuff
   be_ptr<OSThreadQueue> coreRunQueue0;
   be_ptr<OSThreadQueue> coreRunQueue1;
   be_ptr<OSThreadQueue> coreRunQueue2;
   OSThreadLink coreRunQueueLink0;
   OSThreadLink coreRunQueueLink1;
   OSThreadLink coreRunQueueLink2;

   //! Queue the thread is currently waiting on
   be_ptr<OSThreadQueue> queue;

   //! Link used for thread queue
   OSThreadLink link;

   //! Queue of threads waiting to join this thread
   OSThreadQueue joinQueue;

   //! Mutex this thread is waiting to lock
   be_ptr<OSMutex> mutex;

   //! Queue of mutexes this thread owns
   OSMutexQueue mutexQueue;

   //! Link for global active thread queue
   OSThreadLink activeLink;

   //! Stack start (top, highest address)
   be_ptr<be_val<uint32_t>> stackStart;

   //! Stack end (bottom, lowest address)
   be_ptr<be_val<uint32_t>> stackEnd;

   //! Thread entry point set in OSCreateThread
   be_OSThreadEntryPointFn entryPoint;

   UNKNOWN(0x408 - 0x3a0);

   //! GEH Exception handling thread-specifics
   be_ptr<void> _ghs__eh_globals;
   be_ptr<void> _ghs__eh_mem_manage[9];
   be_ptr<void> _ghs__eh_store_globals[6];
   be_ptr<void> _ghs__eh_store_globals_tdeh[76];

   be_val<uint32_t> alarmCancelled;

   //! Thread specific values, accessed with OSSetThreadSpecific and OSGetThreadSpecific.
   be_val<uint32_t> specific[0x10];
   UNKNOWN(0x5c0 - 0x5bc);

   //! Thread name, accessed with OSSetThreadName and OSGetThreadName.
   be_ptr<const char> name;

   //! Alarm the thread is waiting on in OSWaitEventWithTimeout
   be_ptr<OSAlarm> waitEventTimeoutAlarm;

   //! The stack pointer passed in OSCreateThread.
   be_ptr<be_val<uint32_t>> userStackPointer;

   //! Called just before thread is terminated, set with OSSetThreadCleanupCallback
   be_OSThreadCleanupCallbackFn cleanupCallback;

   //! Called just after a thread is terminated, set with OSSetThreadDeallocator
   be_OSThreadDeallocatorFn deallocator;

   //! If TRUE then a thread can be cancelled or suspended, set with OSSetThreadCancelState
   be_val<uint32_t> cancelState;

   //! Current thread request, used for cancelleing and suspending the thread.
   be_val<OSThreadRequest> requestFlag;

   //! Pending suspend request count
   be_val<int32_t> needSuspend;

   //! Result of thread suspend
   be_val<int32_t> suspendResult;

   //! Queue of threads waiting for a thread to be suspended.
   OSThreadQueue suspendQueue;

   UNKNOWN(0xC);

   //! The total amount of core time consumed by this thread (Does not include time while Running)
   be_val<uint64_t> coreTimeConsumedNs;

   //! The number of times this thread has been awoken.
   be_val<uint64_t> wakeCount;

   UNKNOWN(0x664 - 0x610);

   //! Number of TLS sections
   be_val<uint16_t> tlsSectionCount;

   UNKNOWN(0x2);

   //! TLS Sections
   be_ptr<OSTLSSection> tlsSections;

   UNKNOWN(0x69c - 0x66C);
};
CHECK_OFFSET(OSThread, 0x320, tag);
CHECK_OFFSET(OSThread, 0x324, state);
CHECK_OFFSET(OSThread, 0x325, attr);
CHECK_OFFSET(OSThread, 0x326, id);
CHECK_OFFSET(OSThread, 0x328, suspendCounter);
CHECK_OFFSET(OSThread, 0x32c, priority);
CHECK_OFFSET(OSThread, 0x330, basePriority);
CHECK_OFFSET(OSThread, 0x334, exitValue);
CHECK_OFFSET(OSThread, 0x338, coreRunQueue0);
CHECK_OFFSET(OSThread, 0x33C, coreRunQueue1);
CHECK_OFFSET(OSThread, 0x340, coreRunQueue2);
CHECK_OFFSET(OSThread, 0x344, coreRunQueueLink0);
CHECK_OFFSET(OSThread, 0x34C, coreRunQueueLink1);
CHECK_OFFSET(OSThread, 0x354, coreRunQueueLink2);
CHECK_OFFSET(OSThread, 0x35c, queue);
CHECK_OFFSET(OSThread, 0x360, link);
CHECK_OFFSET(OSThread, 0x368, joinQueue);
CHECK_OFFSET(OSThread, 0x378, mutex);
CHECK_OFFSET(OSThread, 0x37c, mutexQueue);
CHECK_OFFSET(OSThread, 0x38c, activeLink);
CHECK_OFFSET(OSThread, 0x394, stackStart);
CHECK_OFFSET(OSThread, 0x398, stackEnd);
CHECK_OFFSET(OSThread, 0x39c, entryPoint);
CHECK_OFFSET(OSThread, 0x578, alarmCancelled);
CHECK_OFFSET(OSThread, 0x57c, specific);
CHECK_OFFSET(OSThread, 0x5c0, name);
CHECK_OFFSET(OSThread, 0x5c4, waitEventTimeoutAlarm);
CHECK_OFFSET(OSThread, 0x5c8, userStackPointer);
CHECK_OFFSET(OSThread, 0x5cc, cleanupCallback);
CHECK_OFFSET(OSThread, 0x5d0, deallocator);
CHECK_OFFSET(OSThread, 0x5d4, cancelState);
CHECK_OFFSET(OSThread, 0x5d8, requestFlag);
CHECK_OFFSET(OSThread, 0x5dc, needSuspend);
CHECK_OFFSET(OSThread, 0x5e0, suspendResult);
CHECK_OFFSET(OSThread, 0x5e4, suspendQueue);
CHECK_OFFSET(OSThread, 0x600, coreTimeConsumedNs);
CHECK_OFFSET(OSThread, 0x608, wakeCount);
CHECK_OFFSET(OSThread, 0x664, tlsSectionCount);
CHECK_OFFSET(OSThread, 0x668, tlsSections);
CHECK_SIZE(OSThread, 0x69c);

struct tls_index
{
   be_val<uint32_t> moduleIndex;
   be_val<uint32_t> offset;
};
CHECK_OFFSET(tls_index, 0x00, moduleIndex);
CHECK_OFFSET(tls_index, 0x04, offset);
CHECK_SIZE(tls_index, 0x08);

#pragma pack(pop)

void
OSCancelThread(OSThread *thread);

int32_t
OSCheckActiveThreads();

int32_t
OSCheckThreadStackUsage(OSThread *thread);

void
OSClearThreadStackUsage(OSThread *thread);

void
OSContinueThread(OSThread *thread);

BOOL
OSCreateThread(OSThread *thread,
               OSThreadEntryPointFn entry,
               uint32_t argc,
               void *argv,
               be_val<uint32_t> *stack,
               uint32_t stackSize,
               int32_t priority,
               OSThreadAttributes attributes);

void
OSDetachThread(OSThread *thread);

void
OSExitThread(int value);

void
OSGetActiveThreadLink(OSThread *thread,
                      OSThreadLink *link);

OSThread *
OSGetCurrentThread();

OSThread *
OSGetDefaultThread(uint32_t coreID);

uint32_t
OSGetStackPointer();

uint32_t
OSGetThreadAffinity(OSThread *thread);

const char *
OSGetThreadName(OSThread *thread);

uint32_t
OSGetThreadPriority(OSThread *thread);

uint32_t
OSGetThreadSpecific(uint32_t id);

void
OSInitThreadQueue(OSThreadQueue *queue);

void
OSInitThreadQueueEx(OSThreadQueue *queue,
                    void *parent);

BOOL
OSIsThreadSuspended(OSThread *thread);

BOOL
OSIsThreadTerminated(OSThread *thread);

BOOL
OSJoinThread(OSThread *thread,
             be_val<int> *exitValue);

void
OSPrintCurrentThreadState();

int32_t
OSResumeThread(OSThread *thread);

BOOL
OSRunThread(OSThread *thread,
            OSThreadEntryPointFn entry,
            uint32_t argc,
            void *argv);

BOOL
OSSetThreadAffinity(OSThread *thread,
                    uint32_t affinity);

BOOL
OSSetThreadCancelState(BOOL state);

OSThreadCleanupCallbackFn
OSSetThreadCleanupCallback(OSThread *thread,
                           OSThreadCleanupCallbackFn callback);
OSThreadDeallocatorFn
OSSetThreadDeallocator(OSThread *thread,
                       OSThreadDeallocatorFn deallocator);

void
OSSetThreadName(OSThread* thread,
                const char *name);

BOOL
OSSetThreadPriority(OSThread* thread,
                    uint32_t priority);

BOOL
OSSetThreadRunQuantum(OSThread* thread,
                      uint32_t quantum);

void
OSSetThreadSpecific(uint32_t id,
                    uint32_t value);

BOOL
OSSetThreadStackUsage(OSThread *thread);

void
OSSleepThread(OSThreadQueue *queue);

void
OSSleepTicks(OSTime ticks);

uint32_t
OSSuspendThread(OSThread *thread);

void
OSTestThreadCancel();

void
OSWakeupThread(OSThreadQueue *queue);

void
OSYieldThread();

void *
tls_get_addr(tls_index *index);

/** @} */

namespace internal
{

void
setDefaultThread(uint32_t core,
                 OSThread *thread);

bool threadSortFunc(OSThread *lhs,
                    OSThread *rhs);

using ThreadQueue = SortedQueue<OSThreadQueue, OSThreadLink, OSThread, &OSThread::link, threadSortFunc>;

} // namespace internal

} // namespace coreinit
