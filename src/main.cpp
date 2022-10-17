#include <Bench.h>
#include <iostream>
#include <rss.h>

int main(int argc, char *argv[]) {

    if(argc < 3) {
        std::cout << "Usage: quicksilver <graphFile> <queriesFile>" << std::endl;
        return 0;
    }

    // args
    std::string graphFile {argv[1]};
    std::string queriesFile {argv[2]};

    // or manually give the directory
//    std::string graphFile = " ";
//    std::string queriesFile = "";

//    estimatorBench(graphFile, queriesFile);
    evaluatorBench(graphFile, queriesFile);

    return 0;
}