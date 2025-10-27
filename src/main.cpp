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

    Board board = Board::initFEN("r3kbnr/2p2ppp/p1p5/1pq1p1B1/1PPPP1b1/5N2/P4PPP/RN1QK2R w KQkq - 1 9");
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

    return 0;
}