#include "inject.hpp"

#include "TypeDeclarationCache.hpp"
#include "Typechecker.hpp"
#include "infer_type.hpp"
#include "logger.hpp"

#include <R_ext/Rdynload.h>
#include <Rinternals.h>
#include <stdlib.h> // for NULL

SEXP R_DotCallSym = NULL;

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
                   SEXP pkg_name,
                   SEXP fun_name,
                   SEXP param_name,
                   SEXP param_idx) {
    Rprintf("Checking parameter: %s (%d) in %s::%s -- %d\n",
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
    const tastr::ast::Node* node =
        TypeDeclarationCache::get_function_parameter_type(
            package_name, function_name, formal_parameter_position);

    TypeChecker type_checker(
        package_name, function_name, parameter_name, formal_parameter_position);

    bool result = type_checker.typecheck(value, *node);

    if (!result) {
        log_error(
            "type checking failed for parameter '%s' (position %d) of %s::%s\n",
            parameter_name.c_str(),
            formal_parameter_position,
            package_name.c_str(),
            function_name.c_str());

        log_raw("\tExpected: %s\n", tastr::parser::to_string(*node).c_str());
        log_raw("\tActual: %s\n", infer_type(value, parameter_name).c_str());
    }
}

void check_return_type(SEXP value,
                       const std::string& package_name,
                       const std::string& function_name) {
    const tastr::ast::Node* node =
        TypeDeclarationCache::get_function_return_type(package_name,
                                                       function_name);

    std::string parameter_name = "return";

    TypeChecker type_checker(package_name, function_name, parameter_name, -1);
    bool result = type_checker.typecheck(value, *node);

    if (!result) {
        log_error("type checking failed for return value of %s::%s\n",
                  package_name.c_str(),
                  function_name.c_str());
        log_raw("\tExpected: %s\n", tastr::parser::to_string(*node).c_str());
        log_raw("\tActual: %s\n", infer_type(value, parameter_name).c_str());
    }
}

SEXP check_type(SEXP value,
                SEXP pkg_name,
                SEXP fun_name,
                SEXP param_name,
                SEXP param_idx) {
    std::string parameter_name(R_CHAR(Rf_asChar(param_name)));
    int formal_parameter_position = Rf_asInteger(param_idx);
    std::string package_name(R_CHAR(Rf_asChar(pkg_name)));
    std::string function_name(R_CHAR(Rf_asChar(fun_name)));

    const tastr::ast::Node* node = nullptr;

    if (formal_parameter_position == -1) {
        check_return_type(value, package_name, function_name);

    } else {
        check_parameter_type(value,
                             package_name,
                             function_name,
                             parameter_name,
                             formal_parameter_position);
    }

    return value;
}

SEXP create_check_type_call(SEXP val, SEXP pkg_name, SEXP fun_name,
                            SEXP param_name, SEXP param_idx) {
    SEXP call = PROTECT(
        Rf_lang6(
            CheckTypeFun,
            val,
            pkg_name,
            fun_name,
            param_name,
            param_idx
        )
    );

    if (CheckTypeFunWrapper != R_NilValue) {
        call =
            PROTECT(Rf_lcons(CheckTypeFunWrapper, call));
    }

    UNPROTECT(CheckTypeFunWrapper != R_NilValue ? 2 : 1);

    return call;
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

    if (!TypeDeclarationCache::function_is_typed(R_CHAR(Rf_asChar(pkg_name)),
                                                 R_CHAR(Rf_asChar(fun_name)))) {
        return R_NilValue;
    }

    SEXP params = FORMALS(fun);
    for (int idx = 0; params != R_NilValue; idx++, params = CDR(params)) {
        SEXP param_sym = TAG(params);
        SEXP val = Rf_findVarInFrame(rho, param_sym);

        SEXP param_idx = PROTECT(Rf_ScalarInteger(idx));
        SEXP param_name = PROTECT(Rf_mkString(R_CHAR(PRINTNAME(param_sym))));
        SEXP val_to_check = NULL;

        if (val == R_UnboundValue || val == R_MissingArg) {
            // FIXME: hardcoded - not possible to test
            check_type(R_MissingArg, pkg_name, fun_name, param_name, param_idx);
        } else if (TYPEOF(val) == PROMSXP) {
            val_to_check = PREXPR(val);
        } else if (param_sym == R_DotsSymbol) {
            val_to_check = R_NilValue;
        } else {
            // TODO: construct a promise to handle optimizations
        }

        if (val_to_check != NULL) {
            SEXP check_type_call = PROTECT(
                create_check_type_call(
                    val_to_check,
                    pkg_name,
                    fun_name,
                    param_name,
                    param_idx
                )
            );

            if (TYPEOF(val) == PROMSXP) {
                SET_PRCODE(val, check_type_call);

#ifdef DEBUG
                Rprintf(
                    "\ncontractR/src/inject.c: injecting '%s' for '%s:::%s' in "
                    "'%s' [%d]\n",
                    R_CHAR(Rf_asChar(Rf_deparse1(check_type_call, FALSE, 0))),
                    R_CHAR(Rf_asChar(pkg_name)),
                    R_CHAR(Rf_asChar(fun_name)),
                    R_CHAR(Rf_asChar(param_name)),
                    idx);
#endif
            } else {
#ifdef DEBUG
                Rprintf(
                    "\ncontractR/src/inject.c: calling '%s' for '%s:::%s' in "
                    "'%s' [%d]\n",
                    R_CHAR(Rf_asChar(Rf_deparse1(check_type_call, FALSE, 0))),
                    R_CHAR(Rf_asChar(pkg_name)),
                    R_CHAR(Rf_asChar(fun_name)),
                    R_CHAR(Rf_asChar(param_name)),
                    idx);
#endif

                Rf_eval(check_type_call, rho);
            }

            UNPROTECT(3);
        }
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

    set_type_check_function(check_type_call, R_DotCallSym);

    UNPROTECT(1);

    return R_NilValue;
}

SEXP environment_name(SEXP env) {
    if (R_IsPackageEnv(env) == TRUE) {
        // cf. builtin.c:432 do_envirName
        return Rf_asChar(R_PackageEnvName(env));
    } else if (R_IsNamespaceEnv(env) == TRUE) {
        // cf. builtin.c:434 do_envirName
        return Rf_asChar(R_NamespaceEnvSpec(env));
    } else {
        return R_NilValue;
    }
}
