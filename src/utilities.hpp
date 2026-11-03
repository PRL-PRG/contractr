#ifndef CONTRACTR_UTILITIES_HPP
#define CONTRACTR_UTILITIES_HPP

#include <Rinternals.h>
#include <string>
#include <functional>
#include <vector>

extern SEXP DelayedAssign;

extern SEXPTYPE MISSINGSXP;

extern const std::string UNDEFINED_STRING_VALUE;

extern "C" {

void initialize_globals();

SEXPTYPE type_of_sexp(SEXP value);

void set_class(SEXP object, const std::string& class_name);

void set_names(SEXP object,
               int size,
               const std::function<std::string(int index)>& get_element);

void set_row_names(SEXP object,
                   int size,
                   const std::function<std::string(int index)>& get_element);

SEXP create_character_vector(
    int size,
    const std::function<std::string(int index)>& get_element);

SEXP create_integer_vector(int size,
                           const std::function<int(int index)>& get_element);

SEXP create_data_frame(const std::vector<SEXP> columns,
                       const std::vector<std::string>& names);

SEXP create_list(const std::vector<SEXP>& values,
                 const std::vector<std::string>& names);
}

#endif /* CONTRACTR_UTILITIES_HPP */
