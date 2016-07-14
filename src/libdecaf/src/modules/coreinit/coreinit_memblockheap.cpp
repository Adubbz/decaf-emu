#include "coreinit.h"
#include "coreinit_memheap.h"
#include "coreinit_spinlock.h"
#include "common/align.h"
#include "common/decaf_assert.h"
#include "common/structsize.h"
#include <array>

namespace coreinit
{

struct MEMBlockHeapBlock;

struct MEMBlockHeapTracking
{
   // 0x00 = 0
   // 0x04 = 0
   UNKNOWN(0x8);
   be_ptr<MEMBlockHeapBlock> blocks;
   be_val<uint32_t> blockCount;
};
CHECK_OFFSET(MEMBlockHeapTracking, 0x08, blocks);
CHECK_OFFSET(MEMBlockHeapTracking, 0x0C, blockCount);
CHECK_SIZE(MEMBlockHeapTracking, 0x10);

struct MEMBlockHeapBlock
{
   //! First address of the data region this block has allocated
   be_val<uint32_t> start;

   //! End address of the data region this block has allocated
   be_val<uint32_t> end;

   //! TRUE if the block is free, FALSE if allocated
   be_val<BOOL> isFree;

   //! Link to previous block, note that this is only set for allocated blocks
   be_ptr<MEMBlockHeapBlock> prev;

   //! Link to next block, always set
   be_ptr<MEMBlockHeapBlock> next;
};
CHECK_OFFSET(MEMBlockHeapBlock, 0x00, start);
CHECK_OFFSET(MEMBlockHeapBlock, 0x04, end);
CHECK_OFFSET(MEMBlockHeapBlock, 0x08, isFree);
CHECK_OFFSET(MEMBlockHeapBlock, 0x0c, prev);
CHECK_OFFSET(MEMBlockHeapBlock, 0x10, next);
CHECK_SIZE(MEMBlockHeapBlock, 0x14);

struct MEMBlockHeap
{
   MEMHeapHeader header;
   UNKNOWN(0xC);

   MEMBlockHeapTracking defaultTrack;
   MEMBlockHeapBlock defaultBlock;

   be_ptr<MEMBlockHeapBlock> firstBlock;
   be_ptr<MEMBlockHeapBlock> lastBlock;
   be_ptr<MEMBlockHeapBlock> firstFreeBlock;
   be_val<uint32_t> numFreeBlocks;
};
CHECK_OFFSET(MEMBlockHeap, 0x00, header);
CHECK_OFFSET(MEMBlockHeap, 0x40, defaultTrack);
CHECK_OFFSET(MEMBlockHeap, 0x50, defaultBlock);
CHECK_OFFSET(MEMBlockHeap, 0x64, firstBlock);
CHECK_OFFSET(MEMBlockHeap, 0x68, lastBlock);
CHECK_OFFSET(MEMBlockHeap, 0x6C, firstFreeBlock);
CHECK_OFFSET(MEMBlockHeap, 0x70, numFreeBlocks);
CHECK_SIZE(MEMBlockHeap, 0x74);

int
MEMAddBlockHeapTracking(MEMBlockHeap *heap,
                        MEMBlockHeapTracking *tracking,
                        uint32_t size)
{
   auto blockCount = (size - sizeof(MEMBlockHeapTracking)) / sizeof(MEMBlockHeapBlock);
   auto blocks = reinterpret_cast<MEMBlockHeapBlock *>(tracking + 1);

   // Setup tracking data
   tracking->blockCount = blockCount;
   tracking->blocks = blocks;

   // Setup block linked list
   for (auto i = 0u; i < blockCount; ++i) {
      auto &block = blocks[i];
      block.prev = nullptr;
      block.next = &blocks[i + 1];
   }

   OSUninterruptibleSpinLock_Acquire(&heap->header.lock);

   // Insert at start of block list
   blocks[blockCount - 1].next = heap->firstFreeBlock;
   heap->firstFreeBlock = blocks;
   heap->numFreeBlocks += tracking->blocks;

   OSUninterruptibleSpinLock_Release(&heap->header.lock);
   return 0;
}

MEMBlockHeap *
MEMInitBlockHeap(MEMBlockHeap *heap,
                 void *start,
                 void *end,
                 MEMBlockHeapTracking *blocks,
                 uint32_t size,
                 uint32_t flags)
{
   auto dataStart = mem::untranslate(start);
   auto dataEnd = mem::untranslate(end);

   // Register heap
   internal::registerHeap(&heap->header, MEMHeapTag::BlockHeap, dataStart, dataEnd, flags);

   // Setup default tracker
   heap->defaultTrack.blockCount = 1;
   heap->defaultTrack.blocks = &heap->defaultBlock;

   // Setup default block
   heap->defaultBlock.start = dataStart;
   heap->defaultBlock.end = dataEnd;
   heap->defaultBlock.isFree = TRUE;
   heap->defaultBlock.next = nullptr;
   heap->defaultBlock.prev = nullptr;

   // Add default block to block list
   heap->firstBlock = &heap->defaultBlock;
   heap->lastBlock = &heap->defaultBlock;

   MEMAddBlockHeapTracking(heap, blocks, size);
   return heap;
}

MEMBlockHeapBlock *
findBlockOwning(MEMBlockHeap *heap,
                void *data)
{
   auto addr = mem::untranslate(data);

   if (addr < heap->header.dataStart) {
      return nullptr;
   }

   if (addr >= heap->header.dataEnd) {
      return nullptr;
   }

   auto distFromEnd = heap->header.dataEnd - addr;
   auto distFromStart = addr - heap->header.dataStart;

   if (distFromStart < distFromEnd) {
      // Look forward from firstBlock
      auto block = heap->firstBlock;

      while (block) {
         if (block->end > addr) {
            return block;
         }

         block = block->next;
      }
   } else {
      // Go backwards from lastBlock
      auto block = heap->lastBlock;

      while (block) {
         if (block->start <= addr) {
            return block;
         }

         block = block->prev;
      }
   }

   return nullptr;
}

enum MEMHeapFillType
{
   Unused,
   Allocated,
   Freed,
   Max
};

std::array<uint32_t, MEMHeapFillType::Max>
sHeapFillVals = {
   0xC3C3C3C3,
   0xF3F3F3F3,
   0xD3D3D3D3,
};

uint32_t
MEMGetFillValForHeap(MEMHeapFillType type)
{
   return sHeapFillVals[type];
}

void
MEMSetFillValForHeap(MEMHeapFillType type, uint32_t value)
{
   sHeapFillVals[type] = value;
}

enum MEMHeapFlags
{
   ZeroAllocated = 1 << 0,
   DebugMode = 1 << 1,
   UseLock = 1 << 2,
};

void *
MEMAllocFromBlockHeapEx(MEMBlockHeap *heap,
                        uint32_t size,
                        int32_t align)
{
   decaf_check(heap->header.tag == MEMHeapTag::BlockHeap);

   if (!size) {
      return nullptr;
   }

   if (heap->header.flags & MEMHeapFlags::UseLock) {
      OSUninterruptibleSpinLock_Acquire(&heap->header.lock);
   }

   /*
   CHECK_OFFSET(MEMBlockHeap, 0x64, firstBlock);
   CHECK_OFFSET(MEMBlockHeap, 0x68, lastBlock);
   */

   // Basically
   // find a block big enough for aligned size

   if (align >= 0) {
      for (auto block = heap->firstBlock; block; block = block->next) {
         if (block->isFree) {
            auto alignedStart = align_up(block->start, align);
            auto alignedEnd = alignedStart + size;

            if (alignedEnd < block->end) {
               // If it fits, i sits
            }
         }
      }
   } else {
      // Allocate from end
      for (auto block = heap->lastBlock; block; block = block->prev) {
         if (block->isFree) {
            auto alignedStart = align_down(block->end - size, align);
            auto alignedEnd = alignedStart + size;

            if (alignedStart >= block->start) {
               // If it fits, i sits
            }
         }
      }
   }

   if (heap->header.flags & MEMHeapFlags::UseLock) {
      OSUninterruptibleSpinLock_Release(&heap->header.lock);
   }

   return nullptr;
}

void
MEMFreeToBlockHeap(MEMBlockHeap *heap,
                   void *data)
{
   if (heap->header.flags & MEMHeapFlags::UseLock) {
      OSUninterruptibleSpinLock_Acquire(&heap->header.lock);
   }

   auto block = findBlockOwning(heap, data);

   if (!block) {
      gLog->warn("MEMFreeToBlockHeap: Could not find block containing data 0x{:08X}", mem::untranslate(data));
      goto out;
   }

   if (block->isFree) {
      gLog->warn("MEMFreeToBlockHeap: Tried to free an already free block");
      goto out;
   }

   if (block->start != data) {
      gLog->warn("MEMFreeToBlockHeap: Tried to free block 0x{:08X} from middle 0x{:08X}", block->start, mem::untranslate(data));
      goto out;
   }

   if (heap->header.flags & MEMHeapFlags::DebugMode) {
      auto fill = MEMGetFillValForHeap(MEMHeapFillType::Freed);
      auto size = block->end - block->start;
      std::memset(mem::translate(block->start), fill, size);
   }

   // Merge with previous free block if possible
   if (auto prev = block->prev) {
      if (prev->isFree) {
         prev->end = block->end;
         prev->next = block->next;

         if (auto next = prev->next) {
            next->prev = prev;
         } else {
            heap->lastBlock = prev;
         }

         block->prev = nullptr;
         block->next = heap->firstFreeBlock;
         heap->numFreeBlocks++;
         heap->firstFreeBlock = block;

         block = prev;
      }
   }

   block->isFree = TRUE;

   // Merge with next free block if possible
   if (auto next = block->next) {
      if (next->isFree) {
         block->end = next->end;
         block->next = next->next;

         if (next->next) {
            next->next->prev = block;
         } else {
            heap->lastBlock = block;
         }

         next->next = heap->firstFreeBlock;
         heap->firstFreeBlock = next;
         heap->numFreeBlocks++;
      }
   }

out:
   if (heap->header.flags & MEMHeapFlags::UseLock) {
      OSUninterruptibleSpinLock_Release(&heap->header.lock);
   }
}

} // coreinit
