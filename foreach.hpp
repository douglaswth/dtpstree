// Foreach for STL
//
// Douglas Thrift
//
// $Id: foreach.hpp 1263 2010-03-21 13:30:15Z douglas $

/* Menes - C++ High-Level Utility Library
 * Copyright (C) 2004  Jay Freeman (saurik)
*/

/*
 *        Redistribution and use in source and binary
 * forms, with or without modification, are permitted
 * provided that the following conditions are met:
 * 
 * 1. Redistributions of source code must retain the
 *    above copyright notice, this list of conditions
 *    and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the
 *    above copyright notice, this list of conditions
 *    and the following disclaimer in the documentation
 *    and/or other materials provided with the
 *    distribution.
 * 3. The name of the author may not be used to endorse
 *    or promote products derived from this software
 *    without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING,
 * BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
 * TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef _foreach_hpp_
#define _foreach_hpp_

#define _forever for (;;)

#define _forall(type, item, begin, end) \
	if (size_t _index = 0); \
	else for (type item(begin), _end(end); item != _end; ++item, ++_index)

#define _rforall(type, item, begin, end) \
	for (type item(end), _begin(begin); item != _begin && (--item, true); )

#define _repeat(count) \
	for (unsigned _index(0), _end(count); _index != _end; ++_index)

template <typename List_, bool noop = true>
struct StrictIterator;

template <typename List_>
struct StrictIterator<List_, true> {
	typedef typename List_::iterator Result;
};

template <typename List_>
struct StrictIterator<const List_, true> {
	typedef typename List_::const_iterator Result;
};

template <typename List_, bool noop = true>
struct ListTraits;

template <typename List_>
struct ListTraits<List_, true>
{
	typedef typename StrictIterator<List_>::Result Iterator;

	static inline Iterator Begin(List_ &arg) {
		return arg.begin();
	}

	static inline Iterator End(List_ &arg) {
		return arg.end();
	}
};

#define _foreach_(type, item, set, forall, _typename) \
	for (bool _stop(true); _stop; ) \
		for (type &_set = set; _stop; _stop = false) \
			forall (_typename ListTraits< type >::Iterator, item, ListTraits< type >::Begin(_set), ListTraits< type >::End(_set))

#define _foreach(type, item, set) \
	_foreach_(type, item, set, _forall, )

#define _rforeach(type, item, set) \
	_foreach_(type, item, set, _rforall, )

#define _tforeach(type, item, set) \
	_foreach_(type, item, set, _forall, typename)

#define _rtforeach(type, item, set) \
	_foreach_(type, item, set, _rforall, typename)

#endif//_foreach_hpp_
