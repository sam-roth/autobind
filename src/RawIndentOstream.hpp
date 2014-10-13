// Copyright (c) 2014, Samuel A. Roth. All rights reserved.
//
// Use of this source code is governed by a BSD-style license that can
// be found in the COPYING file.

#ifndef RAWINDENTOSTREAM_HPP_G873E2
#define RAWINDENTOSTREAM_HPP_G873E2


namespace autobind {

class RawIndentOstream: public llvm::raw_ostream
{
	llvm::raw_ostream &_sink;
	unsigned _indent = 0;
	unsigned _charsThisLine = 0;
public:
	RawIndentOstream(llvm::raw_ostream &sink)
	: _sink(sink) { }

	virtual ~RawIndentOstream() { }

private:
	virtual void write_impl(const char *ptr, size_t size);
};

} // autobind

#endif // RAWINDENTOSTREAM_HPP_G873E2
