
#' @export
set_autoinject <- function(autoinject) {
    if (missing(autoinject)) {
        autoinject <- getOption("contractr.autoinject")
        if (is.null(autoinject)) {
            autoinject <- TRUE
        }
    }

    if (!is_scalar_logical(autoinject) && !is_vector_character(autoinject)) {
        stop("autoinject should be either a logical or vector of package names, not ", autoinject)
    }

    .state$autoinject <- autoinject
}

#' @export
get_autoinject <- function() {
    .state$autoinject
}

#' @exprot
get_autoinject_blacklist <- function() {
    .state$autoinject_blacklist
}
