## FIXME: can i assume that call is always a function definition?
get_annotated_function_name <- function(call) {

    name <- NA_character_

    if (typeof(call) == "language" &&
        length(call) == 3 &&
        call[[1]] == "<-" &&
        call[[3]][[1]] == "function") {
        name <- paste0("`", as.character(call[[2]]), "`")
    }

    name
}

#' @importFrom roxygen2 roxy_tag_parse
#' @importFrom roxygen2 roxy_tag_warning

#' @export
roxy_tag_parse.roxy_tag_type <- function(x) {
    parse_result <- list(state = TRUE, error_message = "hello")

    if (!parse_result$state) {
        message <- sprintf("invalid type format: %s", parse_result$error_message)
        roxy_tag_warning(x, message)
        return()
    }

    x$val <- list(type_declaration = x$raw)

    x
}


#' @importFrom roxygen2 roclet
#' @export
type_roclet <- function() {
    roclet("type")
}


#' @importFrom roxygen2 block_get_tags
#' @importFrom roxygen2 roclet_process
#' @export
roclet_process.roclet_type <- function(x, blocks, env, base_path) {
    results <- list()

    for (block in blocks) {

        tags <- block_get_tags(block, "type")

        tag <- NULL

        if (length(tags) > 1) {
            roxy_tag_warning(x, "multiple type tags encountered, only first one will be processed")
            tag <- tags[[1]]
        }
        else if (length(tags) == 1) {
            tag <- tags[[1]]
        }

        if (is.null(tag)) next

        name <- get_annotated_function_name(block$call)

        if (is.na(name)) next

        element <- list(location = list(file = tag$file,
                                        line = tag$line),
                        declaration = list(name = name,
                                           type = tag$val)
        )

        results <- c(results, list(element))
    }

    results
}

#' @importFrom roxygen2 roclet_output
#' @export
roclet_output.roclet_type <- function(x, results, base_path, ...) {
    INST_DIRPATH <- file.path(base_path, "inst")
    TYPEDECL_FILEPATH <- file.path(INST_DIRPATH, "TYPEDECLARATION")

    dir.create(INST_DIRPATH, showWarnings = FALSE)
    file.create(TYPEDECL_FILEPATH)

    for (element in results) {

        comment <- sprintf("# extracted from %s:%s",
                           element$location$file,
                           element$location$line)

        declaration <- sprintf("type %s %s;\n",
                               element$declaration$name,
                               element$declaration$type)

        write(comment, file = TYPEDECL_FILEPATH, append = TRUE)
        write(declaration, file = TYPEDECL_FILEPATH, append = TRUE)
    }

    invisible(NULL)
}
