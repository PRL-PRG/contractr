#ifndef CONTRACTR_TYPE_DECLARATION_CACHE_H
#define CONTRACTR_TYPE_DECLARATION_CACHE_H

#include "logger.hpp"
#undef length
#include <string>
#include <tastr/ast/ast.hpp>

extern "C" {

void initialize_type_declaration_cache();

int get_function_parameter_count(const std::string& package_name,
                                 const std::string& function_name);

const tastr::ast::Node&
get_function_parameter_type(const std::string& package_name,
                            const std::string& function_name,
                            int formal_parameter_position);

const tastr::ast::Node&
get_function_return_type(const std::string& package_name,
                         const std::string& function_name);

const tastr::ast::FunctionTypeNode&
get_function_type(const std::string& package_name,
                  const std::string& function_name);

/*  this wrapper only exists because R provides a length macro which
 * interferes with C++ library length function leading to compilation issues
 * if libraries are not included in just the right way */
std::string type_to_string(const tastr::ast::Node& node);
}

#endif /* CONTRACTR_TYPE_DECLARATION_CACHE_H */
