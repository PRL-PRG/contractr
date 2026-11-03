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

/* ------------------------------------------------------------------ store */

namespace {

struct TyrFunction {
    std::string name;
    std::string signature;
    std::vector<std::string> parameter_names;
    std::vector<std::string> parameter_types; /* types, never `T?` */
    std::vector<int> parameter_optional;      /* the `?` lives here instead */
    std::string return_type;
};

struct TyrPackage {
    std::string name;
    std::vector<TyrFunction> functions;
};

std::vector<TyrPackage> store;
const std::string EMPTY_STRING = "";

int find_package(const std::string& name) {
    for (std::size_t i = 0; i < store.size(); ++i) {
        if (store[i].name == name)
            return static_cast<int>(i);
    }
    return -1;
}

int find_function(int package_index, const std::string& name) {
    if (package_index < 0 || package_index >= static_cast<int>(store.size()))
        return -1;
    const std::vector<TyrFunction>& fns = store[package_index].functions;
    for (std::size_t i = 0; i < fns.size(); ++i) {
        if (fns[i].name == name)
            return static_cast<int>(i);
    }
    return -1;
}

const TyrFunction* get(int package_index, int function_index) {
    if (package_index < 0 || package_index >= static_cast<int>(store.size()))
        return nullptr;
    const std::vector<TyrFunction>& fns = store[package_index].functions;
    if (function_index < 0 || function_index >= static_cast<int>(fns.size()))
        return nullptr;
    return &fns[function_index];
}

std::vector<std::string> to_string_vector(SEXP value) {
    std::vector<std::string> result;
    if (TYPEOF(value) != STRSXP)
        return result;
    for (int i = 0; i < LENGTH(value); ++i) {
        result.push_back(CHAR(STRING_ELT(value, i)));
    }
    return result;
}

} // namespace

bool tyr_has_function(int package_index, int function_index) {
    return get(package_index, function_index) != nullptr;
}

int tyr_parameter_count(int package_index, int function_index) {
    const TyrFunction* fn = get(package_index, function_index);
    return fn == nullptr ? 0 : static_cast<int>(fn->parameter_names.size());
}

const std::string&
tyr_parameter_type(int package_index, int function_index, int position) {
    const TyrFunction* fn = get(package_index, function_index);
    if (fn == nullptr || position < 0 ||
        position >= static_cast<int>(fn->parameter_types.size()))
        return EMPTY_STRING;
    return fn->parameter_types[position];
}

const std::string&
tyr_parameter_name(int package_index, int function_index, int position) {
    const TyrFunction* fn = get(package_index, function_index);
    if (fn == nullptr || position < 0 ||
        position >= static_cast<int>(fn->parameter_names.size()))
        return EMPTY_STRING;
    return fn->parameter_names[position];
}

const std::string& tyr_return_type(int package_index, int function_index) {
    const TyrFunction* fn = get(package_index, function_index);
    return fn == nullptr ? EMPTY_STRING : fn->return_type;
}

/* -------------------------------------------------------------- inference */

namespace {

const int MAX_DEPTH = 4;

/* An observation is a measurement, so it names the mode the value really had:
 * a double vector is `DBL`. `DBL <= dbl` holds, so it still satisfies the
 * permissive annotation an author would normally write. */

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
    /* an unrepresentable class name leaves the class set open */
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

/* The *type* of the value, not the value: an observation says what shape was
 * seen -- mode, whether it was a scalar, whether it could be `NA` -- and leaves
 * the content out. A singleton (`42.`, `"nopad"`) would be a more precise type,
 * but it puts user data in the record, and only pays off against annotations
 * written as literal unions, which the guide discourages. */
std::string infer_atomic(SEXP value) {
    int n = LENGTH(value);
    bool scalar = n == 1;

    switch (TYPEOF(value)) {
    case LGLSXP:
        if (!scalar) return "LGL";
        return LOGICAL_ELT(value, 0) == NA_LOGICAL ? "LGL1" : "^LGL1";
    case INTSXP:
        if (!scalar) return "INT";
        return INTEGER_ELT(value, 0) == NA_INTEGER ? "INT1" : "^INT1";
    case REALSXP:
        if (!scalar) return "DBL";
        return ISNAN(REAL_ELT(value, 0)) ? "DBL1" : "^DBL1";
    case CPLXSXP: {
        if (!scalar) return "CLX";
        Rcomplex v = COMPLEX_ELT(value, 0);
        return (ISNAN(v.r) || ISNAN(v.i)) ? "CLX1" : "^CLX1";
    }
    case STRSXP:
        if (!scalar) return "CHR";
        return STRING_ELT(value, 0) == NA_STRING ? "CHR1" : "^CHR1";
    case RAWSXP:
        /* raw has no NA representation */
        return scalar ? "^raw1" : "raw";
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
        return "(...: any) -> any";
    case ENVSXP:
        return "env";
    case SYMSXP:
        return "sym";
    case LANGSXP:
        /* a language object can carry a class -- a formula is one, and stringr
         * really does pass `~ toupper(.x)` where a `<formula, ...>` is declared */
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

/* ------------------------------------------------------------------ R API */

SEXP r_tyr_declare(SEXP r_package_name,
                   SEXP r_function_name,
                   SEXP r_signature,
                   SEXP r_parameter_names,
                   SEXP r_parameter_types,
                   SEXP r_parameter_optional,
                   SEXP r_return_type) {
    std::string package_name = CHAR(Rf_asChar(r_package_name));
    std::string function_name = CHAR(Rf_asChar(r_function_name));

    int package_index = find_package(package_name);
    if (package_index == -1) {
        store.push_back(TyrPackage{package_name, {}});
        package_index = static_cast<int>(store.size()) - 1;
    }

    TyrFunction fn;
    fn.name = function_name;
    fn.signature = CHAR(Rf_asChar(r_signature));
    fn.parameter_names = to_string_vector(r_parameter_names);
    fn.parameter_types = to_string_vector(r_parameter_types);
    fn.parameter_optional.clear();
    if (TYPEOF(r_parameter_optional) == LGLSXP) {
        for (int i = 0; i < LENGTH(r_parameter_optional); ++i)
            fn.parameter_optional.push_back(LOGICAL(r_parameter_optional)[i] == TRUE);
    }
    fn.return_type = CHAR(Rf_asChar(r_return_type));

    int function_index = find_function(package_index, function_name);
    if (function_index == -1) {
        store[package_index].functions.push_back(fn);
        function_index =
            static_cast<int>(store[package_index].functions.size()) - 1;
    } else {
        store[package_index].functions[function_index] = fn;
    }

    SEXP result = PROTECT(Rf_allocVector(INTSXP, 2));
    INTEGER(result)[0] = package_index;
    INTEGER(result)[1] = function_index;
    UNPROTECT(1);
    return result;
}

SEXP r_tyr_get_index(SEXP r_package_name, SEXP r_function_name) {
    int package_index = find_package(CHAR(Rf_asChar(r_package_name)));
    int function_index =
        find_function(package_index, CHAR(Rf_asChar(r_function_name)));

    SEXP result = PROTECT(Rf_allocVector(INTSXP, 2));
    INTEGER(result)[0] = package_index;
    INTEGER(result)[1] = function_index;
    UNPROTECT(1);
    return result;
}

SEXP r_tyr_function_names(SEXP r_package_name) {
    int package_index = find_package(CHAR(Rf_asChar(r_package_name)));
    if (package_index == -1)
        return Rf_allocVector(STRSXP, 0);

    const std::vector<TyrFunction>& fns = store[package_index].functions;
    SEXP result = PROTECT(Rf_allocVector(STRSXP, fns.size()));
    for (std::size_t i = 0; i < fns.size(); ++i) {
        SET_STRING_ELT(result, i, Rf_mkChar(fns[i].name.c_str()));
    }
    UNPROTECT(1);
    return result;
}

SEXP r_tyr_package_names() {
    SEXP result = PROTECT(Rf_allocVector(STRSXP, store.size()));
    for (std::size_t i = 0; i < store.size(); ++i) {
        SET_STRING_ELT(result, i, Rf_mkChar(store[i].name.c_str()));
    }
    UNPROTECT(1);
    return result;
}

SEXP r_tyr_clear() {
    store.clear();
    return R_NilValue;
}

SEXP r_tyr_infer_type(SEXP value_sym, SEXP rho) {
    SEXP value = PROTECT(lookup_value(rho, value_sym, true));
    std::string type = tyr_infer_type(value);
    UNPROTECT(1);
    return Rf_mkString(type.c_str());
}
