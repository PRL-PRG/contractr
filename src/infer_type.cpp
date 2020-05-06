#include "infer_type.hpp"

#include "utilities.hpp"

#include <set>
#include <vector>

#include "r_api.hpp"

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

    if (LENGTH(value) != 1) {
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

const std::string union_types(const std::vector<std::string>& types) {
    std::vector<std::string> strings(types);

    int null_index = -1;

    for (int i = 0; i < strings.size(); ++i) {
        if (strings[i] == "null") {
            null_index = i;
            break;
        }
    }

    std::string prefix = "";

    if (null_index != -1) {
        if (strings.size() == 1) {
            prefix = "null";
        } else {
            prefix = "? ";
        }
        strings.erase(strings.begin() + null_index);
    }

    return prefix + join(strings, " | ");
}

std::string infer_list_type(SEXP value) {
    SEXP names = getAttrib(value, R_NamesSymbol);

    if (names != R_NilValue) {
        auto get_name = [names](int index) -> std::string {
            const std::string na_name("^");
            SEXP name = STRING_ELT(names, index);
            if (name == NA_STRING) {
                return na_name;
            } else {
                return std::string("`") + CHAR(name) + "`";
            }
        };

        std::vector<std::string> element_types;

        for (int index = 0; index < LENGTH(value); ++index) {
            std::string name = get_name(index);
            std::string type = infer_type(VECTOR_ELT(value, index));
            element_types.push_back(name + ": " + type);
        }

        return "struct<" + join(element_types, ", ") + ">";
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
               union_types(std::vector<std::string>(element_types.begin(),
                                                    element_types.end())) +
               ">";
    }
}

std::string infer_type(const std::string& parameter_name, SEXP value) {
    SEXP class_names = R_NilValue;

    if (parameter_name == "...") {
        return "...";
    }

    else if (type_of_sexp(value) == MISSINGSXP) {
        return "???";
    }

    else if ((class_names = get_class_names(value)) != R_NilValue &&
             type_of_sexp(class_names) == STRSXP) {
        std::vector<std::string> class_types;

        for (int index = 0; index < LENGTH(class_names); ++index) {
            std::string class_type =
                "$" + std::string(CHAR(STRING_ELT(class_names, index)));

            class_types.push_back(class_type);
        }

        return join(class_types, " & ");
    }

    else if (type_of_sexp(value) == NILSXP) {
        return "null";
    }

    else if (type_of_sexp(value) == VECSXP) {
        /* if list has data.frame class it is a dataframe  */
        if (is_data_frame(value)) {
            return "dataframe";
        }
        /* infer non dataframe list type  */
        else {
            return infer_list_type(value);
        }
    }

    else if (type_of_sexp(value) == CLOSXP) {
        return "any => any";
    }

    else if (type_of_sexp(value) == BUILTINSXP) {
        return "any => any";
    }

    else if (type_of_sexp(value) == SPECIALSXP) {
        return "any => any";
    }

    else if (type_of_sexp(value) == INTSXP) {
        return infer_vector_type(
            value, "integer", [](SEXP vector, int index) -> bool {
                return INTEGER_ELT(vector, index) == NA_INTEGER;
            });
    }

    else if (type_of_sexp(value) == STRSXP) {
        return infer_vector_type(
            value, "character", [](SEXP vector, int index) -> bool {
                return STRING_ELT(vector, index) == NA_STRING;
            });
    }

    else if (type_of_sexp(value) == CPLXSXP) {
        return infer_vector_type(
            value, "complex", [](SEXP vector, int index) -> bool {
                Rcomplex v = COMPLEX_ELT(vector, index);
                return (ISNAN(v.r) || ISNAN(v.i));
            });
    }

    else if (type_of_sexp(value) == REALSXP) {
        return infer_vector_type(
            value, "double", [](SEXP vector, int index) -> bool {
                return ISNAN(REAL_ELT(vector, index));
            });
    }

    else if (type_of_sexp(value) == LGLSXP) {
        return infer_vector_type(
            value, "logical", [](SEXP vector, int index) -> bool {
                return LOGICAL_ELT(vector, index) == NA_LOGICAL;
            });
    }

    else if (type_of_sexp(value) == RAWSXP) {
        return infer_vector_type(
            value, "raw", [](SEXP vector, int index) -> bool {
                // NOTE no such thing as a raw NA
                return false;
            });
    }

    else if (type_of_sexp(value) == ENVSXP) {
        return "environment";
    }

    else if (type_of_sexp(value) == EXPRSXP) {
        return "expression";
    }

    else if (type_of_sexp(value) == LANGSXP) {
        return "language";
    }

    else if (type_of_sexp(value) == SYMSXP) {
        return "symbol";
    }

    else if (type_of_sexp(value) == EXTPTRSXP) {
        return "externalptr";
    }

    else if (type_of_sexp(value) == BCODESXP) {
        return "bytecode";
    }

    else if (type_of_sexp(value) == LISTSXP) {
        return "pairlist";
    }

    else if (type_of_sexp(value) == S4SXP) {
        return "s4";
    }

    else if (type_of_sexp(value) == WEAKREFSXP) {
        return "weakref";
    }

    return "<unhandled case>";
}

std::string infer_type(SEXP value) {
    return infer_type(UNDEFINED_STRING_VALUE, value);
}

SEXP r_infer_type(SEXP value_sym, SEXP parameter_name, SEXP rho) {
    SEXP value = PROTECT(lookup_value(rho, value_sym, true));
    std::string type = infer_type(CHAR(asChar(parameter_name)), value);
    UNPROTECT(1);
    return mkString(type.c_str());
}
