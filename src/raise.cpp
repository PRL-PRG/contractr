#include "raise.hpp"
#include "severity.hpp"
#include <Rinternals.h>

#include <cstdarg>
#include <cstdio>

typedef void (*raise_function_t)(SEXP, const char*, ...);

void raise_return_type_failure(const ContractAssertion* contract,
                               raise_function_t raise_function) {
    const char* RETURN_TYPE_FAILURE_MESSAGE =
        "contract violation for return value of '%s::%s'\n" //
        "   ├── expected: %s\n"                             //
        "   ├── actual: %s\n"                               //
        "   └── trace: %s";                                 //

    raise_function(R_NilValue,
                   RETURN_TYPE_FAILURE_MESSAGE,
                   contract->get_package_name(),
                   contract->get_function_name(),
                   contract->get_expected_type().c_str(),
                   contract->get_actual_type().c_str(),
                   contract->get_call_trace());
}

void raise_parameter_outside_limit(const ContractAssertion* contract,
                                   raise_function_t raise_function) {
    const char* PARAMETER_OUTSIDE_LIMIT_MESSAGE =
        "contract violation for '%s::%s'\n"                               //
        "   ├── declared types for %d parameters\n"                       //
        "   ├── received argument of type %s for untyped parameter '%s' " //
        "(position %d)\n"                                                 //
        "   └── trace: %s";

    raise_function(R_NilValue,
                   PARAMETER_OUTSIDE_LIMIT_MESSAGE,
                   contract->get_package_name(),
                   contract->get_function_name(),
                   contract->get_expected_parameter_count(),
                   contract->get_actual_type().c_str(),
                   contract->get_parameter_name(),
                   /* NOTE: indexing starts from 1 in R */
                   contract->get_parameter_position() + 1,
                   contract->get_call_trace());
}

void raise_argument_type_failure(const ContractAssertion* contract,
                                 raise_function_t raise_function) {
    const char* ARGUMENT_TYPE_FAILURE_MESSAGE =
        "contract violation for parameter '%s' (position %d) of '%s::%s'\n" //
        "   ├── expected: %s\n"                                             //
        "   ├── actual: %s\n"                                               //
        "   └── trace: %s";

    raise_function(R_NilValue,
                   ARGUMENT_TYPE_FAILURE_MESSAGE,
                   contract->get_parameter_name(),
                   /* NOTE: indexing starts from 1 in R */
                   contract->get_parameter_position() + 1,
                   contract->get_package_name(),
                   contract->get_function_name(),
                   contract->get_expected_type().c_str(),
                   contract->get_actual_type().c_str(),
                   contract->get_call_trace());
}

void raise_contract_failure(const ContractAssertion* contract) {
    Severity severity = get_severity();

    if (contract->get_assertion_status()) {
        return;
    }

    if (severity == Severity::Silence) {
        return;
    }

    if (severity == Severity::Undefined) {
        errorcall(R_NilValue, "severity is 'undefined'");
        return;
    }

    raise_function_t raise_function =
        get_severity() == Severity::Warning ? warningcall : errorcall;

    /* return type contract  */
    if (contract->get_parameter_position() == -1) {
        raise_return_type_failure(contract, raise_function);

    }
    /* parameter outside limits  */
    else if (contract->get_parameter_position() >=
             contract->get_expected_parameter_count()) {
        raise_parameter_outside_limit(contract, raise_function);
    }
    /* parameter is within limits  */
    else {
        raise_argument_type_failure(contract, raise_function);
    }
}
