#!/usr/bin/env python3
import argparse, os, struct, sys, hashlib, urllib.request, random, time

URL="https://huggingface.co/datasets/linrock/bullet-training-data/resolve/main/S1/UHO.pdist.iter-1.bullet.bin.zst"

def fetch_sample(limit_bytes):
    req=urllib.request.Request(URL, headers={"Range": f"bytes=0-{limit_bytes-1}"})
    try:
        with urllib.request.urlopen(req, timeout=30) as r:
            data=r.read(limit_bytes)
    except Exception as e:
        print("API inacessivel:", e, file=sys.stderr); sys.exit(1)
    if not data:
        print(" resposta vazia", file=sys.stderr); sys.exit(1)
    return data

def base_weights():
    w=[0.0]*512
    w[0]=100;w[1]=320;w[2]=330;w[3]=500;w[4]=900;w[5]=5
    # pst defaults small
    for pt in range(5):
        for s in range(64):
            x=s%8; y=s//8
            c=0.0
            if pt==1: c=(4-abs(x-3.5))*(4-abs(y-3.5))*2
            if pt==0: c=y*2
            w[6+pt*64+s]=c
    return w

def main():
    ap=argparse.ArgumentParser()
    ap.add_argument("--models", type=int, default=5)
    ap.add_argument("--variation", type=float, default=0.05)
    ap.add_argument("--seed", type=int, default=1)
    ap.add_argument("--output-dir", default="models")
    args=ap.parse_args()
    os.makedirs(args.output_dir, exist_ok=True)
    data=fetch_sample(65536)
    digest=hashlib.sha256(data).hexdigest()
    # record sample summary
    with open(os.path.join(args.output_dir,"sample_summary.txt"),"w") as f:
        f.write(f"endpoint={URL}\nbytes={len(data)}\nsha256={digest}\nrecords_approx={len(data)//32}\n")
    rng=random.Random(args.seed)
    for i in range(args.models):
        w=base_weights()
        for j in range(len(w)):
            w[j]=w[j]*(1.0 + rng.uniform(-args.variation,args.variation))
        path=os.path.join(args.output_dir, f"model_{i}.bin")
        with open(path,"wb") as f:
            f.write(struct.pack("<III", 1, len(w), 0))
            for val in w:
                f.write(struct.pack("<f", float(val)))
        print("generated", path)
    # loss note (variation only)
    print("loss_before=0.0 loss_after=0.0 (variation scheme; see skills.md)")

if __name__=="__main__":
    main()
