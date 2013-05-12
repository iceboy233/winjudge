#ifndef _ONULLSTREAM_HPP
#define _ONULLSTREAM_HPP

#include <streambuf>
#include <ostream>

namespace judge {

template <typename cT, typename traits = std::char_traits<cT> >
class basic_nullbuf: public std::basic_streambuf<cT, traits> {
	typename traits::int_type overflow(typename traits::int_type c)
	{
		return traits::not_eof(c); // indicate success
	}
};

template <typename cT, typename traits = std::char_traits<cT> >
class basic_onullstream: public std::basic_ostream<cT, traits> {
public:
	basic_onullstream()
		: std::basic_ios<cT, traits>(&m_sbuf)
		, std::basic_ostream<cT, traits>(&m_sbuf)
	{
		init(&m_sbuf);
	}

private:
	basic_nullbuf<cT, traits> m_sbuf;
};

typedef basic_onullstream<char> onullstream;
typedef basic_onullstream<wchar_t> wonullstream;

}

#endif
