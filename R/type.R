
#' @export
infer_type <- function(value, parameter_name = "parameter") {
    stopifnot(is_scalar_character(parameter_name))

    .Call(C_infer_type, as.symbol("value"), parameter_name, sys.frame(sys.nframe()))
}


#' @export
check_type <- function(value, type, parameter_name = "param") {
    stopifnot(is_scalar_character(parameter_name))
    stopifnot(is_scalar_character(type))

    type <- paste0("type fun <", type, "> => any;")
    .Call(C_check_type, as.symbol("value"), parameter_name, type, sys.frame(sys.nframe()))
}
