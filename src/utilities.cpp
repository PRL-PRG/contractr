#include "utilities.hpp"

SEXP DotCallSym = NULL;
SEXP DelayedAssign = NULL;
SEXP SystemDotFile = NULL;
SEXP PackageSymbol = NULL;
SEXPTYPE MISSINGSXP = 19883;

void initialize_globals() {
    DotCallSym = Rf_install(".Call");
    DelayedAssign = Rf_install("delayedAssign");
    SystemDotFile = Rf_install("system.file");
    PackageSymbol = Rf_install("package");
}

SEXPTYPE type_of_sexp(SEXP value) {
    if (value == R_MissingArg) {
        return MISSINGSXP;
    }
    return TYPEOF(value);
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

SEXP lang7(SEXP s, SEXP t, SEXP u, SEXP v, SEXP w, SEXP x, SEXP y) {
    PROTECT(s);
    s = LCONS(s, Rf_list6(t, u, v, w, x, y));
    UNPROTECT(1);
    return s;
}

SEXP delayed_assign(SEXP variable,
                    SEXP value,
                    SEXP eval_env,
                    SEXP assign_env,
                    SEXP rho) {
    SEXP call = Rf_lang5(DelayedAssign, variable, value, eval_env, assign_env);
    Rf_eval(call, rho);
    return Rf_findVarInFrame(rho, variable);
}

SEXP system_file(SEXP path) {
    SEXP package_name = PROTECT(mkString("contractR"));
    SEXP call = PROTECT(Rf_lang3(SystemDotFile, path, package_name));
    SET_TAG(CDDR(call), PackageSymbol);
    SEXP result = Rf_eval(call, R_GlobalEnv);
    UNPROTECT(2);
    return result;
}

SEXP lookup_value(SEXP rho, SEXP value_sym, bool evaluate) {
    SEXP value = Rf_findVarInFrame(rho, value_sym);
    if (value == R_UnboundValue || value == R_MissingArg) {
        value = R_MissingArg;
    } else if (evaluate) {
        value = Rf_eval(value, rho);
    }
    return value;
}
