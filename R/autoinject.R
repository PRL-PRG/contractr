## contractr monitors the calls of a package that has an `inst/TYR` file of `#|`
## signatures. It starts when R loads the second of the two packages, the
## package or contractr (see `.onLoad`).

#' Set the packages that contractr monitors automatically
#'
#' contractr monitors the calls of a package that has a `TYR` file of `#|`
#' signatures (`inst/TYR` in the source). `tyr --typedecl` or [type_roclet()]
#' writes this file. contractr starts to monitor the calls when R loads the
#' package and contractr. The load order is not important.
#'
#' When R loads contractr, contractr uses `options(contractr.autoinject=)`. If
#' this option is not set, it uses `TRUE`.
#'
#' @param autoinject `TRUE` for all packages, `FALSE` for no package, or a
#'   character vector of package names.
#' @return For `get_autoinject()`, the current value.
#' @export
set_autoinject <- function(autoinject) {
    if (missing(autoinject)) {
        autoinject <- getOption("contractr.autoinject", TRUE)
    }
    if (!isTRUE(autoinject) && !isFALSE(autoinject) && !is.character(autoinject)) {
        stop("autoinject should be TRUE, FALSE or a vector of package names, not ",
             deparse(autoinject))
    }
    .state$autoinject <- autoinject
    invisible(autoinject)
}

#' @rdname set_autoinject
#' @export
get_autoinject <- function() {
    .state$autoinject
}

## Returns the declaration file of the package at `path`. This is `TYR` for an
## installed package and `inst/TYR` for a source tree (`pkgload::load_all()`).
## Returns `""` if there is no such file.
tyr_declaration_file <- function(path) {
    for (file in file.path(path, c("TYR", file.path("inst", "TYR")))) {
        if (file.exists(file)) return(file)
    }
    ""
}

## Adds contracts to the loaded `package` if it has declarations and
## `get_autoinject()` includes it.
##
## R also calls this function as a hook when it loads a package. This function
## ignores the second argument of the hook. R gives the directory of the
## package, but pkgload gives the directory above the source tree.
## `find.package()` gives the correct directory in the two conditions.
##
## If an error occurs, this function gives a warning. An error must not stop R
## when it loads the package or contractr.
autoinject <- function(package, ...) {
    wanted <- get_autoinject()
    if (!isTRUE(wanted) && !(package %in% wanted)) return(invisible(FALSE))
    tryCatch({
        file <- tyr_declaration_file(find.package(package))
        if (!nzchar(file)) return(invisible(FALSE))
        insert_tyr_contract(package, file)
        invisible(TRUE)
    }, error = function(e) {
        warning(sprintf("contractr: cannot check the calls of %s: %s",
                        package, conditionMessage(e)), call. = FALSE)
        invisible(FALSE)
    })
}
