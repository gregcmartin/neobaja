; BAJANEW Z80 / YM2610 sound driver.
;
; Original work for the Neo Geo cartridge.  The BIOS contract is kept: $01
; switches slots by running from Z80 RAM, $03 soft-resets, and every other
; command is acknowledged as command|$80 on port $0C.
;
; The NMI handler only queues the 68000's command; the main loop drains the
; queue with interrupts masked around YM writes, and the YM2610's Timer B
; interrupt drives the music sequencer.  That way an NMI can never land in
; the middle of a register write.
;
; Command map (see ngsong.py and native/game.c):
;   $02          stop everything
;   $10-$1F      play sound effect n on its ADPCM-A channel
;   $20          stop music
;   $21-$2F      play song n
;   $30          engine off
;   $31-$6F      engine pitch step 0..62 (ADPCM-B loop, delta-N from a table)
;
; Sequencer event bytes per channel stream:
;   $00-$5F note on (semitones from C1)   $60 note off
;   $61 vv volume (0..15)                 $62 ii instrument
;   $63 pp SSG drum: noise period pp, full volume, decays each tick
;   $7F end: restart from the channel's loop point
;   $80-$FF wait (byte - $7F) ticks

        .module bajanew_audio
        .area   _HEADER (ABS)

; ------------------------------------------------------------------ RAM --
CMD_QUEUE       = 0xf800        ; 16 byte ring of pending commands
CMD_HEAD        = 0xf810
CMD_TAIL        = 0xf811
MUSIC_ON        = 0xf812
ENGINE_STEP     = 0xf813
SONG_PTR        = 0xf814        ; 2 bytes: current song header
; Per channel state, 8 channels x 8 bytes from 0xf820:
;   +0/+1 stream pointer, +2/+3 loop pointer, +4 wait, +5 volume,
;   +6 instrument, +7 drum decay countdown / flags
CH_STATE        = 0xf820
CH_SIZE         = 8
CHANNELS        = 7

        .org    0x0000
        di
        jp      reset

        .org    0x0038
        jp      irq_handler

        .org    0x0066
        jp      nmi_handler

        .org    0x0100
reset:
        di
        ld      sp,#0xfffc
        im      1
        xor     a
        out     (0x0c),a
        out     (0x00),a
        ld      (CMD_HEAD),a
        ld      (CMD_TAIL),a
        ld      (MUSIC_ON),a
        ld      (ENGINE_STEP),a
        call    silence_all
        ; The system program that ran before this driver may have left a
        ; timer flag set; the interrupt line stays low until both are reset.
        ld      de,#0x2730
        call    ym_a_write
        ; Timer B: the sequencer tick.
        ld      de,#0x2600
        ld      a,(tick_period)
        ld      e,a
        call    ym_a_write
        ld      de,#0x273a
        call    ym_a_write
        ld      a,#0xa5
        out     (0x0c),a
        xor     a
        out     (0x08),a
        ei
main_loop:
        halt
        call    drain_commands
        jp      main_loop

; ------------------------------------------------------------- commands --
        .org    0x0140
nmi_handler:
        push    af
        push    hl
        in      a,(0x00)
        cp      #0x01
        jp      z,slot_switch
        cp      #0x03
        jp      z,soft_reset
        ld      l,a             ; command
        ld      a,(CMD_TAIL)
        ld      h,a             ; tail index
        inc     a
        and     #0x0f
        ld      (CMD_TAIL),a
        ld      a,l             ; command back in A
        ld      l,h
        ld      h,#0xf8
        ld      (hl),a          ; CMD_QUEUE + tail = command
        or      #0x80
        out     (0x0c),a
        xor     a
        out     (0x00),a
        pop     hl
        pop     af
        retn

slot_switch:
        di
        xor     a
        out     (0x0c),a
        out     (0x00),a
        ld      sp,#0xfffc
        ld      hl,#0xfffd
        ld      (hl),#0xc3
        inc     hl
        ld      (hl),#0xfd
        inc     hl
        ld      (hl),#0xff
        ei
        ld      a,#0x01
        out     (0x0c),a
        jp      0xfffd

soft_reset:
        di
        xor     a
        out     (0x0c),a
        out     (0x00),a
        ld      sp,#0xffff
        jp      0x0000

drain_commands:
        ld      a,(CMD_HEAD)
        ld      b,a
        ld      a,(CMD_TAIL)
        cp      b
        ret     z
        ld      a,b
        ld      h,#0xf8
        ld      l,b
        ld      c,(hl)          ; command
        inc     a
        and     #0x0f
        ld      (CMD_HEAD),a
        ld      a,c
        di
        call    dispatch
        ei
        jr      drain_commands

dispatch:
        cp      #0x02
        jp      z,silence_all
        cp      #0x20
        jp      z,music_stop
        cp      #0x30
        jp      z,engine_off
        cp      #0x10
        jr      c,dispatch_done
        cp      #0x20
        jp      c,play_effect       ; $10-$1F
        cp      #0x31
        jp      c,music_start       ; $21-$2F
        cp      #0x70
        jp      c,engine_pitch      ; $31-$6F
dispatch_done:
        ret

; ------------------------------------------------------------- YM writes --
        .org    0x0200
; DE = register/value, port A: SSG, timers, ADPCM-B, FM channels 1 and 2.
ym_a_write:
        ld      a,d
        out     (0x04),a
        nop
        nop
        nop
        nop
        nop
        nop
        ld      a,e
        out     (0x05),a
        jr      ym_data_delay

; DE = register/value, port B: ADPCM-A, FM channels 3 and 4.
ym_b_write:
        ld      a,d
        out     (0x06),a
        nop
        nop
        nop
        nop
        nop
        nop
        ld      a,e
        out     (0x07),a
ym_data_delay:
        push    bc
        ld      b,#10
delay_loop:
        djnz    delay_loop
        pop     bc
        ret

; Write DE to whichever port FM channel C (0..3) lives on.
ym_fm_write:
        ld      a,c
        cp      #2
        jp      c,ym_a_write
        jp      ym_b_write

silence_all:
        xor     a
        ld      (MUSIC_ON),a
        ld      (ENGINE_STEP),a
        ld      de,#0x073f
        call    ym_a_write
        ld      de,#0x0800
        call    ym_a_write
        ld      de,#0x0900
        call    ym_a_write
        ld      de,#0x0a00
        call    ym_a_write
        ld      de,#0x2801
        call    ym_a_write
        ld      de,#0x2802
        call    ym_a_write
        ld      de,#0x2805
        call    ym_a_write
        ld      de,#0x2806
        call    ym_a_write
        ld      de,#0x00bf
        call    ym_b_write
        ld      de,#0x1001
        call    ym_a_write
        ; Keep Timer B running; the sequencer is simply idle.
        ld      de,#0x273a
        jp      ym_a_write

; -------------------------------------------------------------- effects --
; A = $10 + effect index.  Effect table: 6 bytes each: channel bit, level/pan,
; start lo, start hi, end lo, end hi (256 byte pages).
play_effect:
        and     #0x0f
        ld      hl,#effect_table
        ld      b,a
        add     a,a
        add     a,b
        add     a,a             ; a * 6
        ld      c,a
        ld      b,#0
        add     hl,bc
        ld      a,(hl)          ; channel bit
        or      a
        ret     z
        ld      b,a
        push    bc
        or      #0x80
        ld      e,a             ; stop that channel first
        ld      d,#0x00
        call    ym_b_write
        pop     bc
        push    hl
        ; Channel index 0..5 from the bit, for register offsets.
        ld      c,#0
bit_scan:
        srl     b
        jr      c,bit_found
        inc     c
        jr      bit_scan
bit_found:
        pop     hl
        inc     hl
        ld      a,#0x08
        add     a,c
        ld      d,a
        ld      e,(hl)          ; level and pan
        push    hl
        push    bc
        call    ym_b_write
        pop     bc
        pop     hl
        inc     hl
        ld      a,#0x10
        add     a,c
        ld      d,a
        ld      e,(hl)
        push    hl
        push    bc
        call    ym_b_write
        pop     bc
        pop     hl
        inc     hl
        ld      a,#0x18
        add     a,c
        ld      d,a
        ld      e,(hl)
        push    hl
        push    bc
        call    ym_b_write
        pop     bc
        pop     hl
        inc     hl
        ld      a,#0x20
        add     a,c
        ld      d,a
        ld      e,(hl)
        push    hl
        push    bc
        call    ym_b_write
        pop     bc
        pop     hl
        inc     hl
        ld      a,#0x28
        add     a,c
        ld      d,a
        ld      e,(hl)
        push    bc
        call    ym_b_write
        pop     bc
        ; Master ADPCM-A level, then key on this channel.
        ld      de,#0x013f
        call    ym_b_write
        ld      a,#1
        inc     c
shift_bit:
        dec     c
        jr      z,bit_ready
        add     a,a
        jr      shift_bit
bit_ready:
        ld      e,a
        ld      d,#0x00
        jp      ym_b_write

; --------------------------------------------------------------- engine --
engine_off:
        xor     a
        ld      (ENGINE_STEP),a
        ld      de,#0x1001
        jp      ym_a_write

; A = $31 + step.  Delta-N from the table; the loop is (re)started only when
; the engine was off, so a pitch change never restarts the sample.
engine_pitch:
        sub     #0x31
        ld      hl,#engine_pitch_table
        ld      c,a
        ld      b,#0
        add     hl,bc
        add     hl,bc
        ld      e,(hl)
        inc     hl
        ld      d,(hl)
        push    de
        ld      a,(ENGINE_STEP)
        or      a
        jr      nz,engine_set_pitch
        ; Start the loop: reset, addresses, level, pan, then start with repeat.
        ld      de,#0x1001
        call    ym_a_write
        ld      de,#0x11c0
        call    ym_a_write
        ld      a,(engine_start)
        ld      e,a
        ld      d,#0x12
        call    ym_a_write
        ld      a,(engine_start+1)
        ld      e,a
        ld      d,#0x13
        call    ym_a_write
        ld      a,(engine_end)
        ld      e,a
        ld      d,#0x14
        call    ym_a_write
        ld      a,(engine_end+1)
        ld      e,a
        ld      d,#0x15
        call    ym_a_write
        ld      a,(engine_level)
        ld      e,a
        ld      d,#0x1b
        call    ym_a_write
        ld      de,#0x1c80
        call    ym_a_write
        ld      de,#0x1c00
        call    ym_a_write
        pop     de
        push    de
        call    engine_write_delta
        ld      de,#0x1090
        call    ym_a_write
        pop     de
        ld      a,#1
        ld      (ENGINE_STEP),a
        ret
engine_set_pitch:
        pop     de
engine_write_delta:
        ; DE = delta-N: $19 low, $1A high.
        push    de
        ld      d,#0x19
        call    ym_a_write
        pop     de
        ld      e,d
        ld      d,#0x1a
        jp      ym_a_write

; ---------------------------------------------------------------- music --
music_stop:
        xor     a
        ld      (MUSIC_ON),a
        ld      de,#0x073f
        call    ym_a_write
        ld      de,#0x0800
        call    ym_a_write
        ld      de,#0x0900
        call    ym_a_write
        ld      de,#0x0a00
        call    ym_a_write
        ld      de,#0x2801
        call    ym_a_write
        ld      de,#0x2802
        call    ym_a_write
        ld      de,#0x2805
        call    ym_a_write
        ld      de,#0x2806
        jp      ym_a_write

; A = $21 + song index.
music_start:
        push    af
        call    music_stop
        pop     af
        sub     #0x21
        ld      hl,#song_table
        ld      c,a
        ld      b,#0
        add     hl,bc
        add     hl,bc
        ld      e,(hl)
        inc     hl
        ld      d,(hl)
        ld      (SONG_PTR),de
        ; Song header: 7 channels x (start lo, start hi, loop lo, loop hi).
        ex      de,hl
        ld      ix,#CH_STATE
        ld      b,#CHANNELS
init_channel:
        ld      a,(hl)
        ld      0(ix),a
        inc     hl
        ld      a,(hl)
        ld      1(ix),a
        inc     hl
        ld      a,(hl)
        ld      2(ix),a
        inc     hl
        ld      a,(hl)
        ld      3(ix),a
        inc     hl
        ld      4(ix),#1        ; first event on the next tick
        ld      5(ix),#15
        ld      6(ix),#0xff
        ld      7(ix),#0
        ld      de,#CH_SIZE
        add     ix,de
        djnz    init_channel
        ; SSG mixer: tones A, B, C on, noise off until a drum asks for it.
        ld      de,#0x0738
        call    ym_a_write
        ld      a,#1
        ld      (MUSIC_ON),a
        ret

; Timer B tick: advance every channel, then decay SSG drums.
irq_handler:
        push    af
        push    bc
        push    de
        push    hl
        push    ix
        ld      a,(MUSIC_ON)
        or      a
        jr      z,irq_ack
        ld      ix,#CH_STATE
        ld      c,#0
tick_channel:
        ld      a,0(ix)
        or      1(ix)
        jr      z,tick_next     ; channel unused
        dec     4(ix)
        jr      nz,tick_decay
        call    run_events
tick_decay:
        ld      a,7(ix)
        or      a
        jr      z,tick_next
        dec     7(ix)
        call    ssg_drum_decay
tick_next:
        ld      de,#CH_SIZE
        add     ix,de
        inc     c
        ld      a,c
        cp      #CHANNELS
        jr      c,tick_channel
irq_ack:
        ; Reset both timer flags, keep Timer B loaded and enabled.
        ld      de,#0x273a
        call    ym_a_write
        pop     ix
        pop     hl
        pop     de
        pop     bc
        pop     af
        ei
        reti

; IX = channel state, C = channel index.  Runs events until a wait.
run_events:
        ld      l,0(ix)
        ld      h,1(ix)
event_loop:
        ld      a,(hl)
        inc     hl
        cp      #0x80
        jr      c,event_not_wait
        sub     #0x7f
        ld      4(ix),a
        ld      0(ix),l
        ld      1(ix),h
        ret
event_not_wait:
        cp      #0x7f
        jr      nz,event_not_end
        ld      l,2(ix)
        ld      h,3(ix)
        jr      event_loop
event_not_end:
        cp      #0x60
        jr      c,event_note_on
        jr      z,event_note_off
        cp      #0x61
        jr      z,event_volume
        cp      #0x62
        jr      z,event_instrument
        cp      #0x63
        jr      z,event_drum
        jr      event_loop
event_note_off:
        push    hl
        call    note_off
        pop     hl
        jr      event_loop
event_volume:
        ld      a,(hl)
        inc     hl
        ld      5(ix),a
        jr      event_loop
event_instrument:
        ld      a,(hl)
        inc     hl
        ld      6(ix),a
        push    hl
        call    load_instrument
        pop     hl
        jr      event_loop
event_drum:
        ld      a,(hl)
        inc     hl
        push    hl
        call    ssg_drum
        pop     hl
        jr      event_loop
event_note_on:
        push    hl
        call    note_on
        pop     hl
        jr      event_loop

; ------------------------------------------------------------ channels --
; FM channel register offset (1 or 2) and key-on code for C = 0..3.
fm_offset:
        ld      a,c
        and     #1
        inc     a
        ret
fm_key_code:
        ld      a,c
        cp      #2
        jr      c,fm_key_low
        add     a,#3            ; 2 -> 5, 3 -> 6
        ret
fm_key_low:
        inc     a               ; 0 -> 1, 1 -> 2
        ret

; A = semitone from C1, C = channel.
note_on:
        ld      b,a
        ld      a,c
        cp      #4
        jp      nc,ssg_note_on
        ; FM: key off, pitch, key on.
        push    bc
        call    fm_key_code
        ld      e,a
        ld      d,#0x28
        call    ym_a_write
        pop     bc
        push    bc
        ; Block = semitone / 12, note = semitone % 12 -> fnum table.
        ld      a,b
        ld      d,#0
fm_octave:
        cp      #12
        jr      c,fm_octave_done
        sub     #12
        inc     d
        jr      fm_octave
fm_octave_done:
        ld      hl,#fnum_table
        add     a,a
        ld      e,a
        push    de
        ld      d,#0
        add     hl,de
        pop     de
        ld      e,(hl)          ; fnum low
        inc     hl
        ld      a,(hl)          ; fnum high (3 bits)
        ld      b,d             ; block
        sla     b
        sla     b
        sla     b
        or      b               ; block<<3 | fnum hi
        pop     bc
        push    bc
        push    de
        push    af
        call    fm_offset
        add     a,#0xa4
        ld      d,a
        pop     af
        ld      e,a
        call    ym_fm_write
        pop     de
        push    de
        call    fm_offset
        add     a,#0xa0
        ld      d,a
        call    ym_fm_write
        pop     de
        pop     bc
        call    fm_key_code
        or      #0xf0
        ld      e,a
        ld      d,#0x28
        jp      ym_a_write

note_off:
        ld      a,c
        cp      #4
        jr      nc,ssg_note_off
        call    fm_key_code
        ld      e,a
        ld      d,#0x28
        jp      ym_a_write

; SSG: C = 4..6 -> channel 0..2; B = semitone.
ssg_note_on:
        ld      a,c
        sub     #4
        ld      c,a
        ld      a,b
        ld      hl,#ssg_period_table
        add     a,a
        ld      e,a
        ld      d,#0
        add     hl,de
        ld      a,c
        add     a,a
        ld      d,a             ; register: period low = 2*channel
        ld      e,(hl)
        push    hl
        push    bc
        call    ym_a_write
        pop     bc
        pop     hl
        inc     hl
        ld      a,c
        add     a,a
        inc     a
        ld      d,a
        ld      e,(hl)
        push    bc
        call    ym_a_write
        pop     bc
        ld      a,c
        add     a,#0x08
        ld      d,a
        ld      e,5(ix)
        ld      7(ix),#0        ; a tone cancels a decaying drum
        jp      ym_a_write

ssg_note_off:
        ld      a,c
        sub     #4
        add     a,#0x08
        ld      d,a
        ld      e,#0
        jp      ym_a_write

; A = noise period.  Noise on for this SSG channel, full volume, decaying.
ssg_drum:
        push    bc
        ld      e,a
        ld      d,#0x06
        call    ym_a_write
        pop     bc
        ; Mixer: tones on everywhere, noise on for this channel only:
        ; $38 with bit (3 + channel) cleared.
        ld      a,c
        sub     #4
        ld      b,a
        ld      a,#0x08
drum_mix_shift:
        dec     b
        jp      m,drum_mix_ready
        add     a,a
        jr      drum_mix_shift
drum_mix_ready:
        xor     #0x38
        ld      e,a
        ld      d,#0x07
        push    bc
        call    ym_a_write
        pop     bc
        ld      a,c
        sub     #4
        add     a,#0x08
        ld      d,a
        ld      e,5(ix)
        ld      7(ix),#5        ; ticks of decay
        jp      ym_a_write

; IX = channel; 7(ix) already decremented.  Volume falls with the countdown.
ssg_drum_decay:
        ld      a,c
        sub     #4
        add     a,#0x08
        ld      d,a
        ld      a,5(ix)
        ld      b,7(ix)
        inc     b
        ; volume * (countdown+1) / 6, roughly: shift by steps
        ld      e,a
        ld      a,7(ix)
        or      a
        jr      nz,decay_partial
        ld      e,#0
        push    bc
        call    ym_a_write
        pop     bc
        ; Noise off again for this channel.
        ld      de,#0x0738
        jp      ym_a_write
decay_partial:
        cp      #3
        jr      nc,decay_write
        srl     e
        cp      #2
        jr      nc,decay_write
        srl     e
decay_write:
        jp      ym_a_write

; A = instrument index, C = channel.  FM only; SSG channels ignore it.
; Instruments are 32 bytes each: 28 operator bytes, FB/ALG, L/R, two spare.
load_instrument:
        ld      b,a
        ld      a,c
        cp      #4
        ret     nc
        ld      hl,#instrument_table
        ld      a,b
        and     #7
        rrca
        rrca
        rrca                    ; index * 32 (index < 8)
        ld      e,a
        ld      d,#0
        add     hl,de
        ; 28 operator bytes: registers $30,$40,$50,$60,$70,$80,$90 per slot,
        ; slots at +0,+4,+8,+C, plus the channel's register offset.
        push    bc
        call    fm_offset
        ld      (0xf81a),a      ; channel offset scratch
        pop     bc
        push    bc
        ld      b,#0            ; register group counter 0..6
op_group:
        push    bc
        ld      a,b
        add     a,#3
        rlca
        rlca
        rlca
        rlca                    ; (3+group) << 4 = $30, $40 ...
        ld      d,a
        ld      a,(0xf81a)
        add     a,d
        ld      d,a             ; base register for slot 1
        ld      b,#4
op_slot:
        ld      e,(hl)
        inc     hl
        push    hl
        push    de
        push    bc
        call    ym_fm_write
        pop     bc
        pop     de
        pop     hl
        ld      a,d
        add     a,#4
        ld      d,a
        djnz    op_slot
        pop     bc
        inc     b
        ld      a,b
        cp      #7
        jr      c,op_group
        ; FB/ALG and L/R/AMS/PMS.
        ld      a,(0xf81a)
        add     a,#0xb0
        ld      d,a
        ld      e,(hl)
        inc     hl
        push    hl
        call    ym_fm_write
        pop     hl
        ld      a,(0xf81a)
        add     a,#0xb4
        ld      d,a
        ld      e,(hl)
        call    ym_fm_write
        pop     bc
        ret

        .include "songdata.s"
