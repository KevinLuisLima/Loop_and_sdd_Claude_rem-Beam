#!/usr/bin/env python3
import sys, subprocess, os
ENGINE=os.path.join(os.path.dirname(__file__), "chess_engine")
def main():
    if len(sys.argv)<2:
        print("usage: agent.py <FEN> [model_path]", file=sys.stderr); sys.exit(2)
    fen=sys.argv[1]
    time_ms=9000
    args=["--fen", fen, "--time", str(time_ms)]
    if len(sys.argv)>=3:
        args += ["--model", sys.argv[2]]
    try:
        r=subprocess.run([ENGINE]+args, capture_output=True, text=True, timeout=10)
    except Exception as e:
        print("error", e, file=sys.stderr); sys.exit(1)
    if r.returncode!=0:
        # contingency: if board valid, try first legal? Engine already outputs contingency; otherwise error
        print("error", r.stderr.strip(), file=sys.stderr); sys.exit(r.returncode)
    out=r.stdout.strip().splitlines()
    if not out:
        print("error no output", file=sys.stderr); sys.exit(1)
    print(out[-1])

if __name__=="__main__":
    main()
PY
chmod +x agent.py
python3 agent.py "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1" models/model0.weights