#' @export
inject_type_assertion <- function(fun,
                                  type_declaration = NULL,
                                  fun_name = as.character(substitute(fun)),
                                  pkg_name = get_package_name(fun),
                                  env = environment(fun)) {
  stopifnot(is.function(fun))
  stopifnot(is_scalar_character(fun_name))
  stopifnot(is_scalar_character(pkg_name))

  if (!is.null(type_declaration)) {
    stopifnot(is_scalar_character(type_declaration))
    set_type_declaration(fun, type_declaration, pkg_name, fun_name)
  }

  id <- injectr:::sexp_address(fun)

  if (!exists(id, envir=.injected_functions)) {
      check_params <- substitute({
      .contractr__call_id__ <- contractR:::get_next_call_id();
      .contractr__parameter_count__ <- length(formals(sys.function()))
      .contractr__call_trace__ <- contractR:::concatenate_call_trace(sys.calls())
      .Call(
        contractR:::C_inject_type_assertion,
        .contractr__call_trace__,
        PKG_NAME,
        FUN_NAME,
        .contractr__call_id__,
        .contractr__parameter_count__,
        sys.function(),
        sys.frame(sys.nframe())
      )},
      list(PKG_NAME=pkg_name, FUN_NAME=fun_name)
    )

    check_retval <- substitute({
        .contractr__retval__ <- returnValue(contractR:::.no_retval_marker)     # nolint
        if (!identical(.contractr__retval__, contractR:::.no_retval_marker)) {
          .Call(contractR:::C_assert_type,
                .contractr__retval__,
                FALSE,
                .contractr__call_trace__,
                PKG_NAME,
                FUN_NAME,
                .contractr__call_id__,
                ".contractr__retval__",
                .contractr__parameter_count__,
                -1)
        }
      },
      list(PKG_NAME=pkg_name, FUN_NAME=fun_name)
    )

    old <- injectr:::create_duplicate(fun)

    injectr::inject_code(check_params, fun)
    injectr::inject_code(check_retval, fun, where="onexit")

    assign(
      id,
      list(env = env, pkg_name=pkg_name, fun_name=fun_name, new=fun, old=old),
      envir=.injected_functions
    )
  }

  invisible(NULL)
}

#' @export
inject_environment_type_assertions <- function(env,
                                               env_name = "<unnamed environment>",
                                               unlock = FALSE) {
    stopifnot(is_environment(env))
    stopifnot(is_scalar_character(env_name))
    stopifnot(is_scalar_logical(unlock))

    var_names <- ls(envir=env, all.names=TRUE, sorted=FALSE)

    typed_var_names <- import_type_declarations(env_name)

    required_var_names <- intersect(var_names, typed_var_names)

    modified_var_names <- c()

    for (var_name in required_var_names) {

        fun <- get(var_name, envir=env)
        if (!is.function(fun)) next

        is_locked <- bindingIsLocked(var_name, env)
        if (is_locked && !unlock) next

        tryCatch({

            if (is_locked) unlockBinding(var_name, env)
            inject_type_assertion(fun, fun_name = var_name, pkg_name = env_name, env = env)
            if (is_locked) lockBinding(var_name, env)

            modified_var_names <- c(modified_var_names, var_name)

        }, error=function(e) {
            warning("Unable to insert contracts into `",
                    env_name, ":::", var_name, "`: ", e$message)
        })
    }

    modified_var_names
}

#' @export
insert_package_contract <- function(package_name) {
    needs_prefix <- !(package_name %in% c(".GlobalEnv", "Autoloads", "tools:callr"))
    if (needs_prefix) {
        package_name <- paste0("package:", package_name)
    }
    package_env <- as.environment(package_name)
    inject_environment_type_assertions(package_env,
                                       strip_package_prefix(package_name),
                                       unlock = TRUE)

}


#' @export
is_type_assertion_injected <- function(fun) {
    id <- injectr:::sexp_address(fun)
    exists(id, envir = .injected_functions)
}

#' @export
get_contract_assertions <- function() {
    .Call(C_get_contract_assertions)
}

