/* Android-only binary embedding for Prune. Built with the NDK target assembler. */
.section .rodata

#ifndef HCE
.balign 64
.global baseModel
.type baseModel, %object
baseModel:
.incbin "model.bin"
.size baseModel, .-baseModel
#endif

.balign 64
.global magicsData
.type magicsData, %object
magicsData:
.incbin "magics.out"
.size magicsData, .-magicsData
