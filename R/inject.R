#' @export
inject_type_assertion <- function(fun,
                                  type_declaration = NULL,
                                  fun_name = as.character(substitute(fun)),
                                  pkg_name = get_package_name(fun)) {
  stopifnot(is.function(fun))
  stopifnot(is_scalar_character(fun_name))
  stopifnot(is_scalar_character(pkg_name))

  if (!is.null(type_declaration)) {
    stopifnot(is_scalar_character(type_declaration))
    set_type_declaration(fun, type_declaration, pkg_name, fun_name)
  }

  id <- injectr:::sexp_address(fun)

  if (!exists(id, envir=.injected_functions)) {
    check_params <- substitute(
      .Call(
        contractR:::C_inject_type_assertion,
        PKG_NAME,
        FUN_NAME,
        sys.function(),
        sys.frame(sys.nframe())
      ),
      list(PKG_NAME=pkg_name, FUN_NAME=fun_name)
    )

    check_retval <- substitute({
        .__retval__ <- returnValue(contractR:::.no_retval_marker)     # nolint
        if (!identical(.__retval__, contractR:::.no_retval_marker)) {
          contractR:::assert_type(
            .__retval__,
            FALSE,
            PKG_NAME,
            FUN_NAME,
            ".__retval__",
            -1
          )
        }
      },
      list(PKG_NAME=pkg_name, FUN_NAME=fun_name)
    )

    old <- injectr:::create_duplicate(fun)

    injectr::inject_code(check_params, fun)
    injectr::inject_code(check_retval, fun, where="onexit")

    assign(
      id,
      list(pkg_name=pkg_name, fun_name=fun_name, new=fun, old=old),
      envir=.injected_functions
    )
  }

  invisible(NULL)
}

#' @export
inject_environment_type_assertions <- function(pkg_name, env=getNamespace(pkg_name)) {
  stopifnot(is_scalar_character(pkg_name))

  if (is.null(env)) return(NULL)

  for (name in ls(envir=env, all.names=TRUE, sorted=FALSE)) {
    fun <- get(name, envir=env)
    if (is.function(fun)) {
      tryCatch({
        inject_type_assertion(fun, fun_name = name, pkg_name = pkg_name)
      }, error=function(e) {
        warning("Unable to inject type checks into `",
                pkg_name, ":::", name, "`: ", e$message)
      })
    }
  }
}

#' @export
is_type_assertion_injected <- function(f) {
  identical(body(f)[[3]][[2]], quote(contractR:::C_inject_type_assertion))
}

