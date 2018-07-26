!============================================================
! CS-2200 Homework 1
!
! Please do not change main's functionality, 
! except to change the argument for factorial or to meet your 
! calling convention
!============================================================

main:  	la $sp, stack		! load ADDRESS of stack label into $sp
		lw $sp, 0($sp)	
       	
		la $at, factorial	! load address of factorial label into $at
		addi $a0, $zero, 5	! $a0 = 5, the number to factorialize
		jalr $at, $ra		! jump to factorial, set $ra to return addr
		halt				! when we return, just halt

factorial:	! change me to your factorial implementation

		addi $sp, $sp, -1	! allocate space for frampointer
		sw $fp, 0($sp)		! store sp
		addi $fp, $sp, 0	! update frampointer

		addi $sp, $sp, -1	! allocate space for local var
		sw $a0, 0($sp)		! store local var

		beq $a0, $zero, baseCase	! if a0 == 0 then return
		
		addi $a0, $a0, -1	!subtract 1 from argument

		addi $sp, $sp, -1	! allocate space for return address
		sw $ra, 0($sp)		! store RA, as caller

		jalr $at, $ra		! this will recursively call factorial.
		
		lw $ra, 0($sp)		! restore RA
		addi $sp, $sp, 1	! deallocate stack space
				
		add $t0, $zero, $zero	! t0 is 0
		lw $t1, 0($sp)			! bring arugment from memory into t1
multiply:
		beq $t1, $zero, multDone 	! if mult is done go to return
		addi $t1, $t1, -1		! multiplier counter
		add $t0, $v0, $t0		! store multiplied value into t0
		beq $zero, $zero, multiply ! loop
multDone:
		add $v0, $t0, $zero		! update v0 with t0
		beq $zero, $zero, return! go to return
		
baseCase:
		addi $v0, $zero, 1	! return 1
return: 
		lw $fp, 1($sp)		! restore old frame pointer
		addi $sp, $sp, 2	! clean up stack (frame pointer and local variable)
				
		jalr $ra, $zero		! return to return address
		
stack:	.word 0x4000		! the stack begins here (for example, that is)

