# contractr

contractr monitors the calls of an R package at run time. It uses the
set-theoretic types that [tyr](https://github.com/PRL-PRG/tyr) infers for the
functions of the package. contractr adds contracts to each function that has a
type. At the exit of each call, contractr
sends queries to the [rstt](https://github.com/E-Sh4rk/rstt) type engine. The
queries examine if the call is in the domain of the function and if the result
agrees with its type.

contractr records the verdict of each query. For a violation, it gives a
warning or an error, as the severity specifies.

## Installation

A C++17 compiler is necessary. Run this command:

```sh
R CMD INSTALL contractr
```

To monitor calls, contractr must have the `processx` package and the rstt
REPL. contractr finds the REPL in this order:

1. `options(contractr.rstt=)`.
2. The `CONTRACTR_RSTT` environment variable.
3. `rstt` on the `PATH`.

`tyr_rstt_path()` shows the REPL that contractr uses.

## Usage

1. Write the types of the package with tyr. The file has one
   `#| pkg::name : (params) -> result` line for each overload.

   ```sh
   tyr --typedecl pkg.tyr path/to/pkg
   ```

2. Add the contracts to the functions of the package.

   ```r
   library(contractr)
   library(pkg)
   insert_tyr_contract("pkg", "pkg.tyr")
   ```

3. Call the functions of the package. By default, a violation gives a warning.

   ```r
   pkg::f(...)
   ```

4. Read the verdicts: `ok`, `violation` or `unknown`.

   ```r
   get_contracts()
   ```

## Violations

A violation message shows the call site, the arguments and the blame:

- `blame: caller`: the call is not in the domain of the function.
- `blame: callee`: the function returned a value that its type does not permit.

## Other functions

- `set_severity()` sets what contractr does for a violation: `"warning"` (the
  default), `"error"` or `"silence"`.
- `ignore_contracts(expr)` evaluates `expr` without contracts.
- `capture_contracts(expr)` evaluates `expr` and returns its value and its
  records. By default, these records do not go into `get_contracts()`.
- `clear_contracts()` removes all records.
- `has_contract(f)` returns `TRUE` if the function `f` has contracts.
- `remove_contract(f)` removes the contracts of the function `f`.
- `tyr_infer_type(value)` returns the type that contractr gives to `value`.
- `tyr_reset_oracle()` clears the query results and stops the rstt REPL.

## Options

- `contractr.rstt`: the path of the rstt REPL.
- `contractr.rstt.timeout`: the time limit for one query, in seconds. The
  default is 60.
- `contractr.severity`: the severity when R loads contractr. The default is
  `"warning"`.
- `contractr.autoinject`: the packages that contractr monitors automatically
  (see the next section).

## Packages that supply their types

If a package has its types in an `inst/TYR` file, contractr monitors its calls
automatically. The file contains the same `#|` lines. contractr starts to
monitor the package when R loads the package and contractr. The load order is
not important. This also applies to `devtools::load_all()`.

To stop this, set `options(contractr.autoinject = FALSE)` before you load
contractr. After R loads contractr, use `set_autoinject(FALSE)`. This applies
to the packages that R loads after this call. To monitor only some packages,
give their names: `set_autoinject(c("pkgA", "pkgB"))`.

### Types from roxygen annotations

The `type_roclet` roclet writes `inst/TYR` from `@type` annotations. Write one
annotation for each overload:

```r
#' @type (x: dbl) -> dbl
#' @type (x: chr) -> chr
f <- function(x) x
```

To use the roclet, add it to the `Roxygen` field of the DESCRIPTION file:

```
Roxygen: list(roclets = c("collate", "namespace", "rd", "contractr::type_roclet"))
```

The roclet does not change an `inst/TYR` file that it did not write, for
example a file from `tyr --typedecl`. roxygen2 is necessary for the roclet.
For other tasks, contractr does not use roxygen2.
