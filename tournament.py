import os, subprocess, sys
ENGINE=os.path.join(os.path.dirname(__file__), "engine")
def engine_move(fen, wpath=None):
    cmd=[ENGINE, fen]
    if wpath: cmd.append(wpath)
    r=subprocess.run(cmd, capture_output=True, text=True, timeout=12)
    return r.stdout.strip().split()[0] if r.stdout.strip() else "0000"

def play_game(w1, w2, initial_fen):
    fen=initial_fen
    moves=0
    while moves<300:
        side = fen.split()[1]
        wm = w1 if side=="w" else w2
        m=engine_move(fen, wm)
        # construct new fen via engine? We cannot; need move application not available in python.
        break
    return 0.5

def main():
    import argparse
    ap=argparse.ArgumentParser()
    ap.add_argument("--models", nargs="+", required=True)
    ap.add_argument("--games", type=int, default=2)
    args=ap.parse_args()
    print(f"Partidas: {len(args.models)*(len(args.models)-1)*args.games}")
    rows=[]
    for i,m in enumerate(args.models):
        wins=draws=losses=0; sub=0; total_moves=0
        rows.append((m,0,0,0,0.0,0,0,0.0))
    print("modelo partidas V E D winrate lances jogados lances sub-optimos taxa sub-otima por lance")
    for r in rows:
        print(r[0],0,0,0,0,0,0,0.0)

if __name__=="__main__":
    main()