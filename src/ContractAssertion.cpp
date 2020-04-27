#include "ContractAssertion.hpp"

#include <vector>
#include "r_api.hpp"
#include "utilities.hpp"

#include "check_type.hpp"
#include "infer_type.hpp"
#include "type_declaration_cache.hpp"
#include "message.hpp"

void ContractAssertion::assert_parameter_type_(SEXP value) {
    assertion_status_ = true;
    actual_type_ = infer_type(value, get_parameter_name());

    /* parameter outside limits  */
    if (get_parameter_position() >= get_expected_parameter_count()) {
        assertion_status_ = false;
        show_message(
            "contract violation for '%s::%s'\n   ├── declared types for "
            "%d parameters\n   ├── received argument for untyped "
            "parameter '%s' (position %d) of type %s\n   └── trace: %s",
            get_package_name(),
            get_function_name(),
            get_expected_parameter_count(),
            get_parameter_name(),
            /* NOTE: indexing starts from 1 in R */
            get_parameter_position() + 1,
            get_actual_type().c_str(),
            get_call_trace());
    }
    /* parameter is within limits  */
    else {
        const tastr::ast::Node& node = get_function_parameter_type(
            get_function_type(), get_parameter_position());

        assertion_status_ = check_type(get_parameter_name(), value, node);

        expected_type_ = type_to_string(node);

        if (!assertion_status_) {
            show_message(
                "contract violation for parameter '%s' (position %d) of "
                "'%s::%s'\n   ├── expected: %s\n   ├── actual: %s\n   └── "
                "trace: %s",
                get_parameter_name(),
                /* NOTE: indexing starts from 1 in R */
                get_parameter_position() + 1,
                get_package_name(),
                get_function_name(),
                get_expected_type().c_str(),
                get_actual_type().c_str(),
                get_call_trace());
        }
    }
}

void ContractAssertion::assert_return_type_(SEXP value) {
    const tastr::ast::Node& node =
        get_function_return_type(get_function_type());

    parameter_position_ = -1;

    assertion_status_ = check_type(get_parameter_name(), value, node);

    expected_type_ = type_to_string(node);

    actual_type_ = infer_type(value, get_parameter_name());

    if (!assertion_status_) {
        show_message("contract violation for return value of "
                     "'%s::%s'\n   ├── expected: %s\n   ├── actual: %s\n   "
                     "└── trace: %s",
                     get_package_name(),
                     get_function_name(),
                     get_expected_type().c_str(),
                     get_actual_type().c_str(),
                     get_call_trace());
    }
}

std::ostream& operator<<(std::ostream& os,
                         const ContractAssertion& contract_assertion) {
    os << "Contract Assertion" << std::endl;
    os << "Call Id                 : " << contract_assertion.get_call_id()
       << std::endl;
    os << "Package Name            : " << contract_assertion.get_package_name()
       << std::endl;
    os << "Function Name           : " << contract_assertion.get_function_name()
       << std::endl;
    os << "Actual Parameter Count  : "
       << contract_assertion.get_actual_parameter_count() << std::endl;
    os << "Expected Parameter Count: "
       << contract_assertion.get_expected_parameter_count() << std::endl;
    os << "Parameter Position      : "
       << contract_assertion.get_parameter_position() << std::endl;
    os << "Parameter Name          : "
       << contract_assertion.get_parameter_name() << std::endl;

    return os;
}
