

strip_package_prefix <- function(package_names, prefix = "package:") {
    ifelse(startsWith(package_names, prefix),
           substring(package_names, nchar(prefix) + 1),
           package_names)
}

.onLoad <- function(libname, pkgname) {
    handle_package <- function(package_name, ...) {
        fun_names <- insert_package_contract(package_name)
        if (length(fun_names) != 0) {
            message("Added contract to ", length(fun_names), " ", package_name, " function(s)")
        }
    }

    loaded_packages <- strip_package_prefix(search())
    installed_packages <- installed.packages()[, 1]
    remaining_packages <- setdiff(installed_packages, loaded_packages)

    for (package in loaded_packages) {
        handle_package(package)
    }

    for (package in remaining_packages) {
        setHook(packageEvent(package, "attach"), handle_package)
    }
}

.onDetach <- function(libpath) {

    result <- list()

    increment <- function(pkg_name) {
        count <- result[[pkg_name]]
        count <- if (is.null(count)) 1 else count + 1
        result[[pkg_name]] <<- count
    }

    for (id in ls(.injected_functions)) {
        value <- get(id, envir = .injected_functions)
        env <- value$env
        fun_name <- value$fun_name
        pkg_name <- value$pkg_name
        old <- value$old

        is_locked <- bindingIsLocked(fun_name, env)
        if (is_locked) unlockBinding(fun_name, env)
        assign(fun_name, old, envir = env)
        if (is_locked) lockBinding(fun_name, env)

        increment(pkg_name)
    }

    for (pkg_name in names(result)) {
        count <- result[[pkg_name]]
        message("Removed contract from ", count, " ", pkg_name, " function(s)")
    }

    rm(list = ls(.injected_functions), envir = .injected_functions)
}
