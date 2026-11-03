is_scalar_character <- function(vector) {
    is.character(vector) && (length(vector) == 1) && (nchar(vector) != 0)
}

is_scalar_logical <- function(vector) {
    is.logical(vector) && (length(vector) == 1)
}

is_function <- function(fun) {
    is.function(fun)
}



