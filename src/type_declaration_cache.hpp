#ifndef CONTRACTR_TYPE_DECLARATION_CACHE_H
#define CONTRACTR_TYPE_DECLARATION_CACHE_H

#include "logger.hpp"
#undef length
#include <string>
#include <tastr/ast/ast.hpp>

extern "C" {

void initialize_type_declaration_cache();

const std::string& type_to_string(const tastr::ast::Node& node);

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

}

#endif /* CONTRACTR_TYPE_DECLARATION_CACHE_H */
