#include <cstdio>
#include <cstring>
#include <iostream>
#include <string>

int main() {
    std::setvbuf(stdin, nullptr, _IONBF, 0);
    std::setvbuf(stdout, nullptr, _IONBF, 0);

    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);

    std::string line;
    while (std::getline(std::cin, line)) {
        while (!line.empty() && (line.back() == '\r' || line.back() == '\n' || line.back() == ' ' || line.back() == '\t'))
            line.pop_back();

        if (line == "uci") {
            std::cout << "id name Prune-UCI-Probe" << std::endl;
            std::cout << "id author Android-Test" << std::endl;
            std::cout << "option name Hash type spin default 16 min 1 max 1024" << std::endl;
            std::cout << "uciok" << std::endl;
        } else if (line == "isready") {
            std::cout << "readyok" << std::endl;
        } else if (line == "ucinewgame") {
        } else if (line.rfind("setoption", 0) == 0) {
        } else if (line.rfind("position", 0) == 0) {
        } else if (line.rfind("go", 0) == 0) {
            std::cout << "bestmove e2e4" << std::endl;
        } else if (line == "stop") {
        } else if (line == "ponderhit") {
        } else if (line == "quit") {
            return 0;
        }
    }

    return 0;
}
