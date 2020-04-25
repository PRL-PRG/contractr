#include "utilities.hpp"

SEXP DotCallSymbol = NULL;
SEXP DelayedAssign = NULL;
SEXP SystemDotFile = NULL;
SEXP PackageSymbol = NULL;
SEXP ContractRSymbol = NULL;
SEXP AssertTypeSymbol = NULL;
SEXPTYPE MISSINGSXP = 19883;

void initialize_globals() {
    DotCallSymbol = Rf_install(".Call");
    DelayedAssign = Rf_install("delayedAssign");
    SystemDotFile = Rf_install("system.file");
    PackageSymbol = Rf_install("package");
    ContractRSymbol = Rf_install("contractR");
    AssertTypeSymbol = Rf_install("C_assert_type");
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

SEXP lang8(SEXP s, SEXP t, SEXP u, SEXP v, SEXP w, SEXP x, SEXP y, SEXP z) {
    PROTECT(s);
    s = LCONS(s, list7(t, u, v, w, x, y, z));
    UNPROTECT(1);
    return s;
}

SEXP lang9(SEXP r,
           SEXP s,
           SEXP t,
           SEXP u,
           SEXP v,
           SEXP w,
           SEXP x,
           SEXP y,
           SEXP z) {
    PROTECT(r);
    r = LCONS(r, list8(s, t, u, v, w, x, y, z));
    UNPROTECT(1);
    return r;
}

SEXP lang10(SEXP q,
            SEXP r,
            SEXP s,
            SEXP t,
            SEXP u,
            SEXP v,
            SEXP w,
            SEXP x,
            SEXP y,
            SEXP z) {
    PROTECT(q);
    q = LCONS(q, list9(r, s, t, u, v, w, x, y, z));
    UNPROTECT(1);
    return q;
}

SEXP lang11(SEXP p,
            SEXP q,
            SEXP r,
            SEXP s,
            SEXP t,
            SEXP u,
            SEXP v,
            SEXP w,
            SEXP x,
            SEXP y,
            SEXP z) {
    PROTECT(p);
    p = LCONS(p, list10(q, r, s, t, u, v, w, x, y, z));
    UNPROTECT(1);
    return p;
}

SEXP list7(SEXP s, SEXP t, SEXP u, SEXP v, SEXP w, SEXP x, SEXP y) {
    PROTECT(s);
    s = CONS(s, Rf_list6(t, u, v, w, x, y));
    UNPROTECT(1);
    return s;
}

SEXP list8(SEXP s, SEXP t, SEXP u, SEXP v, SEXP w, SEXP x, SEXP y, SEXP z) {
    PROTECT(s);
    s = CONS(s, list7(t, u, v, w, x, y, z));
    UNPROTECT(1);
    return s;
}

SEXP list9(SEXP r,
           SEXP s,
           SEXP t,
           SEXP u,
           SEXP v,
           SEXP w,
           SEXP x,
           SEXP y,
           SEXP z) {
    PROTECT(r);
    r = CONS(r, list8(s, t, u, v, w, x, y, z));
    UNPROTECT(1);
    return r;
}

SEXP list10(SEXP q,
            SEXP r,
            SEXP s,
            SEXP t,
            SEXP u,
            SEXP v,
            SEXP w,
            SEXP x,
            SEXP y,
            SEXP z) {
    PROTECT(q);
    q = CONS(q, list9(r, s, t, u, v, w, x, y, z));
    UNPROTECT(1);
    return q;
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

void set_class(SEXP object, const std::string& class_name) {
    setAttrib(object, R_ClassSymbol, mkString(class_name.c_str()));
}

void set_names(SEXP object,
               int size,
               const std::function<std::string(int index)>& get_element) {
    setAttrib(
        object, R_NamesSymbol, create_character_vector(size, get_element));
}

void set_row_names(SEXP object,
                   int size,
                   const std::function<std::string(int index)>& get_element) {
    setAttrib(
        object, R_RowNamesSymbol, create_character_vector(size, get_element));
}

SEXP create_character_vector(
    int size,
    const std::function<std::string(int index)>& get_element) {
    SEXP character_vector = PROTECT(allocVector(STRSXP, size));

    for (int i = 0; i < size; ++i) {
        SET_STRING_ELT(character_vector, i, mkChar(get_element(i).c_str()));
    }

    UNPROTECT(1);
    return character_vector;
}

SEXP create_logical_vector(int size,
                           const std::function<bool(int index)>& get_element) {
    SEXP logical_vector = PROTECT(allocVector(LGLSXP, size));

    for (int i = 0; i < size; ++i) {
        LOGICAL(logical_vector)[i] = get_element(i);
    }

    UNPROTECT(1);
    return logical_vector;
}

SEXP create_integer_vector(int size,
                           const std::function<int(int index)>& get_element) {
    SEXP integer_vector = PROTECT(allocVector(INTSXP, size));

    for (int i = 0; i < size; ++i) {
        INTEGER(integer_vector)[i] = get_element(i);
    }

    UNPROTECT(1);
    return integer_vector;
}

SEXP create_data_frame(const std::vector<SEXP> columns,
                       const std::vector<std::string>& names) {
    int column_count = columns.size();
    int row_count = column_count == 0 ? 0 : LENGTH(columns[0]);

    SEXP df = PROTECT(allocVector(VECSXP, column_count));

    for (int i = 0; i < column_count; ++i) {
        SET_VECTOR_ELT(df, i, columns[i]);
    }

    set_class(df, "data.frame");

    set_names(df, column_count, [&names](int index) { return names[index]; });

    set_row_names(
        df, row_count, [](int index) { return std::to_string(index + 1); });

    UNPROTECT(1);

    return df;
}

SEXP create_dot_call(SEXP function, SEXP arguments) {
    SEXP dot_call_arguments = PROTECT(CONS(function, arguments));
    SEXP dot_call = PROTECT(LCONS(DotCallSymbol, dot_call_arguments));
    UNPROTECT(2);
    return dot_call;
}

SEXP create_assert_type_call(SEXP arguments) {
    SEXP assert_type_fun = PROTECT(
        Rf_lang3(R_TripleColonSymbol, ContractRSymbol, AssertTypeSymbol));

    SEXP call = PROTECT(create_dot_call(assert_type_fun, arguments));

    UNPROTECT(2);

    return call;
}

SEXP create_list(const std::vector<SEXP>& values,
                 const std::vector<std::string>& names) {
    int size = values.size();
    SEXP list = PROTECT(allocVector(VECSXP, size));

    for (int i = 0; i < size; ++i) {
        SET_VECTOR_ELT(list, i, values[i]);
    }

    set_names(list, size, [&names](int index) -> std::string {
        return names[index];
    });

    UNPROTECT(1);

    return list;
}
