UNDEFINED_STRING_VALUE <- "<undefined>"


#' @export
infer_type <- function(value, parameter_name = UNDEFINED_STRING_VALUE) {
    stopifnot(is_scalar_character(parameter_name))

    .Call(C_infer_type, as.symbol("value"), parameter_name, sys.frame(sys.nframe()))
}


#' @export
check_type <- function(value, type, parameter_name = UNDEFINED_STRING_VALUE) {
    stopifnot(is_scalar_character(parameter_name))
    stopifnot(is_scalar_character(type))

    type <- paste0("type fun <", type, "> => any;")
    .Call(C_check_type, as.symbol("value"), parameter_name, type, sys.frame(sys.nframe()))
}

#' @export
parse_type <- function(type) {
    stopifnot(is_scalar_character(type))
    type <- sub("^\\(", "<", type)
    type <- sub(") =>", "> =>", type, fixed=TRUE)
    type <- paste0("type t ", type, ";")

    .Call(C_parse_type, type)
}

#' @export
is_function_type <- function(type) {
    stopifnot(inherits(type, "tastr"))

    .Call(C_is_function_type, type)
}

#' @export
is_class_type <- function(type) {
    stopifnot(inherits(type, "tastr"))

    .Call(C_is_class_type, type)
}

#' @export
get_classes <- function(type) {
    stopifnot(inherits(type, "tastr"))

    .Call(C_get_classes, type)
}

#' @export
get_parameter_type <- function(type, param) {
    stopifnot(inherits(type, "tastr"))
    stopifnot(is_scalar_integer(param))
    stopifnot(param >= 0)

    .Call(C_get_parameter_type, type, param)
}

#' @export
format.tastr <- function(x, ...) {
    type <- .Call(C_type_to_sexp_string, x)
    type <- sub("^\\s+", "", type)
    type <- sub("^<", "(", type)
    type <- sub("> =>", ") =>", type, fixed=TRUE)
    type
}

#' @export
print.tastr <- function(x, ...) {
    print(format.tastr(x))
}
