
.injected_functions <- new.env(parent=emptyenv())
.no_retval_marker <- new.env(parent=emptyenv())


strip_package_prefix <- function(package_names, prefix = "package:") {
    ifelse(startsWith(package_names, prefix),
           substring(package_names, nchar(prefix) + 1),
           package_names)
}

.onLoad <- function(libname, pkgname) {
    loaded_packages <- search()
    installed_packages <- installed.packages()[, 1]
    remaining_packages <- setdiff(installed_packages, strip_package_prefix(loaded_packages))

    for (package in loaded_packages) {
        insert_package_contract(package)
    }

    for (package in remaining_packages) {
        setHook(packageEvent(package, "attach"),
                function(pkgname, libname) {
                    insert_package_contract(paste0("package:", pkgname))
                })
    }
}
