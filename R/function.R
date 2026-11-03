
#' @export
has_contract <- function(fun) {
    id <- injectr:::sexp_address(fun)
    has_injected_function(id)
}


#' @export
remove_contract <- function(fun) {

    stopifnot(is_function(fun))

    if (!has_contract(fun)) {
        message <- sprintf("function `%s` does not have a contract.",
                           as.character(substitute(fun)))
        stop(message)
    }

    remove_function_contract(injectr:::sexp_address(fun))

    invisible(NULL)
}


remove_function_contract <- function(id) {
    value <- get_injected_function(id)

    env <- value$env
    fun_name <- value$fun_name
    pkg_name <- value$pkg_name
    old <- value$old

    is_locked <- bindingIsLocked(fun_name, env)
    if (is_locked) unlockBinding(fun_name, env)
    assign(fun_name, old, envir = env)
    if (is_locked) lockBinding(fun_name, env)

    remove_injected_function(id)

    list(pkg_name, fun_name)
}

