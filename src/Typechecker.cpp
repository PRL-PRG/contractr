#include <tastr/parser/parser.hpp>
/* comment prevents reorganization of include files by clang */
#include "Typechecker.hpp"

SEXP r_check_type(SEXP value_sym, SEXP param_name, SEXP type, SEXP rho) {
    const std::string type_string = CHAR(asChar(type));

    tastr::parser::ParseResult parse_result(
        tastr::parser::parse_string(type_string));

    if (!parse_result) {
        errorcall(R_NilValue,
                  "%s in '%s' at %s",
                  parse_result.get_error_message().c_str(),
                  type_string.c_str(),
                  to_string(parse_result.get_error_location()).c_str());
        return R_FalseValue;
    }

    const tastr::ast::FunctionTypeNode& funtype_node =
        tastr::ast::as<tastr::ast::FunctionTypeNode>(
            parse_result.get_top_level_node()->at(0).get_type());

    const tastr::ast::Node& node =
        tastr::ast::as<tastr::ast::ParameterNode>(funtype_node.get_parameter())
            .at(0);

    SEXP value = PROTECT(lookup_value(rho, value_sym, true));

    const std::string package_name = "package";
    const std::string function_name = "function";
    const std::string parameter_name = CHAR(asChar(param_name));
    const int formal_parameter_position = 0;

    TypeChecker checker(
        package_name, function_name, parameter_name, formal_parameter_position);

    bool result = checker.typecheck(value, node);

    UNPROTECT(1);

    return result ? R_TrueValue : R_FalseValue;
}
