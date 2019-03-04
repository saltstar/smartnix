
// For each function in the vDSO ABI, define a symbol in the linker script
// pointing to its address.  The vDSO is loaded immediately after the
// userboot DSO image's last page, which is found by aligning the magic
// symbol _end (generated by the linker) up to the next page boundary.  So
// these symbols tell the linker where each vDSO function will be found at
// runtime.  The userboot code uses normal calls to these, declared as have
// hidden visibility so they won't generate PLT entries.  This results in
// the userboot binary having simple PC-relative calls to addresses outside
// its own image, to where the vDSO will be found at runtime.
#define FUNCTION(name, address, size) \
    PROVIDE_HIDDEN(name = ALIGN(_end, CONSTANT(MAXPAGESIZE)) + address);

#define WEAK_FUNCTION(name, address, size) FUNCTION(name, address, size)