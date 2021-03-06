
#include <asm.h>
#include <arch/x86/asm.h>
#include <arch/x86/descriptor.h>
#include <arch/x86/mmu.h>
#include <arch/x86/registers.h>
#include <zircon/tls.h>

#define ADDR_OFFSET_MASK ((1 << ADDR_OFFSET)-1)
#define SHIFT_OFFSET(_s) ((_s) >> 3)
#define SHIFT_REMAIN(_s) ((_s) - (SHIFT_OFFSET(_s) << 3))

// Set a page table entry for the kernel module relocated 64-bit virtual
// address in 32-bit code. Clobbers the %ecx register.
.macro set_relocated_page_table_entry table, shift, value
    // Extract 32-bit chunk of kernel_relocated_base containing the index bits
    // for this page level shift.
    mov PHYS(kernel_relocated_base + SHIFT_OFFSET(\shift)), %ecx

    // Get the exact portion of the 32-bit value that is the index
    shrl $SHIFT_REMAIN(\shift), %ecx
    andl $ADDR_OFFSET_MASK, %ecx

    // Get the address on the page table of index * 8 and set the value
    shll $3, %ecx
    addl $PHYS(\table), %ecx
    movl \value, (%ecx)
.endm

// This section name is known specially to kernel.ld and gen-kaslr-fixups.sh.
// This code has relocations for absolute physical addresses, which do not get
// adjusted by the boot-time fixups (which this code calls at the end).
.section .text.boot, "ax", @progbits
.align 8
FUNCTION_LABEL(_start)
    /* set up a temporary stack pointer */
    mov $PHYS(_kstack_end), %rsp

    // Save off the bootdata pointer in a register that won't get clobbered.
    mov %esi, %ebx

    // The fixup code in image.S runs in 64-bit mode with paging enabled,
    // so we can't run it too early.  But it overlaps the bss, so we move
    // it before zeroing the bss.  We can't delay zeroing the bss because
    // the page tables we're about to set up are themselves in bss.

    // The first word after the kernel image (at __data_end in our view)
    // gives the size of the following code.  Copy it to _end.
    mov PHYS(__data_end), %ecx
    mov $PHYS(__data_end+4), %esi
    mov $PHYS(_end), %edi
    rep movsb // while (ecx-- > 0) *edi++ = *esi++;

    // Now it's safe to zero the bss.
    movl $PHYS(__bss_start), %edi
    movl $PHYS(_end), %ecx
    sub %edi, %ecx              // Compute the length of the bss in bytes.
    xor %eax, %eax
    rep stosb // while (ecx-- > 0) *edi++ = al;

    // _zbi_base is in bss, so now it's safe to set it.
    mov %ebx, PHYS(_zbi_base)

    /* give the boot allocator a chance to compute the physical address of the kernel */
    call boot_alloc_init

.Lpaging_setup64:
    /* initialize the default page tables */
    /* Setting the First PML4E with a PDP table reference*/
    movl $PHYS(pdp), %eax
    orl  $X86_KERNEL_PD_FLAGS, %eax
    movl %eax, PHYS(pml4)

    /* Setting the First PDPTE with a Page table reference*/
    movl $PHYS(pte), %eax
    orl  $X86_KERNEL_PD_FLAGS, %eax
    movl %eax, PHYS(pdp)

    /* point the pml4e at the second high PDP (for -2GB mapping) */
    movl $PHYS(pdp_high),   %eax
    orl  $X86_KERNEL_PD_FLAGS, %eax
    set_relocated_page_table_entry pml4, PML4_SHIFT, %eax

    /* point the second pdp at the same low level page table */
    movl $PHYS(pte), %eax
    orl  $X86_KERNEL_PD_FLAGS, %eax
    set_relocated_page_table_entry pdp_high, PDP_SHIFT, %eax

    /* map the first 1GB in this table */
    movl $PHYS(pte), %esi
    movl $0x200, %ecx
    xor  %eax, %eax

0:
    mov  %eax, %ebx
    shll $21, %ebx
    orl  $X86_KERNEL_PD_LP_FLAGS, %ebx
    movl %ebx, (%esi)
    addl $8,%esi
    inc  %eax
    loop 0b

    /* set up a linear map of the first 64GB at 0xffffff8000000000 */
    movl $PHYS(linear_map_pdp), %esi
    movl $32768, %ecx
    xor  %eax, %eax

    /* loop across these page tables, incrementing the address by 2MB */
0:
    mov  %eax, %ebx
    shll $21, %ebx
    orl  $X86_KERNEL_PD_LP_FLAGS, %ebx    // lower word of the entry
    movl %ebx, (%esi)
    mov  %eax, %ebx
    shrl $11, %ebx      // upper word of the entry
    movl %ebx, 4(%esi)
    addl $8,%esi
    inc  %eax
    loop 0b

    /* point the high pdp at our linear mapping page tables */
    movl $PHYS(pdp_high), %esi
    movl $64, %ecx
    movl $PHYS(linear_map_pdp), %eax
    orl  $X86_KERNEL_PD_FLAGS, %eax

0:
    movl %eax, (%esi)
    add  $8, %esi
    addl $4096, %eax
    loop 0b

    /*
     * Set PGE to enable global kernel pages
     */
    mov   %cr4, %rax
    or    $(X86_CR4_PGE), %rax
    mov   %rax, %cr4

    /* load the physical pointer to the top level page table */
    mov  $PHYS(pml4), %rax
    mov  %rax, %cr3

    // Load our new GDT by physical pointer.
    // _temp_gdtr has it as a virtual pointer, so copy it and adjust.
    movw PHYS(_temp_gdtr), %ax
    movl PHYS(_temp_gdtr+2), %ecx
    sub $PHYS_ADDR_DELTA, %ecx
    movw %ax, -6(%esp)
    movl %ecx, -4(%esp)
    lgdt -6(%esp)

    /* long jump to our code selector and the high address relocated */
    push  $CODE_64_SELECTOR
    mov  $PHYS(high_entry), %rax
    addq PHYS(kernel_relocated_base), %rax
    pushq %rax
    lretq

// This code runs at the final virtual address, so it should be pure PIC.
.text
high_entry:
    /* zero our kernel segment data registers */
    xor %eax, %eax
    mov %eax, %ds
    mov %eax, %es
    mov %eax, %fs
    mov %eax, %gs
    mov %eax, %ss

    /* load the high kernel stack */
    lea _kstack_end(%rip), %rsp

    // move_fixups_and_zero_bss copied the fixup code to _end.
    // It expects %rdi to contain the actual runtime address of __code_start.
    lea __code_start(%rip), %rdi
    call _end
    // The fixup code won't be used again, so the memory can be reused now.

    /* reload the gdtr after relocations as it relies on relocated VAs */
    lgdt _temp_gdtr(%rip)

    // Set %gs.base to &bp_percpu.  It's statically initialized
    // with kernel_unsafe_sp set, so after this it's safe to call
    // into C code that might use safe-stack and/or stack-protector.
    lea bp_percpu(%rip), %rax
    mov %rax, %rdx
    shr $32, %rdx
    mov $X86_MSR_IA32_GS_BASE, %ecx
    wrmsr

    /* set up the idt */
    lea _idt_startup(%rip), %rdi
    call idt_setup
    call load_startup_idt

    /* assign this core CPU# 0 and initialize its per cpu state */
    xor %edi, %edi
    call x86_init_percpu

    // Fill the stack canary with a random value as early as possible.
    // This isn't done in x86_init_percpu because the hw_rng_get_entropy
    // call would make it eligible for stack-guard checking itself.  But
    // %gs is not set up yet in the prologue of the function, so it would
    // crash if it tried to use the stack-guard.
    call choose_stack_guard

    // Move it into place.
    mov %rcx, %gs:ZX_TLS_STACK_GUARD_OFFSET
    // Don't leak that value to other code.
    xor %ecx, %ecx

    // configure the kernel base address
    // TODO: dynamically figure this out once we allow the x86 kernel to be loaded anywhere
    movl $PHYS_LOAD_ADDRESS, kernel_base_phys(%rip)

    /* call the main module */
    call lk_main

0:                          /* just sit around waiting for interrupts */
    hlt                     /* interrupts will unhalt the processor */
    pause
    jmp 0b                  /* so jump back to halt to conserve power */

.bss
.align 16
DATA(_kstack)
    .skip 4096
DATA(_kstack_end)

// These symbols are used by image.S
.global IMAGE_ELF_ENTRY
IMAGE_ELF_ENTRY = _start

// This symbol is used by gdb python to know the base of the kernel module
.global KERNEL_BASE_ADDRESS
KERNEL_BASE_ADDRESS = KERNEL_BASE - KERNEL_LOAD_OFFSET
