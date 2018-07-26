!=================================================================
! General conventions:
!   1) Stack grows from high addresses to low addresses, and the
!      top of the stack points to valid data
!
!   2) Register usage is as implied by assembler names and manual
!
!   3) Function Calling Convention:
!
!       Setup)
!       * Immediately upon entering a function, push the RA on the stack.
!       * Next, push all the registers used by the function on the stack.
!
!       Teardown)
!       * Load the return value in $v0.
!       * Pop any saved registers from the stack back into the registers.
!       * Pop the RA back into $ra.
!       * Return by executing jalr $ra, $zero.
!=================================================================

!vector table
vector0:    .fill 0x00000000 !0
            .fill 0x00000000 !1
            .fill 0x00000000 !2
            .fill 0x00000000
            .fill 0x00000000 !4
            .fill 0x00000000
            .fill 0x00000000
            .fill 0x00000000
            .fill 0x00000000 !8
            .fill 0x00000000
            .fill 0x00000000
            .fill 0x00000000
            .fill 0x00000000
            .fill 0x00000000
            .fill 0x00000000
            .fill 0x00000000 !15
!end vector table
    
main:   lea $sp, stack                 !initialize the stack pointer
        lw $sp, 0($sp)                  !finish initialization          
                
                                        ! Install timer interrupt handler into vector table

        lea $a2, ti_inthandler
		sw $a2, 1($zero)

        ei                              ! Don't forget to enable interrupts...

        lea $a0, BASE                   !load base for pow
        lw $a0, 0($a0)
        lea $a1, EXP                    !load power for pow
        lw $a1, 0($a1)
        lea $at, POW                    !load address of pow
        jalr $at, $ra                   !run pow
        lea $a0, ANS                    !load base for pow
        sw $v0, 0($a0)

        halt

BASE:   .fill 2
EXP:    .fill 7
ANS:    .fill 0                               ! should come out to 32

POW:    addi $sp, $sp, -1                     ! allocate space for old frame pointer
        sw $fp, 0($sp)
        addi $fp, $sp, 0                      !set new frame pinter
        add $a1, $a1, $zero                   ! check if $a1 is zero
        brz RET1                              ! if the power is 0 return 1
        add $a0, $a0, $zero
        brz RET0                              ! if the base is 0 return 0
        addi $a1, $a1, -1                     ! decrement the power
        lea $at, POW                          ! load the address of POW
        addi $sp, $sp, -2                     ! push 2 slots onto the stack
        sw $ra, -1($fp)                        ! save RA to stack
        sw $a0, -2($fp)                        ! save arg 0 to stack
        jalr $at, $ra                         ! recursively call POW
        add $a1, $v0, $zero                   ! store return value in arg 1
        lw $a0, -2($fp)                       ! load the base into arg 0
        lea $at, MULT                         ! load the address of MULT
        jalr $at, $ra                         ! multiply arg 0 (base) and arg 1 (running product)
        lw $ra, -1($fp)                       ! load RA from the stack
        addi $sp, $sp, 2
        brnzp FIN                             ! return
RET1:   addi $v0, $zero, 1                    ! return a value of 1
        brnzp FIN
RET0:   add $v0, $zero, $zero                 ! return a value of 0
FIN:    lw $fp, 0($fp)                        ! restore old frame pointer
        addi $sp, $sp, 1                      ! pop off the stack
        jalr $ra, $zero


MULT:   add $v0, $zero, $zero            ! zero out return value
AGAIN:  add $v0,$v0, $a0                ! multiply loop
        nand $a2, $zero, $zero
        add $a1, $a1, $a2
        brz  DONE                  ! finished multiplying
        brnzp AGAIN                ! loop again
DONE:   jalr $ra, $zero

ti_inthandler:
		sw $k0, 0($sp)			!save k0

		ei 						! enable interrupt 

		addi $sp, $sp, 15		!save process registers
		sw $at, -14($sp)
		sw $v0, -13($sp)
		sw $a0, -12($sp)
		sw $a1, -11($sp)
		sw $a2, -10($sp)
		sw $t0, -9($sp)
		sw $t1, -8($sp)
		sw $t2, -7($sp)
		sw $s0, -6($sp)
		sw $s1, -5($sp)
		sw $s2, -4($sp)
		sw $k0, -3($sp)
		sw $sp, -2($sp)
		sw $fp, -1($sp)
		sw $ra, 0($sp) 
		lea $s0, seconds
		lw $s0, 0($s0)
		lw $a0, 0($s0)
		addi $a0, $a0, 1
		sw $a0, 0($s0)

		lea $t0, seconds		!content of seconds, minutes, hours are stored
		lw $t0, 0($t0)			!respectively into s0, s1, s2
		lw $s0, 0($t0)
		lea $t1, minutes
		lw $t1, 0($t1)
		lw $s1, 0($t1)
		lea $t2, hours
		lw $t2, 0($t2)
		lw $s2, 0($t2)

		addi $a0, $zero, 60		!increment factor
		addi $s0, $s0, 1		!increment second
		addi $a0, $s0, -60		!to check if it need to increment therefore branch
		brz incMins
		!beq $s0, $a0, incMins   !go to increment minute
		addi $a1, $s1, -60
		brz incHrs
		!beq $s1, $a0, incHrs	!go to increment hour
		brnzp return 			!go to return and restore

incMins:addi $s1, $s1, 1		!increment minute
		add $s0, $zero, $zero   !set seconds back to 0
		brnzp return 			!go to return and restore

incHrs:	addi $s2, $s2, 1		!increment hour
		add $s1, $zero, $zero   !set minute back to 0
		add $s0, $zero, $zero   !set second back to 0


return:	!lea $t0, seconds
		sw $s0, 0($t0)
		!lea $t0, minutes
		sw $s1, 0($t1)
		!lea $t0, hours
		sw $s2, 0($t2)


		lw $at, -14($sp)		!restore saved process registers
		lw $v0, -13($sp)
		lw $a0, -12($sp)
		lw $a1, -11($sp)
		lw $a2, -10($sp)
		lw $t0, -9($sp)
		lw $t1, -8($sp)
		lw $t2, -7($sp)
		lw $s0, -6($sp)
		lw $s1, -5($sp)
		lw $s2, -4($sp)
		lw $k0, -3($sp)
		lw $sp, -2($sp)
		lw $fp, -1($sp)
		lw $ra, 0($sp) 
		addi $sp, $sp, -15

		di						!disable interrupts

		lw $k0, 0($sp)			!restore $k0 back

		reti 					!return from interrupt




stack:      .fill 0xA00000
seconds:    .fill 0xFFFFFC
minutes:    .fill 0xFFFFFD
hours:      .fill 0xFFFFFE
