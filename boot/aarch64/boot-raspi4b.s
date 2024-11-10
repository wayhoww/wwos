.section ".text.wwos.boot"

.global _start

_start:
    // Configure HCR_EL2
    // ------------------
    ORR      w0, wzr, #(1 << 3)             // FMO=1
                                            // IMO=0         Route IRQ to EL1
    ORR      x0, x0,  #(1 << 31)            // RW=1          NS.EL1 is AArch64
                                            // TGE=0         Entry to NS.EL1 is possible
                                            // VM=0          Stage 2 MMU disabled
    ORR      x0, x0, #(1 << 34) 
    MSR      HCR_EL2, x0


    // Set up VMPIDR_EL2/VPIDR_EL1
    // ---------------------------
    MRS      x0, MIDR_EL1
    MSR      VPIDR_EL2, x0
    MRS      x0, MPIDR_EL1
    MSR      VMPIDR_EL2, x0
    

    // Set VMID
    // ---------
    // Although we are not using stage 2 translation, NS.EL1 still cares
    // about the VMID
    MSR     VTTBR_EL2, xzr


    // Set SCTLRs for EL1/2 to safe values
    // ------------------------------------
    MSR     SCTLR_EL2, xzr
    MSR     SCTLR_EL1, xzr

    MOV x0, #0b0101;
    MSR SPSR_EL2, x0;
    
    MOV x0, sp;
    MSR SP_EL1, x0;

    ADR x0, .;
    ADD x0, x0, #16;
    MSR ELR_EL2, x0;
    ERET

    NOP

    MRS     x0, MPIDR_EL1
    AND     x0, x0, #3
    CBZ     x0, begin

1:  WFE
    B       1b

begin:
    LDR     x1, =stack_top
    MOV     sp, x1 
    BL      loader_main
    

.section ".data.wwos.kernel"
.INCBIN "kernel/kernel.img"

.section ".data.wwos.memdisk"
.INCBIN "memdisk.wwfs"

