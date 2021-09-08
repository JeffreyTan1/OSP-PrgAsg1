#include <iostream>

using std::cin;
using std::cout;
using std::endl;

int main(void)
{

    std::cout << "Hello!" << endl;
    bool run = true;
    while (run)
    {
        int choice;
        cout << endl
             << "#################################" << endl
             << "Choose your option: " << endl
             << "1. Reader Writers Problem" << endl
             << "2. Sleeping Barbers Problem" << endl
             << "3. Exit" << endl
             << "#################################" << endl;
        cin >> choice;
        if (choice == 1)
        {
            system("./Reader_Writer");
        }
        else if (choice == 2)
        {
            system("./Sleeping_Barbers");
        }
        else if (choice == 3)
        {
            run = false;
        }
    }

    std::cout << "Goodbye!" << endl;

    return EXIT_SUCCESS;
}