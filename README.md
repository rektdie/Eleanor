<div align="center">
<p>
  <img src="https://i.postimg.cc/bvDr095w/image.png" alt="Eleanor Chess Engine" width="300">
</p>

<h1 align="center">Eleanor Chess Engine</h1>

| Version | Release Date | [CCRL 40/15](https://www.computerchess.org.uk/ccrl/4040/cgi/compare_engines.cgi?family=Eleanor&print=Rating+list&print=Results+table&print=LOS+table&print=Ponder+hit+table&print=Eval+difference+table&print=Comopp+gamenum+table&print=Overlap+table&print=Score+with+common+opponents) | [CCRL Blitz](https://www.computerchess.org.uk/ccrl/404/cgi/compare_engines.cgi?family=Eleanor&print=Rating+list&print=Results+table&print=LOS+table&print=Ponder+hit+table&print=Eval+difference+table&print=Comopp+gamenum+table&print=Overlap+table&print=Score+with+common+opponents) |
| --- | --- | --- | --- |
| 2.0 | 2025-08-23 | 3386? (#87) | ?
| 1.0 | 2025-06-02 | 2856 (#244) | 2759 (#256)

</div>


## Evaluation

Eleanor uses a **fully neural network-based evaluation function**, entirely trained on self-play data from an initially simple HCE using the [bullet](https://github.com/jw1912/bullet) trainer.  

**Architecture:** `(768 -> 1024)x2 -> 1x8`  
A 1024-hidden-layer *perspective net* with 8 output buckets.

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

[Testing site](https://rektdie.pythonanywhere.com/)

## Sources I Used

- [Stockfish Discord Community](https://discord.com/invite/GWDRS3kU6R) — The best possible source for an engine dev.
- [Chess Programming Wiki](https://www.chessprogramming.org/Main_Page) — Contains some good articles. Used it mostly when started the project.
- [Maksim Korzh's Chess Engine in C (YouTube Series)](https://www.youtube.com/watch?v=QUNP-UjujBM&list=PLmN0neTso3Jxh8ZIylk74JpwfiWNI76Cs) — it was a good series helping with some fundamental stuff early on.

---

**The project is a work in progress and actively developed. Contributions are highly appreciated.**

###### Last updated: 2025-08-24
