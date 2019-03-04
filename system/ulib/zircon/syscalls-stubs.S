
// Compatibility aliases

#define ALIAS(oldname, newname) \
.globl oldname ;\
.type oldname,STT_FUNC ;\
oldname = newname ;\

// Currently no aliases!
