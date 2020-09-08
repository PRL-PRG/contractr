
expect_typechecking_warnings <- function(object, ..., specs) {
    pattern <- paste0("expected:", "\\w+", expected, "\n", ".*", "actual:", "\\w+", actual)
    expect_warning(object(...), pattern)
}

typecheck <- function(object, pkg_name = ".GlobalEnv") {
    list(fun = object,
         pkg_name = pkg_name,
         name = as.character(substitute(object)),
         call_expression = "<undefined>",
         warnings = character(0))
}

apply_arguments <- function(object, ...) {
    object$warnings <- capture_warnings(object$fun(...))

    arguments <- list(...)
    call_list <- c(as.symbol(object$name), arguments)
    object$call_expression <- deparse(as.call(call_list))

    object
}

expect_contract_violations <- function(object, expected_count) {
    actual_count <- length(object$warnings)
    if (expected_count != actual_count) {
        fail(message = stringr::str_glue("`{call_expression}` produced {actual_count} warnings, {expected_count} expected",
                                         call_expression = object$call_expression,
                                         actual_count = actual_count,
                                         expected_count = expected_count))
    }

    object
}

expect_arg_contract_violation <- function(object, index, expected_type, actual_type) {
    parameter_name <- names(formals(object$fun))[index]
    patterns <- c(
        stringr::str_glue("parameter\\s+\\Q'{parameter_name}'\\E", parameter_name = parameter_name),
        stringr::str_glue("position\\s+\\Q{index}\\E", index = index),
        stringr::str_glue("expected.*\\Q{expected_type}\\E\\n", expected_type = expected_type),
        stringr::str_glue("actual.*\\Q{actual_type}\\E\\n", actual_type = actual_type)
    )

    warning_message <- object$warnings[index]

    result <-
        patterns %>%
        purrr::map_lgl(function(pattern) stringr::str_detect(warning_message, stringr::regex(pattern)))

    if (!all(result)) {
        expected_pattern <- (patterns[!result])[1]
        error_message <- stringr::str_glue("`{call_expression}` produced unexpected warnings.",
                                           "Expected match: {expected_pattern}",
                                           "Actual values:",
                                           "* {warning_message}",
                                           .sep = "\n",
                                           call_expression = object$call_expression,
                                           expected_pattern = expected_pattern,
                                           warning_message = warning_message)

        fail(message = error_message)
    }

    succeed()
    object
}

expect_ret_contract_violation <- function(object, expected_type, actual_type) {
    patterns <- c(
        stringr::str_glue("return value"),
        stringr::str_glue("expected.*\\Q{expected_type}\\E\\n", expected_type = expected_type),
        stringr::str_glue("actual.*\\Q{actual_type}\\E\\n", actual_type = actual_type)
    )

    warning_message <- tail(object$warnings, n = 1)

    result <-
        patterns %>%
        purrr::map_lgl(function(pattern) stringr::str_detect(warning_message, stringr::regex(pattern)))

    if (!all(result)) {
        expected_pattern <- (patterns[!result])[1]
        error_message <- stringr::str_glue("`{call_expression}` produced unexpected warnings.",
                                           "Expected match: {expected_pattern}",
                                           "Actual values:",
                                           "* {warning_message}",
                                           .sep = "\n",
                                           call_expression = object$call_expression,
                                           expected_pattern = expected_pattern,
                                           warning_message = warning_message)

        fail(message = error_message)
    }

    succeed()
    object
}

create_s4 <- function() {
    setClass("Person", representation(name = "character", age = "numeric"))
    john_doe <- new("Person", name = "John Doe", age = 30)
}

create_weakref <- function() {
    new_weakref(new.env())
}

create_list <- function(...) {
    list(...)
}

na_logical_ <- as.logical(NA_integer_)

expect_type_compatibility <- function(object, parameter_name = "parameter_name") {
    if (missing(object)) {
        expect_true(check_type(, infer_type(, parameter_name), parameter_name))
    }
    else {
        ## 1. Capture object and label
        act <- quasi_label(rlang::enquo(object), arg = "object")

        ## 2. Call expect()
        inferred_type <- infer_type(act$val, parameter_name)
        checked_result <- check_type(act$val, inferred_type, parameter_name)

        ## TODO: show meaningful error message if expectation fails
        if (!checked_result) {
            print(inferred_type)
        }

        expect_true(checked_result)

        ## 3. Invisibly return the value
        invisible(act$val)
    }
}
