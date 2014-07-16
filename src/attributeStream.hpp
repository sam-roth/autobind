// Copyright (c) 2014, Samuel A. Roth. All rights reserved.
//
// Use of this source code is governed by a BSD-style license that can
// be found in the COPYING file.

#ifndef ATTRIBUTESTREAM_HPP_2G0O0K
#define ATTRIBUTESTREAM_HPP_2G0O0K

#include "util.hpp"
#include "stream.hpp"
#include <clang/AST/Decl.h>
#include <clang/AST/Attr.h>

namespace autobind {


AB_RETURN_AUTO(inline attributeStream(clang::Decl &x),
               streams::stream(x.specific_attr_begin<clang::AnnotateAttr>(),
                               x.specific_attr_end<clang::AnnotateAttr>()))


inline bool isPyExport(clang::Decl *d)
{
	using namespace streams;

	auto pred = [](const clang::AnnotateAttr *a) { return a->getAnnotation() == "pyexport"; };
	return any(attributeStream(*d) | transformed(pred));
}

inline bool isPyNoExport(clang::Decl *d)
{
	using namespace streams;

	auto pred = [](const clang::AnnotateAttr *a) { return a->getAnnotation() == "pynoexport"; };
	return any(attributeStream(*d) | transformed(pred));
}

} // autobind


#endif // ATTRIBUTESTREAM_HPP_2G0O0K

