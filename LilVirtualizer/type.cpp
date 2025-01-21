#include "type.h"
const char* OpcodeToString[] = { "halt", "load", "add", "sub", "mul", "div", "cmp", "jne", "je", "jb", "ja",
"jbe", "jae", "jmp", "xor", "or", "and", "shl", "shr", "loadlibrary", "loadfunction", "callfunction"};
const char* RegisterToString[] = { "%r0", "%r1", "%r2", "%r3", "%r4",  "%r5",  "%r6",  "%r7",  "%r8",  "%r9" };