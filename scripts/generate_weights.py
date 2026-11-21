import struct, random, os, argparse, sys
MAGIC=b"ECHW01"
NUM=389
def base():
    w=[0.0]*NUM
    mats=[100,320,330,500,900]
    for i in range(5): w[i]=float(mats[i])
    for t in range(6):
        for sq in range(64):
            idx=5+t*64+sq
            if idx<NUM:
                # simple center value base
                r=sq//8; f=sq%8
                center=(3.5-abs(r-3.5))+(3.5-abs(f-3.5))
                w[idx]=center*5.0
    return w
def main():
    ap=argparse.ArgumentParser()
    ap.add_argument("--models",type=int,default=5)
    ap.add_argument("--variation",type=float,default=0.1)
    ap.add_argument("--seed",type=int,default=1)
    ap.add_argument("--out",default="weights")
    args=ap.parse_args()
    os.makedirs(args.out,exist_ok=True)
    rng=random.Random(args.seed)
    for mi in range(args.models):
        w=base()
        for i in range(NUM):
            w[i]+=rng.uniform(-args.variation*max(1.0,abs(w[i])), args.variation*max(1.0,abs(w[i])))
        path=os.path.join(args.out, f"model_{mi}.bin")
        with open(path,"wb") as f:
            f.write(MAGIC)
            f.write(struct.pack("<III",1,NUM,0))
            f.write(struct.pack("<%df"%NUM, *w))
        print("wrote", path)
if __name__=="__main__":
    main()
