
/*
 * This is an input linker script used in linking the vDSO.
 * The kernel can switch vDSO symbols at boot time to alternate entry points.
 * The alternate code is at hidden symbols.  The linker will GC these
 * entry points without some reference chain from an exported symbol.
 * This file injects synthetic references to keep the symbols alive.
 */
EXTERN(CODE_soft_ticks_get)
