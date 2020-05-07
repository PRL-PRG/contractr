#include "utilities.hpp"
#include <cstring>

SEXP DotCallSymbol = NULL;
SEXP DelayedAssign = NULL;
SEXP SystemDotFile = NULL;
SEXP PackageSymbol = NULL;
SEXP ContractrSymbol = NULL;
SEXP AssertContractSymbol = NULL;

SEXPTYPE MISSINGSXP = 19883;

const std::string UNDEFINED_STRING_VALUE = "<undefined>";

void initialize_globals() {
    DotCallSymbol = Rf_install(".Call");
    DelayedAssign = Rf_install("delayedAssign");
    SystemDotFile = Rf_install("system.file");
    PackageSymbol = Rf_install("package");
    ContractrSymbol = Rf_install("contractr");
    AssertContractSymbol = Rf_install("C_assert_contract");
}

char* copy_c_string(const char* source) {
    int size = strlen(source);
    int bytes = (size + 1) * sizeof(char);
    char* destination = (char*) malloc(bytes);
    /* size + 1 will also copy the null character at the end  */
    memcpy(destination, source, bytes);
    return destination;
}

SEXPTYPE type_of_sexp(SEXP value) {
    if (value == R_MissingArg) {
        return MISSINGSXP;
    }
    return TYPEOF(value);
}

std::string sexptype_to_string(SEXPTYPE type) {
    if (type == NILSXP) {
        return "NULL";
    } else if (type == SYMSXP) {
        return "symbol";
    } else if (type == LISTSXP) {
        return "pairlist";
    } else if (type == CLOSXP) {
        return "closure";
    } else if (type == ENVSXP) {
        return "environment";
    } else if (type == PROMSXP) {
        return "promise";
    } else if (type == LANGSXP) {
        return "language";
    } else if (type == SPECIALSXP) {
        return "special";
    } else if (type == BUILTINSXP) {
        return "builtin";
    } else if (type == CHARSXP) {
        return "char";
    } else if (type == LGLSXP) {
        return "logical";
    } else if (type == INTSXP) {
        return "integer";
    } else if (type == REALSXP) {
        return "double";
    } else if (type == CPLXSXP) {
        return "complex";
    } else if (type == STRSXP) {
        return "character";
    } else if (type == DOTSXP) {
        return "...";
    } else if (type == ANYSXP) {
        return "any";
    } else if (type == EXPRSXP) {
        return "expression";
    } else if (type == VECSXP) {
        return "list";
    } else if (type == EXTPTRSXP) {
        return "externalptr";
    } else if (type == BCODESXP) {
        return "bytecode";
    } else if (type == WEAKREFSXP) {
        return "weakref";
    } else if (type == RAWSXP) {
        return "raw";
    } else if (type == S4SXP) {
        return "S4";
    }
    return "CASE_NOT_HANDLED";
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
    SEXP package_name = PROTECT(mkString("contractr"));
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

std::string get_language_class(SEXP object) {
    SEXP head = CAR(object);
    if (type_of_sexp(head) == SYMSXP) {
        std::string name(CHAR(PRINTNAME(head)));
        if (name == "if" || name == "while" || name == "for" || name == "=" ||
            name == "<-" || name == "(" || name == "{") {
            return name;
        }
    }
    return "call";
}

std::vector<std::string> get_class_names(SEXP object) {
    std::vector<std::string> class_names;

    SEXP klass = getAttrib(object, R_ClassSymbol);

    /* class attribute not present  */
    if (klass == R_NilValue) {
        SEXP dim = getAttrib(object, R_DimSymbol);
        int ndim = length(dim);

        /* dimension attribute not present or of length 0  */
        if (ndim == 0) {
            SEXPTYPE t = TYPEOF(object);

            switch (t) {
            case CLOSXP:
            case SPECIALSXP:
            case BUILTINSXP:
                class_names.push_back("function");
                break;
            case REALSXP:
                /* NOTE: this is handled separately as ^double[]  */
                /* class_names.push_back("numeric"); */
                break;
            case SYMSXP:
                class_names.push_back("name");
                break;
            case LANGSXP:
                class_names.push_back(get_language_class(object));
                break;
            default:
                /* NOTE: these are handled separately */
                /* class_names.push_back(sexptype_to_string(type_of_sexp(object)));
                 */
                break;
            }
        }
        /* two dimensions  */
        else if (ndim == 2) {
            class_names.push_back("matrix");
        }
        /* not two dimensions  */
        else {
            class_names.push_back("array");
        }
    }
    /* class attribute present  */
    else if (type_of_sexp(klass) == STRSXP) {
        for (int index = 0; index < LENGTH(klass); ++index) {
            class_names.push_back(CHAR(STRING_ELT(klass, index)));
        }
    }

    return class_names;
}

bool has_class(SEXP object, const std::string& class_name) {
    std::vector<std::string> class_names = get_class_names(object);

    for (int i = 0; i < class_names.size(); ++i) {
        if (class_names[i] == class_name) {
            return true;
        }
    }

    return false;
}

bool is_data_frame(SEXP object) {
    return (type_of_sexp(object) == VECSXP) && has_class(object, "data.frame");
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

SEXP create_assert_contract_call(SEXP arguments) {
    SEXP assert_contract_fun = PROTECT(
        Rf_lang3(R_TripleColonSymbol, ContractrSymbol, AssertContractSymbol));

    SEXP call = PROTECT(create_dot_call(assert_contract_fun, arguments));

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
