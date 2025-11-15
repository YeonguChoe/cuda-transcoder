#include <cstddef>
#include <cstdio>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <vector>

using namespace std;
int main(int argc, char *argv[])
{
    const char *filename = argv[1];

    // open file
    FILE *f = fopen(filename, "rb");

    // print content
    char line[256];
    while (fgets(line, sizeof(line), f))
    {
        printf("%s", line);
    }

    // close file
    fclose(f);

    return 0;
}
