// Copyright (c) 2014, Samuel A. Roth. All rights reserved.
//
// Use of this source code is governed by a BSD-style license that can
// be found in the COPYING file.

#include "RawIndentOstream.hpp"

namespace autobind {

void RawIndentOstream::write_impl(const char *ptr, size_t size)
{



	const char *const end = ptr + size;

	while(ptr != end)
	{
		if(_charsThisLine == 0 && size > 0)
		{
			_sink.indent(_indent);
		}


		auto lineEnd = std::find(ptr, end, '\n');

		if(lineEnd != end && *lineEnd == '\n')
		{
			++lineEnd;
			_charsThisLine = 0;
		}
		else
		{
			_charsThisLine += (lineEnd - ptr);
		}

		_sink.write(ptr, lineEnd - ptr);

		ptr = lineEnd;
	}
}


} // autobind

