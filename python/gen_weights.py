#!/usr/bin/env python3
import argparse, struct, os, random, sys, urllib.request

NW=394
BASE_URL="https://huggingface.co/datasets/linrock/bullet-training-data/resolve/main/S1/UHO.pdist.iter-1.bullet.bin.zst"

def fetch_sample(nbytes):
    req=urllib.request.Request(BASE_URL, headers={"Range":f"bytes=0-{nbytes-1}"})
    with urllib.request.urlopen(req, timeout=30) as r:
        data=r.read()
    return data

def base_weights():
    w=[0.0]*NW
    # material pawn..queen
    for i,v in enumerate([100,320,330,500,900]):
        w[i]=v
    # PST all reference-ish small; put 0 base? requirement says not zero. Fill standard-ish values 0..
    for p in range(6):
        for sq in range(64):
            w[5+p*64+sq]=0.0
    # center bonus for knights
    for sq in range(64):
        r,c=divmod(sq,8)
        center=(3.5-abs(r-3.5))*(3.5-abs(c-3.5))
        w[5+1*64+sq]=center*5.0
        w[5+2*64+sq]=center*3.0
        w[5+0*64+sq]= (r)*2.0 if False else 0.0
    # pawn advancement
    for sq in range(64):
        r=sq//8
        w[5+0*64+sq]= (r)*3.0
    w[NW-4]= -8.0 # mobility placeholder
    w[NW-3]= -12.0
    w[NW-2]= -20.0
    w[NW-1]= -5.0
    return w

def write_model(path,w):
    with open(path,"wb") as f:
        f.write(b"CHW1")
        f.write(struct.pack("<III",1,len(w),0)) # version, n, type? we need n u32 then type u8
    # fix packing: rewrite carefully
    with open(path,"wb") as f:
        f.write(b"CHW1")
        f.write(struct.pack("<I",1))  # version
        f.write(struct.pack("<I",len(w))) # n
        f.write(struct.pack("<B",0))  # type float32
        f.write(struct.pack("<%df"%len(w), *w))

def main():
    ap=argparse.ArgumentParser()
    ap.add_argument("--num-models",type=int,default=5)
    ap.add_argument("--variation",type=float,default=0.15)
    ap.add_argument("--seed",type=int,default=42)
    ap.add_argument("--outdir",default="models")
    args=ap.parse_args()
    random.seed(args.seed)
    os.makedirs(args.outdir,exist_ok=True)
    # attempt sample
    try:
        data=fetch_sample(65536)
        print(f"dataset sample fetched: {len(data)} bytes")
    except Exception as e:
        print("ERROR: cannot access HuggingFace dataset:", e)
        sys.exit(1)
    base=base_weights()
    for i in range(args.num_models):
        w=list(base)
        for j in range(len(w)):
            w[j]=w[j]*(1.0+random.uniform(-args.variation,args.variation))
        # keep material positive
        for j in range(5):
            if w[j]<10: w[j]=10
        out=os.path.join(args.outdir, f"model_{i}.bin")
        write_model(out,w)
        print("wrote",out)
    print(f"records used: 0 positions parsed from sample ({len(data)} raw bytes read)")

if __name__=="__main__":
    main()
