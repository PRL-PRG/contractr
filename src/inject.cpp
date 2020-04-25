#include "check_type.hpp"
#include "infer_type.hpp"
#include "type_declaration_cache.hpp"
#include "utilities.hpp"
#include "message.hpp"
#include "ContractAssertion.hpp"
#include "r_api.hpp"

void assert_parameter_type(SEXP value,
                           const char* const call_trace,
                           const std::string& package_name,
                           const std::string& function_name,
                           int call_id,
                           const std::string& parameter_name,
                           int actual_parameter_count,
                           int parameter_position) {
    bool assertion_status = true;
    std::string actual_type = infer_type(value, parameter_name);
    std::string expected_type;
    int expected_parameter_count =
        get_function_parameter_count(package_name, function_name);

    if (parameter_position >= expected_parameter_count) {
        assertion_status = false;
        show_message(
            "contract violation for '%s::%s'\n   ├── declared types for "
            "%d parameters\n   ├── received argument for untyped "
            "parameter '%s' (position %d) of type %s\n   └── trace: %s",
            package_name.c_str(),
            function_name.c_str(),
            expected_parameter_count,
            parameter_name.c_str(),
            /* NOTE: indexing starts from 1 in R */
            parameter_position + 1,
            actual_type.c_str(),
            call_trace);
    } else {
        const tastr::ast::Node& node = get_function_parameter_type(
            package_name, function_name, parameter_position);

        assertion_status = check_type(parameter_name, value, node);

        expected_type = type_to_string(node);

        if (!assertion_status) {
            show_message(
                "contract violation for parameter '%s' (position %d) of "
                "'%s::%s'\n   ├── expected: %s\n   ├── actual: %s\n   └── "
                "trace: %s",
                parameter_name.c_str(),
                /* NOTE: indexing starts from 1 in R */
                parameter_position + 1,
                package_name.c_str(),
                function_name.c_str(),
                expected_type.c_str(),
                actual_type.c_str(),
                call_trace);
        }
    }

    add_contract_assertion(call_id,
                           call_trace,
                           package_name,
                           function_name,
                           actual_parameter_count,
                           expected_parameter_count,
                           parameter_position,
                           parameter_name,
                           actual_type,
                           expected_type,
                           assertion_status);
}

void assert_return_type(SEXP value,
                        const char* const call_trace,
                        const std::string& package_name,
                        const std::string& function_name,
                        int call_id,
                        const std::string& parameter_name,
                        int actual_parameter_count) {
    int expected_parameter_count =
        get_function_parameter_count(package_name, function_name);

    const tastr::ast::Node& node =
        get_function_return_type(package_name, function_name);

    int parameter_position = -1;

    bool assertion_status = check_type(parameter_name, value, node);

    std::string expected_type = type_to_string(node);

    std::string actual_type = infer_type(value, parameter_name);

    if (!assertion_status) {
        show_message("contract violation for return value of "
                     "'%s::%s'\n   ├── expected: %s\n   ├── actual: %s\n   "
                     "└── trace: %s",
                     package_name.c_str(),
                     function_name.c_str(),
                     expected_type.c_str(),
                     actual_type.c_str(),
                     call_trace);
    }

    add_contract_assertion(call_id,
                           call_trace,
                           package_name,
                           function_name,
                           actual_parameter_count,
                           expected_parameter_count,
                           parameter_position,
                           parameter_name,
                           actual_type,
                           expected_type,
                           assertion_status);
}

SEXP assert_type(SEXP value,
                 SEXP is_value_missing,
                 SEXP call_trace,
                 SEXP pkg_name,
                 SEXP fun_name,
                 SEXP call_id,
                 SEXP param_name,
                 SEXP param_count,
                 SEXP param_idx) {
    std::string package_name(R_CHAR(Rf_asChar(pkg_name)));
    std::string function_name(R_CHAR(Rf_asChar(fun_name)));
    int int_call_id = Rf_asInteger(call_id);
    std::string parameter_name(R_CHAR(Rf_asChar(param_name)));
    int parameter_count = Rf_asInteger(param_count);
    int parameter_position = Rf_asInteger(param_idx);
    const char* const concatenated_call_trace = CHAR(asChar(call_trace));

    SEXP value_to_check =
        (is_value_missing == R_TrueValue) ? R_MissingArg : value;

    const tastr::ast::Node* node = nullptr;

    if (parameter_position == -1) {
        assert_return_type(value_to_check,
                           concatenated_call_trace,
                           package_name,
                           function_name,
                           int_call_id,
                           parameter_name,
                           parameter_count);
    } else {
        assert_parameter_type(value_to_check,
                              concatenated_call_trace,
                              package_name,
                              function_name,
                              int_call_id,
                              parameter_name,
                              parameter_count,
                              parameter_position);
    }

    return value;
}

void inject_argument_type_assertion(SEXP call_trace,
                                    SEXP pkg_name,
                                    SEXP fun_name,
                                    SEXP call_id,
                                    SEXP param_sym,
                                    SEXP param_name,
                                    SEXP param_count,
                                    SEXP param_index,
                                    SEXP value,
                                    SEXP rho) {
    SEXP call_value = NULL;
    SEXP value_missing = R_FalseValue;
    bool evaluate_call = false;

    if (value == R_UnboundValue || value == R_MissingArg) {
        /* missing argument */
        call_value = R_NilValue;
        value_missing = R_TrueValue;
        evaluate_call = true;
    } else if (param_sym == R_DotsSymbol) {
        /* ... parameter */
        call_value = R_NilValue;
        value_missing = R_FalseValue;
        evaluate_call = true;
    } else if (TYPEOF(value) != PROMSXP) {
        /* value (promise optimized away by compiler)  */
        value = delayed_assign(param_name, value, rho, rho, rho);
        call_value = PREXPR(value);
        value_missing = R_FalseValue;
        evaluate_call = false;
    } else if (TYPEOF(value) == PROMSXP) {
        /* promise */
        call_value = PREXPR(value);
        value_missing = R_FalseValue;
        evaluate_call = false;
    }

    SEXP call = PROTECT(create_assert_type_call(list9(call_value,
                                                      value_missing,
                                                      call_trace,
                                                      pkg_name,
                                                      fun_name,
                                                      call_id,
                                                      param_name,
                                                      param_count,
                                                      param_index)));

#ifdef DEBUG
    Rprintf("\ncontractR/src/inject.c: *** %s '%s' for '%s:::%s' with call id "
            "%d in '%s' [%d]\n",
            evaluate_call ? "calling" : "injecting",
            R_CHAR(Rf_asChar(Rf_deparse1(call, FALSE, 0))),
            R_CHAR(Rf_asChar(pkg_name)),
            R_CHAR(Rf_asChar(fun_name)),
            asInteger(call_id) R_CHAR(Rf_asChar(param_name)),
            asInteger(param_index));
#endif

    if (evaluate_call) {
        Rf_eval(call, rho);
    } else {
        SET_PRCODE(value, call);
    }

    UNPROTECT(1);
}

SEXP inject_type_assertion(SEXP call_trace,
                           SEXP pkg_name,
                           SEXP fun_name,
                           SEXP call_id,
                           SEXP param_count,
                           SEXP fun,
                           SEXP rho) {
    if (TYPEOF(pkg_name) != STRSXP || Rf_length(pkg_name) != 1) {
        Rf_error("pkg_name must be scalar character");
    }

    if (TYPEOF(fun_name) != STRSXP || Rf_length(fun_name) != 1) {
        Rf_error("fun_name must be scalar character");
    }

    if (TYPEOF(fun) != CLOSXP) {
        Rf_error("argument fun must be a function");
    }

    if (TYPEOF(rho) != ENVSXP) {
        Rf_error("argument rho must be an environment");
    }

    if (TYPEOF(call_trace) != STRSXP) {
        Rf_error("argument call_trace must be a character vector");
    }

    SEXP params = FORMALS(fun);
    for (int index = 0; params != R_NilValue; index++, params = CDR(params)) {
        SEXP param_sym = TAG(params);
        SEXP value = Rf_findVarInFrame(rho, param_sym);
        SEXP param_name = PROTECT(Rf_mkString(R_CHAR(PRINTNAME(param_sym))));
        SEXP param_index = PROTECT(Rf_ScalarInteger(index));

        inject_argument_type_assertion(call_trace,
                                       pkg_name,
                                       fun_name,
                                       call_id,
                                       param_sym,
                                       param_name,
                                       param_count,
                                       param_index,
                                       value,
                                       rho);

        UNPROTECT(2);
    }

    return R_NilValue;
}
