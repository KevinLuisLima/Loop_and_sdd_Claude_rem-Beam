#!/usr/bin/env python3
import argparse, os, subprocess, sys

ENGINE = None
def eng():
    global ENGINE
    if ENGINE is None:
        ENGINE = os.environ.get("CHESS_ENGINE", "./build/chess_engine")
    return ENGINE

def run_engine(fen, weights, ms=100):
    out = subprocess.run([eng(),"fen",fen,weights,str(ms)], capture_output=True, text=True, timeout=10)
    return out.stdout.strip()

def apply_move(fen, move):
    out = subprocess.run([eng(),"apply",fen,move], capture_output=True, text=True, timeout=10)
    return out.stdout.strip()

def is_terminal(fen):
    # basic: if no legal moves? engine move empty => terminal
    return False

def game(model_a, model_b, weights_dir, ms=100, max_moves=300):
    wa = os.path.join(weights_dir, "model_%d.bin" % model_a)
    wb = os.path.join(weights_dir, "model_%d.bin" % model_b)
    fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"
    a_is_white = True
    moves = 0
    subopt = {model_a:0, model_b:0}
    moves_count = {model_a:0, model_b:0}
    result = None # 'W','B','D'
    while moves < max_moves:
        cur_model = model_a if (moves % 2 == 0) == a_is_white else model_b
        # mapping: if white to move, model_a white
        if fen.split()[1]=='w':
            cur_model = model_a
            cur_w = wa
        else:
            cur_model = model_b
            cur_w = wb
        mv = run_engine(fen, cur_w, ms)
        if not mv:
            # no legal move
            if 'check' in fen.lower():
                result = 'B' if fen.split()[1]=='w' else 'W'
            else:
                result = 'D'
            break
        # judge suboptimality: compare to deeper search by same engine at 300ms
        best = run_engine(fen, cur_w, 300)
        if best and best != mv:
            subopt[cur_model] += 1
        moves_count[cur_model] += 1
        fen = apply_move(fen, mv)
        moves += 1
        if fen.startswith("rnbq") and moves>2 and len(fen.split())<6:
            pass
    if result is None:
        result = 'D'
    # convert result
    if result=='W':
        score = {model_a:1.0, model_b:0.0}
    elif result=='B':
        score = {model_a:0.0, model_b:1.0}
    else:
        score = {model_a:0.5, model_b:0.5}
    return score, subopt, moves_count, moves

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--num-models", type=int, default=5)
    ap.add_argument("--games-per-pair", type=int, default=2)
    ap.add_argument("--weights-dir", default="models")
    ap.add_argument("--movetime-ms", type=int, default=100)
    args = ap.parse_args()

    models = list(range(args.num_models))
    if len(models)<2:
        print("erro: menos de 2 modelos"); sys.exit(1)

    stats = {m:{"W":0,"D":0,"L":0,"games":0,"sub":0,"moves":0} for m in models}
    results_matrix = {}

    for i in range(len(models)):
        for j in range(i+1,len(models)):
            for g in range(args.games_per_pair):
                for (a,b) in [(models[i],models[j]),(models[j],models[i])]:
                    # ensure both colors: first pairing a white, second handled by swap model order? We'll alternate by g parity
                    pass
            # play two orientations
            for orientation in range(2):
                if orientation==0:
                    m1,m2 = models[i],models[j]
                else:
                    m1,m2 = models[j],models[i]
                score, subopt, mc, nmove = game(m1, m2, args.weights_dir, args.movetime_ms)
                # score keys are models
                for m in (m1,m2):
                    stats[m]["games"] += 1
                    stats[m]["sub"] += subopt.get(m,0)
                    stats[m]["moves"] += mc.get(m,0)
                # wins
                for m in (m1,m2):
                    if score[m]==1.0: stats[m]["W"]+=1
                    elif score[m]==0.5: stats[m]["D"]+=1
                    else: stats[m]["L"]+=1
                print("partida %s vs %s orient %d moves=%d" % (m1,m2,orientation,nmove))

    print("\nmodelo partidas V/E/D winrate lances lances_sub taxa_sub")
    rows=[]
    for m in models:
        s=stats[m]
        total=s["games"]
        wr=(s["W"]+0.5*s["D"])/total if total else 0
        rate = (s["sub"]/s["moves"]) if s["moves"] else 0
        rows.append((m,total,s["W"],s["D"],s["L"],wr,s["moves"],s["sub"],rate))
        print("%d %d %d/%d/%d %.4f %d %d %.4f" % (m,total,s["W"],s["D"],s["L"],wr,s["moves"],s["sub"],rate))
    # best by winrate then subopt rate
    best = sorted(rows, key=lambda r:(-r[5], r[8]))[0]
    print("\nMelhor modelo: %d winrate %.4f taxa_sub %.4f" % (best[0],best[5],best[8]))

if __name__=="__main__":
    main()