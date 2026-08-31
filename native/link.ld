OUTPUT_FORMAT("elf32-m68k")
OUTPUT_ARCH(m68k)
ENTRY(ng_user_entry)

MEMORY
{
    ROM (rx)  : ORIGIN = 0x000000, LENGTH = 512K
    RAM (rwx) : ORIGIN = 0x100000, LENGTH = 60K
}

SECTIONS
{
    .boot 0x000000 :
    {
        KEEP(*(.boot))
    } > ROM

    .text ALIGN(2) :
    {
        *(.text*)
        *(.rodata*)
        . = ALIGN(2);
    } > ROM

    .telemetry 0x100000 (NOLOAD) :
    {
        KEEP(*(.telemetry))
        . = ALIGN(2);
    } > RAM

    .game_telemetry 0x100040 (NOLOAD) :
    {
        KEEP(*(.game_telemetry))
        . = ALIGN(2);
    } > RAM

    .save 0x100080 (NOLOAD) :
    {
        KEEP(*(.save))
        . = ALIGN(2);
    } > RAM

    .data 0x100280 : AT(ALIGN(LOADADDR(.text) + SIZEOF(.text), 2))
    {
        __data_start = .;
        *(.data*)
        . = ALIGN(2);
        __data_end = .;
    } > RAM
    __data_load = LOADADDR(.data);

    .bss (NOLOAD) :
    {
        __bss_start = .;
        *(.bss*)
        *(COMMON)
        . = ALIGN(2);
        __bss_end = .;
    } > RAM

    .crash 0x10EF00 (NOLOAD) :
    {
        KEEP(*(.crash))
        . = ALIGN(2);
    } > RAM

    __rom_end = LOADADDR(.data) + SIZEOF(.data);

    /DISCARD/ :
    {
        *(.comment*)
        *(.note*)
        *(.eh_frame*)
    }

    ASSERT(SIZEOF(.boot) <= 0x400, "Neo Geo boot/header block exceeds 0x400 bytes")
    ASSERT(SIZEOF(.telemetry) <= 0x40, "Forge68 telemetry exceeds its stable ABI window")
    ASSERT(SIZEOF(.game_telemetry) == 0x40, "BAJANEW telemetry must be exactly 64 bytes")
    ASSERT(SIZEOF(.save) == 0x200, "BIOS work-backup area must be exactly 512 bytes")
    ASSERT(__rom_end <= 0x080000, "P-ROM content exceeds the 512 KiB carrier")
    ASSERT(__bss_end <= 0x10EF00, "68K RAM overlaps persistent crash record")
    ASSERT(SIZEOF(.crash) <= 0x100, "persistent crash record exceeds reserved page")
}
