#include <acpi.h>
#include <bitmap.h>
#include <cpu.h>
#include <framebuffer.h>
#include <hh_direct_map.h>
#include <kheap.h>
#include <logging.h>
#include <numbers.h>
#include <rsdt.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <video.h>
#include <vm.h>
#include <vmm.h>
#include <vmm_mapping.h>

RSDT* rsdt_root = NULL;
XSDT* xsdt_root = NULL;

unsigned int rsdtTablesTotal = 0;
uint8_t sdt_version = RSDT_V2;

void parse_SDT(uint64_t address, uint8_t type) {
    if ( type == RSDT_V1) {
        parse_RSDT((RSDPDescriptor *) address);
    } else if ( type == RSDT_V2 ) {
        parse_RSDTv2((RSDPDescriptor20 *) address);
    }
}

void parse_RSDT(RSDPDescriptor *descriptor){
    pretty_logf(Verbose, "- descriptor Address: 0x%x", descriptor->RsdtAddress);
    _bitmap_set_bit_from_address(ALIGN_PHYSADDRESS(descriptor->RsdtAddress));
    rsdt_root = (RSDT *) ensure_address_in_higher_half((uint64_t) descriptor->RsdtAddress, VM_TYPE_MMIO);

    //rsdt_root = (RSDT_*) vmm_alloc(KERNEL_PAGE_SIZE, VMM_FLAGS_MMIO | VMM_FLAGS_PRESENT | VMM_FLAGS_WRITE_ENABLE);
    //rsdt_root = (RSDT *) vmm_alloc(header.Length, VMM_FLAGS_PRESENT | VMM_FLAGS_WRITE_ENABLE | VMM_FLAGS_MMIO, NULL); //ensure_address_in_higher_half((uint64_t) descriptor->RsdtAddress);
    map_phys_to_virt_addr((void *) ALIGN_PHYSADDRESS(descriptor->RsdtAddress), rsdt_root, VMM_FLAGS_PRESENT | VMM_FLAGS_WRITE_ENABLE);
    //rsdt_root = (RSDT *) hhdm_get_variable((uintptr_t) descriptor->RsdtAddress);
    ACPISDTHeader header = rsdt_root->header;
    pretty_logf(Verbose, "- RSDT_Signature: %.4s - Length: %d - HH addr: 0x%x", header.Signature, header.Length, rsdt_root);
    //while(1)
    sdt_version = RSDT_V1;

    // Ok here we are,  and we have mapped the "head of rsdt", it will stay most likely in one page, but there is no way
    // to know the length of the whole table before mapping its header. So now we are able to check if we need to map extra pages
    size_t required_extra_pages = (header.Length / KERNEL_PAGE_SIZE) + 1;
    if (required_extra_pages > 1) {
        //uintptr_t rsdt_extra = (uintptr_t) vmm_alloc(header.Length, VMM_FLAGS_PRESENT | VMM_FLAGS_WRITE_ENABLE | VMM_FLAGS_MMIO);
        //pretty_logf(Verbose, "- RSDT_PAGES_NEEDED: %d", required_extra_pages);
        for (size_t j = 1; j < required_extra_pages; j++) {
            uint64_t new_physical_address = descriptor->RsdtAddress + (j * KERNEL_PAGE_SIZE);
            map_phys_to_virt_addr((void *) ALIGN_PHYSADDRESS(new_physical_address), (void *) ensure_address_in_higher_half(new_physical_address, VM_TYPE_MMIO), VMM_FLAGS_PRESENT | VMM_FLAGS_WRITE_ENABLE);
           // map_phys_to_virt_addr((void *) ALIGN_PHYSADDRESS(new_physical_address), (void *) rsdt_extra, VMM_FLAGS_PRESENT | VMM_FLAGS_WRITE_ENABLE);
            _bitmap_set_bit_from_address(ALIGN_PHYSADDRESS(new_physical_address));
            //rsdt_extra += KERNEL_PAGE_SIZE);
        }
    }
    rsdtTablesTotal = (header.Length - sizeof(ACPISDTHeader)) / sizeof(uint32_t);
    pretty_logf(Verbose, "- Total rsdt Tables: %d", rsdtTablesTotal);

    for(uint32_t i=0; i < rsdtTablesTotal; i++) {
        map_phys_to_virt_addr((void *) ALIGN_PHYSADDRESS(rsdt_root->tables[i]), (void *) ensure_address_in_higher_half(rsdt_root->tables[i], VM_TYPE_MMIO), VMM_FLAGS_PRESENT | VMM_FLAGS_WRITE_ENABLE);
        ACPISDTHeader *tableHeader = (ACPISDTHeader *) ensure_address_in_higher_half(rsdt_root->tables[i], VM_TYPE_MMIO);
        pretty_logf(Verbose, "\t%d): Signature: %.4s Length: %d phys_addr: 0x%x", i, tableHeader->Signature, tableHeader->Length, rsdt_root->tables[i]);
    }
}

void parse_RSDTv2(RSDPDescriptor20 *descriptor){
    pretty_logf(Verbose, "- Descriptor physical address: 0x%x", ALIGN_PHYSADDRESS(descriptor->XsdtAddress));
    map_phys_to_virt_addr((void *) ALIGN_PHYSADDRESS(descriptor->XsdtAddress), (void *) ensure_address_in_higher_half(descriptor->XsdtAddress, VM_TYPE_MMIO), VMM_FLAGS_PRESENT | VMM_FLAGS_WRITE_ENABLE);
    _bitmap_set_bit_from_address(ALIGN_PHYSADDRESS(descriptor->XsdtAddress));
    xsdt_root = (XSDT *) ensure_address_in_higher_half((uint64_t) descriptor->XsdtAddress, VM_TYPE_MMIO);
    pretty_logf(Verbose, "- XSDT_Length: 0x%x", descriptor->Length);
    ACPISDTHeader header = xsdt_root->header;
    pretty_logf(Verbose, "- XSDT_Signature: %.4s", header.Signature);
    sdt_version = RSDT_V2;

    size_t required_extra_pages = (header.Length / KERNEL_PAGE_SIZE) + 1;

    if (required_extra_pages > 1) {
        for (size_t j = 1; j < required_extra_pages; j++) {
            uint64_t new_physical_address = descriptor->XsdtAddress + (j * KERNEL_PAGE_SIZE);
            map_phys_to_virt_addr((uint64_t *) new_physical_address, (uint64_t *) ensure_address_in_higher_half(new_physical_address, VM_TYPE_MMIO), VMM_FLAGS_PRESENT | VMM_FLAGS_WRITE_ENABLE);
            _bitmap_set_bit_from_address(ALIGN_PHYSADDRESS(new_physical_address));
        }
    }

    rsdtTablesTotal = (header.Length - sizeof(ACPISDTHeader)) / sizeof(uint64_t);
    pretty_logf(Verbose, "- Total xsdt Tables: %d", rsdtTablesTotal);

    for(uint32_t i=0; i < rsdtTablesTotal; i++) {
        map_phys_to_virt_addr((uint64_t *) ALIGN_PHYSADDRESS(xsdt_root->tables[i]), (uint64_t *) ensure_address_in_higher_half(xsdt_root->tables[i], VM_TYPE_MMIO), VMM_FLAGS_PRESENT | VMM_FLAGS_WRITE_ENABLE);
        _bitmap_set_bit_from_address(ALIGN_PHYSADDRESS(xsdt_root->tables[i]));
        ACPISDTHeader *tableHeader = (ACPISDTHeader *) ensure_address_in_higher_half(xsdt_root->tables[i], VM_TYPE_MMIO);
        pretty_logf(Verbose, "\t%d): Signature: %.4s", i, tableHeader->Signature);
    }

}


ACPISDTHeader* get_SDT_item(char* table_name) {
    if((sdt_version == RSDT_V1 && rsdt_root == NULL) || (sdt_version == RSDT_V2 && xsdt_root == NULL)) {
        return NULL;
    }
    for(uint32_t i=0; i < rsdtTablesTotal; i++){
        ACPISDTHeader *tableItem;
        switch(sdt_version) {
            case RSDT_V1:
                tableItem = (ACPISDTHeader *) ensure_address_in_higher_half(rsdt_root->tables[i], VM_TYPE_MMIO);
                break;
            case RSDT_V2:
                tableItem = (ACPISDTHeader *) ensure_address_in_higher_half(xsdt_root->tables[i], VM_TYPE_MMIO);
                break;
            default:
                pretty_log(Fatal, "That should not happen, PANIC");
                return NULL;
        }
        int return_value = strncmp(table_name, tableItem->Signature, 4);
        pretty_logf(Info, "%d - Table name: %.4s", i, tableItem->Signature);
        if(return_value == 0) {
            return tableItem;
        }
    }
    return NULL;
}

/*! \brief It validate the RSDP (v1 and v2) checksum
 *
 * Given the descriptor as a byte array, it sums each byte, and if the last byte of the sum is 0 this means that the structure is valid.
 * @param decriptor the RSDPv1/v2 descriptor
 * @param the size of the struct used by descriptor (should be the size of: RSDPDescriptor or RSDPDescriptorv2)
 * @return true if the validation is succesfull
 */
bool validate_SDT(char *descriptor, size_t size){
    uint32_t sum = 0;
    for (uint32_t i=0; i < size; i++){
        sum += ((char*) descriptor)[i];
    }
    pretty_logf(Verbose, "Checksum of RSDP is: 0x%x", sum&0xFF);
    return ((sum&0xFF) == 0);
}
