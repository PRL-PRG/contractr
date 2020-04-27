
#' @export
import_type_declarations <- function(package_name) {
    stopifnot(is_scalar_character(package_name))
    filepath <- system.file("TYPEDECLARATION", package = package_name)
    if (filepath == "" || dir.exists(filepath)) {
        filepath <- system.file(file.path("TYPEDECLARATION", package_name),
                                package = "contractR")
    }
    if (filepath == "" || dir.exists(filepath)) {
        msg <- sprintf("No type declarations found for package %s", package_name)
        message(msg)
        character(0)
    } else {
        .Call(C_import_type_declarations, package_name, filepath)
    }
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
show_function_type_declaration <- function(fun,
                                           package_name = get_package_name(fun),
                                           function_name = as.character(substitute(fun)),
                                           style = TRUE) {
    stopifnot(is_scalar_character(package_name))
    stopifnot(is_scalar_character(function_name))
    stopifnot(is_scalar_logical(style))
    invisible(.Call(C_show_function_type_declaration, package_name, function_name, style))
}

#' @export
show_package_type_declarations <- function(package_name,
                                           style = TRUE) {
    stopifnot(is_scalar_character(package_name))
    stopifnot(is_scalar_logical(style))
    invisible(.Call(C_show_package_type_declarations, package_name, style))
}

#' @export
show_type_declarations <- function(style = TRUE) {
    stopifnot(is_scalar_logical(style))
    invisible(.Call(C_show_type_declarations, style))
}
