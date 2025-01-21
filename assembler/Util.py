from globals import *   
import re
import struct

currentLineNumberOfSourceFile = 1

def ExitWithError(message:str):
    print("Assembler error")
    print(f"[Line {currentLineNumberOfSourceFile}] {message}")
    exit(-1)

def IncrementCurrentLine():
    global currentLineNumberOfSourceFile
    currentLineNumberOfSourceFile += 1

def InitializeInstructions():
    from Instruction import Instruction, OperandType
    # !!! MAKE SURE TO USE CAPITAL LETTERS 
    #data movement
    Instruction.New("load", [OperandType.REGISTER, OperandType.IMMEDIATE], [OperandType.REGISTER, OperandType.IMMEDIATE])
    
    Instruction.New("halt", [], [])
    Instruction.New("add", [OperandType.REGISTER], [OperandType.REGISTER])
    Instruction.New("div", [OperandType.REGISTER], [OperandType.REGISTER])
    Instruction.New("mul", [OperandType.REGISTER], [OperandType.REGISTER])
    Instruction.New("and", [OperandType.REGISTER], [OperandType.REGISTER])
    Instruction.New("xor", [OperandType.REGISTER], [OperandType.REGISTER])
    Instruction.New("or", [OperandType.REGISTER], [OperandType.REGISTER])
    Instruction.New("jmp", [OperandType.LABEL], [])

    Instruction.New("cmp", [OperandType.REGISTER], [OperandType.REGISTER])

    # control flow
    Instruction.New("jne", [OperandType.LABEL], [])
    Instruction.New("je", [OperandType.LABEL], [])
    Instruction.New("jb", [OperandType.LABEL], [])
    Instruction.New("jbe", [OperandType.LABEL], [])
    Instruction.New("jae", [OperandType.LABEL], [])
    Instruction.New("ja", [OperandType.LABEL], [])

    # other
    Instruction.New("loadlibrary", [], [])
    Instruction.New("loadfunction", [], [])
    Instruction.New("callfunction", [OperandType.REGISTER, OperandType.IMMEDIATE], [OperandType.REGISTER, OperandType.IMMEDIATE])


def IsValidRegister(string:str)->list[bool, str, int]:
    return string in VALID_REGISTERS

def IsValidIdentifier(string:str)->bool:
    return re.match(VARIABLE_NAME_MATCH_REGEX, string)

def TryParseInteger(string)->list[bool, str, int]:
    
    match = re.findall(VARIABLE_VALUE_SIGNED_INTEGER_MATCH_REGEX, string)
    
    if (len(match) != 1):
        print("no match")
        return False, "", 0
    
    if ("0x" in match[0]):
        matchInteger = int(match[0].replace("0x",""), 16)
    else:
        matchInteger = int(match[0])
    print(f"matchInteger: {matchInteger}")
    return True, "", matchInteger

def RemoveCommentsFromLine(string):
    index = string.find('//')
    if index >= 0:
        return string[:index]
    return string
