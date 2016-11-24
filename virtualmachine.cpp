#include "virtualmachine.h"
#include <iostream>
#include <fstream>
#include <cstring>
#include <cmath>

using std::cout;
using std::endl;

#include "executionsettings.h"
#include "helpers.h"

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
    std::ifstream ifs(fileName, std::ios_base::ate | std::ios_base::binary);

    if(!ifs.is_open())
    {
        cout << "File '" << fileName << "' isn't exist\n";
        return false;
    }

    int length = ifs.tellg();

    ifs.seekg(0, std::ios_base::beg);

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

    //Pointers
    byte* bPtr;
    int16_t* i16Ptr;
    int32_t* i32Ptr;
    int64_t* i64Ptr;
    int_env* iEnvPtr;
    //

    while(m_program < programEnd)
    {
        GenCodes* opCode = (GenCodes*)m_programBegin;

        m_program += sizeof(int32_t);

        switch(*opCode)
        {
        case GenCodes::Func:
        {
            m_currentLocals = m_stack;

            m_localsSize = *((int32_t*)(m_program));

            m_program += sizeof(int32_t);
        }
            break;
        case GenCodes::CallFunc:
        {
            int32_t offset = *((int32_t*)(m_program));
            m_program += sizeof(int32_t);

            if(checkStackFreeSpace(sizeof(int_env) + sizeof(int_env) + sizeof(int32_t)))
            {
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
            }
            else
            {
                //error
            }
        }
            break;
        case GenCodes::FuncEnd:
        {
            m_stack = m_currentLocals;

            m_stack -= sizeof(int32_t);
            m_localsSize = *((int32_t*)(m_stack));

            m_stack -= sizeof(int_env);
            m_currentLocals = ( (byte*)(*((int_env*)(m_stack))) );

            m_stack -= sizeof(int_env);
            m_program = ( (byte*)(*((int_env*)(m_stack))) );
        }
            break;
        case GenCodes::FuncReturn:
        {
            int32_t retSize = *((int32_t*)(m_program));
            m_program += sizeof(int32_t);

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
                //error wtf? compiler bug?
            }
            memcpy(m_stack, bPtr, retSize);
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

                        cout << *((int32_t*)(m_stack)) << endl;
                    }
                break;
                //TODO: new types
            }
            m_program += sizeof(byte);
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
                //error
            }

            memcpy(m_stack, m_currentLocals + offset, byteCount);

            m_stack += byteCount;
        }
            break;
        case GenCodes::Push:
        {
            i32Ptr = (int32_t*)m_program;
            int32_t byteCount = *i32Ptr;
            m_program += sizeof(int32_t);

            if(!checkStackFreeSpace(byteCount))
            {
                //error
            }

            memcpy(m_stack, m_program, byteCount);

            m_program += byteCount;
        }
            break;
        case GenCodes::Pop:
        {
            i32Ptr = (int32_t*)m_program;
            int32_t byteCount = *i32Ptr;
            m_program += sizeof(int32_t);

            m_stack -= byteCount;
        }
            break;
        case GenCodes::Add:
        {
            byte type = *m_program;

            m_program += sizeof(byte);

            switch(type)
            {
            case TypesIndex::Int32:
            {
                int32_t val = *(m_stack - sizeof(int32_t)*2) + *(m_stack - sizeof(int32_t));

                m_stack -= sizeof(int32_t)*2;

                memcpy(m_stack, &val, sizeof(int32_t));
            }
                break;
            }
        }
            break;
        case GenCodes::Subtract:
        {
            byte type = *m_program;

            m_program += sizeof(byte);

            switch(type)
            {
            case TypesIndex::Int32:
            {
                int32_t val = *(m_stack - sizeof(int32_t)*2) - *(m_stack - sizeof(int32_t));

                m_stack -= sizeof(int32_t)*2;

                memcpy(m_stack, &val, sizeof(int32_t));
            }
                break;
            }
        }
            break;
        case GenCodes::Multiply:
        {
            byte type = *m_program;

            m_program += sizeof(byte);

            switch(type)
            {
            case TypesIndex::Int32:
            {
                int32_t val = *(m_stack - sizeof(int32_t)*2) * *(m_stack - sizeof(int32_t));

                m_stack -= sizeof(int32_t)*2;

                memcpy(m_stack, &val, sizeof(int32_t));
            }
                break;
            }
        }
            break;
        case GenCodes::Divide:
        {
            byte type = *m_program;

            m_program += sizeof(byte);

            switch(type)
            {
            case TypesIndex::Int32:
            {
                int32_t val = *(m_stack - sizeof(int32_t)*2) / *(m_stack - sizeof(int32_t));

                m_stack -= sizeof(int32_t)*2;

                memcpy(m_stack, &val, sizeof(int32_t));
            }
                break;
            }
        }
            break;
        case GenCodes::Exponent:
        {
            byte type = *m_program;

            m_program += sizeof(byte);

            switch(type)
            {
            case TypesIndex::Int32:
            {
                int32_t val = pow(*(m_stack - sizeof(int32_t)*2), *(m_stack - sizeof(int32_t)));

                m_stack -= sizeof(int32_t)*2;

                memcpy(m_stack, &val, sizeof(int32_t));
            }
                break;
            }
        }
            break;
        case GenCodes::Negate:
        {
            byte type = *m_program;

            m_program += sizeof(byte);

            switch(type)
            {
            case TypesIndex::Int32:
            {
                int32_t val = -(*(m_stack - sizeof(int32_t)));

                m_stack -= sizeof(int32_t);

                memcpy(m_stack, &val, sizeof(int32_t));
            }
                break;
            }
        }
            break;
        }
    }
}
