.injected_functions <- new.env(parent=emptyenv())
.no_retval_marker <- new.env(parent=emptyenv())
.call_id <- new.env()

.call_id$counter <- -1L

get_next_call_id <- function() {
    .call_id$counter <- .call_id$counter + 1
    .call_id$counter
}
