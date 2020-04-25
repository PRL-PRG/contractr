#ifndef CONTRACTR_CHECK_TYPE_HPP
#define CONTRACTR_CHECK_TYPE_HPP

#include <tastr/ast/ast.hpp>
#include <string>
#include <Rinternals.h>


bool check_type(const std::string& parameter_name,
                SEXP value,
                const tastr::ast::Node& node);

#endif /* CONTRACTR_CHECK_TYPE_HPP */
