#!/usr/bin/env python3
import argparse, os, glob, subprocess, random, sys
import chess

ENGINE=os.path.join(os.path.dirname(os.path.dirname(__file__)),"chess_engine")

def engine_move(fen, model, time_ms=1000):
    r=subprocess.run([ENGINE,"--fen",fen,"--time",str(time_ms),"--model",model], capture_output=True, text=True, timeout=15)
    return r.stdout.strip().split()[-1] if r.returncode==0 and r.stdout.strip() else None

def play_game(model_a, model_b, opening_moves):
    board=chess.Board()
    for m in opening_moves:
        board.push_uci(m)
    mvcount=0
    subopt={"a":0,"b":0}
    total_moves={"a":0,"b":0}
    while not board.is_game_over() and mvcount<300:
        turn="a" if board.turn==chess.WHITE else "b"
        # map turn to model depending starting color: model_a always white in this call; swap outside
        model = model_a if board.turn==chess.WHITE else model_b
        fen=board.fen()
        mv=engine_move(fen, model, 1000)
        if mv is None:
            break
        try:
            move=chess.Move.from_uci(mv)
        except:
            break
        if move not in board.legal_moves:
            break
        # judge: deeper search with model? use engine no model? Use same model high time for best
        # Use engine with 3000ms as judge for suboptimal threshold (move equality)
        judge_mv=engine_move(fen, model, 3000)
        total_moves[turn]+=1
        if judge_mv is not None and judge_mv!=mv:
            subopt[turn]+=1
        board.push(move)
        mvcount+=1
    result=board.result() if board.is_game_over() else "1/2-1/2"
    if result=="1-0": return 1.0,0.0,subopt,total_moves
    if result=="0-1": return 0.0,1.0,subopt,total_moves
    return 0.5,0.5,subopt,total_moves

OPENINGS=["e2e4 e7e5","d2d4 d7d5","g1f3 g8f6","c2c4 c7c5","e2e4 c7c5","d2d4 g8f6"]

def round_robin(models, games_per_pair):
    n=len(models)
    scores=[0.0]*n
    wld=[[0,0,0] for _ in range(n)]
    total_sub=[0]*n
    total_mv=[0]*n
    for i in range(n):
        for j in range(i+1,n):
            for g in range(games_per_pair):
                opening=random.choice(OPENINGS).split()
                # alternate colors: even game i white, odd swapped
                if g%2==0:
                    sa,sb,sub,tmv=play_game(models[i],models[j],opening)
                    scores[i]+=sa; scores[j]+=sb
                else:
                    sb,sa,sub,tmv=play_game(models[j],models[i],opening)
                    scores[i]+=sa; scores[j]+=sb
                # map subopt
                # total_moves mapping crude: all attributed by index via turn? We store raw counts not enough; print aggregate
                total_sub[i]+=sub["a"]+sub["b"]  # simplified
                total_mv[i]+=tmv["a"]+tmv["b"]
                total_sub[j]+=0; total_mv[j]+=0
                wld[i][0]+= 1 if (sa>sb) else 0
                wld[i][1]+= 1 if (sa==sb) else 0
                wld[i][2]+= 1 if (sa<sb) else 0
    # winrate
    games_played=[0]*n
    for i in range(n):
        games_played[i]= (n-1)*games_per_pair
    print("Model,Games,W,D,L,Winrate,Subopt,SuboptRate")
    for i,m in enumerate(models):
        rate = scores[i]/games_played[i] if games_played[i]>0 else 0
        subrate = total_sub[i]/max(1,total_mv[i])
        print(f"{os.path.basename(m)},{games_played[i]},{wld[i][0]},{wld[i][1]},{wld[i][2]},{rate:.3f},{total_sub[i]},{subrate:.4f}")
    return scores

def main():
    ap=argparse.ArgumentParser()
    ap.add_argument("--models", default="models/*.weights")
    ap.add_argument("--games", type=int, default=2)
    ap.add_argument("--generations", type=int, default=5)
    ap.add_argument("--output", default="models")
    args=ap.parse_args()
    models=sorted(glob.glob(args.models))
    if len(models)<2:
        print("Need at least 2 models"); sys.exit(1)
    for gen in range(args.generations):
        print(f"\n=== Generation {gen} ===")
        scores=round_robin(models, args.games)
        # best
        bi=max(range(len(models)), key=lambda i:(scores[i], -i))
        print("Best model:", os.path.basename(models[bi]), "winrate", scores[bi]/max(1,(len(models)-1)*args.games))
        # next population: mutate best into new models, keep original count
        new_models=[]
        for k in range(len(models)):
            # copy+mutate via generate script? simple mutate bytes in process
            path=models[bi]
            # read weights
            import struct
            with open(path,"rb") as f:
                hdr=f.read(12); ws=list(struct.unpack("<%df"%( (os.path.getsize(path)-12)//4), f.read()))
            rng=random.Random(1000*gen+k)
            for i in range(len(ws)):
                ws[i] += rng.uniform(-0.05,0.05)*max(1,abs(ws[i]))
            outp=os.path.join(args.output, f"gen{gen}_model{k}.weights")
            with open(outp,"wb") as f:
                f.write(hdr);
                for x in ws: f.write(struct.pack("<f", float(x)))
            new_models.append(outp)
        models=new_models

if __name__=="__main__":
    main()
