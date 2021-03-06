#include "coreinit.h"
#include "coreinit_semaphore.h"
#include "coreinit_scheduler.h"
#include "common/decaf_assert.h"

namespace coreinit
{

const uint32_t
OSSemaphore::Tag;


/**
 * Initialise semaphore object with count.
 */
void
OSInitSemaphore(OSSemaphore *semaphore, int32_t count)
{
   OSInitSemaphoreEx(semaphore, count, nullptr);
}


/**
 * Initialise semaphore object with count and name.
 */
void
OSInitSemaphoreEx(OSSemaphore *semaphore, int32_t count, char *name)
{
   semaphore->tag = OSSemaphore::Tag;
   semaphore->name = name;
   semaphore->count = count;
   OSInitThreadQueueEx(&semaphore->queue, semaphore);
}


/**
 * Decrease the semaphore value.
 *
 * If the value is less than or equal to zero the current thread will be put to
 * sleep until the count is above zero and it can decrement it safely.
 */
int32_t
OSWaitSemaphore(OSSemaphore *semaphore)
{
   int32_t previous;
   internal::lockScheduler();
   decaf_check(semaphore && semaphore->tag == OSSemaphore::Tag);

   while (semaphore->count <= 0) {
      // Wait until we can decrease semaphore
      internal::sleepThreadNoLock(&semaphore->queue);
      internal::rescheduleSelfNoLock();
   }

   previous = semaphore->count--;
   internal::unlockScheduler();
   return previous;
}


/**
 * Try to decrease the semaphore value.
 *
 * If the value is greater than zero then it will be decremented, else the function
 * will return immediately with a value <= 0 indicating a failure.
 *
 * \return Returns previous semaphore count, before the decrement in this function.
 *         If the value is >0 then it means the call was succesful.
 */
int32_t
OSTryWaitSemaphore(OSSemaphore *semaphore)
{
   int32_t previous;
   internal::lockScheduler();
   decaf_check(semaphore && semaphore->tag == OSSemaphore::Tag);

   // Try to decrease semaphore
   previous = semaphore->count;

   if (semaphore->count > 0) {
      semaphore->count--;
   }

   internal::unlockScheduler();
   return previous;
}


/**
 * Increase the semaphore value.
 *
 * If any threads are waiting for semaphore, they are woken.
 */
int32_t
OSSignalSemaphore(OSSemaphore *semaphore)
{
   int32_t previous;
   internal::lockScheduler();
   decaf_check(semaphore && semaphore->tag == OSSemaphore::Tag);

   // Increase semaphore
   previous =  semaphore->count++;

   // Wakeup any waiting threads
   internal::wakeupThreadNoLock(&semaphore->queue);
   internal::rescheduleAllCoreNoLock();

   internal::unlockScheduler();
   return previous;
}


/**
 * Get the current semaphore count.
 */
int32_t
OSGetSemaphoreCount(OSSemaphore *semaphore)
{
   int32_t count;
   internal::lockScheduler();
   decaf_check(semaphore && semaphore->tag == OSSemaphore::Tag);

   // Return count
   count = semaphore->count;

   internal::unlockScheduler();
   return count;
}

void
Module::registerSemaphoreFunctions()
{
   RegisterKernelFunction(OSInitSemaphore);
   RegisterKernelFunction(OSInitSemaphoreEx);
   RegisterKernelFunction(OSWaitSemaphore);
   RegisterKernelFunction(OSTryWaitSemaphore);
   RegisterKernelFunction(OSSignalSemaphore);
   RegisterKernelFunction(OSGetSemaphoreCount);
}

} // namespace coreinit
