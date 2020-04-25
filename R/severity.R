#' @export
set_severity <- function(severity) {
    if (missing(severity)) {
        severity <- getOption("contractr.severity")
        if (is.null(severity)) {
            severity <- "warning"
        }
    }

    if (!(severity %in% c("warning", "error", "silence"))) {
        stop("severity should be 'warning', 'error' or 'silence'; not '", severity, "'")
    }

    invisible(.Call(C_set_severity, severity))
}

#' @export
get_severity <- function() {
    .Call(C_get_severity)
}
