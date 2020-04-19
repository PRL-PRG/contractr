
#' @export
infer_type <- function(value, parameter_name = "parameter") {
    stopifnot(is_scalar_character(parameter_name))
    .Call(C_infer_type, as.symbol("value"), parameter_name, sys.frame(sys.nframe()))
}
