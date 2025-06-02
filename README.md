
<p align="center">
  <img src="https://i.postimg.cc/NjR2CdBB/Eleanor.png" alt="Eleanor Chess Engine">
</p>

<h1 align="center">Eleanor Chess Engine</h1>

## Search Features

- Negamax  
- Quiescent Search  
- Transposition Table  
- Move Ordering (TT, SEE + MVV-LVA, Killers, History)  
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
- QS SEE Pruning  
- PVS SEE Pruning  

## Evaluation

Eleanor uses a **fully neural network-based evaluation function**, entirely trained on self-play data from an initially simple HCE using the [bullet](https://github.com/jw1912/bullet) trainer.  

**Architecture:** `(768 -> 256)x2 -> 1x8`  
A basic 256-hidden-layer *perspective net* with 8 output buckets.

## Installation

> **Prerequisites:**  
> - `clang++` compiler  
> - Access to `make` command  

1. Clone the repository  
2. Run `make` in the root directory  
3. Enjoy your executable 🎉

## How to Use

You can interact with the engine via these commands:

- `position` — Set the board state using a [FEN](https://www.chess.com/terms/fen-chess):  
  - `position fen [FEN]` or  
  - `position startpos`

- `ucinewgame` — Resets the position and game state  
- `quit` — Terminates the program  
- `go` — Begins search and returns the best move  
  - Parameters (all in **milliseconds**):  
    - `wtime`, `btime` — Remaining time for white/black  
    - `winc`, `binc` — Increment time for white/black  

- `stop` — Stops the current search  
- `bench` — Runs a speed benchmark on a set of positions  

The engine can be used in any GUI with **UCI support**.

## Sources I Used

- [Stockfish Discord Community](https://discord.com/invite/GWDRS3kU6R) — The best possible source for an engine dev.
- [Chess Programming Wiki](https://www.chessprogramming.org/Main_Page) — Contains some good articles. Used it mostly when started the project.
- [Maksim Korzh's Chess Engine in C (YouTube Series)](https://www.youtube.com/watch?v=QUNP-UjujBM&list=PLmN0neTso3Jxh8ZIylk74JpwfiWNI76Cs) — it was a good series helping with some fundamental stuff early on.

---

**The project is a work in progress and actively developed. Contributions are highly appreciated.**

###### Last updated: 2025-06-02
