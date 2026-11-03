#include "utilities.hpp"

SEXP DelayedAssign = NULL;

SEXPTYPE MISSINGSXP = 19883;

const std::string UNDEFINED_STRING_VALUE = "<undefined>";

void initialize_globals() {
    DelayedAssign = Rf_install("delayedAssign");
}

SEXPTYPE type_of_sexp(SEXP value) {
    if (value == R_MissingArg) {
        return MISSINGSXP;
    }
    return TYPEOF(value);
}

void set_class(SEXP object, const std::string& class_name) {
    Rf_setAttrib(object, R_ClassSymbol, Rf_mkString(class_name.c_str()));
}

void set_names(SEXP object,
               int size,
               const std::function<std::string(int index)>& get_element) {
    Rf_setAttrib(
        object, R_NamesSymbol, create_character_vector(size, get_element));
}

void set_row_names(SEXP object,
                   int size,
                   const std::function<std::string(int index)>& get_element) {
    Rf_setAttrib(
        object, R_RowNamesSymbol, create_character_vector(size, get_element));
}

SEXP create_character_vector(
    int size,
    const std::function<std::string(int index)>& get_element) {
    SEXP character_vector = PROTECT(Rf_allocVector(STRSXP, size));

    for (int i = 0; i < size; ++i) {
        SET_STRING_ELT(character_vector, i, Rf_mkChar(get_element(i).c_str()));
    }

    UNPROTECT(1);
    return character_vector;
}

SEXP create_integer_vector(int size,
                           const std::function<int(int index)>& get_element) {
    SEXP integer_vector = PROTECT(Rf_allocVector(INTSXP, size));

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

    SEXP df = PROTECT(Rf_allocVector(VECSXP, column_count));

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

SEXP create_list(const std::vector<SEXP>& values,
                 const std::vector<std::string>& names) {
    int size = values.size();
    SEXP list = PROTECT(Rf_allocVector(VECSXP, size));

    for (int i = 0; i < size; ++i) {
        SET_VECTOR_ELT(list, i, values[i]);
    }

    set_names(list, size, [&names](int index) -> std::string {
        return names[index];
    });

    UNPROTECT(1);

    return list;
}
