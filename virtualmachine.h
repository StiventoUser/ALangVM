#ifndef VIRTUALMACHINE_H
#define VIRTUALMACHINE_H

#include <cstdint>

#include "platforminfo.h"

using byte = char;

#ifdef ENVIRONMENT64
using int_env = int64_t;
#else
using int_env = int32_t;
#endif

enum GenCodes : int32_t
{
    NewLVar, SetLVarVal, GetLVarVal,
    Push, Pop,
    Add, Subtract, Multiply, Divide, Exponent, Negate,
    Func,
    CallFunc, FuncEnd, FuncReturn,
    Meta, Print/*temporary*/
};

enum TypesIndex : byte
{
    Int64, Int32, Int16, Int8, Double, Single, String, Bool
};

class VirtualMachine
{
public:
    VirtualMachine();
    ~VirtualMachine();

    bool loadFile(const char* fileName);
    void execute();

private:
    inline bool checkStackFreeSpace(int32_t size);
private:
    byte* m_programBegin;
    byte* m_program;
    int32_t m_programLength;

    byte* m_stackBegin;
    byte* m_stackEnd;

    byte* m_stack;
    int32_t m_stackLength;

    byte* m_currentLocals;
    int32_t m_localsSize;

    //
    byte i8_1;
    byte i8_2;
    int32_t i32_1;
    int32_t i32_2;
    int64_t i64_1;
    int64_t i64_2;
    int_env i_env_1;
    int_env i_env_2;

    int64_t retVal;
};

#endif // VIRTUALMACHINE_H
