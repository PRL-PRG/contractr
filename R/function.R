
#' @export
has_contract <- function(fun) {
    id <- injectr:::sexp_address(fun)
    has_injected_function(id)
}


#' @export
insert_contract <- function(fun,
                            type_declaration,
                            fun_name = as.character(substitute(fun)),
                            pkg_name = get_package_name(fun),
                            env = environment(fun)) {
    stopifnot(is_function(fun))
    stopifnot(is_scalar_character(type_declaration))
    stopifnot(is_scalar_character(fun_name))
    stopifnot(is_scalar_character(pkg_name))
    stopifnot(is_environment(env))

    set_type_declaration(fun, type_declaration, pkg_name, fun_name)

    insert_function_contract(fun, fun_name, pkg_name, env)
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


insert_function_contract <- function(fun,
                                     fun_name = as.character(substitute(fun)),
                                     pkg_name = get_package_name(fun),
                                     env = environment(fun)) {
    stopifnot(is_function(fun))
    stopifnot(is_scalar_character(fun_name))
    stopifnot(is_scalar_character(pkg_name))
    stopifnot(is_environment(env))

    if (has_contract(fun)) {
        msg <- sprintf("Contract already inserted in '%s::%s'", pkg_name, fun_name)
        message(msg)
    }
    else {
        id <- injectr:::sexp_address(fun)
        old <- modify_function(fun, fun_name, pkg_name)
        add_injected_function(id, list(env = env,
                                       pkg_name=pkg_name,
                                       fun_name=fun_name,
                                       new=fun,
                                       old=old))
    }

    invisible(NULL)
}


modify_function <- function(fun, fun_name, pkg_name) {
    old <- injectr:::create_duplicate(fun)

    check_params <- create_argval_contract_code(fun_name, pkg_name)
    injectr::inject_code(check_params, fun)

    check_retval <- create_retval_contract_code(fun_name, pkg_name)
    injectr::inject_code(check_retval, fun, where="onexit")

    old
}


create_argval_contract_code <- function(fun_name, pkg_name) {
    substitute({
        ## TODO: sys.calls needs to be handled properly
        .contractr__contract <- .Call(contractr:::C_create_result_contract,
                                      contractr:::get_next_call_id(),
                                      Map(deparse, sys.calls()),
                                      PKG_NAME,
                                      FUN_NAME,
                                      TYPE_INDEX)
        .Call(contractr:::C_insert_function_contract,
              .contractr__contract, sys.function(),
              sys.frame(sys.nframe()))

    }, list(PKG_NAME=pkg_name,
            FUN_NAME=fun_name,
            TYPE_INDEX=.Call(contractr:::C_get_type_index, pkg_name, fun_name)))
}


create_retval_contract_code <- function(fun_name, pkg_name) {
    substitute({
        .contractr__retval__ <- returnValue(contractr:::.no_retval_marker)     # nolint
        if (!identical(.contractr__retval__, contractr:::.no_retval_marker)) {
            .Call(contractr:::C_assert_contract,
                  .contractr__contract,
                  .contractr__retval__,
                  FALSE)
        }
    }, list(PKG_NAME=pkg_name,
            FUN_NAME=fun_name))
}
