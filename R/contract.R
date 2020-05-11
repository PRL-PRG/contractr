#' @export
get_contracts <- function() {
    .Call(C_get_contracts)
}

#' @export
clear_contracts <- function() {
    .Call(C_clear_contracts)
}

#' @export
capture_contracts <- function(code, separate = TRUE) {
    stopifnot(is_scalar_logical(separate))
    .Call(C_enable_contracts)
    result <- tryCatch(.Call(C_capture_contracts,
                             as.symbol("code"),
                             environment(),
                             separate),
                       error = function(e) {
                           .Call(C_reinstate_contract_status)
                           stop(e)
                       })
    .Call(C_reinstate_contract_status)
    result
}

#' @export
ignore_contracts <- function(code) {
    .Call(C_disable_contracts)
    result <- tryCatch(code,
                       error = function(e) {
                           .Call(C_reinstate_contract_status)
                           stop(e)
                       })
    .Call(C_reinstate_contract_status)
    result
}
