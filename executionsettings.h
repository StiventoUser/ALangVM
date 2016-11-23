#ifndef EXECUTIONSETTINGS_H
#define EXECUTIONSETTINGS_H

#include <cstdint>

class ExecutionSettings
{
public:
    const int32_t DefaultStackSize = 1024*1024;
public:
    static ExecutionSettings* instance()
    {
        if(m_settings == nullptr)
            m_settings = new ExecutionSettings();

        return m_settings;
    }

    void lockSettings()
    {
        m_isLocked = true;
    }

    int32_t getStackSize()
    {
        return m_stackSize;
    }
    void setStackSize(int32_t val)
    {
        if(!m_isLocked)
            m_stackSize = val;
    }

private:
    ExecutionSettings() {}

private:
    static ExecutionSettings* m_settings;

    bool m_isLocked;
    int32_t m_stackSize;
};

#endif // EXECUTIONSETTINGS_H
