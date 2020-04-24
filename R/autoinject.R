
#' @export
set_autoinject <- function(autoinject) {
    if (missing(autoinject)) {
        autoinject <- getOption("contractr.autoinject")
        if (is.null(autoinject)) {
            autoinject <- TRUE
        }
    }

    if (!is_scalar_logical(autoinject)) {
        stop("autoinject should be TRUE or FALSE; not ", autoinject)
    }

    .state$autoinject <- autoinject
}

#' @export
should_autoinject <- function() {
    .state$autoinject
}
