
#ifndef DISCOVERYVISITOR_HPP_25YR9D
#define DISCOVERYVISITOR_HPP_25YR9D

#include "prefix.hpp"
#include "Export.hpp"
#include "ModuleManager.hpp"

namespace autobind {



void discoverTranslationUnit(autobind::ModuleManager &mgr,
                             clang::TranslationUnitDecl &tu);



} // autobind


#endif // DISCOVERYVISITOR_HPP_25YR9D

