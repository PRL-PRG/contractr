#include "inject.hpp"

#include "Typechecker.hpp"
#include "infer_type.hpp"
#include "logger.hpp"
#include "type_declaration_cache.hpp"
#include "utilities.hpp"
#include "ContractAssertion.hpp"
#include "message.hpp"
#include "Severity.hpp"

#include <R_ext/Rdynload.h>
#include <Rinternals.h>
#include <stdlib.h> // for NULL

SEXP r_set_severity(SEXP severity) {
    set_severity(to_severity(CHAR(asChar(severity))));
    return R_NilValue;
}

SEXP r_get_severity() {
    return mkString(to_string(get_severity()).c_str());
}

std::string concatenate_call_trace(SEXP call_trace,
                                   const std::string& indentation = "") {
    int size = LENGTH(call_trace);

    std::string result = "<" + std::to_string(size) + " frame(s)>";

    for (int i = size - 1; i >= 0; --i) {
        SEXP call = VECTOR_ELT(call_trace, i);
        int call_length = LENGTH(call);

        result.append("\n")
            .append(indentation)
            .append(i == 0 ? "└── " : "├── ")
            .append(CHAR(STRING_ELT(call, 0)));

        for (int j = 1; j < call_length; ++j) {
            result.append("\n")
                .append(indentation)
                .append(i != 0 ? "│   " : "    ")
                .append(CHAR(STRING_ELT(call, j)));
        }
    }
    return result;
}

SEXP r_concatenate_call_trace(SEXP call_trace) {
    const std::string call_trace_string =
        concatenate_call_trace(call_trace, std::string(14, ' '));

    return Rf_mkString(call_trace_string.c_str());
}

void assert_parameter_type(SEXP value,
                           const char* const call_trace,
                           const std::string& package_name,
                           const std::string& function_name,
                           int call_id,
                           const std::string& parameter_name,
                           int parameter_count,
                           int formal_parameter_position) {
    bool assertion_status = true;
    std::string actual_type = infer_type(value, parameter_name);
    std::string expected_type;

    if (parameter_count <= formal_parameter_position) {
        assertion_status = false;
        show_message(
            "contract violation for '%s::%s'\n   ├── declared types for "
            "%d parameters\n   ├── received argument for untyped "
            "parameter '%s' (position %d) of type %s\n   └── trace: %s",
            package_name.c_str(),
            function_name.c_str(),
            parameter_count,
            parameter_name.c_str(),
            /* NOTE: indexing starts from 1 in R */
            formal_parameter_position + 1,
            actual_type.c_str(),
            call_trace);

    } else {
        const tastr::ast::Node& node = get_function_parameter_type(
            package_name, function_name, formal_parameter_position);

        TypeChecker type_checker(package_name,
                                 function_name,
                                 parameter_name,
                                 formal_parameter_position);

        assertion_status = type_checker.typecheck(value, node);

        expected_type = type_to_string(node);

        if (!assertion_status) {
            show_message(
                "contract violation for parameter '%s' (position %d) of "
                "'%s::%s'\n   ├── expected: %s\n   ├── actual: %s\n   └── "
                "trace: %s",
                parameter_name.c_str(),
                /* NOTE: indexing starts from 1 in R */
                formal_parameter_position + 1,
                package_name.c_str(),
                function_name.c_str(),
                expected_type.c_str(),
                actual_type.c_str(),
                call_trace);
        }
    }

    add_contract_assertion(package_name,
                           function_name,
                           call_id,
                           parameter_name,
                           parameter_count,
                           formal_parameter_position,
                           actual_type,
                           expected_type,
                           assertion_status,
                           call_trace);
}

void assert_return_type(SEXP value,
                        const char* const call_trace,
                        const std::string& package_name,
                        const std::string& function_name,
                        int call_id,
                        const std::string& parameter_name,
                        int parameter_count) {
    const tastr::ast::Node& node =
        get_function_return_type(package_name, function_name);

    int formal_parameter_position = -1;

    TypeChecker type_checker(
        package_name, function_name, parameter_name, formal_parameter_position);

    bool assertion_status = type_checker.typecheck(value, node);

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

    add_contract_assertion(package_name,
                           function_name,
                           call_id,
                           parameter_name,
                           parameter_count,
                           formal_parameter_position,
                           actual_type,
                           expected_type,
                           assertion_status,
                           call_trace);
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
    int formal_parameter_position = Rf_asInteger(param_idx);
    const char* const concatenated_call_trace = CHAR(asChar(call_trace));

    SEXP value_to_check =
        (is_value_missing == R_TrueValue) ? R_MissingArg : value;

    const tastr::ast::Node* node = nullptr;

    if (formal_parameter_position == -1) {
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
                              formal_parameter_position);
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

SEXP r_get_contract_assertions() {
    int size = get_contract_assertion_count();

    auto get_package_name = [](int index) -> std::string {
        return get_contract_assertion(index).get_package_name();
    };

    auto get_function_name = [](int index) -> std::string {
        return get_contract_assertion(index).get_function_name();
    };

    auto get_call_id = [](int index) -> int {
        return get_contract_assertion(index).get_call_id();
    };

    auto get_parameter_name = [](int index) -> std::string {
        return get_contract_assertion(index).get_parameter_name();
    };

    auto get_parameter_count = [](int index) -> int {
        return get_contract_assertion(index).get_parameter_count();
    };

    auto get_parameter_position = [](int index) -> int {
        return get_contract_assertion(index).get_parameter_position();
    };

    auto get_actual_type = [](int index) -> std::string {
        return get_contract_assertion(index).get_actual_type();
    };

    auto get_expected_type = [](int index) -> std::string {
        return get_contract_assertion(index).get_expected_type();
    };

    auto get_assertion_status = [](int index) -> bool {
        return get_contract_assertion(index).get_assertion_status();
    };

    auto get_call_trace = [](int index) -> std::string {
        return get_contract_assertion(index).get_call_trace();
    };

    std::vector<SEXP> columns = {
        PROTECT(create_character_vector(size, get_package_name)),
        PROTECT(create_character_vector(size, get_function_name)),
        PROTECT(create_integer_vector(size, get_call_id)),
        PROTECT(create_character_vector(size, get_parameter_name)),
        PROTECT(create_integer_vector(size, get_parameter_count)),
        PROTECT(create_integer_vector(size, get_parameter_position)),
        PROTECT(create_character_vector(size, get_actual_type)),
        PROTECT(create_character_vector(size, get_expected_type)),
        PROTECT(create_logical_vector(size, get_assertion_status)),
        PROTECT(create_character_vector(size, get_call_trace))};

    std::vector<std::string> names = {"package_name",
                                      "function_name",
                                      "call_id",
                                      "parameter_name",
                                      "parameter_count",
                                      "parameter_position",
                                      "actual_type",
                                      "expected_type",
                                      "assertion_status",
                                      "call_trace"};

    SEXP df = PROTECT(create_data_frame(columns, names));

    UNPROTECT(columns.size() + 1);

    return df;
}
