#include <iostream>
using namespace std;

#include "virtualmachine.h"

int main(int argc, char* argv[])
{
    VirtualMachine* machine = new VirtualMachine();

    /*if(argc < 2)
    {
        cout << "No arguments.\n";
        return 0;
    }*/

    //cout << "Argument: " << argv[1];

    bool ok = machine->loadFile("/home/stivento/workspace/ALang/program.alang");

    if(!ok)
    {
        cout << "Load error.\n";
        return 1;
    }

    machine->execute();

    delete machine;

    return 0;
}

