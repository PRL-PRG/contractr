#' @export
clear_type_declaration_cache <- function() {
    ## TODO: get typed packages and functions and revert them to uninjected state
    .Call(C_clear_type_declaration_cache)
}

#' @export
import_type_declarations <- function(package_name) {
    stopifnot(is_scalar_character(package_name))
    .Call(C_import_type_declarations, package_name)
}

#' @export
get_typed_package_names <- function() {
    .Call(get_typed_package_names)
}

#' @export
get_typed_function_names <- function(package_name) {
    stopifnot(is_scalar_character(package_name))
    .Call(C_get_typed_function_names, package_name)
}

#' @export
is_package_typed <- function(package_name) {
    stopifnot(is_scalar_character(package_name))
    .Call(C_is_package_typed, package_name)
}

#' @export
is_function_typed <- function(fun,
                              package_name = get_package_name(fun),
                              function_name = as.character(substitute(fun))) {
    stopifnot(is_scalar_character(package_name))
    stopifnot(is_scalar_character(function_name))
    .Call(C_is_function_typed, package_name, function_name)
}

#' @export
set_type_declaration <- function(fun,
                                 type_declaration,
                                 package_name = get_package_name(fun),
                                 function_name = as.character(substitute(fun))) {
    stopifnot(is_scalar_character(package_name))
    stopifnot(is_scalar_character(function_name))
    stopifnot(is_scalar_character(type_declaration))
    type_declaration <- paste0("type ", function_name, " ", type_declaration, ";")
    invisible(.Call(C_set_type_declaration, package_name, function_name, type_declaration))
}

#' @export
remove_type_declaration <- function(fun,
                                    package_name = get_package_name(fun),
                                    function_name = as.character(substitute(fun))) {
    stopifnot(is_scalar_character(package_name))
    stopifnot(is_scalar_character(function_name))
    .Call(C_remove_type_declaration, package_name, function_name)
}

#' @export
show_function_type_declaration <- function(fun,
                                           package_name = get_package_name(fun),
                                           function_name = as.character(substitute(fun))) {
    stopifnot(is_scalar_character(package_name))
    stopifnot(is_scalar_character(function_name))
    invisible(.Call(C_show_function_type_declaration, package_name, function_name))
}

#' @export
show_package_type_declarations <- function(package_name) {
    stopifnot(is_scalar_character(package_name))
    invisible(.Call(C_show_package_type_declarations, package_name))
}

#' @export
show_type_declarations <- function() {
    invisible(.Call(C_show_type_declarations))
}

