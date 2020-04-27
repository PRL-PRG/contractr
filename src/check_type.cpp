#include <tastr/parser/parser.hpp>
#include "TypeChecker.hpp"
#include "r_api.hpp"
#include "check_type.hpp"

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

    const std::string parameter_name = CHAR(asChar(param_name));

    bool result = check_type(parameter_name, value, node);

    UNPROTECT(1);

    return result ? R_TrueValue : R_FalseValue;
}

bool check_type(const std::string& parameter_name,
                SEXP value,
                const tastr::ast::Node& node) {
    return TypeChecker().typecheck(parameter_name, value, node);
}

bool check_type(SEXP value, const tastr::ast::Node& node) {
    return check_type(UNDEFINED_NAME, value, node);
}
