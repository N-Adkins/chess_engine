#include "SDL3/SDL_events.h"
#include "SDL3/SDL_init.h"
#include "SDL3/SDL_render.h"
#include "SDL3/SDL_video.h"
#include "SDL3_image/SDL_image.h"
#include "board.hpp"
#include "magic.hpp"

#include <cmath>
#include <iostream>
#include <memory>

template<auto Fn> 
struct SDL_Deleter { 
    void operator()(auto* p) const noexcept { 
        if (p) {
            Fn(p);
        }
    } 
};

template<class T, auto Fn> 
using SDL_Pointer = std::unique_ptr<T, SDL_Deleter<Fn>>;

struct Textures {
    struct {
        SDL_Texture *pawn, *knight, *bishop, *rook, *queen, *king;
    } white;

    struct {
        SDL_Texture *pawn, *knight, *bishop, *rook, *queen, *king;
    } black;
};

static void drawFilledCircle(SDL_Renderer* r, float cx, float cy, float radius) {
    float rr = radius * radius;
    int ir = static_cast<int>(std::ceil(radius));
    for (int dy = -ir; dy <= ir; ++dy) {
        float y = static_cast<float>(dy);
        float rx = rr - y * y;
        if (rx < 0.0f) continue; // guard float error
        float dx = std::sqrt(rx);
        SDL_RenderLine(r, cx - dx, cy + y, cx + dx, cy + y);
    }
}

int main() {
    initMagic();

    std::stack<Board> undo_stack;

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        std::cout << "Failed to init SDL3, " << SDL_GetError() << "\n";
        return 1;
    }


    auto window = SDL_Pointer<SDL_Window, SDL_DestroyWindow>(
        SDL_CreateWindow("Chess", 800, 800, SDL_WINDOW_RESIZABLE)
    );
    if (!window) {
        std::cout << "Failed to create SDL3 window, " << SDL_GetError() << "\n";
        return 1;
    }

    auto renderer = SDL_Pointer<SDL_Renderer, SDL_DestroyRenderer>(
        SDL_CreateRenderer(window.get(), nullptr)
    );
    if (!renderer) {
        std::cout << "Failed to create SDL3 renderer, " << SDL_GetError() << "\n";
        return 1;
    }
    SDL_SetRenderDrawBlendMode(renderer.get(), SDL_BLENDMODE_BLEND);
 
    Textures textures;
    textures.white.pawn = IMG_LoadTexture(renderer.get(), "assets/white-pawn.png");
    textures.white.knight = IMG_LoadTexture(renderer.get(), "assets/white-knight.png");
    textures.white.bishop = IMG_LoadTexture(renderer.get(), "assets/white-bishop.png");
    textures.white.rook = IMG_LoadTexture(renderer.get(), "assets/white-rook.png");
    textures.white.queen = IMG_LoadTexture(renderer.get(), "assets/white-queen.png");
    textures.white.king = IMG_LoadTexture(renderer.get(), "assets/white-king.png");
    textures.black.pawn = IMG_LoadTexture(renderer.get(), "assets/black-pawn.png");
    textures.black.knight = IMG_LoadTexture(renderer.get(), "assets/black-knight.png");
    textures.black.bishop = IMG_LoadTexture(renderer.get(), "assets/black-bishop.png");
    textures.black.rook = IMG_LoadTexture(renderer.get(), "assets/black-rook.png");
    textures.black.queen = IMG_LoadTexture(renderer.get(), "assets/black-queen.png");
    textures.black.king = IMG_LoadTexture(renderer.get(), "assets/black-king.png");

    auto texture_for_piece = [&](char c) -> SDL_Texture* {
        switch (c) {
        case 'P': return textures.white.pawn;
        case 'N': return textures.white.knight;
        case 'B': return textures.white.bishop;
        case 'R': return textures.white.rook;
        case 'Q': return textures.white.queen;
        case 'K': return textures.white.king;
        case 'p': return textures.black.pawn;
        case 'n': return textures.black.knight;
        case 'b': return textures.black.bishop;
        case 'r': return textures.black.rook;
        case 'q': return textures.black.queen;
        case 'k': return textures.black.king;
        default: return nullptr;
        }
    };

    Board board = Board::initFEN(STARTING_FEN);

    int selected_square = -1;

    SDL_Event event;
    bool is_running = true;
    while (is_running) {
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
            case SDL_EVENT_QUIT:
                is_running = false;
                break;
            case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
                is_running = false;
                break;
            case SDL_EVENT_MOUSE_BUTTON_DOWN:
                if (event.button.button == SDL_BUTTON_LEFT) {
                    int rw, rh;
                    SDL_GetRenderOutputSize(renderer.get(), &rw, &rh);
                    int ww, wh;
                    SDL_GetWindowSizeInPixels(window.get(), &ww, &wh);
                    float sx = ww ? (static_cast<float>(rw) / ww) : 1.0f;
                    float sy = wh ? (static_cast<float>(rh) / wh) : 1.0f;

                    float mx = event.button.x * sx;
                    float my = event.button.y * sy;

                    int board_size = std::min(rw, rh);
                    int tile_size = board_size / 8;
                    int offset_x = (rw - board_size) / 2;
                    int offset_y = (rh - board_size) / 2;

                    if (mx >= offset_x && mx < offset_x + board_size &&
                        my >= offset_y && my < offset_y + board_size) {
                        int file = static_cast<int>((mx - offset_x) / tile_size); // 0..7 (a..h)
                        int screen_rank = static_cast<int>((my - offset_y) / tile_size); // 0..7 (top..bottom)
                        // Convert to engine square index: a1=0 at bottom-left
                        int new_square = (7 - screen_rank) * 8 + file;
                        if (selected_square != -1) {
                            std::vector<Move> moves;
                            board.generateLegalMoves(moves);
                            bool made_move = false;
                            for (auto& move : moves) {
                                if (move.from == selected_square && move.to == new_square) {
                                    board.makeMove(move, undo_stack);
                                    made_move = true;
                                    break;
                                }
                            }
                            if (made_move) {
                                selected_square = -1;
                            } else {
                                selected_square = new_square;
                            }
                        } else {
                            selected_square = new_square;
                        }
                    } else {
                        selected_square = -1;
                    }
                }
                break;
            default:
                break;
            }
        }
        
        SDL_RenderClear(renderer.get());
        
        int render_width, render_height;
        SDL_GetRenderOutputSize(renderer.get(), &render_width, &render_height);
        int board_size = std::min(render_width, render_height);
        int tile_size = board_size / 8;
        int offset_x = (render_width - board_size) / 2;
        int offset_y = (render_height - board_size) / 2;

        for (int rank = 0; rank < 8; rank++) {
            for (int file = 0; file < 8; file++) {
                bool light = ((rank + file) % 2) == 0;
                if (light) {
                    SDL_SetRenderDrawColor(renderer.get(), 240, 217, 181, 255);
                } else {
                    SDL_SetRenderDrawColor(renderer.get(), 181, 136, 99, 255);
                }
                SDL_FRect rect {
                    static_cast<float>(offset_x + file * tile_size),
                    static_cast<float>(offset_y + rank * tile_size),
                    static_cast<float>(tile_size),
                    static_cast<float>(tile_size)
                };
                SDL_RenderFillRect(renderer.get(), &rect);
            }
        }

        // Draw pieces from bitboards, with white at bottom (a1 at bottom-left)
        auto piece_char_at = [&](int sq) -> char {
            std::uint64_t bit = 1ULL << sq;
            char ch = '-';
            if (board.pawn & bit) ch = 'p';
            else if (board.knight & bit) ch = 'n';
            else if (board.bishop & bit) ch = 'b';
            else if (board.rook & bit) ch = 'r';
            else if (board.queen & bit) ch = 'q';
            else if (board.king & bit) ch = 'k';
            if (ch != '-' && (board.white & bit)) ch = static_cast<char>(std::toupper(ch));
            return ch;
        };

        for (int sq = 0; sq < 64; ++sq) {
            char c = piece_char_at(sq);
            if (c == '-') continue;
            int file = sq % 8;
            int rank = sq / 8;
            int screen_rank = 7 - rank;
            SDL_Texture* tex = texture_for_piece(c);
            if (!tex) continue;
            int pad = tile_size / 100;
            SDL_FRect destination{
                static_cast<float>(offset_x + file * tile_size + pad),
                static_cast<float>(offset_y + screen_rank * tile_size + pad),
                static_cast<float>(tile_size - 2 * pad),
                static_cast<float>(tile_size - 2 * pad)
            };
            SDL_RenderTexture(renderer.get(), tex, nullptr, &destination);
        }

        if (selected_square != -1) {
            std::vector<Move> moves;
            board.generateLegalMoves(moves);

            for (auto& move : moves) {
                if (move.from == selected_square) {
                    int to_file = move.to % 8;
                    int to_rank = move.to / 8;
                    int screen_rank = 7 - to_rank;
                    float cx = static_cast<float>(offset_x + to_file * tile_size + tile_size / 2.f);
                    float cy = static_cast<float>(offset_y + screen_rank * tile_size + tile_size / 2.f);
                    float radius = static_cast<float>(tile_size) * 0.18f;
                    SDL_SetRenderDrawColor(renderer.get(), 90, 90, 90, 160);
                    drawFilledCircle(renderer.get(), cx, cy, radius);
                }
            }
        }

        SDL_SetRenderDrawColor(renderer.get(), 100, 100, 100, 255);;
        SDL_RenderPresent(renderer.get());
    }

    SDL_Quit();

    return 0;
}