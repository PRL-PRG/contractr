is_scalar_character <- function(package_name) {
    is.character(package_name) && length(package_name) == 1
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
is_function_typed <- function(package_name, function_name) {
    stopifnot(is_scalar_character(package_name))
    stopifnot(is_scalar_character(function_name))
    .Call(C_is_function_typed, package_name, function_name)
}
