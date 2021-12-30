#ifndef _MADT_H
#define _MADT_H

#include <stdint.h>
#include <rsdt.h>

#define MADT_PROCESSOR_LOCAL_APIC    0
#define MADT_IO_APIC 1
#define MADT_IO_APIC_INTTERUPT_SOURCE_OVERRIDE 2
#define MADT_NMI_INTERRUPT_SOURCE    3
#define MADT_LOCAL_APIC_NMI  4
#define MADT_LOCAL_APIC_ADDRESS_OVERRIDE 5
#define MADT_PRORCESSOR_LOCAL_X2APIC 9

typedef struct MADT_Item {
    uint8_t type;
    uint8_t length;
} MADT_Item;

MADT_Item* get_MADT_item(MADT*, uint8_t);
#endif