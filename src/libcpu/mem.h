#pragma once
#include "common/byte_swap.h"
#include "common/decaf_assert.h"
#include "common/types.h"
#include <cassert>

namespace mem
{

enum AddressSpace : ppcaddr_t
{
   SystemBase        = 0x01000000,
   SystemEnd         = 0x02000000,
   SystemSize        = SystemEnd - SystemBase,

   MEM2Base          = 0x02000000,
   MEM2End           = 0x42000000,
   MEM2Size          = MEM2End - MEM2Base,

   AperturesBase     = 0xB0000000,
   AperturesEnd      = 0xC0000000,
   AperturesSize     = AperturesEnd - AperturesBase,

   ForegroundBase    = 0xE0000000,
   ForegroundEnd     = 0xE4000000,
   ForegroundSize    = ForegroundEnd - ForegroundBase,

   MEM1Base          = 0xF4000000,
   MEM1End           = 0xF6000000,
   MEM1Size          = MEM1End - MEM1Base,

   LockedCacheBase   = 0xF6000000,
   LockedCacheEnd    = 0xF600C000,
   LockedCacheSize   = LockedCacheEnd - LockedCacheBase,

   SharedDataBase    = 0xF8000000,
   SharedDataEnd     = 0xFB000000,
   SharedDataSize    = SharedDataEnd - SharedDataBase,

   // This is decaf specific, we need to have a region large
   //  enough to load all of an applications symbols, which is
   //  potentially quite large (over 0x02000000 bytes).  We
   //  use this region, but keep in mind it is only allocated
   //  while the loader is actually running...
   LoaderBase        = 0xE6000000,
   LoaderEnd         = 0xEA000000,
   LoaderSize        = LoaderEnd - LoaderBase,
};

void
initialise();

size_t
base();

bool
valid(ppcaddr_t address);

bool
protect(ppcaddr_t address, size_t size);

// Translate WiiU virtual address to host address
template<typename Type = uint8_t>
inline Type *
translate(ppcaddr_t address)
{
   if (!address) {
      return nullptr;
   } else {
      return reinterpret_cast<Type*>(base() + address);
   }
}

// Translate host address to WiiU virtual address
inline ppcaddr_t
untranslate(const void *ptr)
{
   if (!ptr) {
      return 0;
   }

   auto sptr = reinterpret_cast<size_t>(ptr);
   auto sbase = base();
   decaf_check(sptr > sbase);
   decaf_check(sptr <= sbase + 0xFFFFFFFF);
   return static_cast<ppcaddr_t>(sptr - sbase);
}

// Read Type from virtual address with no endian byte_swap
template<typename Type>
inline Type
readNoSwap(ppcaddr_t address)
{
   return *reinterpret_cast<Type*>(translate(address));
}

// Read Type from virtual address
template<typename Type>
inline Type
read(ppcaddr_t address)
{
   return byte_swap(readNoSwap<Type>(address));
}

// Write Type to virtual address with no endian byte_swap
template<typename Type>
inline void
writeNoSwap(ppcaddr_t address, Type value)
{
   *reinterpret_cast<Type*>(translate(address)) = value;
}

// Write Type to virtual address
template<typename Type>
inline void
write(ppcaddr_t address, Type value)
{
   writeNoSwap(address, byte_swap(value));
}

} // namespace mem
