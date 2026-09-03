| The FIX flush's row compare in 68000 assembly: twenty long words of the
| next frame against the shadow of what VRAM holds, writing only the cells
| that changed.  Built into the cartridge only; native/game.c keeps the C
| twin for the host and for POOL_ASM=0 proofs.
|
| uint16_t bajanew_fix_flush_row(uint16_t *shadow, const uint16_t *next,
|                                uint16_t address_of_column_zero);
        .text
        .globl  bajanew_fix_flush_row
bajanew_fix_flush_row:
        movem.l %d2-%d4/%a2,-(%sp)      | a2 is the caller's: args shadow 20, next 24, address 28 (word at 30)
        move.l  20(%sp),%a0
        move.l  24(%sp),%a1
        move.w  30(%sp),%d1             | VRAM address of column 0 in this row
        lea     0x3C0000,%a2
        moveq   #0,%d2                  | words written
        moveq   #19,%d0                 | twenty column pairs
.Lpair:
        cmpm.l  (%a1)+,(%a0)+
        dbne    %d0,.Lpair              | run through the equal pairs
        beq.s   .Ldone                  | the last pair was equal too
        subq.l  #4,%a0
        subq.l  #4,%a1
        moveq   #19,%d4                 | column 2i from the pairs left: 19 - d0
        sub.w   %d0,%d4
        lsl.w   #6,%d4                  | times 64 words, two columns of 32
        add.w   %d1,%d4
        move.w  (%a1)+,%d3
        cmp.w   (%a0)+,%d3
        beq.s   .Lsecond
        move.w  %d3,-2(%a0)             | shadow keeps the raw word
        and.w   #0x0fff,%d3             | palette 0, twelve bits of tile
        move.w  %d4,(%a2)               | VRAMADDR: column 2i
        nop
        nop
        nop
        nop
        move.w  %d3,2(%a2)
        addq.w  #1,%d2
.Lsecond:
        move.w  (%a1)+,%d3
        cmp.w   (%a0)+,%d3
        beq.s   .Lnext
        move.w  %d3,-2(%a0)
        and.w   #0x0fff,%d3
        add.w   #32,%d4                 | column 2i + 1 is one column (32 words) on
        move.w  %d4,(%a2)
        nop
        nop
        nop
        nop
        move.w  %d3,2(%a2)
        addq.w  #1,%d2
.Lnext:
        dbra    %d0,.Lpair
.Ldone:
        move.w  %d2,%d0
        and.l   #0xffff,%d0
        movem.l (%sp)+,%d2-%d4/%a2
        rts
