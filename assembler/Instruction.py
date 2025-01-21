from enum import Enum
from Util import IsValidRegister, IsValidIdentifier, TryParseInteger, ExitWithError
from Variable import Variable
from globals import *
import struct
from FileHeader import FileHeader
import re
from Label import Label
import binascii

class OperandType(Enum):
    REGISTER = 1
    MEMORY = 2
    IMMEDIATE = 3
    LABEL = 4

class OpcodeFlags():
	dereferenceOperand1 = 1 << 0
	dereferenceOperand2 = 1 << 1
	operand1IsRegister = 1 << 2
	operand1IsImmediate = 1 << 3
	operand2IsRegister = 1 << 4
	operand2IsImmediate = 1 << 5

from typing import List

class Instruction:
    allInstructions = []

    def __init__(self, name: str, validOperandsForOperand1: List[OperandType], validOperandsForOperand2: List[OperandType]):
        self.name = name
        self.validOperandsForOperand1 = validOperandsForOperand1
        self.validOperandsForOperand2 = validOperandsForOperand2
        Instruction.allInstructions.append(self)

    @staticmethod
    def New(name: str, validOperandsForOperand1: List[OperandType], validOperandsForOperand2: List[OperandType]):
        assert isinstance(validOperandsForOperand1, list) and isinstance(validOperandsForOperand2, list)
        return Instruction(name, validOperandsForOperand1, validOperandsForOperand2)

    @staticmethod
    def GetByName(name:str):
        for instruction in Instruction.allInstructions:
            if (instruction.name == name):
                return instruction
        
        return None
codeData = b""

class CompiledInstruction:
    allCompiledInstructions = []
    nextIntructionAddress = 0

    def __init__(self):
        self.opcode = b''
        self.opcodeFlags = b''
        self.operand1 = b''
        self.operand2 = b''
        self.address = CompiledInstruction.nextIntructionAddress
        CompiledInstruction.nextIntructionAddress += COMPILED_INSTRUCTION_SIZE_BYTES
        CompiledInstruction.allCompiledInstructions.append(self)
    
    def SetOperand1(self, value):
        self.operand1 = value
    
    def SetOperand2(self, value):
        self.operand2 = value

    @staticmethod
    def GetCompiledCode():
        compiledCode = b''
        for compiledInstruction in CompiledInstruction.allCompiledInstructions:
            compiledCode += compiledInstruction.opcode
            compiledCode += compiledInstruction.opcodeFlags
            compiledCode += compiledInstruction.operand1
            compiledCode += compiledInstruction.operand2
        return compiledCode
    
    @staticmethod
    def GetNextCompiledInstructionAddress() -> int:
        return CompiledInstruction.nextIntructionAddress


def IsValidLabel(string:str)->bool:
    return re.match(LABEL_MATCH_REGEX, string)

class Operand:
    def __init__(self, type, string):
        self.type = type
        self.string = string

def GetOperand(string):
    if (IsValidIdentifier(string)):
        return Operand(OperandType.IMMEDIATE, )


def GetOperands(instructionStringArray, instructionObject, compiledInstruction):
    """
    Determines operand type and the string

    Returns:
        Tuple(operand1, operand2)
    """
    operand1 = instructionStringArray[1]
    operand2 = instructionStringArray[2]

    flags = 0
    operand1Bytes = b''
    operand2Bytes = b''

    if re.search(OPERAND_IS_DEREFERENCE, operand1):
        flags |= OpcodeFlags.dereferenceOperand1
    if re.search(OPERAND_IS_DEREFERENCE, operand2):
        flags |= OpcodeFlags.dereferenceOperand2
    
    # remove dereference tokens
    operand1 = operand1.replace("[", "")
    operand1 = operand1.replace("]", "")
    operand2 = operand2.replace("[", "")
    operand2 = operand2.replace("]", "")

    operand2IsInteger, message, operand2AsInteger = TryParseInteger(operand2)

    # integer
    if (operand2IsInteger):
        operand2Bytes = struct.pack("<i", operand2AsInteger)
        flags |= OpcodeFlags.operand2IsImmediate
    if IsValidRegister(operand1):
        operand1Bytes = struct.pack("<i", VALID_REGISTERS.index(operand1))
        flags |= OpcodeFlags.operand1IsRegister
    if IsValidRegister(operand2):
        operand2Bytes = struct.pack("<i", VALID_REGISTERS.index(operand2))
        flags |= OpcodeFlags.operand2IsRegister
    # variable
    if IsValidIdentifier(operand1) and OperandType.IMMEDIATE in instructionObject.validOperandsForOperand1:
        operand1Bytes = struct.pack("<i", Variable.GetByName(operand1).relativeAddress)
        flags |= OpcodeFlags.operand1IsImmediate
    # variable
    if IsValidIdentifier(operand2) and OperandType.IMMEDIATE in instructionObject.validOperandsForOperand2:
        operand2Bytes = struct.pack("<i", Variable.GetByName(operand2).relativeAddress)
        flags |= OpcodeFlags.operand2IsImmediate
    # label
    if IsValidIdentifier(operand1) and OperandType.LABEL in instructionObject.validOperandsForOperand1:
        label = Label.GetLabel(operand1)
        flags |= OpcodeFlags.operand1IsImmediate
        # if label exists, set first operand to it's relative address
        if (label != None):
            operand1Bytes = struct.pack("<i", label.relativeAddress)
        else:
            Label.WaitForLabel(operand1, compiledInstruction)
    
    if (len(instructionObject.validOperandsForOperand1) > 0 and len(operand1) == 0):
        ExitWithError("Expected identifier type of {} for first operand.".format(instructionObject.validOperandsForOperand1))

    if (len(instructionObject.validOperandsForOperand2) > 0 and len(operand2) == 0):
        ExitWithError("Expected identifier type of {} for first operand.".format(instructionObject.validOperandsForOperand2))
    
    # if valid number of params but operand not set, fill with zeros
    if (len(operand1)==0):
        operand1Bytes = struct.pack("<i", 0)
    if (len(operand2)==0):
        operand2Bytes = struct.pack("<i", 0)
    opcodeFlagsBytes = struct.pack("B", flags)

    return opcodeFlagsBytes, operand1Bytes, operand2Bytes



def TryAssembleInstruction(instructionStringArray: List[str]) -> tuple[bool, str, str]:
    """
    Args:
        Instruction string
    Returns:
        tuple containing success boolean and error message
    """
    print("-----------------------")
    print(instructionStringArray)
    # handle label
    if (len(instructionStringArray) == 1 and IsValidLabel(instructionStringArray[0])):
        labelNameWithColon = instructionStringArray[0]
        labelName = labelNameWithColon[0:len(labelNameWithColon)-1]

        Label(labelName, CompiledInstruction.GetNextCompiledInstructionAddress())

        return True, "", ""
    elif (len(instructionStringArray)==0): #empty line
        return True, "", ""
    
    global codeData

    # just quick fix so we dont get IndexErrors
    if (len(instructionStringArray) < 3):
        for i in range(0, 3-len(instructionStringArray)):
            instructionStringArray.append("")
    
    # check if the instruction is valid
    instructionObject = Instruction.GetByName(instructionStringArray[0])
    if (instructionObject == None):
        return False, "Invalid instruction", ""
    
    
    # create the CompiledInstruction object
    compiledInstruction=CompiledInstruction()
    opcodeFlags, operand1Bytes, operand2Bytes = GetOperands(instructionStringArray, instructionObject, compiledInstruction)

    compiledInstruction.opcode = struct.pack("B", VALID_INSTRUCTIONS.index(instructionObject.name))
    compiledInstruction.opcodeFlags = opcodeFlags
    compiledInstruction.operand1 = operand1Bytes
    compiledInstruction.operand2 = operand2Bytes
    
    print(f"Assembled instruction: {binascii.hexlify(compiledInstruction.opcode)} {binascii.hexlify(compiledInstruction.opcodeFlags)} {binascii.hexlify(compiledInstruction.operand1)} {binascii.hexlify(compiledInstruction.operand2)}")

    return True, "", ""

def GetCodeData():
    global codeData
    return codeData