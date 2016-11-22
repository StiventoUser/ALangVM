#include <iostream>
using namespace std;

#include "virtualmachine.h"

int main()
{
    VirtualMachine* machine = new VirtualMachine();

    delete machine;

    return 0;
}

