import argparse, struct, random, os, sys

NUM_WEIGHTS=6
def main():
    ap=argparse.ArgumentParser()
    ap.add_argument("--models", type=int, default=5)
    ap.add_argument("--variation", type=float, default=0.2)
    ap.add_argument("--seed", type=int, default=1)
    ap.add_argument("--out", default="models")
    args=ap.parse_args()
    os.makedirs(args.out, exist_ok=True)
    rng=random.Random(args.seed)
    base=[1.0,1.0,1.0,1.0,1.0,1.0]
    for i in range(args.models):
        w=[max(0.1, b + rng.uniform(-args.variation, args.variation)) for b in base]
        # ensure meaningful differences
        w[0] += i*0.01
        path=os.path.join(args.out, f"model_{i}.bin")
        with open(path,"wb") as f:
            f.write(struct.pack("<II", 1, NUM_WEIGHTS))
            f.write(struct.pack("<"+"f"*NUM_WEIGHTS, *w))
    print(f"Generated {args.models} models in {args.out}")

if __name__=="__main__":
    main()