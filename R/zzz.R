
.onLoad <- function(libname, pkgname) {

    set_severity()
    set_autoinject()

    handle_package <- function(package_name, ...) {
        autoinject_packages <- get_autoinject()
        if (package_name %in% autoinject_packages || any(autoinject_packages == "all")) {
            tryCatch(
                fun_names <- insert_package_contract(package_name),
                error = function(e) {
                    expected_message <- sprintf("there is no package called '%s'", package_name)
                    if (length(grep(expected_message, e$message)) == 1) {
                        message(expected_message)
                    }
                    else {
                        stop(e)
                    }
                }
            )
            if (length(fun_names) != 0) {
                message("Added contract to ", length(fun_names), " ", package_name, " function(s)")
            }
        }
    }

    remove_packages <- c(".GlobalEnv", "Autoloads", "tools:callr", "tools:rstudio")
    loaded_packages <- setdiff(remove_package_prefix(search()), remove_packages)
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

    for (id in get_injected_function_ids()) {
        result <- remove_function_contract(id)
        increment(result$pkg_name)
    }

    for (pkg_name in names(result)) {
        count <- result[[pkg_name]]
        message("Removed contract from ", count, " ", pkg_name, " function(s)")
    }
}
