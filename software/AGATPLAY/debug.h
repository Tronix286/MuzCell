/** Debug6502() **********************************************/
/** This function should exist if DEBUG is #defined. When   **/
/** Trace!=0, it is called after each command executed by   **/
/** the CPU, and given the 6502 registers. Emulation exits  **/
/** if Debug6502() returns 0.                               **/
/*************************************************************/
byte Debug6502(register M6502 *R);
