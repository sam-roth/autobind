#ifndef PREFIX_HPP_MW5CAJ
#define PREFIX_HPP_MW5CAJ

// Standard Library
#include <array>
#include <iostream>
#include <memory>
#include <ostream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>


// Boost
#include <boost/format.hpp>

// LLVM
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/YAMLTraits.h>

// Clang
#include <clang/AST/ASTConsumer.h>
#include <clang/AST/ASTContext.h>
#include <clang/AST/Attr.h>
#include <clang/AST/AttrIterator.h>
#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/Frontend/ChainedDiagnosticConsumer.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Frontend/FrontendActions.h>
#include <clang/Lex/LiteralSupport.h>
#include <clang/Lex/Pragma.h>
#include <clang/Lex/Preprocessor.h>
#include <clang/Tooling/CommonOptionsParser.h>
#include <clang/Tooling/Tooling.h>
#include <clang/Sema/Sema.h>
#include <clang/Sema/TemplateDeduction.h>

// Internal
#include "stream.hpp"
#include "printing.hpp"


#endif // PREFIX_HPP_MW5CAJ
