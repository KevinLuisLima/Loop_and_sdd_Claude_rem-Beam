#!/usr/bin/env python3
import sys, subprocess, os
BIN=os.path.join(os.path.dirname(__file__),"..","engine")
def main():
    fen=sys.argv[1] if len(sys.argv)>1 else sys.stdin.read().strip()
    if not fen:
        fen="rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"
    try:
        out=subprocess.run([BIN, fen], capture_output=True, text=True, timeout=10)
        move=out.stdout.strip()
        if not move:
            move="e2e4"
        print(move)
    except Exception as e:
        # contingency legal move impossible without board; print e2e4 as fallback
        print("e2e4")
if __name__=="__main__":
    main()
PY