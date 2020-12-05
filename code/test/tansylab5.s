	.file	1 "tansylab5.c"
gcc2_compiled.:
__gnu_compiled_c:
	.rdata
	.align	2
$LC0:
	.ascii	"test_fork\000"
	.align	2
$LC1:
	.ascii	"this content is for test fork func\000"
	.text
	.align	2
	.globl	func
	.ent	func
func:
	.frame	$fp,40,$31		# vars= 16, regs= 2/0, args= 16, extra= 0
	.mask	0xc0000000,-4
	.fmask	0x00000000,0
	subu	$sp,$sp,40
	sw	$31,36($sp)
	sw	$fp,32($sp)
	move	$fp,$sp
	la	$2,$LC0
	sw	$2,16($fp)
	lw	$4,16($fp)
	jal	Create
	li	$3,-1			# 0xffffffff
	beq	$2,$3,$L3
	lw	$4,16($fp)
	jal	Open
	sw	$2,20($fp)
	lw	$2,20($fp)
	li	$3,-1			# 0xffffffff
	beq	$2,$3,$L3
	li	$2,35			# 0x23
	sw	$2,24($fp)
	la	$4,$LC1
	lw	$5,24($fp)
	lw	$6,20($fp)
	jal	Write
	lw	$4,20($fp)
	jal	Close
$L4:
$L3:
	move	$4,$0
	jal	Exit
$L2:
	move	$sp,$fp
	lw	$31,36($sp)
	lw	$fp,32($sp)
	addu	$sp,$sp,40
	j	$31
	.end	func
	.align	2
	.globl	main
	.ent	main
main:
	.frame	$fp,32,$31		# vars= 8, regs= 2/0, args= 16, extra= 0
	.mask	0xc0000000,-4
	.fmask	0x00000000,0
	subu	$sp,$sp,32
	sw	$31,28($sp)
	sw	$fp,24($sp)
	move	$fp,$sp
	jal	__main
	la	$4,func
	jal	Fork
	sw	$2,16($fp)
	move	$4,$0
	jal	Exit
$L5:
	move	$sp,$fp
	lw	$31,28($sp)
	lw	$fp,24($sp)
	addu	$sp,$sp,32
	j	$31
	.end	main
