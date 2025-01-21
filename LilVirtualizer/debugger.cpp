#include "debugger.h"
#include <cstdint>
#include <Windows.h>
#include <string>
#include <iostream>
#include "type.h"
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>
#include "utils.h"

using namespace std;
extern uint16_t program_counter;
extern uint8_t codeSection[];
extern uint8_t dataSection[];
extern uint8_t eflags[];
extern uint32_t registers[];
extern size_t sizeOfCode;
extern void ExecuteInstruction(const Instruction& instr);
extern Instruction* FetchInstruction();
BEGIN_NAMESPACE(debugger)
std::vector<uint32_t> breakpoints;
std::stringstream message;
void WaitDebugLine();
void DumpRegisters();
void PrintPos(int x, int y, char* buffer, int ansiColor = 97);
void DumpCode();
HANDLE hConsole;
int highestCursorYCoord = 0;
bool canExecuteInstruction = true;
enum class RunMode
{
    SINGLE_STEP,
    RUN,
    WAIT,
    BREAK
};

RunMode runMode = RunMode::WAIT;
class Command
{
private:
    static std::vector<Command> allCommands;

public:
    std::string m_keyword;
    int m_argCount;
    void (*m_functionPointer)(std::vector<std::string>);
    std::string m_tip;

    Command() {};

    Command(std::string keyword, int argCount, void (*functionPointer)(std::vector<std::string>), std::string tip)
    {
        m_keyword = keyword;
        m_argCount = argCount;
        m_functionPointer = functionPointer;
        m_tip = tip;
        allCommands.push_back(*this);
    }

    static bool GetCommand(std::string commandKeyword, Command& command)
    {
        for (int i = 0; i < allCommands.size(); i++)
        {
            if (allCommands[i].m_keyword.compare(commandKeyword) == 0)
            {
                command = allCommands[i];
                return true;
            }
        }
        return false;
    }
};
std::vector<Command> Command::allCommands;

void ContinueCommand(std::vector<std::string>)
{
    runMode = RunMode::RUN;
}

void BreakpointCommand(std::vector<std::string> args)
{
    std::string hex_string = args[1];
    int address = 0;
    try
    {
        address = std::stoi(hex_string, nullptr, 16);
    }
    catch (std::invalid_argument& e) {
        message << "Invalid argument.";
        return;
    }
    catch (std::out_of_range& e) {
        message << "Out of range";
        return;
    }
    
    // needs to be divisible by size
    if (address % sizeof(Instruction) != 0)
    {
        message << "address needs to be divisible by " << sizeof(Instruction);
        return;
    }

    auto it = std::find(breakpoints.begin(), breakpoints.end(), address);

    if (it != breakpoints.end())
    {
        // breakpoint was already set
        breakpoints.erase(it);
        message << "breakpoint removed at 0x" << hex_string;
        return;
    }
    
    // breakpoint not set
    breakpoints.push_back(address);
    message << "breakpoint set at 0x" << hex_string;
    
    return;
    
}

void StepCommand(std::vector<std::string> args)
{
    runMode = RunMode::SINGLE_STEP;
}

void MemdumpCommand(std::vector<std::string> args)
{
    std::string dumpAddress = args[1];
    int address = 0;
    try
    {
        address = std::stoi(dumpAddress, nullptr, 16);
    }
    catch (std::invalid_argument& e) {
        message << "Invalid argument.";
        return;
    }
    catch (std::out_of_range& e) {
        message << "Out of range";
        return;
    }

    std::string dumpLength = args[2];
    int length = 0;
    try
    {
        length = std::stoi(dumpLength, nullptr, 10);
    }
    catch (std::invalid_argument& e) {
        message << "Invalid argument.";
        return;
    }
    catch (std::out_of_range& e) {
        message << "Out of range";
        return;
    }

    const int maxBytesPerLine = 10;
    int toPrintBytesAmount = length;
    while (toPrintBytesAmount > maxBytesPerLine)
    {
        // write address
        message << "0x" << std::setfill('0') << std::setw(2 * 4) << std::hex << (int32_t*)(address) << std::dec << ": ";

        printHexAsciiDumpLine(message, (uint8_t*)(dataSection + address), maxBytesPerLine);

        message << std::endl;

        toPrintBytesAmount -= maxBytesPerLine;
        address += maxBytesPerLine;
    }

    int restPrintCount = maxBytesPerLine - toPrintBytesAmount;


    message << "0x" << std::setfill('0') << std::setw(2 * 4) << std::hex << (int32_t*)(address) << std::dec << ": ";
    printHexAsciiDumpLine(message, (uint8_t*)(dataSection + address), toPrintBytesAmount);

    // 0x" << std::setfill('0') << std::setw(2 * 4) << std::hex << program_counter << std::dec
}

void HandleCommand(std::string commandString)
{
    std::vector<std::string> args = StringSplit(commandString, ' ');

    if (args.size() == 0)
        return;

    Command command;
    bool commandExists = Command::GetCommand(args[0], command);

    if (!commandExists)
        return;

    if (command.m_argCount != args.size() - 1)
        return;

    command.m_functionPointer(args);
}

void Initialize()
{
    Command("bp", 1, BreakpointCommand, "bp [address]; to toggle breakpoint at address");
    Command("step", 0, StepCommand, "step; enter single stepping mode");
    Command("continue", 0, ContinueCommand, "continue; run indefinitely, break at breakpoints");
    Command("memdump", 2, MemdumpCommand, "memdump [address] [len]; dump data from dataSection");
}

void GetConsoleCursorPosition(int& x, int& y)
{
    CONSOLE_SCREEN_BUFFER_INFO info;
    GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &info);
    x = info.srWindow.Left;
    y = info.dwCursorPosition.Y;
}



void Debug()
{
    hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    ClearConsole();

    DumpCode();
    DumpRegisters();

    COORD coord{ 0, highestCursorYCoord + 5 };
    SetConsoleCursorPosition(hConsole, coord);
    std::string messageString = message.str();
    if (messageString.length() > 0)
    {
        std::cout << messageString << std::endl;
        message.str("");
    }
}

void RunProgram()
{
    bool breakpointHit = false;
    // Run the program in the VM without debugger
    while (true) {
        if (runMode == RunMode::SINGLE_STEP || runMode == RunMode::RUN)
        {
            // fetch the next instruction to be executed
            Instruction* instruction = (Instruction*)(&codeSection[program_counter]);
            //program_counter += sizeof(Instruction);

            // check if there is breakpoint at the next instruction to be executed
            auto it = std::find(breakpoints.begin(), breakpoints.end(), program_counter);
            if (it != breakpoints.end() && !breakpointHit)
            {
                // we should indeed break at this instruction
                runMode = RunMode::WAIT;
                breakpointHit = true;

                message.str("");
                message << "breakpoint hit at 0x" << std::setfill('0') << std::setw(2 * 4) << std::hex << program_counter << std::dec << std::endl;
            }
            else // otherwise execute
            {
                if (instruction->opcode == Opcode::HALT) {
                    break;
                }
                program_counter += sizeof(Instruction);
                ExecuteInstruction(*instruction);

                if (breakpointHit)
                    breakpointHit = false;
            }
        }
        if (runMode != RunMode::RUN)
            Debug();

        
        if (runMode == RunMode::SINGLE_STEP || runMode == RunMode::WAIT)
            WaitDebugLine();
    }
}

void WaitDebugLine()
{
    std::string input;
    std::getline(std::cin, input);
    
    if (input.length() > 0)
    {
        runMode = RunMode::WAIT;
    }
    else
    {
        // only hitting enter will put you in single step mode
        runMode = RunMode::SINGLE_STEP;
    }

    HandleCommand(input);

}

const int registersXOffset = 60;
const int registersYOffset = 1;
void DumpRegisters()
{
    PrintPos(registersXOffset, registersYOffset, (char*)"registers");
    PrintPos(registersXOffset, registersYOffset + 1, (char*)" ----------------------");
    std::string maxInt = std::to_string(MAXINT64);
    int maxIntLength = maxInt.length();
    for (size_t i = 0; i < (size_t)Register::MAX_REGISTER_COUNT; i++)
    {
        std::stringstream stream{};
        stream << "| " << "r" << i << ": 0x" << std::setfill('0') << std::setw(2 * 4) << std::hex << registers[i] << std::dec << "\t";
        PrintPos(registersXOffset, i + registersYOffset + 2, (char*)stream.str().c_str());
        PrintPos(registersXOffset + maxIntLength + 4, i + registersYOffset + 2, (char*)"|");
    }
    PrintPos(registersXOffset, registersYOffset + 2 + (size_t)Register::MAX_REGISTER_COUNT, (char*)" ----------------------");
}

void PrintPos(int x, int y, char* buffer, int ansiColor)
{
    // Set the cursor position
    COORD cursorPos = { x,y };
    SetConsoleCursorPosition(hConsole, cursorPos);

    std::cout << "\033[1;" << std::to_string(ansiColor) << "m" << buffer << "\033[0m" << std::endl;

    if (y > highestCursorYCoord)
        highestCursorYCoord = y;
}

const int codeDumpXOffset = 1;
const int amountOfCodeToDumpBeforeAndAfterCurrentInstructionInInstructions = 14;

void DumpCode()
{
    PrintPos(codeDumpXOffset, 1, (char*)" disassembly                    ");
    PrintPos(codeDumpXOffset, 2, (char*)" -------------------------------------------------");

    size_t totalInstructionCount = sizeOfCode / sizeof(Instruction);
    for (int i = 0; i < amountOfCodeToDumpBeforeAndAfterCurrentInstructionInInstructions + 1; i++)
    {
        uint16_t currentInstructionAddress = program_counter + i * sizeof(Instruction);

        // overflow check
        if (((((int)program_counter) + i * sizeof(Instruction)) > program_counter && i < 0) || (size_t)currentInstructionAddress >= (size_t)sizeOfCode)
        {
            PrintPos(codeDumpXOffset, i + 3, (char*)"|"); // (char*)(OpcodeToString[(uint8_t)currentInstruction->opcode])
            PrintPos(codeDumpXOffset + 50, i + 3, (char*)"|"); // (char*)(OpcodeToString[(uint8_t)currentInstruction->opcode])
            continue;
        }

        Instruction currentInstruction = *(Instruction*)&codeSection[currentInstructionAddress];

        int ansiColor;

        // write instruction as hex byte array
        std::stringstream stream{};
        stream << "| 0x" << std::setfill('0') << std::setw(2 * 4) << std::hex << currentInstructionAddress << std::dec << ":\t";

        // write disassembled opcode
        stream << OpcodeToString[(uint8_t)currentInstruction.opcode] << " ";

        // --------------------------------------
        // write operand 1
        // --------------------------------------
        bool dereferenceOperand1 = false;
        bool dontWriteComma = false;
        if (currentInstruction.opcodeFlags & OpcodeFlags::dereferenceOperand1)
            dereferenceOperand1 = true;

        if (dereferenceOperand1)
            stream << "[";

        if (currentInstruction.opcodeFlags & OpcodeFlags::operand1IsRegister)
        {
            stream << RegisterToString[(int)currentInstruction.operand1];
        }
        else if (currentInstruction.opcodeFlags & OpcodeFlags::operand1IsImmediate)
            stream << "0x" << std::setfill('0') << std::setw(2) << std::hex << currentInstruction.operand1 << std::dec << "\t";
        else
            dontWriteComma = true;

        if ((int)(currentInstruction.opcodeFlags & OpcodeFlags::operand2IsRegister) == 0 && (int)(currentInstruction.opcodeFlags & OpcodeFlags::operand2IsImmediate) == 0)
            dontWriteComma = true;

        if (dereferenceOperand1)
            stream << "]";

        if (!dontWriteComma)
            stream << ", ";
        // --------------------------------------
        // write operand 2
        // --------------------------------------
        bool dereferenceOperand2 = false;
        if (currentInstruction.opcodeFlags & OpcodeFlags::dereferenceOperand2)
            dereferenceOperand2 = true;

        if (dereferenceOperand2)
            stream << "[";

        if (currentInstruction.opcodeFlags & OpcodeFlags::operand2IsRegister)
        {
            stream << RegisterToString[(int)currentInstruction.operand2];
        }
        else if (currentInstruction.opcodeFlags & OpcodeFlags::operand2IsImmediate)
            stream << "0x" << std::setfill('0') << std::setw(2) << std::hex << currentInstruction.operand2 << std::dec << "\t";


        if (dereferenceOperand2)
            stream << "]";

        auto it = std::find(breakpoints.begin(), breakpoints.end(), currentInstructionAddress);

        if (it != breakpoints.end())
        {
            ansiColor = 31;
        } else if (i == 0)
        {

            ansiColor = 36;
        }
        else
        {
            ansiColor = 33;
        }
        std::string maxInt = std::to_string(MAXINT64);
        int maxIntLength = maxInt.length();
        PrintPos(codeDumpXOffset, i + 3, (char*)stream.str().c_str(), ansiColor); // (char*)(OpcodeToString[(uint8_t)currentInstruction->opcode])
        PrintPos(codeDumpXOffset + 50, i + 3, (char*)"|", ansiColor); // (char*)(OpcodeToString[(uint8_t)currentInstruction->opcode])

    }
    std::cout << "  -------------------------------------------------\n";

}

END_NAMESPACE