#' @export
inject_type_check_call <- function(fun, fun_name, pkg_name) {
  stopifnot(is.function(fun))
  stopifnot(is.character(fun_name) && length(fun_name) == 1)
  stopifnot(is.character(pkg_name) && length(pkg_name) == 1)

  id <- injectr:::sexp_address(fun)

  if (!exists(id, envir=.injected_functions)) {
    code <- substitute(
      .Call(
        contractR:::inject_type_check,
        PKG_NAME,
        FUN_NAME,
        sys.function(),
        sys.frame(sys.nframe())
      ),
      list(PKG_NAME=pkg_name, FUN_NAME=fun_name)
    )

    old <- injectr:::create_duplicate(fun)

    injectr::inject_code(code, fun)

    assign(
      id,
      list(pkg_name=pkg_name, fun_name=fun_name, new=fun, old=old),
      envir=.injected_functions
    )
  }

  invisible(NULL)
}

#' @export
inject_type_check_calls <- function(pkg_name, env=getNamespace(pkg_name)) {
  stopifnot(is.character(pkg_name) && length(pkg_name) == 1)

  if (is.null(env)) return(NULL)

  for (name in ls(envir=env, all.names=TRUE, sorted=FALSE)) {
    fun <- get(name, envir=env)
    if (is.function(fun)) {
      tryCatch({
        inject_type_check_call(fun, name, pkg_name)
      }, error=function(e) {
        warning("Unable to inject type checks into `",
                pkg_name, ":::", name, "`: ", e$message)
      })
    }
  }
}

#' @export
is_type_check_injected <- function(f) {
  identical(body(f)[[2]][[2]], quote(contractR:::inject_type_check))
}

.injected_functions <- new.env(parent=emptyenv())
