#include "inject.hpp"

#include "Typechecker.hpp"
#include "infer_type.hpp"
#include "logger.hpp"
#include "type_declaration_cache.hpp"
#include "utilities.hpp"

#include <R_ext/Rdynload.h>
#include <Rinternals.h>
#include <stdlib.h> // for NULL

// A symbol (SYMSXP) or a call LANSXP to get the function
// to be called to do the actual type checking. The function
// should take 5 parameters:
// - the value to be checked (any SEXP),
// - the package name (STRSXP),
// - the function name (STRSXP),
// - the parameter name (STRSXP) and
// - the parameter index (INTSXP)
// and it has to return the value!
static SEXP CheckTypeFun = NULL;

// A function that the CheckTypeFun will be wrapped into or NULL.
// If not-null the final call will be CheckTypeFunWrapper(CheckTypeFun, ...)
static SEXP CheckTypeFunWrapper = NULL;

SEXP log_insertion(SEXP value,
                   SEXP argument_is_missing,
                   SEXP parameter_is_vararg,
                   SEXP pkg_name,
                   SEXP fun_name,
                   SEXP param_name,
                   SEXP param_idx) {
    Rprintf("Checking parameter: %s (%d) in '%s::%s' -- %d\n",
            R_CHAR(Rf_asChar(param_name)),
            Rf_asInteger(param_idx),
            R_CHAR(Rf_asChar(pkg_name)),
            R_CHAR(Rf_asChar(fun_name)),
            TYPEOF(value));
    return value;
}

void check_parameter_type(SEXP value,
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
            "parameter '%s' at position %d of type %s",
            package_name.c_str(),
            function_name.c_str(),
            parameter_count,
            parameter_name.c_str(),
            /* NOTE: indexing starts from 1 in R */
            formal_parameter_position + 1,
            infer_type(value, parameter_name).c_str());
    }

    else {
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

void check_return_type(SEXP value,
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

SEXP check_type(SEXP value,
                SEXP argument_is_missing,
                SEXP parameter_is_vararg,
                SEXP pkg_name,
                SEXP fun_name,
                SEXP param_name,
                SEXP param_idx) {
    std::string parameter_name(R_CHAR(Rf_asChar(param_name)));
    int formal_parameter_position = Rf_asInteger(param_idx);
    std::string package_name(R_CHAR(Rf_asChar(pkg_name)));
    std::string function_name(R_CHAR(Rf_asChar(fun_name)));

    SEXP value_to_check =
        (argument_is_missing == R_TrueValue) ? R_MissingArg : value;

    const tastr::ast::Node* node = nullptr;

    if (formal_parameter_position == -1) {
        check_return_type(value_to_check, package_name, function_name);

    } else {
        check_parameter_type(value_to_check,
                             package_name,
                             function_name,
                             parameter_name,
                             formal_parameter_position);
    }

    return value;
}

SEXP create_check_type_call(SEXP value,
                            SEXP argument_is_missing,
                            SEXP parameter_is_vararg,
                            SEXP pkg_name,
                            SEXP fun_name,
                            SEXP param_name,
                            SEXP param_index) {
    int protect_counter = 0;

    SEXP call = PROTECT(lang8(CheckTypeFun,
                              value,
                              argument_is_missing,
                              parameter_is_vararg,
                              pkg_name,
                              fun_name,
                              param_name,
                              param_index));
    ++protect_counter;

    if (CheckTypeFunWrapper != R_NilValue) {
        call = PROTECT(Rf_lcons(CheckTypeFunWrapper, call));
        ++protect_counter;
    }

    UNPROTECT(protect_counter);

    return call;
}

/* NOTE: if DEBUG is undefined, this function will be empty and should be
 * eliminated by the compiler  */
void show_insertion_message(SEXP check_type_call,
                            bool evaluate_call,
                            SEXP pkg_name,
                            SEXP fun_name,
                            SEXP param_name,
                            SEXP param_index) {
#ifdef DEBUG
    const char* action = "injecting";
    if (evaluate_call) {
        action = "calling";
    }

    Rprintf("\ncontractR/src/inject.c: %s '%s' for '%s:::%s' in "
            "'%s' [%d]\n",
            action,
            R_CHAR(Rf_asChar(Rf_deparse1(check_type_call, FALSE, 0))),
            R_CHAR(Rf_asChar(pkg_name)),
            R_CHAR(Rf_asChar(fun_name)),
            R_CHAR(Rf_asChar(param_name)),
            asInteger(param_index));
#endif
}

void inject_argument_type_check(SEXP pkg_name,
                                SEXP fun_name,
                                SEXP param_sym,
                                SEXP param_name,
                                SEXP param_index,
                                SEXP value,
                                SEXP rho) {
    SEXP call_value = NULL;
    SEXP argument_is_missing = R_FalseValue;
    SEXP parameter_is_vararg = R_FalseValue;
    bool evaluate_call = false;

    /* missing argument */
    if (value == R_UnboundValue || value == R_MissingArg) {
        call_value = R_NilValue;
        argument_is_missing = R_TrueValue;
        parameter_is_vararg = R_FalseValue;
        evaluate_call = true;
    }

    /* ... parameter */
    else if (param_sym == R_DotsSymbol) {
        call_value = R_NilValue;
        argument_is_missing = R_FalseValue;
        parameter_is_vararg = R_TrueValue;
        evaluate_call = true;
    }

    /* value (promise optimized away by compiler)  */
    else if (TYPEOF(value) != PROMSXP) {
        value = delayed_assign(param_sym, value, rho, rho, rho);
        call_value = PREXPR(value);
        argument_is_missing = R_FalseValue;
        parameter_is_vararg = R_FalseValue;
        evaluate_call = false;
    }

    /* promise */
    else if (TYPEOF(value) == PROMSXP) {
        call_value = PREXPR(value);
        argument_is_missing = R_FalseValue;
        parameter_is_vararg = R_FalseValue;
        evaluate_call = false;
    }

    SEXP call = PROTECT(create_check_type_call(call_value,
                                               argument_is_missing,
                                               parameter_is_vararg,
                                               pkg_name,
                                               fun_name,
                                               param_name,
                                               param_index));

    show_insertion_message(
        call, evaluate_call, pkg_name, fun_name, param_name, param_index);

    if (evaluate_call) {
        Rf_eval(call, rho);
    } else {
        SET_PRCODE(value, call);
    }

    UNPROTECT(3);
}

SEXP inject_type_check(SEXP pkg_name, SEXP fun_name, SEXP fun, SEXP rho) {
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

        inject_argument_type_check(
            pkg_name, fun_name, param_sym, param_name, param_index, value, rho);
    }

    return R_NilValue;
}

SEXP get_type_check_function() {
    return CheckTypeFun;
}

SEXP get_type_check_function_wrapper() {
    return CheckTypeFunWrapper;
}

SEXP set_type_check_function(SEXP fun, SEXP wrapper) {
    if (TYPEOF(fun) != LANGSXP && TYPEOF(fun) != SYMSXP) {
        Rf_error("fun must be a call or a symbol");
    }

    if (wrapper != R_NilValue && TYPEOF(wrapper) != LANGSXP &&
        TYPEOF(wrapper) != SYMSXP) {
        Rf_error("wrapper must be a call or a symbol or NULL");
    }

    if (CheckTypeFun != R_NilValue) {
        R_ReleaseObject(CheckTypeFun);
    }

    if (CheckTypeFunWrapper != R_NilValue) {
        R_ReleaseObject(CheckTypeFunWrapper);
    }

    CheckTypeFun = fun;
    R_PreserveObject(CheckTypeFun);
    CheckTypeFunWrapper = wrapper;
    R_PreserveObject(CheckTypeFunWrapper);

    return R_NilValue;
}

SEXP reset_type_check_function() {
    SEXP check_type_call = PROTECT(Rf_lang3(R_TripleColonSymbol,
                                            Rf_install("contractR"),
                                            Rf_install("check_type")));

    set_type_check_function(check_type_call, DotCallSym);

    UNPROTECT(1);

    return R_NilValue;
}
