insert_environment_contract <- function(env, envname, ...) {
    stopifnot(is_environment(env))
    stopifnot(is_scalar_character(envname))

    var_names <- ls(envir=env, ...)

    typed_var_names <- get_typed_function_names(envname)

    required_var_names <- intersect(var_names, typed_var_names)

    modified_var_names <- character(0)

    for (var_name in required_var_names) {

        fun <- get(var_name, envir=env)
        if (!is.function(fun)) next

        tryCatch({
            insert_function_contract(fun, fun_name = var_name, pkg_name = envname, env = env)
            modified_var_names <- c(modified_var_names, var_name)

        }, error = function(e) {
            message <- sprintf("cannot insert contract in `%s::%s`: %s",
                               envname, var_name, e$message)
            stop(message)
        })
    }

    modified_var_names
}

insert_package_contract <- function(package_name) {
    stopifnot(is_scalar_character(package_name))

    package_env <- getNamespace(package_name)
    import_type_declarations(package_name)

    insert_environment_contract(package_env, package_name)
}
