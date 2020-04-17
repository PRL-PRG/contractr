#ifndef CONTRACTR_TYPE_DECLARATION_CACHE_H
#define CONTRACTR_TYPE_DECLARATION_CACHE_H

#include "logger.hpp"
#undef length
#include <string>
#include <tastr/ast/ast.hpp>

extern "C" {

void initialize_type_declaration_cache();

SEXP clear_type_declaration_cache();

SEXP import_type_declarations(SEXP pkg_name);

SEXP get_typed_package_names();

SEXP get_typed_function_names(SEXP pkg_name);

SEXP is_package_typed(SEXP pkg_name);

SEXP is_function_typed(SEXP pkg_name, SEXP fun_name);

SEXP set_type_declaration(SEXP pkg_name, SEXP fun_name, SEXP type_decl);

SEXP remove_type_declaration(SEXP pkg_name, SEXP fun_name);

const tastr::ast::Node*
get_function_parameter_type(const std::string& package_name,
                            const std::string& function_name,
                            int formal_parameter_position);

const tastr::ast::Node*
get_function_return_type(const std::string& package_name,
                         const std::string& function_name);

const tastr::ast::FunctionTypeNode*
get_function_type(const std::string& package_name,
                  const std::string& function_name);

/*  this wrapper only exists because R provides a length macro which interferes
 * with C++ library length function leading to compilation issues if libraries
 * are not included in just the right way */
std::string type_to_string(const tastr::ast::Node& node);
}

#endif /* CONTRACTR_TYPE_DECLARATION_CACHE_H */
