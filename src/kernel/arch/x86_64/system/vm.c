#include <framebuffer.h>
#include <logging.h>
#include <main.h>
#include <video.h>
#include <vm.h>
#include <vmm.h>

extern uint32_t FRAMEBUFFER_MEMORY_SIZE;

void page_fault_handler(uint64_t error_code) {
    // TODO: Add ptable info when using 4k pages
    pretty_log(Verbose, "Welcome to #PF world - Not ready yet... ");
    uint64_t cr2_content = 0;
    uint64_t pd;
    uint64_t pdpr;
    uint64_t pml4;
    asm ("mov %%cr2, %0" : "=r" (cr2_content) );
    pretty_logf(Verbose, "-- Error code value: %d", error_code);
    pretty_logf(Verbose, "--  Faulting address: 0x%X", cr2_content);
    cr2_content = cr2_content & VM_OFFSET_MASK;
    pretty_logf(Verbose, "-- Address prepared for PD/PT extraction (CR2): 0x%x", cr2_content);
    pd = PD_ENTRY(cr2_content);
    pdpr = PDPR_ENTRY(cr2_content);
    pml4 = PML4_ENTRY(cr2_content);
    pretty_logf(Verbose, "Error flags: FETCH(%d) - RSVD(%d) - ACCESS(%d) - WRITE(%d) - PRESENT(%d)", \
            error_code&FETCH_VIOLATION, \
            error_code&RESERVED_VIOLATION, \
            error_code&ACCESS_VIOLATION, \
            error_code&WRITE_VIOLATION, \
            error_code&PRESENT_VIOLATION);
    uint64_t *pd_table = (uint64_t *) (SIGN_EXTENSION | ENTRIES_TO_ADDRESS(510l,510l, (uint64_t) pml4, (uint64_t) pdpr));
    uint64_t *pdpr_table = (uint64_t *) (SIGN_EXTENSION | ENTRIES_TO_ADDRESS(510l,510l, 510l, (uint64_t) pml4));
    uint64_t *pml4_table = (uint64_t *) (SIGN_EXTENSION | ENTRIES_TO_ADDRESS(510l,510l, 510l, 510l));
    pretty_logf(Verbose, "Entries: pd: 0x%X - pdpr: 0x%X - PML4 0x%X", pd, pdpr, pml4);
    pretty_logf(Verbose, "Entries: pd[0x%x]: 0x%X", pd, pd_table[pd]);
    pretty_logf(Verbose, "Entries: pdpr[0x%x]: 0x%X", pdpr, pdpr_table[pdpr]);
    pretty_logf(Verbose, "Entries: pml4[0x%x]: 0x%X", pml4, pml4_table[pml4]);
    asm("hlt");
}

void clean_new_table( uint64_t *table_to_clean ) {
    for(int i = 0; i < VM_PAGES_PER_TABLE; i++){
        table_to_clean[i] = 0x00l;
    }
}

void load_cr3( void* cr3_value ) {
    //This function is used only when the kernel needs to load a new pml4 table (paging root for x86_64)
    //I should add support for the flags
    //invalidate_page_table((uint64_t) cr3_value);
    //pretty_logf(Verbose, "(load_cr3) Loading pml4 table at address: %u", cr3_value);
    asm volatile("mov %0, %%cr3" :: "r"((uint64_t)cr3_value) : "memory");
}

void invalidate_page_table( uint64_t *table_address ) {
	asm volatile("invlpg (%0)"
    	:
    	: "r"((uint64_t)table_address)
    	: "memory");
}

/**
 * This function given an address if it is not in the higher half, it return the same address + HIGHER_HALF_ADDRESS_OFFSET already defined.
 *
 *
 * @param address the physical address we want to map
 * @param type the accepted values are: #VM_TYPE_MEMORY #VM_TYPE_MMIO
 * @return virtuial address in the higher half
 */
uint64_t ensure_address_in_higher_half( uint64_t address, uint8_t type) {
    uint64_t offset = 0;

    if ( type == VM_TYPE_MEMORY ) {
        offset = HIGHER_HALF_ADDRESS_OFFSET;
    } else if ( type == VM_TYPE_MMIO ){
        offset = MMIO_HIGHER_HALF_ADDRESS_OFFSET;
    } else {
        return 0;
    }

    if ( address > offset + VM_KERNEL_MEMORY_PADDING) {
            return address;
    } else {
        return address + offset + VM_KERNEL_MEMORY_PADDING;
    }
}

bool is_address_higher_half(uint64_t address) {
    if ( address & (1l << 62) ) {
        return true;
    }
    return false;
}
