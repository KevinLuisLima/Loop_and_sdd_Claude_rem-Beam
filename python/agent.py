import subprocess, sys
def main():
    fen = sys.argv[1] if len(sys.argv)>1 else sys.stdin.read().strip()
    weights = sys.argv[2] if len(sys.argv)>2 else "models/model_0.bin"
    time_ms = sys.argv[3] if len(sys.argv)>3 else "7000"
    r = subprocess.run(["./engine","--weights",weights,"--time",time_ms, fen], capture_output=True, text=True)
    if r.returncode != 0:
        print("0000")
        return
    print(r.stdout.strip() or "0000")
if __name__=="__main__":
    main()
