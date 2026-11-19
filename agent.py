import subprocess, sys, os

ENGINE=os.path.join(os.path.dirname(__file__), "engine")

def get_move(fen):
    try:
        out=subprocess.run([ENGINE, fen], capture_output=True, text=True, timeout=10)
        move=out.stdout.strip().split()[0] if out.stdout.strip() else "0000"
        if not move: move="0000"
        return move
    except Exception:
        return "0000"

if __name__=="__main__":
    fen=sys.stdin.read().strip() if not sys.stdin.isatty() else "startpos"
    if len(sys.argv)>1:
        fen=" ".join(sys.argv[1:])
    if not fen:
        fen="rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"
    print(get_move(fen))