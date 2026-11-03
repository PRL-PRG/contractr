.onLoad <- function(libname, pkgname) {
    set_severity()
    set_autoinject()

    ## Monitor the packages that R loaded before contractr. Then add a hook that
    ## monitors each other installed package when R loads it.
    for (package in loadedNamespaces()) {
        autoinject(package)
    }
    installed <- basename(list.dirs(.libPaths(), recursive = FALSE))
    for (package in setdiff(installed, loadedNamespaces())) {
        setHook(packageEvent(package, "onLoad"), autoinject)
    }
}

.onUnload <- function(libpath) {
    rstt_stop()
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
