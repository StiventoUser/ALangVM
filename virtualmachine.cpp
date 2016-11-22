#include "virtualmachine.h"
#include <iostream>
#include <fstream>
#include <cstring>

using std::cout;

#include "executionsettings.h"

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

void VirtualMachine::checkStackFreeSpace(int32_t size)
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
    int32_t* i32Ptr;
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
                    iEnvPtr = m_stack;
                    *iEnvPtr = (int_env)(m_program);

                    m_stack += sizeof(int_env);

                    iEnvPtr = m_stack;
                    *iEnvPtr = (int_env)(m_currentLocals);

                    m_stack += sizeof(int_env);

                    i32Ptr = m_stack;
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

                bPtr -= sizeof(int32_t);
                m_localsSize = *((int32_t*)(bPtr));

                bPtr -= sizeof(int_env);
                m_currentLocals = *((int_env*)(bPtr));

                bPtr -= sizeof(int_env);
                m_program = *((int_env*)(bPtr));
            }
            break;
        case GenCodes::FuncReturn:
            {
                int32_t retSize = *((int32_t*)(m_program));
                m_program += sizeof(int32_t);

                bPtr = m_stack;

                m_stack = m_currentLocals;

                bPtr -= sizeof(int32_t);
                m_localsSize = *((int32_t*)(bPtr));

                bPtr -= sizeof(int_env);
                m_currentLocals = *((int_env*)(bPtr));

                bPtr -= sizeof(int_env);
                m_program = *((int_env*)(bPtr));

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
                    case TypeIndex::Int32:
                        {
                            m_stack -= sizeof(int32_t);
                            i32Ptr = *((int32_t*)(m_stack));

                            cout << *i32Ptr << endl;
                        }
                    break;
                    //TODO: new types
                }
                m_program += 1;
            }
            break;
        case GenCodes::NewLVar:
            {

            }
        }
    }
}
