#' @export
get_contracts <- function() {
    .Call(C_get_contracts)
}

#' @export
capture_contracts <- function(code, separate = TRUE) {
    stopifnot(is_scalar_logical(separate))
    .Call(C_capture_contracts, as.symbol("code"), environment(), separate)
}
