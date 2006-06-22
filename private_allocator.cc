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
#include "zone.h"

namespace libcwd {
  namespace _private_ {

// Maximum overhead needed for non-internal allocations.
#if CWDEBUG_MAGIC
static size_t const malloc_overhead_c = sizeof(prezone) + sizeof(size_t) - 1 + sizeof(postzone) + CW_MALLOC_OVERHEAD;
#else
static size_t const malloc_overhead_c = CW_MALLOC_OVERHEAD;
#endif

// The minimum size we allocate: two or three page sizes minus the (maximum) malloc
// overhead (modulo the page size). Since this is rather large, we assume
// the same for internal allocations (which is only not true when configuring
// libcwd with --disable-magic).
static size_t const page_size_c = 4096;
#if defined(LIBCWD_REDZONE_BLOCKS) && LIBCWD_REDZONE_BLOCKS > 0
static size_t const block_size_c = 3 * page_size_c - (malloc_overhead_c % page_size_c);
#else
static size_t const block_size_c = 2 * page_size_c - malloc_overhead_c;
#endif

char* FreeList::allocate(int power, size_t size)
{
  BlockList& list(M_list_notfull[power - minimum_size_exp]);	// The linked list with Block's that aren't full yet.
  Node const* const end = list.end();				// The 'end' node of the list.
  BlockNode* block = list.begin();				// Make block point to begin.
  if (block == end)						// No non-empty blocks left?
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
    ++M_count[power - minimum_size_exp];
  }
  ChunkNode* chunk = block->M_chunks.begin();			// Get the first chunk of this (free) list.
  chunk->unlink();						// Unlink the chunk from the free list.
  ++block->M_chunks.M_used_count;				// Number of chunks in use in this block.
  if (block->M_chunks.empty())
  {
    // There are no more (free) chunks in this block, need to move
    // it from M_list_notfull to M_list_full.
    block->unlink();							// Remove it from M_list_notfull.
    M_list_full[power - minimum_size_exp].insert(block);		// Add it to M_list_full.
  }
  *reinterpret_cast<BlockNode**>(chunk) = block;
  return (char*)chunk + sizeof(BlockNode*);
}

void FreeList::deallocate(char* ptr, int power, size_t /*size*/)
{
  ptr -= sizeof(BlockNode*);
  BlockNode* block = *reinterpret_cast<BlockNode**>(ptr);
  Node* chunk = reinterpret_cast<Node*>(ptr);
  if (block->M_chunks.empty())
  {
    // Move the block back to M_list_notfull.
    block->unlink();							// Remove it from M_list_full.
    M_list_notfull[power - minimum_size_exp].insert_back(block);	// Add it to M_list_notfull.
  }
  block->M_chunks.insert(chunk);
  if (--block->M_chunks.M_used_count == 0 && M_count[power - minimum_size_exp] > M_keep[power - minimum_size_exp])
  {
    // ChunkList of block empty (and not the last block): remove it.
    block->unlink();
    ::operator delete(block);
    --M_count[power - minimum_size_exp];
  }
}

void BlockList::initialize(unsigned int* count_ptr, unsigned short internal)
{
  M_next = M_prev = this;
  M_count_ptr = count_ptr;
  M_internal = internal;
}

void BlockList::uninitialize(void)
{
#if CWDEBUG_DEBUG
  consistency_check();
#endif
  // No need for locking here-- this is only called when there is just one thread left.
  BlockList& list(*this);
  BlockNode* block = list.begin();		// There can be at most M_keep empty blocks.
  Node const* const end = list.end();		// The 'end' node of the list.
  // Search for first non-empty BlockNode.
  while (block != end && block->M_chunks.M_used_count == 0)
  {
    block->unlink();
    BlockNode* next = block->next();
    LIBCWD_TSD_DECLARATION;
    if (M_internal)
      set_alloc_checking_off(LIBCWD_TSD);
    ::operator delete(block);
    if (M_internal)
      set_alloc_checking_on(LIBCWD_TSD);
    --(*M_count_ptr);
    block = next;
  }
}

void FreeList::uninitialize(void)
{
#if CWDEBUG_DEBUG
  consistency_check();
#endif
  // Delete all empty blocks from now on.
  for (int i = 0; i < bucket_sizes; ++i)
    M_keep[i] = 0;
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
  {
    M_count[i] = 0;
    M_keep[i] = 1;
    M_list_notfull[i].initialize(&M_count[i], (__libcwd_tsd.internal > 0) ? 1 : 0);
    M_list_full[i].initialize(&M_count[i], (__libcwd_tsd.internal > 0) ? 1 : 0);
  }
  M_initialized = true;
}

#if CWDEBUG_DEBUG
void FreeList::consistency_check(void)
{
  assert(M_initialized);
  // M_mutex is either already locked when we get here, or there is just one thread.
  for (int i = 0; i < bucket_sizes; ++i)
  {
    assert(M_keep[i] == 1);
    assert(M_list_notfull[i].M_count_ptr == &M_count[i]);
    assert(M_list_full[i].M_count_ptr == &M_count[i]);
    M_list_notfull[i].consistency_check();
    M_list_full[i].consistency_check();
    unsigned int count = 0;
    for (Node* iter = M_list_notfull[i].begin(); iter != M_list_notfull[i].end(); iter = iter->M_next)
      ++count;
    for (Node* iter = M_list_full[i].begin(); iter != M_list_full[i].end(); iter = iter->M_next)
      ++count;
    assert(count == M_count[i]);
  }
}

void BlockList::consistency_check(void)
{
  assert(begin() == M_next);
  assert(end() == this);
  Node* prev_node = this;
  Node* node = begin();
  int count = 0;
  do
  {
    assert(node->M_prev == prev_node);
    prev_node = node;
    node = node->M_next;
    ++count;
  }
  while (node != M_next && count != 123456);
  assert(count != 123456);
}
#endif

  } // namespace _private_
} // namespace libcwd

#endif // __GNUC__ == 4

