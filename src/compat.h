#ifndef CONTRACTR_COMPAT_H
#define CONTRACTR_COMPAT_H

/* ---------------------------------------------------------------------------
 * R 4.5.0 C API compliance shims.
 *
 * R 4.5.0 removed or hid a number of long-standing entry points that were
 * never formally part of the C API (see "Writing R Extensions", section
 * "Moving into C API compliance"). This header routes every affected use in
 * contractr through a public replacement where an exact equivalent exists,
 * and otherwise keeps using the historical entry point behind a version
 * guard so that the package continues to build on both old and new R.
 *
 * All shims are spelled contractr_<OLDNAME>() so the call sites document that
 * they are going through the compliance layer.
 * ------------------------------------------------------------------------- */

#include <Rversion.h>
#include <Rinternals.h>

#ifndef R_VERSION
#define R_VERSION 0
#endif

/* --- Closure formals ------------------------------------------------------
 * The FORMALS() accessor was dropped from the public headers. R_ClosureFormals()
 * is the public replacement added in R 4.5.0. */
#if R_VERSION >= R_Version(4, 5, 0)
#define contractr_FORMALS(fun) R_ClosureFormals(fun)
#else
#define contractr_FORMALS(fun) FORMALS(fun)
#endif

/* --- Raw frame lookup -----------------------------------------------------
 * Rf_findVarInFrame() is no longer part of the public C API as of R 4.5.0.
 * The public replacements (R_getVar / R_getVarEx) are NOT drop-in equivalents:
 * they force promises (eval) and raise an error on a missing argument. contractr
 * must inspect the *raw* binding -- an unforced PROMSXP, R_MissingArg or
 * R_UnboundValue -- so neither replacement is usable here. The symbol is still
 * exported by libR, so we declare it ourselves for newer R and keep the public
 * declaration on older R. */
#if R_VERSION >= R_Version(4, 5, 0)
#ifdef __cplusplus
extern "C" SEXP Rf_findVarInFrame(SEXP rho, SEXP symbol);
#else
extern SEXP Rf_findVarInFrame(SEXP rho, SEXP symbol);
#endif
#endif
#define contractr_findVarInFrame(rho, symbol) Rf_findVarInFrame((rho), (symbol))

/* --- Promise code (read / write) ------------------------------------------
 * PREXPR() and SET_PRCODE() were removed from the public API. A promise shares
 * the cons-cell memory layout, so its "code" slot is exactly the CDR slot
 * (u.promsxp.expr aliases u.listsxp.cdrval). CDR() / SETCDR() are public API on
 * every supported R version and apply the same write barrier that SET_PRCODE()
 * did, so they are exact equivalents for the argument promises we manipulate
 * (whose code is always source, never byte-compiled). */
#define contractr_PREXPR(prom) CDR(prom)
#define contractr_SET_PRCODE(prom, code) SETCDR((prom), (code))

/* --- Promise environment --------------------------------------------------
 * PRENV() is likewise gone from the public headers. A promise's environment
 * slot (u.promsxp.env) aliases the cons-cell TAG slot, and TAG() is public API
 * on every supported R version. Used to tell a caller-supplied argument (whose
 * promise points at the caller's environment) from an unsupplied one (whose
 * promise is the formal's default, evaluated in the callee's own frame). */
#define contractr_PRENV(prom) TAG(prom)

/* --- Attribute list -------------------------------------------------------
 * ATTRIB() sits behind ENABLE_LEGACY_NONAPI_FUNS as of R 4.5. There is no
 * public way to *enumerate* a value's attributes (Rf_getAttrib() answers only
 * for a symbol you already know), and typing a value means describing all of
 * them, so declare the entry point ourselves: it is still exported by R. */
#if R_VERSION >= R_Version(4, 5, 0)
#ifdef __cplusplus
extern "C" SEXP ATTRIB(SEXP x);
#else
extern SEXP ATTRIB(SEXP x);
#endif
#endif
#define contractr_ATTRIB(x) ATTRIB(x)

#endif /* CONTRACTR_COMPAT_H */
