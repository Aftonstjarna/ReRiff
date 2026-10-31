#include "playback.hpp"
#include <cassert>
#include <cstring>
#include <fstream>
#include <iostream>

int main(int argc, char **argv) {
    reriff::SongPath path;
    path.start = 1;
    path.finish = 3;
    path.beatSpan = 10;
    path.distances = {0, 10, 20, 50, 60, 70};
    assert(path.parameter(0) == 1);
    assert(path.parameter(2.5) == 2);
    assert(path.parameter(6.25) == 2.5);
    assert(path.parameter(10) == 3);
    assert(path.parameter(-100) == 1);
    assert(path.parameter(100) == 3);
    path.paceOffset = -.5;
    assert(path.parameter(.5) == 1);
    assert(path.parameter(3) == 2);
    if (argc == 1)
        return 0;
    if (argc != 3)
        return 2;
    std::ifstream input(argv[1], std::ios::binary);
    char magic[8];
    uint32_t fields[5];
    float values[3];
    input.read(magic, 8);
    input.read(reinterpret_cast<char *>(fields), 20);
    input.read(reinterpret_cast<char *>(values), 12);
    if (!input || std::memcmp(magic, "RRPLAY1\0", 8) || fields[0] < 4 || fields[0] > 100000)
        return 2;
    path.start = fields[1];
    path.finish = fields[2];
    path.beatSpan = values[0];
    path.paceOffset = 0;
    path.distances.clear();
    for (uint32_t i = 0; i < fields[0]; ++i) {
        float row[7];
        uint32_t visible;
        input.read(reinterpret_cast<char *>(row), 28);
        input.read(reinterpret_cast<char *>(&visible), 4);
        path.distances.push_back(row[6]);
    }
    if (!input)
        return 2;
    std::ifstream samples(argv[2]);
    float time, expected, distance;
    float maxError = 0;
    int count = 0;
    while (samples >> time >> expected >> distance) {
        maxError = std::max(maxError, std::fabs(path.parameter(time) - expected));
        ++count;
    }
    std::cout << "{\"samples\":" << count << ",\"max_parameter_error\":" << maxError
              << ",\"tolerance\":0.001,\"passed\":"
              << ((count >= 100 && maxError < .001) ? "true" : "false") << "}\n";
    return count >= 100 && maxError < .001 ? 0 : 1;
}
