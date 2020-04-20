insert_contract <- function(pkgname) {
    pkgenv <- as.environment(pkgname)

    package_prefix <- "package:"

    if (startsWith(pkgname, package_prefix)) {
        pkgname <- substring(pkgname, nchar(package_prefix) + 1)
    }

    bindings <- ls(pkgenv)
    typed_bindings <- import_type_declarations(pkgname)
    required_bindings <- intersect(bindings, typed_bindings)

    for (binding in required_bindings) {
        closure <- get(binding, pkgenv)

        is_locked <- bindingIsLocked(binding, pkgenv)
        if (is_locked) unlockBinding(binding, pkgenv)
        inject_type_check_call(closure,
                               fun_name = binding,
                               pkg_name = pkgname)
        if (is_locked) lockBinding(binding, pkgenv)
    }
}

.onLoad <- function(libname, pkgname) {
    loaded_packages <- search()
    installed_packages <- installed.packages()[, 1]

    for (package in loaded_packages) {
        insert_contract(package)
    }

    for (package in setdiff(installed_packages, loaded_packages)) {
        setHook(packageEvent(package, "attach"),
                function(pkgname, libname) {
                    insert_contract(paste0("package:", pkgname))
                })
    }
}
