#' @export
get_assertions <- function() {
    .Call(C_get_assertions)
}
