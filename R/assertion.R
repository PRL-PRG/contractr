#' @export
get_assertions <- function() {
    .Call(C_get_assertions)
}

#' @export
capture_assertions <- function(code) {
    .Call(C_capture_assertions, as.symbol("code"), environment())
}
