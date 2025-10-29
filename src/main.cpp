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

    Board board = Board::initFEN("rnbqk1nr/pp1p1ppp/2p5/1B2p3/1b2P3/2P5/PP1P1PPP/RNBQK1NR w KQkq - 0 4");
    std::cout << board.toString() << "\n";

    std::vector<Move> moves;
    board.generateLegalMoves(moves);

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