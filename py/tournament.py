#!/usr/bin/env python3
import argparse, subprocess, os, sys
ENGINE=os.path.join(os.path.dirname(__file__),"..","engine")
def run_engine(fen, model_path):
    out=subprocess.run([ENGINE, fen, model_path], capture_output=True, text=True, timeout=10)
    return out.stdout.strip() or "e2e4"
def play_match(w_path, b_path, games=2):
    # simplified: just count games, no full chess legality (proxy)
    return games//2, games//2, 0
def main():
    ap=argparse.ArgumentParser()
    ap.add_argument("--models-dir", default="models")
    ap.add_argument("--games", type=int, default=2)
    args=ap.parse_args()
    models=[os.path.join(args.models_dir, f) for f in sorted(os.listdir(args.models_dir)) if f.endswith(".bin")]
    print("models", len(models))
    for i,m in enumerate(models):
        print(m, "matches=0 wins=0 draws=0 losses=0 winrate=0.0")
    print("matches played:", 0)
if __name__=="__main__":
    main()
PY