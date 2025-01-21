# LilVirtualizer documentation

## Goal
Goal of this project is to teach myself about virtual machines. Here are the features to be implemented:
- Project should have basic working assembly instructions
- Project should have basic labels, jumping to labels etc.
- Be able to define variables, arrays and strings
- Be able to import DLL  and call any function within the dll.
- support comments

## General
Throughout the whole source file you define immediates, immediates can be written as integers (base 10) like 123 or -539. They can be also written as hexadecimal like 0x123, 0x53 or -0x54. This way of representing integers is also used in storing variables and arrays.

In the source input file, we use [data] tag to specify data section and [code] tag to specify the code section. In data section we put variables and arrays (data) and in code section we put the program code.
Variables can contain lowercase and uppercase letters, numbers and underscores but must start with a letter or an underscore.
<br>This is how you define a variable:
```
myVariable = 0x123
```

You can also define arrays. However if you use them in your program you must keep track of the length by yourself.
How you define an array (array elements are separated by whitespace): 
```
myArray = 0x10 0x20 -50 -12 0x1234
```

You can define strings. All strings are stored as null-terminated:
```
myString = "hello world"
```

You can also data buffer like so:
```
myBuffer=(123)
```
If you allocate memory like this all of it will be initialized to zero.
Number inside brackets specifies how many bytes of memory we will allocate. This is useful if you wish to take input from the user.

You can write comments by prefixing string in source file with //

You define label in the code section by writing a valid label. Valid label is a string that is of format of a valid variable name and ends in ':'. <br>Example of a valid label:
```
myLabel:
```

### Loading dlls & calling their functions

## Registers
Registers are r0, r1, ..., r9.
					
## Data
Everything works with dwords and are treated as signed integers.


## Instruction encoding
Every instruction is 10 bytes in length.<br> This is the format of the instruction: `[opcode(byte)] [opcodeFlags(byte)] [operand1(dword)] [operand2(dword)]`


## Instructions
```
load [dest] [src]
```

Destination can be immediate (in this case it is actually memory address) or register, source can be register or immediate.

Under the hood load has opcodes: `loadii`, `loadrr`, `loadir`. It is important to note that on the left side the i letter actually means a memory address. On the right side it is just an immediate, however if you were to do something like `load %r0, myVar1`, that would be assembled as `loadri %r0, 0x123` where 0x123 is actually the relative address of the variable myVar1.

We can use dereference "[]" with either destination or source operand. If we do that, then we will encode the opcodeFlags accordingly, if we only dereference the operand1, we set opcodeFlags to 00000010, if we dereference operand2 we set it to 00000001 and if we dereference both we will set it to 00000011.

```
cmp [register] [register]
```
Only registers comparison for the sake of simplicity. CMP works like in normal x86 assembly. If operands are equal, ZF flag is set to 1. Otherwise it is set to 0. If operand1 < operand2 the CF is set to 1 otherwise it is set to 0.

```
jcc [label]
```
Will check ZF and CF and jump based on those values. Valid jcc:s are ``je, jne, jb, jbe, ja, jae``

```
input [pBuffer] [bufferSize]
```

Get input from command line, store in buffer

```
output [szVariableName]
```
Output to console


## Opcode flags
```
enum class OpcodeFlags : uint8_t
{
	dereferenceOperand1 = 1 << 0, // 00000001
	dereferenceOperand2 = 1 << 1,  // 00000010
	operand1IsRegister = 1 << 2, // 00000100
	operand1IsImmediate = 1 << 3,
	operand2IsRegister = 1 << 4,
	operand2IsImmediate = 1 << 5
};
```

## Debugger
*(Debugger documentation is work in progress)*<br>
Debugger shows always register values and code around current instruction. 


Single step by pressing enter.
#### Debugger commands
``continue`` - execute program normally, stop at breakpoints<br>
``bp [addressBase16]`` - toggle breakpoint at address<br>
``memdump [addressBase16] [lenBase10]`` - dump memory to console starting at address<br>