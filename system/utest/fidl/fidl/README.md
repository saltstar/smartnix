## FIDL definitions used in unit testing

The FIDL workflow is tested at multiple levels. `fidl_coded_types.cpp` contains hand-written
coding tables for the message types, and their corresponding C structure definitions are found in
`fidl_structs.h`. Most tests in encoding/decoding exercise these manual coding table definitions.
Though not one-to-one generated, `messages.fidl` contains a general outline of the FIDL definitions
under test, for reference.

On the other hand, certain FIDL constructs are used in the higher layers, but are not supported
by the C bindings right now, e.g. tables. `fidlc` is able to generate the coding tables for FIDL
tables, but cannot generate their binding APIs. In order to unit test the table code paths, we will
generate and check in their coding tables `extra_messages.cpp` from `extra_messages.fidl`.

The command to generate `extra_messages.cpp` from `extra_messages.fidl` is:

```bash
make -j8 tools HOST_USE_ASAN=true
./build-x64/tools/fidlc --tables system/utest/fidl/fidl/extra_messages.cpp \
                        --files system/utest/fidl/fidl/extra_messages.fidl
```

The manual generation/checking-in should go away once we have a more flexible build process that
allows a test to declare dependency only on the coding tables, not the C client/server bindings.
Alternatively we could add tables support to C/low-level C++ bindings (FIDL-431).
