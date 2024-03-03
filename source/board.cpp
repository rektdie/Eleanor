#include "board.h"
#include "types.h"
#include "bitboards.h"
#include <iostream>

void Board::Init() {
	SetByFen(StartingFen);
}

void Board::Reset() {
	castlingRights = { false, false, false, false };
	enPassantTarget = 0ULL;
	halfMoves = 0;
	fullMoves = 0;
	sideToMove = White;
	occupied = 0ULL;
	std::cout << '\n';

	pieces = std::array<Bitboard, 6>();
	colors = std::array<Bitboard, 2>();
}

void Board::SetByFen(const char* fen) {
	Reset();

	// Starting from top left
	Bitboard currentSquare = 0x100000000000000;

	int digits = 0;
	while (*fen != ' ' && *fen) {
		PIECE pieceType = Pawn;
		COLOR pieceColor = Black;

		switch (*fen) {
		case 'p':
			pieceType = Pawn;
			pieceColor = Black;
			break;
		case 'P':
			pieceType = Pawn;
			pieceColor = White;
			break;
		case 'n':
			pieceType = Knight;
			pieceColor = Black;
			break;
		case 'N':
			pieceType = Knight;
			pieceColor = White;
			break;
		case 'b':
			pieceType = Bishop;
			pieceColor = Black;
			break;
		case 'B':
			pieceType = Bishop;
			pieceColor = White;
			break;
		case 'r':
			pieceType = Rook;
			pieceColor = Black;
			break;
		case 'R':
			pieceType = Rook;
			pieceColor = White;
			break;
		case 'k':
			pieceType = King;
			pieceColor = Black;
			break;
		case 'K':
			pieceType = King;
			pieceColor = White;
			break;
		case 'q':
			pieceType = Queen;
			pieceColor = Black;
			break;
		case 'Q':
			pieceType = Queen;
			pieceColor = White;
			break;
		case '/':
			currentSquare = currentSquare >> 8 + digits;
			digits = 0;
			fen++;
			continue;
		default:
			int num = (*fen) - 48;
			fen++;
			if (*fen != '/') {
				currentSquare = currentSquare << num;
				digits += num;
			}
			continue;
		}

		pieces[pieceType] |= currentSquare;
		colors[pieceColor] |= currentSquare;

		fen++;
		if (*fen != '/') {
			digits++;
			currentSquare = currentSquare << 1;
		}
	}

	fen++; // skipping previous space
	int section = 0;

	while (*fen) {
		switch (*fen) {
		case ' ':
			fen++;
			section++;
			continue;
		case 'w':
			sideToMove = White;
			break;
		case 'b':
			sideToMove = Black;
			break;
		case 'K':
			castlingRights[White] = true;
			break;
		case 'Q':
			castlingRights[White + 2] = true;
			break;
		case 'k':
			castlingRights[Black] = true;
			break;
		case 'q':
			castlingRights[Black + 2] = true;
			break;
		default:
			if (*fen != '-') {
				if (section == 2) {
					int passantFile;
					switch (*fen)
					{
					case 'a':
						passantFile = 0;
						break;
					case 'b':
						passantFile = 1;
						break;
					case 'c':
						passantFile = 2;
						break;
					case 'd':
						passantFile = 3;
						break;
					case 'e':
						passantFile = 4;
						break;
					case 'f':
						passantFile = 5;
						break;
					case 'g':
						passantFile = 6;
						break;
					case 'h':
						passantFile = 7;
						break;
					}
					fen++;
					const int passantRank = *fen - 49;
					enPassantTarget = Bitboard::GetSquare(passantFile + passantRank*8);
				}
				else if (section == 3) {
					halfMoves = (*fen) - 48;
				}
				else if (section == 4) {
					fullMoves = (*fen) - 48;
				}
			}

			break;
		}
		fen++;
	}

	occupied = (colors[White] | colors[Black]);
}