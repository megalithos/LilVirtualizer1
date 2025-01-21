import argparse
import re
from enum import Enum
from globals import *
from Variable import Variable
from Instruction import *
from Parser import *
import Util
from FileHeader import FileHeader

InitializeInstructions()

# Create an ArgumentParser object
parser = argparse.ArgumentParser()

# Add the -i and -o command line arguments
parser.add_argument('-i', '--input', required=True, help='input code file')
parser.add_argument('-o', '--output', required=True, help='output code filename')

# Parse the command line arguments
args = parser.parse_args()

# Get the input and output file names from the command line arguments
input_filename = args.input
output_filename = args.output
pointerToData = 0

FileHeader.fileHeaderMagic = FILE_MAGIC
FileHeader.SetDataPointer()

# Open the output file in binary write mode
with open(output_filename, 'wb') as outputFile:
    # Write the bytes to the beginning of the file
    # Open the file in read mode
    with open(input_filename, 'r') as f:
        # Read all the lines of the file
        lines = f.readlines()

        # Flag to track whether we are past the "[data]" line
        currentlyIteratedSection = ""

        # Iterate over the lines
        for line in lines:
            # remove comments
            line = RemoveCommentsFromLine(line)

            if DATA_SECTION_IDENTIFIER in line:
                currentlyIteratedSection = DATA_SECTION_IDENTIFIER
            elif CODE_SECTION_IDENTIFIER in line:
                currentlyIteratedSection = CODE_SECTION_IDENTIFIER
            elif (currentlyIteratedSection==DATA_SECTION_IDENTIFIER):
                
                variable = TryParseVariableRightHandArgumentFromLine(line)
            elif (currentlyIteratedSection==CODE_SECTION_IDENTIFIER):

                sizeOfData = Variable.GetSizeOfData()
                if (sizeOfData == 0):
                    ExitWithError("Data section needs to come first!")
                
                FileHeader.sizeOfData = sizeOfData
                print(f"Assembling line: {line}")
                TryParseInstructionFromLine(line)
                #outputFile.write(instructionBytes)
            IncrementCurrentLine()
    
    FileHeader.SetCodePointer()
    compiledCode = CompiledInstruction.GetCompiledCode()
    outputFile.write(FileHeader.fileHeaderMagic.encode())
    outputFile.write(struct.pack("<I", FileHeader.sizeOfData))
    outputFile.write(struct.pack("<I", FileHeader.pointerToData))
    outputFile.write(struct.pack("<I", len(compiledCode)))
    outputFile.write(struct.pack("<I", FileHeader.pointerToCode))
    outputFile.write(Variable.GetData())
    outputFile.write(compiledCode)

Variable.PrintVariables()
        
        


print('Assembled to \"{}\"'.format(output_filename))