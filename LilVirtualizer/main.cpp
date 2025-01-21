#include <iostream>
#include <cstdint>
#include <vector>
#include "utils.h"
#include "debugger.h"
#include "type.h"
#include "globals.h"
#include <Windows.h>
#include <tchar.h>
#include <thread>
#include <cassert>
#define REGISTER_COUNT 9

//* ------------------------------------------------ */
// forward declarations
//* ------------------------------------------------ */
enum class ConsoleCommandType
{
    OUTPUT,
    INPUT,
    MAX_COUNT
};


void DumpRegisters();
DWORD InitializeConsole();
DWORD ConnectToConsole();
DWORD ConsoleCommand(ConsoleCommandType commandType, const char buffer[], size_t bufferLength);
bool runConsoleMutexCheckerThread = true;
void ConsoleMutexCheckerThread();

//* ------------------------------------------------ */
// globals
//* ------------------------------------------------ */
// Implement the virtual machine's memory and registers.
constexpr int CODE_SECTION_SIZE = 65536;
uint8_t codeSection[CODE_SECTION_SIZE];
constexpr int DATA_SECTION_SIZE = 65536;
uint8_t dataSection[DATA_SECTION_SIZE];
size_t sizeOfCode = 0;

uint8_t accumulator = 0;
uint16_t program_counter = 0;
#define BUFSIZE 512

HANDLE hPipe = INVALID_HANDLE_VALUE;
HANDLE hCreatedConsole = nullptr;
const LPCTSTR lpszPipename = TEXT("\\\\.\\pipe\\lilvirtualizercomms");
const wchar_t* lpszMutexName = L"lilvirtualizervmmutex";
const wchar_t* lpszMutexConsoleName = L"lilvirtualizervmmutexconsole";

const wchar_t* lpszEventInitializationDoneName = L"lilvirtualizereventinitializationdone";
uint32_t registers[(int)Register::MAX_REGISTER_COUNT];
uint8_t eflags[(int)EFlags::MAX_FLAGS_COUNT];

// Load a program into the virtual machine's memory.
void LoadProgram(char* buffer, size_t size) {
    FileHeader* fileHeader = (FileHeader*)buffer;
    char* data = buffer + fileHeader->pointerToData;
    memcpy_s(dataSection, DATA_SECTION_SIZE, buffer + fileHeader->pointerToData, fileHeader->sizeOfData);

    sizeOfCode = fileHeader->sizeOfCode;

    char* code = buffer + fileHeader->pointerToCode;
    Instruction* currentInstruction = (Instruction*)(code);
    for (size_t i = 0; i < fileHeader->sizeOfCode/9; i++) {
        unsigned long offset = i * sizeof(Instruction);
        codeSection[offset] = static_cast<uint8_t>(currentInstruction->opcode);
        offset += sizeof(Opcode);
        codeSection[offset] = static_cast<uint8_t>(currentInstruction->opcodeFlags);
        offset += sizeof(OpcodeFlags);
        *(uint32_t*)(codeSection+offset) = currentInstruction->operand1;
        offset += sizeof(Instruction::operand1);
        *(uint32_t*)(codeSection+offset) = currentInstruction->operand2;
        offset += sizeof(Instruction::operand2);
        currentInstruction++;
    }
}

// Fetch the next instruction from memory.
Instruction* FetchInstruction() {
    Instruction* instruction = (Instruction*)(&codeSection[program_counter]);
    program_counter += sizeof(Instruction);
    return instruction;
}

// Execute the specified instruction.
void ExecuteInstruction(const Instruction& instr) {
    switch (instr.opcode) {
    case Opcode::HALT:
        std::cout << "HALT\n";
        break;
    /* data movement */
    case Opcode::LOAD:
    {
        int32_t valueToCopy = 0;
        if (instr.opcodeFlags & OpcodeFlags::operand2IsImmediate)
            valueToCopy = instr.operand2;
        if (instr.opcodeFlags & OpcodeFlags::operand2IsRegister)
            valueToCopy = registers[instr.operand2];
        if (instr.opcodeFlags & OpcodeFlags::dereferenceOperand2)
            valueToCopy = dataSection[valueToCopy];

        if (instr.opcodeFlags & OpcodeFlags::dereferenceOperand1)
        {
            if (instr.opcodeFlags & OpcodeFlags::operand1IsRegister)
                dataSection[registers[instr.operand1]] = valueToCopy;
            else if (instr.opcodeFlags & OpcodeFlags::operand1IsImmediate)
                dataSection[dataSection[instr.operand1]] = valueToCopy;
        }
        else
        {
            if (instr.opcodeFlags & OpcodeFlags::operand1IsRegister)
                registers[instr.operand1] = valueToCopy;
            else if (instr.opcodeFlags & OpcodeFlags::operand1IsImmediate)
                dataSection[instr.operand1] = valueToCopy;
        }

        break;

    }
    /* arithmetic & bitwise operations */
    case Opcode::ADD:
        registers[instr.operand1] += registers[instr.operand2];
        break;
    case Opcode::SUB:
        registers[instr.operand1] -= registers[instr.operand2];
        break;
    case Opcode::MUL:
        registers[instr.operand1] *= registers[instr.operand2];
        break;
    case Opcode::DIV:
        registers[instr.operand1] /= registers[instr.operand2];
        break;
    case Opcode::SHL:
        registers[instr.operand1] <<= registers[instr.operand2];
        break;
    case Opcode::SHR:
        registers[instr.operand1] >>= registers[instr.operand2];
        break;
    case Opcode::AND:
        registers[instr.operand1] &= registers[instr.operand2];
        break;
    case Opcode::XOR:
        registers[instr.operand1] ^= registers[instr.operand2];
        break;
    case Opcode::OR:
        registers[instr.operand1] |= registers[instr.operand2];
        break;
    /* condition stuff */
    case Opcode::CMP:
        if (registers[instr.operand1] == registers[instr.operand2])
            eflags[(uint8_t)EFlags::ZF] = (uint8_t)1;
        else
            eflags[(uint8_t)EFlags::ZF] = (uint8_t)0;

        if (registers[instr.operand1] < registers[instr.operand2])
            eflags[(uint8_t)EFlags::CF] = (uint8_t)1;
        else if (registers[instr.operand1] > registers[instr.operand2])
            eflags[(uint8_t)EFlags::CF] = (uint8_t)0;

        break;
    /* control flow */
    case Opcode::JMP:
        program_counter = (uint16_t)(instr.operand1);
        break;
    case Opcode::JE:
        if (eflags[(uint8_t)EFlags::ZF] == 1)
            program_counter = (uint16_t)(instr.operand1);
        break;
    case Opcode::JNE:
        if (eflags[(uint8_t)EFlags::ZF] == 0)
            program_counter = (uint16_t)(instr.operand1);
        break;
    case Opcode::JB:
        if (eflags[(uint8_t)EFlags::CF] == 1)
            program_counter = (uint16_t)(instr.operand1);
        break;
    case Opcode::JBE:
        if (eflags[(uint8_t)EFlags::CF] == 1 || eflags[(uint8_t)EFlags::ZF] == 1)
            program_counter = (uint16_t)(instr.operand1);
        break;
    case Opcode::JA:
        if (eflags[(uint8_t)EFlags::CF] == 0)
            program_counter = (uint16_t)(instr.operand1);
        break;
    case Opcode::JAE:
        if (eflags[(uint8_t)EFlags::CF] == 0 || eflags[(uint8_t)EFlags::ZF] == 1)
            program_counter = (uint16_t)(instr.operand1);
        break;
    case Opcode::LOADLIBRARY:
    {
        char* libName = (char*)&dataSection[registers[0]];
        HMODULE hModule = LoadLibraryA(libName);
        if (hModule == NULL)
        {
            std::cout << "Failed to load library " << libName << std::endl;
            exit(EXIT_FAILURE);
        }

        *(uint64_t*)(dataSection + registers[1]) = (uint64_t)hModule;
        break;
	}
    case Opcode::LOADFUNCTION:
    {
        char* functionName = (char*)&dataSection[registers[0]];
        HMODULE hModule = *(HMODULE*)&dataSection[registers[2]];
        FARPROC functionAddress = GetProcAddress(hModule, functionName);
        if (functionAddress == NULL) {
            std::cout << "Failed to load function " << functionName << std::endl;
            exit(EXIT_FAILURE);
        }
        *(uint64_t*)(dataSection + registers[1]) = (uint64_t)functionAddress;
        break;
    }
    case Opcode::CALLFUNCTION:
    {
        FARPROC functionAddress = *(FARPROC*)&dataSection[instr.operand1];
        int argc = instr.operand2;

        typedef int(*GenericFunctionPtr)(...);
        GenericFunctionPtr functionPtr = (GenericFunctionPtr)functionAddress;

        assert(argc >= 0);
        switch (argc)
        {
			case 0:
				registers[0] = functionPtr();
			case 1:
                registers[0] = functionPtr((void*)&dataSection[registers[0]]);
                break;
            case 2:
                registers[0] = functionPtr((void*)&dataSection[registers[0]], (void*)&dataSection[registers[1]]);
                break;
            case 3:
                registers[0] = functionPtr((void*)&dataSection[registers[0]], (void*)&dataSection[registers[1]], (void*)&dataSection[registers[2]]);
                break;
            case 4:
                registers[0] = functionPtr((void*)&dataSection[registers[0]], (void*)&dataSection[registers[1]], (void*)&dataSection[registers[2]], (void*)&dataSection[registers[3]]);
                break;
            case 5:
                registers[0] = functionPtr((void*)&dataSection[registers[0]], (void*)&dataSection[registers[1]], (void*)&dataSection[registers[2]], (void*)&dataSection[registers[3]],
                    (void*)&dataSection[registers[4]]);
                break;

            default:
                assert(false && "not implemented");
        }
        break;
    }
    }
}
int main() {
#ifdef _DEBUG
    auto bufferObject = load_file("../bin");
#else
    auto bufferObject = load_file("bin");
#endif
    char* buffer = (char*)bufferObject.data();
    assert(buffer != NULL && "failed to load program");
    size_t bufferLength = bufferObject.size();

    bool magicEquals = compareStrings(globals::fileHeaderMagic, fileHeaderMagicLength, buffer, fileHeaderMagicLength);
    if (!magicEquals)
    {
        printf("File is not a %s file!\n", globals::fileHeaderMagic);
        return -1;
    }

    LoadProgram(buffer, bufferLength);

    bool useDebugger = false;

    if (useDebugger)
    {
        debugger::Initialize();
        debugger::RunProgram();
    }
    else
    {
        // Run the program in the VM without debugger
        while (true) {
            Instruction instr = *FetchInstruction();
            if (instr.opcode == Opcode::HALT) {
                break;
            }
            ExecuteInstruction(instr);
        }
    }


    CloseHandle(hCreatedConsole);
    runConsoleMutexCheckerThread = false;
    return 0;
}

DWORD ConnectToConsole()
{

    return ERROR_SUCCESS;
}

// if the console process we spawned is closed, kill this process too
void ConsoleMutexCheckerThread()
{
    while (runConsoleMutexCheckerThread)
    {
        HANDLE hMutex = OpenMutex(SYNCHRONIZE, FALSE, lpszMutexConsoleName);

        if (hMutex == NULL)
        {
            TerminateProcess(GetCurrentProcess(), 1);
        }
        else
        {
            CloseHandle(hMutex);
        }

        Sleep(100);
    }
}