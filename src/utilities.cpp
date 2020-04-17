#include "utilities.hpp"

SEXP DotCallSym = NULL;
SEXP DelayedAssign = NULL;
SEXP SystemDotFile = NULL;
SEXP PackageSymbol = NULL;

void initialize_globals() {
    DotCallSym = Rf_install(".Call");
    DelayedAssign = Rf_install("delayedAssign");
    SystemDotFile = Rf_install("system.file");
    PackageSymbol = Rf_install("package");
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

SEXP list7(SEXP s, SEXP t, SEXP u, SEXP v, SEXP w, SEXP x, SEXP y) {
    PROTECT(s);
    s = CONS(s, Rf_list6(t, u, v, w, x, y));
    UNPROTECT(1);
    return s;
}

SEXP lang8(SEXP s, SEXP t, SEXP u, SEXP v, SEXP w, SEXP x, SEXP y, SEXP z) {
    PROTECT(s);
    s = LCONS(s, list7(t, u, v, w, x, y, z));
    UNPROTECT(1);
    return s;
}

SEXP delayed_assign(SEXP variable,
                    SEXP value,
                    SEXP eval_env,
                    SEXP assign_env,
                    SEXP rho) {
    SEXP call =
        Rf_lang5(DelayedAssign, variable, value, eval_env, assign_env);
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
