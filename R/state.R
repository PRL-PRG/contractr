.injected_functions <- new.env(parent=emptyenv())

.no_retval_marker <- new.env(parent=emptyenv())

.state <- new.env()
.state$call_id <- -1L
.state$autoinject <- TRUE

get_next_call_id <- function() {
    .state$call_id <- .state$call_id + 1
    .state$call_id
}
