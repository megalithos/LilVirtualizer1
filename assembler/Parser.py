from globals import *
from Util import *
from Variable import *
from Instruction import *
import re

def StringIntBase10OrBase16ToInt(intString)->int:
    if ("0x" in intString):
        matchInteger = int(intString.replace("0x",""), 16)
    else:
        matchInteger = int(intString)

    return matchInteger


def TryParseVariableRightHandArgumentFromLine(line:str)->Variable:
    try:
        line = line.replace("\n", "")
        
        line = line.split(VARIABLE_ASSIGNMENT_TOKEN)
        variableName = line[0].replace(" ", "")
        if (not IsValidIdentifier(variableName)):
            return None
        
        # ---------------------------------------------
        # string initialization
        # ---------------------------------------------
        variableValue = line[1]
        quoteCount = variableValue.count("\"")
        if (quoteCount > 2):
            ExitWithError("Invalid string format. Make sure to only use 2 quotes when defining a string.")
        elif (quoteCount == 2):
            start = variableValue.find('"')
            end = variableValue.find('"', start + 1)



            substring = variableValue[start+1:end]
            if (len(substring) == 0):
                ExitWithError("String cannot be empty.")
            
            print(f"sub: {substring}")

            substring = substring.replace("\\n", "\n")
            variableDataAsNullTerminatedByteString = substring.encode() + bytes([0])
            Variable.NewVariable(variableName, variableDataAsNullTerminatedByteString)
            return

        
        
        match = variableValue.split(" ")
        
        toRemove =[]
        for i in match:
            if (len(i) == 0):
                toRemove.append(i)
        
        for i in toRemove:
            match.remove(i)

        print(match)
        
        arrayOfBytes = b''
        
        # ---------------------------------------------
        # memory allocation with zeroes
        # ---------------------------------------------
        currentElement = match[0]
        startParentheses = currentElement.find("(")
        endParentheses = currentElement.find(")")

        if (startParentheses<endParentheses):
            # we allocate memory here
            currentElement = currentElement[startParentheses+1:endParentheses]
            integer = StringIntBase10OrBase16ToInt(currentElement)

            # fill with (integer) x zeroes
            for i in range(integer):
                arrayOfBytes += bytes([0])
            Variable.NewVariable(variableName, arrayOfBytes)
            return
        
        # ---------------------------------------------
        # array
        # ---------------------------------------------
        for matchIntegerString in match:
            matchInteger = StringIntBase10OrBase16ToInt(matchIntegerString)
            print("attempting to pack matchInteger: " + str(matchInteger))
            pack = struct.pack("<q", matchInteger)
            arrayOfBytes += pack
        
        Variable.NewVariable(variableName, arrayOfBytes)
        
    except IndexError as e:
        ExitWithError("Couldn't parse variable")

def TryParseInstructionFromLine(line:str):
    # try:
    tokens = ",! "
    line= line.strip()
    line = line.replace("%", "%%")
    pattern = "|".join(map(re.escape, tokens))
    line = re.split(pattern, line)
    
    line = [s for s in line if s != ""]
    [success, message, result] = TryAssembleInstruction(line)
    
    print(result)

    if (not success):
        ExitWithError(f"Couldn't assemble instruction: {message}")
    else:
        return result

        
    # except IndexError as e:
    #     ExitWithError(f"Couldn't parse instruction error:{e}")
