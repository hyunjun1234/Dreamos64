#ifndef _VM_H
#define _VM_H

#include <stdint.h>
#include <stdbool.h>
#include <bitmap.h>

#define PML4_ENTRY(address)((address>>39) & 0x1ff)
#define PDPR_ENTRY(address)((address>>30) & 0x1ff)
#define PD_ENTRY(address)((address>>21) & 0x1ff)
#define PT_ENTRY(address)((address>>12) & 0x1ff)
#define SIGN_EXTENSION 0xFFFF000000000000
#define ENTRIES_TO_ADDRESS(pml4, pdpr, pd, pt)((pml4 << 39) | (pdpr << 30) | (pd << 21) |  (pt << 12))

#define PAGE_ALIGNMENT_MASK (PAGE_SIZE_IN_BYTES-1)

#define ALIGN_PHYSADDRESS(address)(address & (~(PAGE_ALIGNMENT_MASK)))

#define MMIO_HIGHER_HALF_ADDRESS_OFFSET 0xFFFF800000000000
#define MMIO_RESERVED_SPACE_SIZE 0x280000000
#define HIGHER_HALF_ADDRESS_OFFSET  (MMIO_HIGHER_HALF_ADDRESS_OFFSET + MMIO_RESERVED_SPACE_SIZE)

#define PRESENT_BIT 1
#define WRITE_BIT 0b10
#define HUGEPAGE_BIT 0b10000000

#define VM_PAGES_PER_TABLE 0x200

#define PRESENT_VIOLATION   0x1
#define WRITE_VIOLATION 0x2
#define ACCESS_VIOLATION    0x4
#define RESERVED_VIOLATION   0x8
#define FETCH_VIOLATION 0x10

#define VM_PAGE_TABLE_BASE_ADDRESS_MASK 0xFFFFFFF000

#if SMALL_PAGES == 0
#define VM_OFFSET_MASK 0xFFFFFFFFFFE00000
#define PAGE_ENTRY_FLAGS PRESENT_BIT | WRITE_BIT | HUGEPAGE_BIT
#elif SMALL_PAGES == 1
#define VM_OFFSET_MASK 0xFFFFFFFFFFFFF000
#define PAGE_ENTRY_FLAGS PRESENT_BIT | WRITE_BIT
#endif

#define VM_TYPE_MEMORY 0
#define VM_TYPE_MMIO 1

void page_fault_handler( uint64_t error_code );

void initialize_vm();

void clean_new_table( uint64_t *table_to_clean );

void invalidate_page_table( uint64_t *table_address );

void load_cr3( void* cr3_value );

uint64_t ensure_address_in_higher_half( uint64_t address, uint8_t type);

bool is_address_higher_half(uint64_t address);
#endif
