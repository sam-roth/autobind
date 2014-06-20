#pragma once
#include <ostream>

// from http://stackoverflow.com/questions/9599807/how-to-add-indention-to-the-stream-operator
class IndentingOStreambuf : public std::streambuf
{
    std::streambuf*     myDest;
    bool                myIsAtStartOfLine;
    std::string         myIndent;
    std::ostream*       myOwner;
protected:
    virtual int         overflow( int ch )
    {
        if ( myIsAtStartOfLine && ch != '\n' ) {
            myDest->sputn( myIndent.data(), myIndent.size() );
        }
        myIsAtStartOfLine = ch == '\n';
        return myDest->sputc( ch );
    }
public:
    explicit            IndentingOStreambuf( 
                            std::streambuf* dest, int indent = 4 )
        : myDest( dest )
        , myIsAtStartOfLine( true )
        , myIndent( indent, ' ' )
        , myOwner( NULL )
    {
    }

	explicit IndentingOStreambuf(std::ostream &dest, const std::string &indent, bool atLineStart=true)
	: myDest(dest.rdbuf())
	, myIsAtStartOfLine(atLineStart)
	, myIndent(indent)
	, myOwner(&dest)
	{
		myOwner->rdbuf(this);
	}
	
    explicit            IndentingOStreambuf(
                            std::ostream& dest, int indent = 4 )
        : myDest( dest.rdbuf() )
        , myIsAtStartOfLine( true )
        , myIndent( indent, ' ' )
        , myOwner( &dest )
    {
        myOwner->rdbuf( this );
    }
    virtual             ~IndentingOStreambuf()
    {
        if ( myOwner != NULL ) {
            myOwner->rdbuf( myDest );
        }
    }
};


template <typename T>
struct IndentWrapper
{
	int indent;
	const T &object;
};

template <typename T>
IndentWrapper<T> indented(const T &what, int levels=4)
{
	return IndentWrapper<T>{levels, what};
}


template <typename T>
inline std::ostream &operator <<(std::ostream &os,
                                 const IndentWrapper<T> &w)
{
	IndentingOStreambuf buf(os, w.indent);
	os << w.object;

	return os;
}


template <typename T, typename U>
struct ConcatWrapper
{
	T first;
	U second;
};

template <typename T, typename U>
ConcatWrapper<T, U> concatted(T first,
                              U second)
{
	return ConcatWrapper<T, U>{first, second};
}


template <typename T, typename U>
inline std::ostream &operator<<(std::ostream &os,
                                const ConcatWrapper<T, U> &w)
{
	os << w.first;
	os << w.second;
	return os;
}


template <typename Delim, typename Coll>
struct JoinWrapper
{
	const Delim &delim;
	const Coll &collection;
};

template <typename Coll, typename Delim=const char *>
JoinWrapper<Delim, Coll> joined(const Coll &coll, const Delim &delim=", ")
{
	return JoinWrapper<Delim, Coll>{delim, coll};
}

template <typename Delim, typename Coll>
inline std::ostream &operator<<(std::ostream &os,
                                const JoinWrapper<Delim, Coll> &w)
{
	for(auto b = w.collection.begin(), e = w.collection.end(), it = b; it != e; ++it)
	{
		if(it != b)
		{
			os << w.delim;
		}

		os << *it;
	}

	return os;
}

