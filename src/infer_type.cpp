#include "infer_type.hpp"

#include <set>
#include <vector>

template <typename T>
bool has_na(SEXP value, T check_na) {
    int length = LENGTH(value);

    for (int i = 0; i < length; ++i) {
        if (check_na(value, i)) {
            return true;
        }
    }
    return false;
}

template <typename T>
std::string
infer_vector_type(SEXP value, const std::string& infix, T check_na) {
    std::string na_prefix = "";
    std::string vector_suffix = "";

    if (has_na(value, check_na)) {
        na_prefix = "^";
    }

    if (LENGTH(value) > 1) {
        vector_suffix = "[]";
    }

    return na_prefix + infix + vector_suffix;
}

std::string join(const std::vector<std::string>& strings,
                 const std::string& delimiter) {
    std::string result;

    if (strings.size() == 0) {
        return result;
    }

    result = strings[0];

    for (int i = 1; i < strings.size(); ++i) {
        result = result + delimiter + strings[i];
    }
    return result;
}

std::string infer_list_type(SEXP value) {
    SEXP names = getAttrib(value, R_NamesSymbol);

    if (names != R_NilValue) {
        auto get_name = [names](int index) -> std::string {
            if (index > LENGTH(names)) {
                return std::string("^");
            } else {
                return std::string("`") + CHAR(STRING_ELT(names, index)) +
                       std::string("`");
            }
        };

        std::vector<std::string> element_types;

        for (int index = 0; index < LENGTH(value); ++index) {
            std::string name = get_name(index);
            std::string type = infer_type(VECTOR_ELT(value, index));
            element_types.push_back(name + " : " + type);
        }

        return "list<" + join(element_types, ", ") + ">";
    }

    else if (LENGTH(value) <= 5) {
        std::vector<std::string> element_types;

        for (int index = 0; index < LENGTH(value); ++index) {
            element_types.push_back(infer_type(VECTOR_ELT(value, index)));
        }

        return "tuple<" + join(element_types, ", ") + ">";
    }

    else {
        std::set<std::string> element_types;

        for (int index = 0; index < LENGTH(value); ++index) {
            element_types.insert(infer_type(VECTOR_ELT(value, index)));
        }

        return "list<" +
               join(std::vector<std::string>(element_types.begin(),
                                             element_types.end()),
                    "|") +
               ">";
    }
}

std::string infer_type(SEXP value, const std::string& parameter_name) {
    if (parameter_name == "...") {
        return "...";
    }

    else if (value == R_MissingArg) {
        return "???";
    }

    else if (value == R_NilValue) {
        return "null";
    }

    else if (TYPEOF(value) == VECSXP) {
        return infer_list_type(value);
    }

    else if (TYPEOF(value) == INTSXP) {
        return infer_vector_type(
            value, "integer", [](SEXP vector, int index) -> bool {
                return INTEGER_ELT(vector, index) == NA_INTEGER;
            });
    }

    else if (TYPEOF(value) == CHARSXP) {
        return infer_vector_type(
            value, "character", [](SEXP vector, int index) -> bool {
                return STRING_ELT(vector, index) == NA_STRING;
            });
    }

    else if (TYPEOF(value) == CPLXSXP) {
        return infer_vector_type(
            value, "complex", [](SEXP vector, int index) -> bool {
                Rcomplex v = COMPLEX_ELT(vector, index);
                return (ISNAN(v.r) || ISNAN(v.i));
            });
    }

    else if (TYPEOF(value) == REALSXP) {
        return infer_vector_type(
            value, "double", [](SEXP vector, int index) -> bool {
                return ISNAN(REAL_ELT(vector, index));
            });
    }

    else if (TYPEOF(value) == LGLSXP) {
        return infer_vector_type(
            value, "logical", [](SEXP vector, int index) -> bool {
                return LOGICAL_ELT(vector, index) == NA_LOGICAL;
            });
    }

    else if (TYPEOF(value) == RAWSXP) {
        return infer_vector_type(
            value, "raw", [](SEXP vector, int index) -> bool {
                // NOTE no such thing as a raw NA
                return false;
            });
    }

    else if (TYPEOF(value) == ENVSXP) {
        return "environment";
    }

    else if (TYPEOF(value) == EXPRSXP) {
        return "expression";
    }

    else if (TYPEOF(value) == LANGSXP) {
        return "language";
    }

    else if (TYPEOF(value) == SYMSXP) {
        return "symbol";
    }

    else if (TYPEOF(value) == EXTPTRSXP) {
        return "externalptr";
    }

    else if (TYPEOF(value) == BCODESXP) {
        return "bytecode";
    }

    else if (TYPEOF(value) == LISTSXP) {
        return "pairlist";
    }

    else if (TYPEOF(value) == S4SXP) {
        return "s4";
    }

    else if (TYPEOF(value) == WEAKREFSXP) {
        return "weakref";
    }

    return "<unhandled case>";
}
