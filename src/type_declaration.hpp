#ifndef CONTRACTR_TYPE_DECLARATION_H
#define CONTRACTR_TYPE_DECLARATION_H

#include "logger.hpp"
#undef length
#include <string>
#include <tastr/ast/ast.hpp>

extern "C" {

const std::string& type_to_string(const tastr::ast::Node& node);

int get_function_parameter_count(
    const tastr::ast::FunctionTypeNode* function_type);

const tastr::ast::Node&
get_function_parameter_type(const tastr::ast::FunctionTypeNode* function_type,
                            int formal_parameter_position);

const tastr::ast::Node&
get_function_return_type(const tastr::ast::FunctionTypeNode* function_type);

const tastr::ast::FunctionTypeNode* get_function_type(int package_index,
                                                      int function_index);
}

#endif /* CONTRACTR_TYPE_DECLARATION_CACHE_H */
