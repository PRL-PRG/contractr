.injected_functions <- new.env(parent=emptyenv())

.no_retval_marker <- new.env(parent=emptyenv())

.state <- new.env(parent=emptyenv())
.state$call_id <- -1L
.state$autoinject <- getOption("contractr.autoinject", FALSE)
.state$autoinject_blacklist <- c("tools:callr", "tools:rstudio", "org:r-libs")

get_next_call_id <- function() {
    .state$call_id <- .state$call_id + 1
    .state$call_id
}

add_injected_function <- function(key, value) {
    assign(key, value, envir=.injected_functions)
}

get_injected_function <- function(key) {
    get(key, envir=.injected_functions)
}

has_injected_function <- function(key) {
    exists(key, envir=.injected_functions)
}

get_injected_function_ids <- function() {
    ls(envir=.injected_functions, all.names = TRUE)
}

remove_injected_function <- function(key) {
    rm(list = key, envir=.injected_functions)
}

get_injected_function_count <- function() {
    length(get_injected_function_ids())
}
