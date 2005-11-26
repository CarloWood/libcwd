// $Header$
//
// \file private_allocator.cc
// This file contains the implementation of our own STL allocator, used with g++-4.x.
//
// Copyright (C) 2005, by
// 
// Carlo Wood, Run on IRC <carlo@alinoe.com>
// RSA-1024 0x624ACAD5 1997-01-26                    Sign & Encrypt
// Fingerprint16 = 32 EC A7 B6 AC DB 65 A6  F6 F6 55 DD 1C DC FF 61
//
// This file may be distributed under the terms of the Q Public License
// version 1.0 as appearing in the file LICENSE.QPL included in the
// packaging of this file.
//

#include "sys.h"

#if __GNUC__ == 4

#include <libcwd/private_allocator.h>
#include <libcwd/private_set_alloc_checking.h>

namespace libcwd {
  namespace _private_ {

// Maximum overhead needed for non-internal allocations.
#ifdef LIBCWD_NEED_WORD_ALIGNMENT
static size_t const malloc_overhead_c = 16 + LIBCWD_MALLOC_OVERHEAD;
#else
static size_t const malloc_overhead_c = 12 + LIBCWD_MALLOC_OVERHEAD;
#endif
// The minimum size we allocate: two page sizes minus the (maximum) malloc overhead.
// Since this is rather large, we assume the same for internal allocation (which is only
// not true when configuring libcwd with --disable-magic.
static size_t const block_size_c = 8192 - malloc_overhead_c;

char* FreeList::allocate(int power, size_t size)
{
  BlockList& list(M_list[power - minimum_size_exp]);	// The root of the linked list.
  BlockNode* const begin = list.begin();		// Make block point to begin.
  Node const* const end = list.end();			// The 'end' node of the list.
  // Search for first non-empty BlockNode.
  BlockNode* block = begin;
  while (block != end && block->M_chunks.empty())
    block = block->next();
  if (block == end)
  {
    // Allocate a new BlockNode.
    block = reinterpret_cast<BlockNode*>(::operator new(block_size_c));
    // Partition BlockNode.
    Node* first_chunk = block->M_data;		// First chunk.
    block->M_chunks.M_next = first_chunk;	// Make root point to the first chunk.
    first_chunk->M_prev = &block->M_chunks;	// Make first chunk point back to the root.
    int const offset_of_M_data = (char*)first_chunk - (char*)block;
    int const number_of_chunks = (block_size_c - offset_of_M_data) / size;
    Node* chunk = first_chunk;
    for (int i = 1; i < number_of_chunks; ++i)
    {
      Node* prev_chunk = chunk;
      chunk = reinterpret_cast<Node*>((char*)chunk + size);	// Next chunk.
      chunk->M_prev = prev_chunk;
      prev_chunk->M_next = chunk;
    }
    // Link root and last chunk.
    block->M_chunks.M_prev = chunk;
    chunk->M_next = &block->M_chunks;
    // The number of allocated chunks.
    block->M_chunks.M_used_count = 0;

    // Insert block at the front of the chain.
    list.insert(block);
    ++list.M_count;
  }
  else if (block != begin)
  {
    // In order to speed up subsequential allocations, put full blocks at the end of the list.
    list.M_prev->M_next = begin;
    begin->M_prev = list.M_prev;
    block->M_prev->M_next = &list;
    list.M_prev = block->M_prev;
    list.M_next = block;
    block->M_prev = &list;
  }
  ChunkNode* chunk = block->M_chunks.begin();
  chunk->unlink();
  ++block->M_chunks.M_used_count;
  return (char*)chunk;
}

void FreeList::deallocate(char* ptr, int power, size_t /*size*/)
{
  Node* chunk = reinterpret_cast<Node*>(ptr);
  BlockList& list(M_list[power - minimum_size_exp]);		// The root of the linked list.
  BlockNode* block = list.begin();				// Make block point to begin.
  // Find the block that the chunk belongs to.
#if 0
  while (ptr < (char*)block || ptr > (char*)block + block_size_c)
    block = block->next();
#else
  // Faster code(?) ... attempt to short circuit the boolean expression
  // by assuming that the blocks are reasonably ordered in memory.
  // Also unroll the loops a factor of two.
  if (ptr > (char*)block + block_size_c)
  {
    do
    {
      BlockNode* block2 = (BlockNode*)block->M_next;
      if (__builtin_expect(ptr <= (char*)block2 + block_size_c && ptr >= (char*)block2, false))
      {
        block = block2;
	break;
      }
      block = (BlockNode*)block2->M_next;
    }
    while (__builtin_expect(ptr > (char*)block + block_size_c || ptr < (char*)block, true));
  }
  else if (ptr < (char*)block)
  {
    do
    {
      BlockNode* block2 = (BlockNode*)block->M_next;
      if (__builtin_expect(ptr >= (char*)block2 && ptr <= (char*)block2 + block_size_c, false))
      {
        block = block2;
	break;
      }
      block = (BlockNode*)block2->M_next;
    }
    while (__builtin_expect(ptr < (char*)block || ptr > (char*)block + block_size_c, true));
  }
#endif
  block->M_chunks.insert(chunk);
  if (--block->M_chunks.M_used_count == 0 && list.M_count > list.M_keep)
  {
    // ChunkList of block empty (and not the last block): remove it.
    block->unlink();
    ::operator delete(block);
    --list.M_count;
  }
}

void BlockList::initialize(unsigned short internal)
{
  M_next = M_prev = this;
  M_count = 0;
  M_keep = 1;
  M_internal = internal;
}

void BlockList::uninitialize(void)
{
  // No need for locking here-- this is only called when there is just one thread left.
  BlockList& list(*this);
  BlockNode* block = list.begin();		// There can be at most M_keep empty blocks.
  Node const* const end = list.end();		// The 'end' node of the list.
  // Search for first non-empty BlockNode.
  while (block != end && block->M_chunks.M_used_count == 0)
  {
    block->unlink();
    LIBCWD_TSD_DECLARATION;
    if (M_internal)
      set_alloc_checking_off(LIBCWD_TSD);
    ::operator delete(block);
    if (M_internal)
      set_alloc_checking_on(LIBCWD_TSD);
    --list.M_count;
    block = block->next();
  }
  // Delete all empty blocks from now on.
  M_keep = 0;
}

void FreeList::initialize(LIBCWD_TSD_PARAM)
{
#if LIBCWD_THREAD_SAFE
  // FIXME: should be only done once.
  pthread_mutexattr_t mutex_attr;
#if CWDEBUG_DEBUGT
  pthread_mutexattr_settype(&mutex_attr, PTHREAD_MUTEX_ERRORCHECK);
#else
  pthread_mutexattr_settype(&mutex_attr, PTHREAD_MUTEX_NORMAL);
#endif
  pthread_mutex_init(&M_mutex, &mutex_attr);
#endif
  for (int i = 0; i < bucket_sizes; ++i)
    M_list[i].initialize((__libcwd_tsd.internal > 0) ? 1 : 0);
  M_initialized = true;
}

  } // namespace _private_
} // namespace libcwd

#endif // __GNUC__ == 4

