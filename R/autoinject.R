
#' @export
set_autoinject <- function(autoinject) {
    if (missing(autoinject)) {
        autoinject <- getOption("contractr.autoinject")
        if (is.null(autoinject)) {
            autoinject <- "all"
        }
    }

    if (!is_vector_character(autoinject)) {
        stop("autoinject should be 'all' or vector of package names, not ", autoinject)
    }

    .state$autoinject <- autoinject
}

#' @export
get_autoinject <- function() {
    .state$autoinject
}
