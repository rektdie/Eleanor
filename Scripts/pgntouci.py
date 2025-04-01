##########################################
# convert pgn to uci
##########################################
import chess
import chess.pgn

pgn = open("game.pgn")

game = chess.pgn.read_game(pgn)

board = game.board()

moves = "position fen " + board.fen() + " moves"

for move in game.mainline_moves():
    board.push(move)
    moves += " " + move.uci()

print(moves)