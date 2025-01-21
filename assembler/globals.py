FILE_MAGIC = "LilVirtualizer"
DATA_SECTION_IDENTIFIER = "[data]"
CODE_SECTION_IDENTIFIER = "[code]"
VARIABLE_ASSIGNMENT_TOKEN = '='
VARIABLE_NAME_MATCH_REGEX = r'^[_a-zA-Z][_a-zA-Z0-9]*$'
VARIABLE_VALUE_MATCH_REGEX_SIGNED_DECIMAL_INTEGER = r'^[-+0-9][0-9]*$'
VARIABLE_VALUE_MATCH_REGEX_SIGNED_HEXADECIMAL_INTEGER = r'^[-+]*0x[0-9][0-9]*$'
VARIABLE_VALUE_SIGNED_INTEGER_MATCH_REGEX = r"^\s*[-+]*0?x?[a-fA-F0-9]+\s*$"
VARIABLE_VALUE_STRING_MATCH_REGEX = r"^\s*\".[^\"]*\"\s*$"
LABEL_MATCH_REGEX = r'^[_a-zA-Z][_a-zA-Z0-9]*:$'
OPERAND_IS_DEREFERENCE = r"^\s*\[.+\]\s*$"
BASE_VARIABLE_SIZE_BYTES = 4
SINGLE_LINE_COMMENT_TOKEN = "//"
# Documentation has explanation for these
VALID_INSTRUCTIONS = ["halt", "load", "add", "sub", "mul", "div", "cmp", "jne", "je", "jb", "ja", "jbe", "jae", "jmp", "xor", "or", "and", "shl", "shr", "loadlibrary", "loadfunction", "callfunction"]
VALID_REGISTERS = ["%%r0", "%%r1", "%%r2", "%%r3", "%%r4", "%%r5", "%%r6", "%%r7", "%%r8", "%%r9"]
COMPILED_INSTRUCTION_SIZE_BYTES = 10
