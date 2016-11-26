#include "virtualmachine.h"
#include <iostream>
#include <fstream>
#include <cstring>
#include <cmath>

using std::cout;
using std::endl;

#include "executionsettings.h"
#include "helpers.h"

const char* const GenCodesName[] =
{
    "NewLVar", "SetLVarVal", "GetLVarVal",
    "Push", "Pop",
    "Add", "Subtract", "Multiply", "Divide", "Exponent", "Negate",
    "Func",
    "CallFunc", "FuncEnd", "FuncReturn",
    "Meta", "Print"
};
const char* const TypesIndexName[] =
{
    "Int64", "Int32", "Int16", "Int8", "Double", "Single", "String", "Bool"
};

const char* getGenCodeName(GenCodes code, bool& ok)
{
    ok = true;

    if(code < 0 || code >= GenCodes::GenCodesCount)
    {
        cout << "Invalid GenCode value: " << code << endl;

        ok = false;
        return nullptr;
    }
    return GenCodesName[(int32_t)code];
}
const char* getTypeIndexName(TypesIndex index, bool& ok)
{
    ok = true;

    if(index < 0 || index >= TypesIndex::TypesIndexCount)
    {
        cout << "Invalid TypeIndex value: " << index << endl;

        ok = false;
        return nullptr;
    }
    return TypesIndexName[(int32_t)index];
}

template<typename T>
void logValue(byte* arr)
{
    cout << "Value at: " << (void*)arr << ", is: " << (T*)arr;
}

void logBytes(byte* arr, int size)
{
    cout << "Arr: (";
    for(int i = 0; i < size; ++i)
    {
        cout << std::hex << (int32_t)arr[i] << std::dec << " ";
    }
    cout << ")";
}

void logAdress(byte* p)
{
    cout << "Pointer: " << (void*)p;
}

VirtualMachine::VirtualMachine()
{

}

VirtualMachine::~VirtualMachine()
{
    delete[] m_programBegin;
    delete[] m_stackBegin;
}


bool VirtualMachine::loadFile(const char* fileName)
{
    std::ifstream ifs(fileName, std::ios_base::binary);

    if(!ifs.is_open())
    {
        cout << "File '" << fileName << "' isn't exist\n";
        return false;
    }

    char type[6];
    ifs.read(type, 5);
    type[5] = '\0';

    if(strcmp(type, "ALang") != 0)
    {
        cout << "It's not a ALang file\n";
        return false;
    }

    int32_t headerSize = 0;
    ifs.read((char*)&headerSize, sizeof(int32_t));
    //ignore header

    int32_t length = 0;
    ifs.read((char*)&length, sizeof(int32_t));

    m_programBegin = new byte[length];

    ifs.read(m_programBegin, length);

    m_programLength = length;
    m_program = m_programBegin;

    return true;
}

bool VirtualMachine::checkStackFreeSpace(int32_t size)
{
    return (m_stack + size) < m_stackEnd;
}

void VirtualMachine::execute()
{
    m_stackLength = ExecutionSettings::instance()->getStackSize();
    m_stack = m_stackBegin = new byte[m_stackLength];
    m_stackEnd = m_stackBegin + m_stackLength;

    byte* programEnd = m_programBegin + m_programLength;
    GenCodes* opCode;

    bool ok;

    //Pointers
    byte* bPtr;
    int16_t* i16Ptr;
    int32_t* i32Ptr;
    int64_t* i64Ptr;
    int_env* iEnvPtr;
    //

    m_currentLocals = nullptr;
    m_localsSize = 0;

    logAdress(m_programBegin);
    cout << endl;

    logAdress(programEnd);
    cout << endl;

    while(m_program < programEnd)
    {
        if(m_program < m_programBegin || m_program >= programEnd)
        {
            cout << "!!! Program is out of boundaries !!!\n";
            abort();
        }
        if(m_stack < m_stackBegin || m_program >= m_stackEnd)
        {
            cout << "!!! Stack is out of boundaries !!!\n";
            //abort();
        }
        if(m_currentLocals != nullptr && m_stack < (m_currentLocals + m_localsSize))
        {
            cout << "!!! Stack is in locals !!!\n";
            //abort();
        }

        GenCodes* opCode = (GenCodes*)m_program;

#ifdef DEBUG_VM
        cout << endl << "Code: " << getGenCodeName(*opCode, ok) << endl;
#endif
        m_program += sizeof(int32_t);

        switch(*opCode)
        {
        case GenCodes::Func:
        {
            m_currentLocals = m_stack;

            m_localsSize = *((int32_t*)(m_program));

            m_program += sizeof(int32_t);

            m_stack += m_localsSize;
#ifdef DEBUG_VM
            cout << "Locals at: ";
            logAdress(m_currentLocals);
            cout << "\nAnd program at: ";
            logAdress(m_program);
            cout << "\nLocals size: " << m_localsSize << endl;
            cout << "Stack at: ";
            logAdress(m_stack);
            cout << endl;
#endif
        }
            break;
        case GenCodes::CallFunc:
        {
            //TODO: check "Func" instruction

            int32_t offset = *((int32_t*)(m_program));
            m_program += sizeof(int32_t);

            int32_t argsSize = *((int32_t*)(m_program));

            m_program += sizeof(int32_t);
#ifdef DEBUG_VM
            cout << "Call offset: " << offset << endl;
#endif
            int32_t stackPointerSize = sizeof(int_env) + sizeof(int_env) + sizeof(int32_t);
            if(checkStackFreeSpace(stackPointerSize))
            {
#ifdef DEBUG_VM
                cout << "Stack at: ";
                logAdress(m_stack);
                cout << endl;
#endif
                memcpy(m_stack - argsSize + stackPointerSize, m_stack - argsSize, argsSize);

                m_stack -= argsSize;

                iEnvPtr = (int_env*)m_stack;
                *iEnvPtr = (int_env)(m_program);

                m_stack += sizeof(int_env);

                iEnvPtr = (int_env*)m_stack;
                *iEnvPtr = (int_env)(m_currentLocals);

                m_stack += sizeof(int_env);

                i32Ptr = (int32_t*)m_stack;
                *i32Ptr = m_localsSize;

                m_stack += sizeof(int32_t);

                m_program = m_programBegin + offset;
#ifdef DEBUG_VM
                cout << "Now stack at: ";
                logAdress(m_stack);
                cout << endl;

                cout << "And program at: ";
                logAdress(m_program);
                cout << ", offset in bytes: " << (int)(m_program - m_programBegin) << endl;
#endif
            }
            else
            {
                cout << "No free space";
                return;
                //TODO: error
            }
        }
            break;
        case GenCodes::FuncEnd:
        {
#ifdef DEBUG_VM
            cout << "Stack at: ";
            logAdress(m_stack);
            cout << endl;
#endif
            m_stack = m_currentLocals;

            m_stack -= sizeof(int32_t);
            m_localsSize = *((int32_t*)(m_stack));

            m_stack -= sizeof(int_env);
            m_currentLocals = ( (byte*)(*((int_env*)(m_stack))) );

            m_stack -= sizeof(int_env);
            m_program = ( (byte*)(*((int_env*)(m_stack))) );
#ifdef DEBUG_VM
            cout << "Now stack at: ";
            logAdress(m_stack);
            cout << endl;

            cout << "Locals at: ";
            logAdress(m_currentLocals);
            cout << endl;

            cout << "And program at: ";
            logAdress(m_program);
            cout << ", offset in bytes: " << (int)(m_program - m_programBegin) << endl;
#endif
        }
            break;
        case GenCodes::FuncReturn:
        {
#ifdef DEBUG_VM
            cout << "Stack at: ";
            logAdress(m_stack);
            cout << endl;
#endif
            int32_t retSize = *((int32_t*)(m_program));
            m_program += sizeof(int32_t);
#ifdef DEBUG_VM
            cout << "Return value(s) size in bytes: " << retSize << endl;
#endif
            bPtr = m_stack;

            m_stack = m_currentLocals;

            m_stack -= sizeof(int32_t);
            m_localsSize = *((int32_t*)(m_stack));

            m_stack -= sizeof(int_env);
            m_currentLocals = ( (byte*)(*((int_env*)(m_stack))) );

            m_stack -= sizeof(int_env);
            m_program = ( (byte*)(*((int_env*)(m_stack))) );

            if(!checkStackFreeSpace(retSize))
            {
                cout << "No free space. And WTF?\n";
                return;
                //error wtf? compiler bug?
            }
            memcpy(m_stack, bPtr, retSize);

#ifdef DEBUG_VM
            cout << "Now stack at: ";
            logAdress(m_stack);
            cout << endl;

            cout << "Locals at: ";
            logAdress(m_currentLocals);
            cout << endl;

            cout << "And program at: ";
            logAdress(m_program);
            cout << ", offset in bytes: " << (int)(m_program - m_programBegin) << endl;

            cout << "Return bytes: ";
            logBytes(bPtr, retSize);
            cout << endl;
#endif
        }
            break;
        case GenCodes::Meta:
            break;
        case GenCodes::Print:
        {
            switch(*m_program)
            {
            case TypesIndex::Int32:
                {
                    m_stack -= sizeof(int32_t);

                    cout << "Int32: " << *((int32_t*)(m_stack)) << endl;
                }
                break;
            default:
                cout << "Unknown type";
                return;

                //TODO: new types
            }
            m_program += sizeof(byte);

#ifdef DEBUG_VM
            cout << "Now stack at: ";
            logAdress(m_stack);
            cout << endl;
#endif
        }
            break;
        case GenCodes::NewLVar:
        {
            i32Ptr = (int32_t*)m_program;
            int32_t offset = *i32Ptr;
            m_program += sizeof(int32_t);

            i32Ptr = (int32_t*)m_program;
            int32_t byteCount = *i32Ptr;
            m_program += sizeof(int32_t);

            m_stack -= byteCount;

            memcpy(m_currentLocals + offset, m_stack, byteCount);

#ifdef DEBUG_VM
            cout << "Variable offset: " << offset << endl
                 << "Size in bytes: " << byteCount << endl
                 << "Now stack at: ";
            logAdress(m_stack);
            cout << endl;
#endif
         }
            break;
        case GenCodes::SetLVarVal:
        {
            i32Ptr = (int32_t*)m_program;
            int32_t offset = *i32Ptr;
            m_program += sizeof(int32_t);

            i32Ptr = (int32_t*)m_program;
            int32_t byteCount = *i32Ptr;
            m_program += sizeof(int32_t);

            m_stack -= byteCount;

            memcpy(m_currentLocals + offset, m_stack, byteCount);

#ifdef DEBUG_VM
            cout << "Variable offset: " << offset << endl
                 << "Size in bytes: " << byteCount << endl
                 << "Now stack at: ";
            logAdress(m_stack);
            cout << endl;
#endif
        }
            break;
        case GenCodes::GetLVarVal:
        {
            i32Ptr = (int32_t*)m_program;
            int32_t offset = *i32Ptr;
            m_program += sizeof(int32_t);

            i32Ptr = (int32_t*)m_program;
            int32_t byteCount = *i32Ptr;
            m_program += sizeof(int32_t);

            if(!checkStackFreeSpace(byteCount))
            {
                cout << "No free space\n";
                return;
                //TODO: error
            }

            memcpy(m_stack, m_currentLocals + offset, byteCount);

            m_stack += byteCount;

#ifdef DEBUG_VM
            cout << "Variable offset: " << offset << endl
                 << "Size in bytes: " << byteCount << endl
                 << "Now stack at: ";
            logAdress(m_stack);
            cout << endl << "Value: ";
            logBytes(m_currentLocals + offset, byteCount);
            cout << endl;
#endif
        }
            break;
        case GenCodes::Push:
        {
            i32Ptr = (int32_t*)m_program;
            int32_t byteCount = *i32Ptr;
            m_program += sizeof(int32_t);

            if(!checkStackFreeSpace(byteCount))
            {
                cout << "No free space\n";
                return;
                //TODO: error
            }

            memcpy(m_stack, m_program, byteCount);

            m_stack += byteCount;
            m_program += byteCount;
#ifdef DEBUG_VM
            cout << "Value size in bytes: " << byteCount << endl
                 << "Value: ";
            logBytes(m_stack - sizeof(int32_t), byteCount);
            cout << endl << "Now stack at: ";
            logAdress(m_stack);
            cout << endl;
#endif
        }
            break;
        case GenCodes::Pop:
        {
            i32Ptr = (int32_t*)m_program;
            int32_t byteCount = *i32Ptr;
            m_program += sizeof(int32_t);

            if((m_stack - byteCount) < m_stackBegin)
            {
                cout << "Can't remove value from stack\n";
                return;
                //TODO: error
            }
            m_stack -= byteCount;

#ifdef DEBUG_VM
            cout << "Value size in bytes: " << byteCount << endl;
            cout << "Now stack at: ";
            logAdress(m_stack);
            cout << endl;
#endif
        }
            break;
        case GenCodes::Add:
        {
            byte type = *m_program;

            m_program += sizeof(byte);
#ifdef DEBUG_VM
            auto typeName = getTypeIndexName((TypesIndex)type, ok);
            cout << "Type: " << (ok ? typeName : "Unknown") << endl;
#endif
            switch(type)
            {
            case TypesIndex::Int32:
            {
                int32_t val1 = *((int32_t*)(m_stack - sizeof(int32_t)*2));
                int32_t val2 = *((int32_t*)(m_stack - sizeof(int32_t)));

                int32_t val = val1 + val2;

                m_stack -= sizeof(int32_t)*2;

                memcpy(m_stack, &val, sizeof(int32_t));

                m_stack += sizeof(int32_t);
#ifdef DEBUG_VM
                cout << "Value: " << val << endl;
#endif
            }
                break;
            default:
                cout << "Unknown type for Add: " << type << endl;
                return;
            }

#ifdef DEBUG_VM
            cout << "Now stack at: ";
            logAdress(m_stack);
            cout << endl;
#endif
        }
            break;
        case GenCodes::Subtract:
        {
            byte type = *m_program;

            m_program += sizeof(byte);

#ifdef DEBUG_VM
            auto typeName = getTypeIndexName((TypesIndex)type, ok);
            cout << "Type: " << (ok ? typeName : "Unknown") << endl;
#endif

            switch(type)
            {
            case TypesIndex::Int32:
            {
                int32_t val = *(int32_t*)(m_stack - sizeof(int32_t)*2) - *(int32_t*)(m_stack - sizeof(int32_t));

                m_stack -= sizeof(int32_t)*2;

                memcpy(m_stack, &val, sizeof(int32_t));

                m_stack += sizeof(int32_t);
#ifdef DEBUG_VM
                cout << "Value: " << val << endl;
#endif
            }
                break;
            default:
                cout << "Unknown type for Subtract: " << type << endl;
                return;
            }

#ifdef DEBUG_VM
            cout << "Now stack at: ";
            logAdress(m_stack);
            cout << endl;
#endif
        }
            break;
        case GenCodes::Multiply:
        {
            byte type = *m_program;

            m_program += sizeof(byte);

#ifdef DEBUG_VM
            auto typeName = getTypeIndexName((TypesIndex)type, ok);
            cout << "Type: " << (ok ? typeName : "Unknown") << endl;
#endif

            switch(type)
            {
            case TypesIndex::Int32:
            {
                int32_t val = *(int32_t*)(m_stack - sizeof(int32_t)*2) * *(int32_t*)(m_stack - sizeof(int32_t));

                m_stack -= sizeof(int32_t)*2;

                memcpy(m_stack, &val, sizeof(int32_t));

                m_stack += sizeof(int32_t);
#ifdef DEBUG_VM
                cout << "Value: " << val << endl;
#endif
            }
                break;
            }

#ifdef DEBUG_VM
            cout << "Now stack at: ";
            logAdress(m_stack);
            cout << endl;
#endif
        }
            break;
        case GenCodes::Divide:
        {
            byte type = *m_program;

            m_program += sizeof(byte);

#ifdef DEBUG_VM
            auto typeName = getTypeIndexName((TypesIndex)type, ok);
            cout << "Type: " << (ok ? typeName : "Unknown") << endl;
#endif

            switch(type)
            {
            case TypesIndex::Int32:
            {
                int32_t val = *(int32_t*)(m_stack - sizeof(int32_t)*2) / *(int32_t*)(m_stack - sizeof(int32_t));

                m_stack -= sizeof(int32_t)*2;

                memcpy(m_stack, &val, sizeof(int32_t));

                m_stack += sizeof(int32_t);
#ifdef DEBUG_VM
                cout << "Value: " << val << endl;
#endif
            }
                break;
            }

#ifdef DEBUG_VM
            cout << "Now stack at: ";
            logAdress(m_stack);
            cout << endl;
#endif
        }
            break;
        case GenCodes::Exponent:
        {
            byte type = *m_program;

            m_program += sizeof(byte);

#ifdef DEBUG_VM
            auto typeName = getTypeIndexName((TypesIndex)type, ok);
            cout << "Type: " << (ok ? typeName : "Unknown") << endl;
#endif

            switch(type)
            {
            case TypesIndex::Int32:
            {
                int32_t val = pow(*(int32_t*)(m_stack - sizeof(int32_t)*2), *(int32_t*)(m_stack - sizeof(int32_t)));

                m_stack -= sizeof(int32_t)*2;

                memcpy(m_stack, &val, sizeof(int32_t));

                m_stack += sizeof(int32_t);
#ifdef DEBUG_VM
                cout << "Value: " << val << endl;
#endif
            }
                break;
            }

#ifdef DEBUG_VM
            cout << "Now stack at: ";
            logAdress(m_stack);
            cout << endl;
#endif
        }
            break;
        case GenCodes::Negate:
        {
            byte type = *m_program;

            m_program += sizeof(byte);

#ifdef DEBUG_VM
            auto typeName = getTypeIndexName((TypesIndex)type, ok);
            cout << "Type: " << (ok ? typeName : "Unknown") << endl;
#endif

            switch(type)
            {
            case TypesIndex::Int32:
            {
                int32_t val = -*(int32_t*)(m_stack - sizeof(int32_t));

                m_stack -= sizeof(int32_t);

                memcpy(m_stack, &val, sizeof(int32_t));

                m_stack += sizeof(int32_t);
#ifdef DEBUG_VM
                cout << "Value: " << val << endl;
#endif
            }
                break;
            }

#ifdef DEBUG_VM
            cout << "Now stack at: ";
            logAdress(m_stack);
            cout << endl;
#endif
        }
            break;
        case GenCodesCount://For compiler
            break;
        default:
            cout << "Unknown instruction code: " << *opCode << endl;
            return;
        }
    }
    cout << "Program finished\n";
}
