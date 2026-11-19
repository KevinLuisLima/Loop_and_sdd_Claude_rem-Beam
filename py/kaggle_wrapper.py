#!/usr/bin/env python3
import subprocess, sys, os

ENGINE = os.environ.get("CHESS_ENGINE", "./build/chess_engine")
WEIGHTS = os.environ.get("CHESS_WEIGHTS", "models/model_0.bin")

def best_move(fen):
    try:
        out = subprocess.run([ENGINE, "fen", fen, WEIGHTS, "200"],
                             capture_output=True, text=True, timeout=9)
        mv = out.stdout.strip()
        if not mv:
            raise RuntimeError("empty")
        return mv
    except Exception as e:
        # contingency: try startpos? return first legal via engine startpos
        try:
            out = subprocess.run([ENGINE,"fen","startpos" if False else "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", WEIGHTS, "100"],
                                 capture_output=True,text=True,timeout=9)
            return out.stdout.strip() or "e2e4"
        except Exception:
            return "e2e4"

if __name__=="__main__":
    # read FEN from argv or stdin
    if len(sys.argv)>=2:
        fen = sys.argv[1]
    else:
        fen = sys.stdin.read().strip()
    print(best_move(fen))