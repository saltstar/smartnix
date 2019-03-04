
#include <fuzz-utils/fuzzer.h>

int main(int argc, char* argv[]) {
    return fuzzing::Fuzzer::Main(argc, argv);
}
