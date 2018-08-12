/**
 * @file cwlist.h
 * @brief A linked list with iterators that advance when their element is deleted.
 *
 * Copyright (c) 2013, Carlo Wood.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution.
 *
 * CHANGELOG
 *   and additional copyright holders.
 *
 *   05/02/2013
 *   Initial version, written by Carlo Wood, Aleric @ FreeNode.
 */

#ifndef CWLIST_H
#define CWLIST_H

#include <list>
#include <execinfo.h>

template<typename T, typename _Alloc>
class CWList;

template<typename T, typename _Alloc>
class CWConstListIterator;

/*
 * CWNode<T>
 *
 * The actual data stored in a std::list when using CWList<T, _Alloc>.
 * T is the template parameter used with CWList.
 * count are the number of iterators that currently point to this element.
 * dead is 0 or 1, where 1 means that the element was erased from the CWList
 * (but not yet from the internal std::list, so that any iterators to it
 * are NOT invalidated).
 */
template<typename T>
struct CWNode {
  T mElement;						// The user visual data.
  mutable unsigned short count;		// Number of iterators pointing to this element.
  mutable unsigned short dead;		// Whether or not the element is "erased".

  CWNode() : count(0), dead(0) { }
  explicit CWNode(T const& __val) : mElement(__val), count(0), dead(0) { }

  // Equivalence operators.
  // __node may not be dead. Dead nodes in the list are "skipped" (obviously), meaning that they are never equal or always unequal.
  bool operator==(CWNode const& __node) const { LIBCWD_ASSERT(!__node.dead); return mElement == __node.mElement && !dead; }
  bool operator!=(CWNode const& __node) const { LIBCWD_ASSERT(!__node.dead); return mElement != __node.mElement || dead; }

  // Default ordering for sort().
  bool operator<(CWNode const& __node) const { return mElement < __node.mElement; }
};

/*
 * CWListIterator<T, _Alloc>
 *
 * A non-const iterator to an element of CWList<T, _Alloc>.
 */
template<typename T, typename _Alloc>
class CWListIterator {
  private:
	typedef CWListIterator<T, _Alloc>			_Self;
	typedef std::list<CWNode<T>, _Alloc>			_Container;
	typedef typename _Container::iterator	_Iterator;

	_Container* mContainer;			// A pointer to the associated container, or NULL when (still) singular.
									// Note that this code does not allow a container to be destructed while
									// any non-end iterators still exist (although that could be legal).
									// If an iterator points to end() and the container is destructed then
									// this pointer is NOT reset to NULL.
	_Iterator mIterator;			// Internal iterator to element of mContainer (or singular when mContainer is NULL).
	int index;
	void* buffer[128000];

	// Increment reference counter for mIterator (if not singular and not end()).
	void ref()
	{
	  if (mContainer && mContainer->end() != mIterator)
	  {
		backtrace(&buffer[index], 32);
		index += 32;
		index %= 128000;
		// It would be bad a new iterator was created that pointed to a dead element.
		// Also, as a sanity check, make sure there aren't a ridiculous number of iterators
		// pointing to this element.
		LIBCWD_ASSERT(!mIterator->dead && mIterator->count < 100);
		++(mIterator->count);
	  }
	}

	// Decrement reference counter for mIterator (if not singular and not end()).
	// If this was the last iterator pointing to a dead element, then really erase it.
	void unref()
	{
	  if (mContainer && mContainer->end() != mIterator)
	  {
		backtrace(&buffer[index], 32);
		index += 32;
		index %= 128000;
		LIBCWD_ASSERT(mIterator->count > 0);
		if (--(mIterator->count) == 0 && mIterator->dead)
		{
		  mContainer->erase(mIterator);
		}
	  }
	}

  public:
	// Some standard typedefs that have to exist for iterators.
	typedef typename _Iterator::difference_type		difference_type;
	typedef typename _Iterator::iterator_category	iterator_category;
	typedef T										value_type;
	typedef T*										pointer;
	typedef T&										reference;

	// Construct a singular iterator.
	CWListIterator() : mContainer(NULL), index(0) { std::memset(buffer, 0, sizeof(buffer)); }

	// Construct an iterator to a given element of std::list. Only for internal use by CWList<T, _Alloc>.
	CWListIterator(_Container* __c, _Iterator const& __i) : mContainer(__c), mIterator(__i), index(0)
	{
	  std::memset(buffer, 0, sizeof(buffer));
	  LIBCWD_ASSERT(mContainer);
	  ref();
	}

	// Copy constructor.
	CWListIterator(CWListIterator const& __i) : mContainer(__i.mContainer), mIterator(__i.mIterator), index(0)
	{
	  std::memset(buffer, 0, sizeof(buffer));
	  ref();
	}

	// Destructor.
	~CWListIterator()
	{
	  unref();
	}

	// Assignment operator.
	_Self& operator=(_Self const& __x)
	{
	  unref();							// We no longer point to whatever we were pointing.
	  mContainer = __x.mContainer;
	  mIterator = __x.mIterator;
	  LIBCWD_ASSERT(mContainer);
	  ref();							// We now point an(other) element.
	  return *this;
	}

	// Dereference operator.
	reference operator*() const
	{
	  // Iterator may not be singular or dead.
	  LIBCWD_ASSERT(mContainer && !mIterator->dead);
	  return mIterator->mElement;
	}

	// Dereference operator.
	pointer operator->() const
	{
	  // Iterator may not be singular or dead.
	  LIBCWD_ASSERT(mContainer && !mIterator->dead);
	  return &mIterator->mElement;
	}

	// Pre-increment operator (not being singular is implied).
	_Self& operator++()
	{
	  LIBCWD_ASSERT(mContainer && mIterator != mContainer->end());
	  LIBCWD_ASSERT(mIterator->count > 0);				// This iterator is still pointing to it.
	  _Iterator cur = mIterator;					// Make copy of mIterator.
	  ++cur;										// Advance it.
	  unref();										// We will no longer be pointing to this element. This might invalidate mIterator!
	  while(cur != mContainer->end() && cur->dead)	// Advance till the first non-dead element.
	  {
		++cur;
	  }
	  mIterator = cur;								// Put result back into mIterator.
	  ref();										// We are now pointing to a valid element again.
	  return *this;
	}

	// Post-increment operator (not being singular is implied).
	_Self operator++(int)
	{
	  _Self tmp = *this;
	  this->operator++();
	  return tmp;
	}

	// Pre-decrement operator (not being singular is implied).
	_Self& operator--()
	{
	  LIBCWD_ASSERT(mContainer && mIterator != mContainer->begin());
	  LIBCWD_ASSERT(mIterator->count > 0);				// This iterator is still pointing to it.
	  _Iterator cur = mIterator;					// See operator++().
	  --cur;
	  unref();
	  while(cur->dead)
	  {
		--cur;
	  }
	  mIterator = cur;
	  ref();
	  return *this;
	}

	// Post-decrement operator (not being singular is implied).
	_Self operator--(int)
	{
	  _Self tmp = *this;
	  this->operator--();
	  return tmp;
	}

	// Equivalence operators.
	// We allow comparing with dead iterators, because if one of them is not-dead
	// then the result is "unequal" anyway, which is probably what you want.
	bool operator==(_Self const& __x) const { return mIterator == __x.mIterator; }
	bool operator!=(_Self const& __x) const { return mIterator != __x.mIterator; }

	friend class CWList<T, _Alloc>;
	friend class CWConstListIterator<T, _Alloc>;
	template<typename T2, typename A> friend bool operator==(CWListIterator<T2, A> const& __x, CWConstListIterator<T2, A> const& __y);
	template<typename T2, typename A> friend bool operator!=(CWListIterator<T2, A> const& __x, CWConstListIterator<T2, A> const& __y);

	// Return the total number of iterators pointing to the element that this iterator is pointing to.
	// Unless the iterator points to end, a positive number will be returned since this iterator is
	// pointing to the element. If it points to end (or is singular) then 0 is returned.
	int count() const
	{
	  if (mContainer && mIterator != mContainer->end())
	  {
		return mIterator->count;
	  }
	  return 0;
	}

	void release()
	{
	  // Silently destruct this operator (it was copied with memcpy).
	  // Make sure that destructing this object will not do anything to any underlaying CWNode.
	  mContainer = NULL;
#ifdef _GLIBCXX_DEBUG
	  // Also stop debug code from considering this a valid iterator. Setting everything to zero should make it singular.
	  memset(&mIterator, 0, sizeof(mIterator));
#endif
	}
};

/*
 * CWConstListIterator<T, _Alloc>
 *
 * A const iterator to an element of CWList<T, _Alloc>.
 *
 * Because this class is very simular to CWListIterator<T, _Alloc>, see above for detailed comments.
 */
template<typename T, typename _Alloc = std::allocator<T> >
class CWConstListIterator {
  private:
	typedef CWConstListIterator<T, _Alloc>				_Self;
	typedef std::list<CWNode<T>, _Alloc>				_Container;
	typedef typename _Container::iterator			_Iterator;
	typedef typename _Container::const_iterator		_ConstIterator;
	typedef CWListIterator<T, _Alloc>					iterator;

	_Container const* mContainer;
	_Iterator mConstIterator;		// This has to be an _Iterator instead of _ConstIterator, because the compiler doesn't accept a const_iterator for erase yet (C++11 does).

	void ref()
	{
	  if (mContainer && mContainer->end() != mConstIterator)
	  {
		LIBCWD_ASSERT(mConstIterator->count < 100);
		mConstIterator->count++;
	  }
	}

	void unref()
	{
	  if (mContainer && mContainer->end() != mConstIterator)
	  {
		LIBCWD_ASSERT(mConstIterator->count > 0);
		mConstIterator->count--;
		if (mConstIterator->count == 0 && mConstIterator->dead)
		{
		  const_cast<_Container*>(mContainer)->erase(mConstIterator);
		}
	  }
	}

  public:
	typedef typename _ConstIterator::difference_type	difference_type;
	typedef typename _ConstIterator::iterator_category	iterator_category;
	typedef T											value_type;
	typedef T const*									pointer;
	typedef T const&									reference;

	CWConstListIterator() : mContainer(NULL) { }
	CWConstListIterator(_Container const* __c, _Iterator const& __i) : mContainer(__c), mConstIterator(__i)
	{
	  LIBCWD_ASSERT(mContainer);
	  ref();
	}
	// Allow to construct a const_iterator from an iterator.
	CWConstListIterator(iterator const& __x) : mContainer(__x.mContainer), mConstIterator(__x.mIterator)
	{
	  ref();
	}
	CWConstListIterator(CWConstListIterator const& __i) : mContainer(__i.mContainer), mConstIterator(__i.mConstIterator)
	{
	  ref();
	}
	~CWConstListIterator()
	{
	  unref();
	}

	_Self& operator=(_Self const& __x)
	{
	  unref();
	  mContainer = __x.mContainer;
	  mConstIterator = __x.mConstIterator;
	  LIBCWD_ASSERT(mContainer);
	  ref();
	  return *this;
	}

	// Allow to assign from a non-const iterator.
	_Self& operator=(iterator const& __x)
	{
	  unref();
	  mContainer = __x.mContainer;
	  mConstIterator = __x.mIterator;
	  LIBCWD_ASSERT(mContainer);
	  ref();
	  return *this;
	}

	reference operator*() const
	{
	  LIBCWD_ASSERT(mContainer && !mConstIterator->dead);
	  return mConstIterator->mElement;
	}

	pointer operator->() const
	{
	  LIBCWD_ASSERT(mContainer && !mConstIterator->dead);
	  return &mConstIterator->mElement;
	}

	_Self& operator++()
	{
	  _Iterator cur = mConstIterator;
	  ++cur;
	  unref();
	  while(cur != mContainer->end() && cur->dead)
	  {
		++cur;
	  }
	  mConstIterator = cur;
	  ref();
	  return *this;
	}

	_Self operator++(int)
	{
	  _Self tmp = *this;
	  this->operator++();
	  return tmp;
	}

	_Self& operator--()
	{
	  _Iterator cur = mConstIterator;
	  --cur;
	  unref();
	  while(cur->dead)
	  {
		--cur;
	  }
	  mConstIterator = cur;
	  ref();
	  return *this;
	}

	_Self operator--(int)
	{
	  _Self tmp = *this;
	  this->operator--();
	  return tmp;
	}

	bool operator==(_Self const& __x) const { return mConstIterator == __x.mConstIterator; }
	bool operator!=(_Self const& __x) const { return mConstIterator != __x.mConstIterator; }
	bool operator==(iterator const& __x) const { return mConstIterator == __x.mIterator; }
	bool operator!=(iterator const& __x) const { return mConstIterator != __x.mIterator; }

	template<typename T2, typename A> friend bool operator==(CWListIterator<T2, A> const& __x, CWConstListIterator<T2, A> const& __y);
	template<typename T2, typename A> friend bool operator!=(CWListIterator<T2, A> const& __x, CWConstListIterator<T2, A> const& __y);

	int count() const
	{
	  if (mContainer && mConstIterator != mContainer->end())
	  {
		return mConstIterator->count;
	  }
	  return 0;
	}
};

template<typename T, typename _Alloc>
inline bool operator==(CWListIterator<T, _Alloc> const& __x, CWConstListIterator<T, _Alloc> const& __y)
{
  return __x.mIterator == __y.mConstIterator;
}

template<typename T, typename _Alloc>
inline bool operator!=(CWListIterator<T, _Alloc> const& __x, CWConstListIterator<T, _Alloc> const& __y)
{
  return __x.mIterator != __y.mConstIterator;
}

/*
 * CWList<T, _Alloc>
 *
 * A linked list that allows elements to be erased while one or more iterators
 * are still pointing to that element, after which pre-increment and pre-decrement
 * operators still work.
 *
 * For example:
 *
 * for (CWList<int>::iterator i = l.begin(); i != l.end(); ++i)
 * {
 *   int x = *i;
 *   f(i);				// Might erase any element of list l.
 *   // Should not dereference i anymore here (because it might be erased).
 * }
 */
template<typename T, typename _Alloc>
class CWList {
  private:
	typedef std::list<CWNode<T>, _Alloc>				_Container;
	typedef typename _Container::iterator			_Iterator;
	typedef typename _Container::const_iterator		_ConstIterator;

	_Container mContainer;
	size_t mSize;

  public:
	typedef T										value_type;
	typedef T*										pointer;
	typedef T const*								const_pointer;
	typedef T&										reference;
	typedef T const&								const_reference;
	typedef CWListIterator<T, _Alloc>					iterator;
	typedef CWConstListIterator<T, _Alloc>				const_iterator;
	typedef std::reverse_iterator<iterator>			reverse_iterator;
	typedef std::reverse_iterator<const_iterator>	const_reverse_iterator;
	typedef size_t									size_type;
	typedef ptrdiff_t								difference_type;

	// Default constructor. Create an empty list.
	CWList() : mSize(0) { }
#ifdef CWDEBUG_DEBUGM
	// Destructor calls clear() to check if there are no iterators left pointing to this list. Destructing an empty list is trivial.
	~CWList() { clear(); }
#endif

	// Construct a list with __n elements of __val.
	explicit CWList(size_type __n, value_type const& __val = value_type()) : mContainer(__n, CWNode<T>(__val)), mSize(__n) { }

	// Copy constructor.
	CWList(CWList const& __list) : mSize(0)
	{
	  for (_ConstIterator __i = __list.mContainer.begin(); __i != __list.mContainer.end(); ++__i)
	  {
		if (!__i->dead)
		{
		  mContainer.push_back(CWNode<T>(__i->mElement));
		  ++mSize;
		}
	  }
	}

	// Construct a list from the range [__first, __last>.
	template<typename _InputIterator>
	  CWList(_InputIterator __first, _InputIterator __last) : mSize(0)
	{
	  for (_InputIterator __i = __first; __i != __last; ++__i)
	  {
		mContainer.push_back(CWNode<T>(*__i));
		++mSize;
	  }
	}

	// Assign from another list.
	CWList& operator=(CWList const& __list)
	{
	  clear();
	  for (_ConstIterator __i = __list.mContainer.begin(); __i != __list.mContainer.end(); ++__i)
	  {
		if (!__i->dead)
		{
		  mContainer.push_back(CWNode<T>(__i->mElement));
		  ++mSize;
		}
	  }
	  return *this;
	}

	iterator begin()
	{
	  _Iterator __i = mContainer.begin();
	  while(__i != mContainer.end() && __i->dead)
	  {
		++__i;
	  }
	  return iterator(&mContainer, __i);
	}

	const_iterator begin() const
	{
	  _Iterator __i = const_cast<_Container&>(mContainer).begin();
	  while(__i != mContainer.end() && __i->dead)
	  {
		++__i;
	  }
	  return const_iterator(&mContainer, __i);
	}

	iterator end() { return iterator(&mContainer, mContainer.end()); }
	const_iterator end() const { return const_iterator(&mContainer, const_cast<_Container&>(mContainer).end()); }
	reverse_iterator rbegin() { return reverse_iterator(end()); }
	const_reverse_iterator rbegin() const { return const_reverse_iterator(end()); }
	reverse_iterator rend() { return reverse_iterator(begin()); }
	const_reverse_iterator rend() const { return const_reverse_iterator(begin()); }

	bool empty() const { return mSize == 0; }
	size_type size() const { return mSize; }
	size_type max_size() const { return mContainer.max_size(); }

	reference front() { iterator __i = begin(); return *__i; }
	const_reference front() const { const_iterator __i = begin(); return *__i; }
	reference back() { iterator __i = end(); --__i; return *__i; }
	const_reference back() const { const_iterator __i = end(); --__i; return *__i; }

	void push_front(value_type const& __x)
	{
	  mContainer.push_front(CWNode<T>(__x));
	  ++mSize;
	}
	void pop_front()
	{
	  iterator __i = begin();
	  erase(__i);
	}
	void push_back(value_type const& __x)
	{
	  mContainer.push_back(CWNode<T>(__x));
	  ++mSize;
	}
	void pop_back()
	{
	  iterator __i = end();
	  --__i;
	  erase(__i);
	}

	iterator insert(iterator __position, value_type const& __x)
	{
	  assert(__position.mContainer == &mContainer && (__position.mIterator == mContainer.end() || !__position.mIterator->dead));
	  ++mSize;
	  return iterator(&mContainer, mContainer.insert(__position.mIterator, CWNode<T>(__x)));
	}

	void clear()
	{
#ifdef CWDEBUG_DEBUGM
	  // There should be no iterators left pointing at any element here.
	  for (_Iterator __i = mContainer.begin(); __i != mContainer.end(); ++__i)
	  {
		LIBCWD_ASSERT(__i->count == 0);
	  }
#endif
	  mContainer.clear();
	  mSize = 0;
	}

	void erase(iterator __position)
	{
	  // Mark the element __position points to as being erased.
	  // Iterator may not be singular, point to end, or be dead already.
	  // Obviously count must be larger than zero since __position is still pointing to it.
	  LIBCWD_ASSERT(__position.mContainer == &mContainer && __position.mIterator != mContainer.end() && __position.mIterator->count > 0 && !__position.mIterator->dead);
	  __position.mIterator->dead = 1;
	  --mSize;
	}

	// Remove all elements, designated by the iterator where, for which *where == __val.
	void remove(value_type const& __val)
	{
	  _Iterator const __e = mContainer.end();
	  for (_Iterator __i = mContainer.begin(); __i != __e;)
	  {
		if (!__i->dead && __i->mElement == __val)
		{
		  --mSize;
		  if (__i->count == 0)
		  {
			mContainer.erase(__i++);
			continue;
		  }
		  // Mark the element as being erased.
		  __i->dead = 1;
		}
		++__i;
	  }
	}

	void sort()
	{
#ifdef CWDEBUG_DEBUGM
	  // There should be no iterators left pointing at any element here.
	  for (_Iterator __i = mContainer.begin(); __i != mContainer.end(); ++__i)
	  {
		LIBCWD_ASSERT(__i->count == 0);
	  }
#endif
	  mContainer.sort();
	}
	template<typename _StrictWeakOrdering>
	  struct PredWrapper
	  {
		_StrictWeakOrdering mPred;
		PredWrapper(_StrictWeakOrdering const& pred) : mPred(pred) { }
		bool operator()(CWNode<T> const& __x, CWNode<T> const& __y) const { return mPred(__x.mElement, __y.mElement); }
	  };
	template<typename _StrictWeakOrdering>
	  void sort(_StrictWeakOrdering const& pred)
	{
#ifdef CWDEBUG_DEBUGM
	  // There should be no iterators left pointing at any element here.
	  for (_Iterator __i = mContainer.begin(); __i != mContainer.end(); ++__i)
	  {
		LIBCWD_ASSERT(__i->count == 0);
	  }
#endif
	  mContainer.sort(PredWrapper<_StrictWeakOrdering>(pred));
	}
};

#endif
