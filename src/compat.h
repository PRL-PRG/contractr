#ifndef CONTRACTR_COMPAT_H
#define CONTRACTR_COMPAT_H

/* ---------------------------------------------------------------------------
 * Replacements for the C API of R 4.5.0.
 *
 * R 4.5.0 removed or hid some old entry points. These entry points were never
 * a formal part of the C API (see "Writing R Extensions", section "Moving into
 * C API compliance"). This header gives each use in contractr a public
 * replacement if an equivalent exists. If not, it keeps the old entry point
 * behind a version test. Thus the package compiles on old and new versions of R.
 *
 * The name of each replacement is `contractr_<OLDNAME>()`. Thus each call site
 * shows that it uses this header.
 * ------------------------------------------------------------------------- */

#include <Rversion.h>
#include <Rinternals.h>

#ifndef R_VERSION
#define R_VERSION 0
#endif

/* --- Closure formals ------------------------------------------------------
 * R 4.5.0 removed `FORMALS()` from the public headers. `R_ClosureFormals()` is
 * the public replacement that R 4.5.0 added. */
#if R_VERSION >= R_Version(4, 5, 0)
#define contractr_FORMALS(fun) R_ClosureFormals(fun)
#else
#define contractr_FORMALS(fun) FORMALS(fun)
#endif

/* --- Raw frame lookup -----------------------------------------------------
 * From R 4.5.0, `Rf_findVarInFrame()` is not part of the public C API. The
 * public replacements `R_getVar()` and `R_getVarEx()` do not do the same work.
 * They force promises, and they signal an error on a missing argument.
 *
 * contractr must read the raw binding: a promise that R did not force,
 * `R_MissingArg` or `R_UnboundValue`. Thus contractr cannot use the
 * replacements. libR exports the symbol. Thus this header declares it from
 * R 4.5.0. Older versions of R use the public declaration. */
#if R_VERSION >= R_Version(4, 5, 0)
#ifdef __cplusplus
extern "C" SEXP Rf_findVarInFrame(SEXP rho, SEXP symbol);
#else
extern SEXP Rf_findVarInFrame(SEXP rho, SEXP symbol);
#endif
#endif
#define contractr_findVarInFrame(rho, symbol) Rf_findVarInFrame((rho), (symbol))

/* --- Promise code (read only) ---------------------------------------------
 * R removed `PREXPR()` from the public API. A promise has the memory layout
 * of a cons cell, and its code slot is the CDR slot (`u.promsxp.expr` is an
 * alias of `u.listsxp.cdrval`). `CDR()` is public API in all supported R
 * versions.
 *
 * `CDR()` gives the raw code of the promise. If a byte-compiled caller made
 * the promise, this code can be bytecode. `PREXPR()` gave the expression of
 * the bytecode. contractr only examines the type of the code. Thus bytecode
 * does not cause a problem. */
#define contractr_PREXPR(prom) CDR(prom)

/* --- Promise value --------------------------------------------------------
 * `PRVALUE()` is not public. The value slot (`u.promsxp.value`) is an alias of
 * the CAR of the cons cell. It is `R_UnboundValue` until R forces the promise.
 * contractr only reads this slot. It does not force promises itself. */
#define contractr_PRVALUE(prom) CAR(prom)

/* --- Promise environment --------------------------------------------------
 * The public headers do not declare `PRENV()`. The environment slot of a
 * promise (`u.promsxp.env`) is an alias of the TAG slot of the cons cell.
 * `TAG()` is public API in all supported R versions.
 *
 * contractr uses this environment to find the difference between an argument
 * that the caller supplied and a formal bound to its default. The promise of a
 * supplied argument has the environment of the caller. The promise of a default
 * has the frame of the callee. contractr also uses the environment to find the
 * variables in the code of the promise. */
#define contractr_PRENV(prom) TAG(prom)

/* --- Attribute list -------------------------------------------------------
 * From R 4.5, `ATTRIB()` is behind `ENABLE_LEGACY_NONAPI_FUNS`. The public API
 * cannot give a list of all the attributes of a value. `Rf_getAttrib()` only
 * gives the value of an attribute with a name that you know. The type of a
 * value must include all of its attributes. R exports `ATTRIB()`. Thus this
 * header declares it. */
#if R_VERSION >= R_Version(4, 5, 0)
#ifdef __cplusplus
extern "C" SEXP ATTRIB(SEXP x);
#else
extern SEXP ATTRIB(SEXP x);
#endif
#endif
#define contractr_ATTRIB(x) ATTRIB(x)

#endif /* CONTRACTR_COMPAT_H */
