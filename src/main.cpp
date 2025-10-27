#include "board.hpp"
#include "magic.hpp"

#include <iostream>

constexpr static std::string squareToName(int square) {
    std::string name = "  ";
    name[0] = static_cast<char>('A' + (square % 8));
    name[1] = static_cast<char>('1' + (square / 8));
    return name;
}

int main() {
    initMagic();

    Board board = Board::initFEN("1Q3bnr/2p2ppp/2p1k3/P1P3B1/2P1P1N1/8/P4PPP/RN2K2R w KQ - 1 16");
    std::cout << board.toString() << "\n";

    std::vector<Move> moves;
    board.generatePseudoMoves(moves);

    for (auto& move : moves) {
        std::string from = squareToName(move.from);
        std::string to = squareToName(move.to);
        std::cout << "From " << from << ", To " << to;
        if (move.flags & static_cast<std::uint8_t>(MoveFlag::Capture)) {
            std::cout << ", Capture";
        }
        std::cout << "\n";
    }

    std::cout << "Score: " << board.evaluate() << "\n";

    return 0;
}