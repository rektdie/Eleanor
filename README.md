![Eleanor](https://i.ibb.co/T1ZV9wN/91dea356-a725-45fd-b7a2-cd92afddcba4-1.jpg)
# Eleanor is a chess engine
## Features

- Bitboard board representation
- Magic Bitboards
- Legal move generation
- Precalculated attack tables
- Storing moves in a 16 bit word
- Copy/make moving
- Simple evaluation (material and piece-square tables, MVV-LVA)
- Principal Variation Search
- Move ordering
- Time management
- UCI

## Installation

> Prerequisites:
> - A C++ compiler
> - Access to `make` command

 1. Clone the repo to your PC
 
 2. Run the command `make` in the root directory 
 > You can specify the compiler with the flag `CXX` 
 > and the executable name with `EXE`. 
 > By default they are `g++` and `Eleanor` respectively.

3. Run the executable

## How to use
There are some commands that you can use the engine with.

- **`position`**: Set the current position of the board with [FEN](https://www.chess.com/terms/fen-chess). 
`position [FEN]` or `position startpos` can be used.
- **`ucinewgame`** resets the position and game state
- **`quit`** terminates the program
- **`go`** is the command that asks what is the best move in the current position (search),
but we have to give it some parameters:
`wtime` `btime` `winc` `binc`
The first letter tells the color of player
wtime sets the remaining time of the white player
and winc sets the increments for the white player.
**It's important that these parameters are given in milliseconds!**
- **`stop`** stops the current search
- **`bench`** tests the search speed of the engine on a set of positions

Also, the engine can be connected to any GUI software that has UCI support.

## Sources I used
[Chess Programming Wiki](https://www.chessprogramming.org/Main_Page "Chess Programming Wiki")

[Maksim Korzh&apos;s chess engine in C series](https://www.youtube.com/watch?v=QUNP-UjujBM&list=PLmN0neTso3Jxh8ZIylk74JpwfiWNI76Cs "Maksim Korzh&apos;s chess engine in C series")

[Stockfish discord helping community](https://discord.com/invite/GWDRS3kU6R "Stockfish discord community")

[Logo is generated by AI](https://deepai.org/ "Logo is generated by AI")

<hr>

**The project is still in progress and developed constantly.  Contributions are highly appreciated.**

###### Last edited: 2025-01-23

