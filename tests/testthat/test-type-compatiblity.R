test_that("type inference for ... works", {

    expect_type_compatibility(NULL, "...")

    expect_type_compatibility(, "...")

    expect_type_compatibility(2, "...")

})


test_that("type inference for missing values works", {

    expect_type_compatibility()

})


test_that("type inference for null values works", {

    expect_type_compatibility(NULL)

})


test_that("type inference for list values works", {

    expect_type_compatibility(list(NULL, NULL, NULL, NULL, NULL, NULL))

    ## FIXME
    ## expect_type_compatibility(list(1, 2, "3", NULL, list(1, 2), list(TRUE, FALSE)))

    expect_type_compatibility(create_tuple(0))

    ## FIXME
    ## expect_type_compatibility(list(1, "3", NULL, list(1, 2), list(1, "2", TRUE, list(1, 2), max)))

    expect_type_compatibility(create_struct(0))

    expect_type_compatibility(create_struct(1))

    ## FIXME
    ## expect_type_compatibility(create_struct(1, NA))

    ## FIXME
    ## object <- list(name1 = 1, "3", name3 = NULL, list(1, 2), `another name` = list(1, "2", TRUE, list(1, 2), max))
    ## expect_type_compatibility(object)

    ## FIXME
    ## names(object) <- NA
    ## expect_type_compatibility(object)

})


test_that("type inference for function values works", {

    expect_type_compatibility(max)

    expect_type_compatibility(`if`)

    expect_type_compatibility(function(x) x)

})


test_that("type inference for integer values works", {

    expect_type_compatibility(integer())

    expect_type_compatibility(2L)

    expect_type_compatibility(c(2L, 3L))

    expect_type_compatibility(NA_integer_)

    expect_type_compatibility(c(32L, NA_integer_))

})


test_that("type inference for character values works", {

    expect_type_compatibility(character())

    expect_type_compatibility("c")

    expect_type_compatibility(c("2", '3'))

    expect_type_compatibility(NA_character_)

    expect_type_compatibility(c("32", NA_character_))

})


test_that("type inference for complex values works", {

    expect_type_compatibility(complex())

    expect_type_compatibility(2 + 2i)

    expect_type_compatibility(c(2 + 1i, 3))

    expect_type_compatibility(NA_complex_)

    expect_type_compatibility(c(32 + 9i, NA_complex_))

})


test_that("type inference for double values works", {

    expect_type_compatibility(double())

    expect_type_compatibility(2)

    expect_type_compatibility(c(2, 3))

    expect_type_compatibility(NA_real_)

    expect_type_compatibility(c(32, NA_real_))

})


test_that("type inference for logical values works", {

    expect_type_compatibility(logical())

    expect_type_compatibility(TRUE)

    expect_type_compatibility(c(TRUE, FALSE))

    expect_type_compatibility(na_logical_)

    expect_type_compatibility(c(TRUE, na_logical_))

})


test_that("type inference for raw values works", {

    expect_type_compatibility(raw())

    expect_type_compatibility(as.raw(40))

    expect_type_compatibility(as.raw(c(2, 3)))

})


test_that("type inference for environment values works", {

    expect_type_compatibility(new.env())

})


test_that("type inference for expression values works", {

    expect_type_compatibility(expression(1 + 2))

})


test_that("type inference for language values works", {

    expect_type_compatibility(quote(1 + 2))

})


test_that("type inference for symbol values works", {

    expect_type_compatibility(as.symbol("sym"))

})


test_that("type inference for pairlist values works", {

    expect_type_compatibility(pairlist(1, 2, 3))

})


test_that("type inference for S4 values works", {

    expect_type_compatibility(create_s4())

})


test_that("type inference for weakref values works", {

    expect_type_compatibility(create_weakref())

})

