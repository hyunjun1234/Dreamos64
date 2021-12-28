#include <cpu.h>
#include <acpi.h>
#include <video.h>
#include <framebuffer.h>
#include <stdio.h>
#include <rsdt.h>
#include <stddef.h>
#include <string.h>


RSDT* rsdt_root = NULL;
unsigned int rsdtTablesTotal = 0;

void parse_RSDT(RSDPDescriptor *descriptor){
    printf("Parse RSDP Descriptor\n");
    rsdt_root = (RSDT *) descriptor->RsdtAddress;
    printf("descriptor Address: 0x%x\n", descriptor->RsdtAddress);
    ACPISDTHeader header = rsdt_root->header;
    
    printf("RSDT_Signature: %.4s\n", header.Signature);
    printf("RSDT_Lenght: %d\n", header.Length);
    rsdtTablesTotal = (header.Length - sizeof(ACPISDTHeader)) / sizeof(uint32_t);
    printf("Total rsdt Tables: %d\n", rsdtTablesTotal);
    
    for(int i=0; i < rsdtTablesTotal; i++){
        ACPISDTHeader *tableHeader = (ACPISDTHeader *) rsdt_root->tables[i];
        printf("\tTable header %d: Signature: %.4s\n", i, tableHeader->Signature);
    }
    MADT* madt_table = (MADT*) get_RSDT_Item(MADT_ID);
    printf("Madt SIGNATURE: %.4s\n", madt_table->header.Signature);
//    _printStr(header.Signature);
//    _fb_putchar(header.Signature[0], 1, 3, 0x000000, 0xFFFFFF);

//    printf("RSDT Address: 0x%x", root->header);
    //#if USE_FRAMEBUFFER == 1*/
    //_fb_putchar(header.Signature[0], 1, 3, 0x000000, 0xFFFFFF);
//    _fb_putchar(header.Signature[1], 2, 3, 0x000000, 0xFFFFFF);
//    _fb_putchar(header.Signature[2], 3, 3, 0x000000, 0xFFFFFF);
//    _fb_putchar(header.Signature[3], 4, 3, 0x000000, 0xFFFFFF);
/*    _fb_printStr(header.Signature, 0, 3, 0x000000, 0xFFFFFF);
    #endif
   _printNewLine();*/
    
}

ACPISDTHeader* get_RSDT_Item(char* table_name) {
    if(rsdt_root == NULL) {
        return NULL;
    }    
    for(int i=0; i < rsdtTablesTotal; i++){
        ACPISDTHeader *tableItem = (ACPISDTHeader *) rsdt_root->tables[i];
        int return_value = strncmp(table_name, tableItem->Signature, 4);
        if(return_value == 0) {
            printf("Item Found...\n");
            return tableItem;
        }
    }
    printf("\n");
    return NULL;
}

void parse_RSDTv2(RSDPDescriptor20 *descriptor){
    printf("Parse RSDP v2 Descriptor");
}

int validate_RSDP(RSDPDescriptor *descriptor){
    uint8_t sum = 0;
    char number[30];

    for (uint32_t i=0; i < sizeof(RSDPDescriptor); i++){
        sum += ((char*) descriptor)[i];
        _getHexString(number, sum);
        _printStr(number);
        _printStr(" ");
    }
    _printNewLine();
    _getHexString(number, sum);
    _printStr("Checksum of RSDP is: ");
    _printStr(number);
    _printNewLine();
    return sum == 0;
}
