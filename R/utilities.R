is_scalar_character <- function(vector) {
    is.character(vector) && (length(vector) == 1) && (nchar(vector) != 0)
}

is_scalar_logical <- function(vector) {
    is.logical(vector) && (length(vector) == 1)
}

is_environment <- function(env) {
    is.environment(env)
}

get_package_name <- function(fun) {
    name <- environmentName(environment(fun))
    if (name == "R_GlobalEnv") ".GlobalEnv"
    else name
}
