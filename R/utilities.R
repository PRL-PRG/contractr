is_scalar_character <- function(package_name) {
    is.character(package_name) && (length(package_name) == 1) && (nchar(package_name) != 0)
}

get_package_name <- function(fun) {
    name <- environmentName(environment(fun))
    if(name == "R_GlobalEnv") ".GlobalEnv"
    else name
}
