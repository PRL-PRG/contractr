#include "inject.hpp"

#include "Typechecker.hpp"
#include "infer_type.hpp"
#include "logger.hpp"
#include "type_declaration_cache.hpp"
#include "utilities.hpp"

#include <R_ext/Rdynload.h>
#include <Rinternals.h>
#include <stdlib.h> // for NULL

void assert_parameter_type(SEXP value,
                          const std::string& package_name,
                          const std::string& function_name,
                          const std::string& parameter_name,
                          int formal_parameter_position) {
    int parameter_count =
        get_function_parameter_count(package_name, function_name);

    if (parameter_count <= formal_parameter_position) {
        warningcall(
            R_NilValue,
            "contract violation for '%s::%s'\n   ├── declared types for "
            "%d parameters\n   └── received argument for untyped "
            "parameter '%s' (position %d) of type %s",
            package_name.c_str(),
            function_name.c_str(),
            parameter_count,
            parameter_name.c_str(),
            /* NOTE: indexing starts from 1 in R */
            formal_parameter_position + 1,
            infer_type(value, parameter_name).c_str());
    } else {
        const tastr::ast::Node& node = get_function_parameter_type(
            package_name, function_name, formal_parameter_position);

        TypeChecker type_checker(package_name,
                                 function_name,
                                 parameter_name,
                                 formal_parameter_position);

        bool result = type_checker.typecheck(value, node);

        if (!result) {
            warningcall(
                R_NilValue,
                "contract violation for parameter '%s' (position %d) of "
                "'%s::%s'\n   ├── expected: %s\n   └── actual: %s",
                parameter_name.c_str(),
                /* NOTE: indexing starts from 1 in R */
                formal_parameter_position + 1,
                package_name.c_str(),
                function_name.c_str(),
                type_to_string(node).c_str(),
                infer_type(value, parameter_name).c_str());
        }
    }
}

void assert_return_type(SEXP value,
                       const std::string& package_name,
                       const std::string& function_name) {
    const tastr::ast::Node& node =
        get_function_return_type(package_name, function_name);

    std::string parameter_name = "return";

    TypeChecker type_checker(package_name, function_name, parameter_name, -1);
    bool result = type_checker.typecheck(value, node);

    if (!result) {
        warningcall(R_NilValue,
                    "contract violation for return value of "
                    "'%s::%s'\n   ├── expected: %s\n   └── actual: %s",
                    package_name.c_str(),
                    function_name.c_str(),
                    type_to_string(node).c_str(),
                    infer_type(value, parameter_name).c_str());
    }
}

SEXP assert_type(SEXP value, SEXP is_value_missing, SEXP pkg_name,
                 SEXP fun_name, SEXP param_name, SEXP param_idx) {

    std::string parameter_name(R_CHAR(Rf_asChar(param_name)));
    int formal_parameter_position = Rf_asInteger(param_idx);
    std::string package_name(R_CHAR(Rf_asChar(pkg_name)));
    std::string function_name(R_CHAR(Rf_asChar(fun_name)));

    SEXP value_to_check =
        (is_value_missing == R_TrueValue) ? R_MissingArg : value;

    const tastr::ast::Node* node = nullptr;

    if (formal_parameter_position == -1) {
        assert_return_type(value_to_check, package_name, function_name);
    } else {
        assert_parameter_type(value_to_check,
                             package_name,
                             function_name,
                             parameter_name,
                             formal_parameter_position);
    }

    return value;
}

void inject_argument_type_assertion(SEXP pkg_name, SEXP fun_name, SEXP param_sym,
                                    SEXP param_name, SEXP param_index, SEXP value, SEXP rho) {
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

    // TODO: make a constant
    SEXP check_type_fun = PROTECT(
        Rf_lang3(
            R_TripleColonSymbol,
            Rf_install("contractR"),
            Rf_install("assert_type")
        )
    );
    SEXP call = PROTECT(
        lang7(
            check_type_fun,
            call_value,
            value_missing,
            pkg_name,
            fun_name,
            param_name,
            param_index
        )
    );

#ifdef DEBUG
    Rprintf("\ncontractR/src/inject.c: *** %s '%s' for '%s:::%s' in '%s' [%d]\n",
            evaluate_call ? "calling" : "injecting",
            R_CHAR(Rf_asChar(Rf_deparse1(call, FALSE, 0))),
            R_CHAR(Rf_asChar(pkg_name)),
            R_CHAR(Rf_asChar(fun_name)),
            R_CHAR(Rf_asChar(param_name)),
            asInteger(param_index));
#endif

    if (evaluate_call) {
        Rf_eval(call, rho);
    } else {
        SET_PRCODE(value, call);
    }

    UNPROTECT(2);
}

SEXP inject_type_assertion(SEXP pkg_name, SEXP fun_name, SEXP fun, SEXP rho) {
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

    SEXP params = FORMALS(fun);
    for (int index = 0; params != R_NilValue; index++, params = CDR(params)) {
        SEXP param_sym = TAG(params);
        SEXP value = Rf_findVarInFrame(rho, param_sym);
        SEXP param_name = PROTECT(Rf_mkString(R_CHAR(PRINTNAME(param_sym))));
        SEXP param_index = PROTECT(Rf_ScalarInteger(index));

        inject_argument_type_assertion(
            pkg_name, fun_name, param_sym, param_name, param_index, value, rho);

        UNPROTECT(2);
    }

    return R_NilValue;
}
