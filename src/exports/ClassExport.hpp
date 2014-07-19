// Copyright (c) 2014, Samuel A. Roth. All rights reserved.
//
// Use of this source code is governed by a BSD-style license that can
// be found in the COPYING file.

#ifndef CLASSEXPORT_HPP_8U83ZH
#define CLASSEXPORT_HPP_8U83ZH

#include "../Export.hpp"

namespace autobind {

class ClassData;

class ClassExport: public virtual Export
{
	const ClassData &_classData;
public:
	ClassExport(std::string name,
	            const ClassData &classData)
	: Export(name)
	, _classData(classData)
	{ }

	const ClassData &classData() const { return _classData; }

	virtual void codegenGetSet(std::ostream &) const { }

	virtual ~ClassExport() { }
};


} // autobind


#endif // CLASSEXPORT_HPP_8U83ZH



