# Embedded coding conventions

Applies to active first-party C/C++ code, including examples and native tests.

- Every function has at most one `return`, as its last statement at function
  body scope. Void functions, constructors and destructors may have none.
- Precede function definitions in C/C++ sources and Arduino sketches with a
  three-line block-comment separator: an asterisk rule, the function name
  (class-qualified for out-of-class methods), and a closing asterisk rule.
  Keep separator rules at 80 columns including indentation; preserve Doxygen.
- Use explicit initialized result/status variables and small helper functions
  for error paths. Do not replace early returns with goto or exit macros.
- Reuse `lib/PlotterLib/src/common/` for bit masks, bitsets, wire byte conversion
  and CRC. Keep protocol constants/semantics in the protocol layer.
- Use unsigned fixed-width operands for bit operations. Check runtime shift
  counts. Macros must parenthesize operands and evaluate each argument once;
  use typed static inline functions for operations requiring validation.
- Preserve bounded execution, no heap in the core, and explicit transport
  buffer ownership. Do not claim safety-standard compliance from style checks.
- Keep the entire PlotterLib package platform-independent, including optional
  C++ wrappers. No SDK headers, vendor types or target-dependent API branches.
  Hardware adapters live in application/example code and use callbacks or
  PlotterStream. Target macros must not alter the library's public API/layout.
- Run `python tools/test_native.py --app ../PlotterApp` and affected PlatformIO
  targets after changes. This runs the single-return structural check too.
- Document active headers with Doxygen: brief, detailed contracts and Usage for
  macros, types, fields and functions; parameter directions and return semantics
  for callable interfaces. Keep `Doxyfile` inputs current and run Doxygen after
  header documentation changes. Generated output belongs under `docs/api/`.
- `legacy/` is archived, excluded from builds, and not production code.
  `lib/Events` is an independent third-party submodule; do not rewrite it as
  part of PlotterLib style changes.
