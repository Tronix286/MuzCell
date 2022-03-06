
option expr32
option casemap:none

; Begin of file export.h
NULL		EQU		0t
RTCFREQ		EQU		4000t
Bit8u		TYPEDEF		BYTE

Bitu		TYPEDEF		WORD

false		EQU		0t
true		EQU		1t

bool		TYPEDEF		SWORD

MPU_EVENT		EQU		0t
RESET_DONE		EQU		1t
EOI_HANDLER		EQU		2t
NUM_EVENTS		EQU		3t

EventID		TYPEDEF		SWORD

M_MPU401		EQU		0t
M_SBMIDI		EQU		1t
M_SERIAL		EQU		2t
M_CELL		EQU		3t

OutputMode		TYPEDEF		SWORD

@t_0		TYPEDEF		FAR PTR 
@tag_0		STRUCT 2t
installed		bool		?
qpi_entry		@t_0		?
@tag_0		ENDS

QEMMInfo		TYPEDEF		@tag_0

@proto_0		TYPEDEF		PROTO C :bool
PIC_Update		PROTO		@proto_0

@proto_1		TYPEDEF		PROTO C :FAR PTR , :Bitu, :Bitu, :Bitu, :Bitu, :Bitu, :OutputMode, :bool, :bool
MPU401_Init		PROTO		@proto_1

@proto_2		TYPEDEF		PROTO C :bool
MPU401_SetEnableSBIRQ		PROTO		@proto_2

@proto_3		TYPEDEF		PROTO C :bool
MPU401_SetEnableMPUVerFix		PROTO		@proto_3

@proto_4		TYPEDEF		PROTO C 
MPU401_ReadStatus		PROTO		@proto_4

@proto_5		TYPEDEF		PROTO C :Bitu
MPU401_WriteCommand		PROTO		@proto_5

@proto_6		TYPEDEF		PROTO C 
MPU401_ReadData		PROTO		@proto_6

@proto_7		TYPEDEF		PROTO C :Bitu
MPU401_WriteData		PROTO		@proto_7

; End of file export.h
