#pragma once
#include <stdint.h>
#include "globals.h"

// Define the instruction set for the virtual machine.
enum class Opcode : uint8_t {
    HALT,
    LOAD,
    ADD, // add [r1] [r2] store in r1
    SUB,
    MUL,
    DIV,
    CMP,
    JNE,
    JE,
    JB,
    JA,
    JBE,
    JAE,
    JMP,
    XOR,
    OR,
    AND,
    SHL,
    SHR,
    LOADLIBRARY,
    LOADFUNCTION,
    CALLFUNCTION
};

extern const char* OpcodeToString[];

enum class OpcodeFlags : uint8_t
{
    dereferenceOperand1 = 1 << 0, // 00000001
    dereferenceOperand2 = 1 << 1,  // 00000010
    operand1IsRegister = 1 << 2, // 00000100
    operand1IsImmediate = 1 << 3,
    operand2IsRegister = 1 << 4,
    operand2IsImmediate = 1 << 5
};

enum class EFlags : uint8_t
{
    CF = 0,
    ZF,
    MAX_FLAGS_COUNT,
};

enum class Register : uint8_t
{
    R0 = 0, R1, R2, R3, R4, R5, R6, R7, R8, R9, MAX_REGISTER_COUNT
};
extern const char* RegisterToString[];

#pragma pack(1)
struct FileHeader
{
    uint8_t fileHeader[fileHeaderMagicLength];
    uint32_t sizeOfData;
    uint32_t pointerToData;
    uint32_t sizeOfCode;
    uint32_t pointerToCode;
};

inline uint8_t operator&(OpcodeFlags a, OpcodeFlags b)
{
    return static_cast<uint8_t>(static_cast<uint8_t>(a) & static_cast<uint8_t>(b));
}

#pragma pack(1)
struct Instruction {
    Opcode opcode;
    OpcodeFlags opcodeFlags;
    uint32_t operand1;
    uint32_t operand2;
};