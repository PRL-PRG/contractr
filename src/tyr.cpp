#include "tyr.hpp"

#include "utilities.hpp"
#include "r_api.hpp"
#include "compat.h"

#undef length

#include <algorithm>
#include <cmath>
#include <map>
#include <string>
#include <vector>

/* -------------------------------------------------------------- inference */

namespace {

const int MAX_DEPTH = 4;

/* The type of a value gives the mode that the value has. For example, the
 * type of a double vector uses the mode `DBL`. `DBL <= dbl` is correct. Thus
 * the type also agrees with the larger type `dbl` that authors usually write. */

bool is_identifier(const std::string& name) {
    if (name.empty())
        return false;
    if (!(std::isalpha(static_cast<unsigned char>(name[0])) || name[0] == '.'))
        return false;
    for (char c: name) {
        if (!(std::isalnum(static_cast<unsigned char>(c)) || c == '.' ||
              c == '_'))
            return false;
    }
    return true;
}

std::string infer(SEXP value, int depth);

std::string decorate(SEXP value,
                     const std::string& base,
                     bool skip_names,
                     int depth) {
    std::vector<std::string> classes;
    std::vector<std::string> attributes;
    bool opaque_attribute = false;

    for (SEXP node = contractr_ATTRIB(value); node != R_NilValue; node = CDR(node)) {
        SEXP tag = TAG(node);
        if (TYPEOF(tag) != SYMSXP)
            continue;
        std::string name = CHAR(PRINTNAME(tag));
        if (name == "class") {
            SEXP klass = CAR(node);
            if (TYPEOF(klass) == STRSXP) {
                for (int i = 0; i < LENGTH(klass); ++i) {
                    std::string cls = CHAR(STRING_ELT(klass, i));
                    if (is_identifier(cls))
                        classes.push_back(cls);
                    else
                        opaque_attribute = true;
                }
            }
            continue;
        }
        if (skip_names && name == "names")
            continue;
        if (!is_identifier(name)) {
            opaque_attribute = true;
            continue;
        }
        attributes.push_back(name + ": " + infer(CAR(node), depth + 1));
    }

    std::string head = base;

    std::string result = head + "<";
    for (std::size_t i = 0; i < classes.size(); ++i) {
        if (i != 0)
            result += ", ";
        result += classes[i];
    }
    /* If a class name or an attribute name is not an identifier, the type
     * cannot contain it. Then `...` opens a class list that is not empty, and
     * `any` opens the attributes. */
    if (opaque_attribute && !classes.empty())
        result += ", ...";
    result += "> with { ";
    for (std::size_t i = 0; i < attributes.size(); ++i) {
        if (i != 0)
            result += ", ";
        result += attributes[i];
    }
    if (opaque_attribute) {
        if (!attributes.empty())
            result += ", ";
        result += "any";
    }
    result += " }";
    return result;
}

/* Returns the type of an atomic vector, not its content. The type gives the
 * mode and if the vector is a scalar. For a scalar, it also shows if the value
 * is `NA`. A singleton type (`42.`, `"nopad"`) is more accurate, but it puts
 * user data in the record. It helps only against annotations that are unions
 * of literals. The annotation guide tells authors not to write these. */
std::string infer_atomic(SEXP value) {
    int n = LENGTH(value);
    bool scalar = n == 1;

    /* `LGL` contains all logical vectors, also the scalars. Thus a vector
     * that is not a scalar gets the type `(LGL \ LGL1)`. With this type,
     * contractr finds a violation when an overload `(x: vec1) -> LGL1`
     * returns `c(TRUE, FALSE)`. */
    auto many = [](const char* mode) -> std::string {
        return std::string("(") + mode + " \\ " + mode + "1)";
    };

    switch (TYPEOF(value)) {
    case LGLSXP:
        if (!scalar) return many("LGL");
        return LOGICAL_ELT(value, 0) == NA_LOGICAL ? "LGL1" : "^LGL1";
    case INTSXP:
        if (!scalar) return many("INT");
        return INTEGER_ELT(value, 0) == NA_INTEGER ? "INT1" : "^INT1";
    case REALSXP:
        if (!scalar) return many("DBL");
        return ISNAN(REAL_ELT(value, 0)) ? "DBL1" : "^DBL1";
    case CPLXSXP: {
        if (!scalar) return many("CLX");
        Rcomplex v = COMPLEX_ELT(value, 0);
        return (ISNAN(v.r) || ISNAN(v.i)) ? "CLX1" : "^CLX1";
    }
    case STRSXP:
        if (!scalar) return many("CHR");
        return STRING_ELT(value, 0) == NA_STRING ? "CHR1" : "^CHR1";
    case RAWSXP:
        /* A raw value cannot be `NA`. */
        return scalar ? "^RAW1" : many("RAW");
    default:
        return "any";
    }
}

std::string infer_list(SEXP value, int depth) {
    int n = LENGTH(value);
    SEXP names = Rf_getAttrib(value, R_NamesSymbol);
    bool has_names = TYPEOF(names) == STRSXP && LENGTH(names) == n;

    std::vector<std::string> fields;
    std::vector<std::string> loose;
    bool opaque = false;

    for (int i = 0; i < n; ++i) {
        std::string element = infer(VECTOR_ELT(value, i), depth + 1);
        std::string name;
        if (has_names && STRING_ELT(names, i) != NA_STRING)
            name = CHAR(STRING_ELT(names, i));
        if (name.empty()) {
            loose.push_back(element);
        } else if (is_identifier(name)) {
            fields.push_back(name + ": " + element);
        } else {
            loose.push_back(element);
            opaque = true;
        }
    }

    std::sort(loose.begin(), loose.end());
    loose.erase(std::unique(loose.begin(), loose.end()), loose.end());

    std::string result = "{ ";
    for (std::size_t i = 0; i < fields.size(); ++i) {
        if (i != 0)
            result += ", ";
        result += fields[i];
    }
    if (!loose.empty()) {
        if (!fields.empty())
            result += ", ";
        for (std::size_t i = 0; i < loose.size(); ++i) {
            if (i != 0)
                result += " | ";
            result += loose[i];
        }
    }
    result += " }";
    (void) opaque;
    return result;
}

std::string infer(SEXP value, int depth) {
    if (depth > MAX_DEPTH)
        return "any";

    SEXPTYPE type = type_of_sexp(value);

    if (type == MISSINGSXP)
        return "absent";

    switch (type) {
    case NILSXP:
        return "null";
    case CLOSXP:
    case BUILTINSXP:
    case SPECIALSXP:
        /* In rstt, `(...: any) -> any` does not contain `(x: int) -> int`.
         * The type of all functions is `empty -> any`. */
        return "empty -> any";
    case ENVSXP:
        return "env";
    case SYMSXP:
        return "sym";
    case LANGSXP:
        /* A language object can have a class. For example, a formula has a
         * class, and stringr gives `~ toupper(.x)` to a parameter of the type
         * `<formula, ...>`. */
        return decorate(value, "lang", false, depth);
    case EXTPTRSXP:
        return "externalptr(any)";
    case LGLSXP:
    case INTSXP:
    case REALSXP:
    case CPLXSXP:
    case STRSXP:
    case RAWSXP:
        return decorate(value, infer_atomic(value), false, depth);
    case VECSXP:
        return decorate(value, infer_list(value, depth), true, depth);
    default:
        return "any";
    }
}

} // namespace

std::string tyr_infer_type(SEXP value) {
    return infer(value, 0);
}
