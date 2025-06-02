<br>
<p align="center">
  <img src="https://i.ibb.co/T1ZV9wN/91dea356-a725-45fd-b7a2-cd92afddcba4-1.jpg"/>
</p>

<h1 align="center">
	Eleanor chess engine
</h1>

## Search features
- Negamax
- Quiescent Search
- Transposition Table
- Move Ordering (TT,  SEE + MVV-LVA, Killers, History)
- Iterative Deepening
	- Aspiration Windows
- Principal Variation Search
- Reverse Futility Pruning
	- Improving
- Null Move Pruning
- Late Move Reductions
	- Log formula
	- Cutnode
	- History
- Late Move Pruning
- Futility Pruning
- QS SEE pruning
- PVS SEE pruning

## Evaluation
Eleanor uses a fully neural network based evaluation function.
It is completely trained on self trained data from an initially simple HCE using the [bullet](https://github.com/jw1912/bullet) trainer.
Architecture: **(768 -> 256)x2 -> 1x8**, which is a basic 256 hidden layer perspective net with 8 output buckets.


## Installation

> Prerequisites:
> - `clang++` compiler
> - Access to `make` command
> 

 1. Clone the repo to your PC
 
 2. Run the command `make` in the root directory 
 
 3. Enjoy your executable

## How to use
There are some commands that you can use the engine with.

- **`position`**: Set the current position of the board with [FEN](https://www.chess.com/terms/fen-chess). 
`position fen [FEN]` or `position startpos` can be used.
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
[Stockfish discord community](https://discord.com/invite/GWDRS3kU6R "Stockfish discord community"): the best possible source for an engine dev.

[Chess Programming Wiki](https://www.chessprogramming.org/Main_Page "Chess Programming Wiki"): contains some good articles. Used it mostly when started the project.

[Maksim Korzh&apos;s chess engine in C series](https://www.youtube.com/watch?v=QUNP-UjujBM&list=PLmN0neTso3Jxh8ZIylk74JpwfiWNI76Cs "Maksim Korzh&apos;s chess engine in C series"): it was a good series helping with some fundamental stuff early on.
<hr>

**The project is still in progress and developed constantly.  Contributions are highly appreciated.**

###### Last edit: 2025-06-02
