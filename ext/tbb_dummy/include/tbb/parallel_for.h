#pragma once

// Dummy implementation of tbb's parallel_for for function:
// Simgle threaded, range used only for boundaries
// (Tekari only uses a small subset of tbb's feature, so no need to keep original requirements)

namespace tbb
{
	// dummy (trivial) implementation of blocked_range
	// for webassembly compilation
	template<typename Value>
	class blocked_range {
	public:
	    //! Type of a value
	    /** Called a const_iterator for sake of algorithms that need to treat a blocked_range
	        as an STL container. */
	    typedef Value const_iterator;

	    //! Type for size of a range
	    typedef std::size_t size_type;

	    //! Construct range with default-constructed values for begin and end.
	    /** Requires that Value have a default constructor. */
	    blocked_range() : my_end(), my_begin() {}

	    //! Construct range over half-open interval [begin,end), with the given grainsize.
	    blocked_range( Value begin_, Value end_, size_type grainsize_=1 ) :
	        my_end(end_), my_begin(begin_), my_grainsize(grainsize_)
	    {
	        assert( my_grainsize>0 );
	        assert( !(end()<begin()) );
	    }

	    //! Beginning of range.
	    const_iterator begin() const {return my_begin;}

	    //! One past last value in range.
	    const_iterator end() const {return my_end;}

	    //! Size of the range
	    /** Unspecified if end()<begin(). */
	    size_type size() const {
	        return size_type(my_end-my_begin);
	    }

	    //! The grain size for this range.
	    size_type grainsize() const {return my_grainsize;}

	private:
	    Value my_end;
	    Value my_begin;
	    size_type my_grainsize;

	};

	// Single threaded implementation of "parallel" for (just call the body with the given range)
	template<typename Range, typename Body>
	void parallel_for( const Range& range, const Body& body ) {
		body(range);
	}
}