import subprocess, sys
def get_move(fen, weights="weights/model_0.bin", time_ms=2000):
    r=subprocess.run(["./engine", fen, weights, str(time_ms)], capture_output=True, text=True)
    if r.returncode != 0:
        # fallback legal move: first piece push? return None
        return None
    return r.stdout.strip().split()[-1] if r.stdout.strip() else None
if __name__=="__main__":
    fen=sys.argv[1]
    w=sys.argv[2] if len(sys.argv)>2 else "weights/model_0.bin"
    print(get_move(fen,w))
