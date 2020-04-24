insert_environment_contract <- function(env, envname, unlock, ...) {
    stopifnot(is_environment(env))
    stopifnot(is_scalar_character(envname))
    stopifnot(is_scalar_logical(unlock))

    var_names <- ls(envir=env, ...)

    typed_var_names <- get_typed_function_names(envname)

    required_var_names <- intersect(var_names, typed_var_names)

    modified_var_names <- character(0)

    for (var_name in required_var_names) {

        fun <- get(var_name, envir=env)
        if (!is.function(fun)) next

        is_locked <- bindingIsLocked(var_name, env)
        if (is_locked && !unlock) next

        tryCatch({

            if (is_locked) unlockBinding(var_name, env)
            insert_function_contract(fun, fun_name = var_name, pkg_name = envname, env = env)
            if (is_locked) lockBinding(var_name, env)

            modified_var_names <- c(modified_var_names, var_name)

        }, error = function(e) {
            message <- sprintf("cannot insert contract in `%s::%s`: %s",
                               envname, var_name, e$message)
            stop(message)
        })
    }

    modified_var_names
}


insert_package_contract <- function(package_name,
                                    unlock = TRUE,
                                    without_prefix = character(0)) {
    without_prefix <- c(without_prefix, ".GlobalEnv", "Autoloads", "tools:callr")
    needs_prefix <- !(package_name %in% without_prefix)

    prefixed_package_name <- if (needs_prefix)
                                 add_package_prefix(package_name)
                             else
                                 package_name

    package_env <- as.environment(prefixed_package_name)
    import_type_declarations(package_name)
    insert_environment_contract(package_env, package_name, unlock = unlock)
}
