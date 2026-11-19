#!/usr/bin/env python3
import argparse, struct, os, sys, hashlib, random, urllib.request

WEIGHT_COUNT = 400
HF_API = "https://huggingface.co/api/datasets/linrock/bullet-training-data"
HF_TREE = "https://huggingface.co/api/datasets/linrock/bullet-training-data/tree/main"

def fetch_sample():
    try:
        req = urllib.request.Request(HF_API, headers={"User-Agent":"beam3"})
        with urllib.request.urlopen(req, timeout=15) as r:
            data = r.read(2048)
        req2 = urllib.request.Request(HF_TREE, headers={"User-Agent":"beam3"})
        with urllib.request.urlopen(req2, timeout=15) as r:
            data2 = r.read(2048)
        if not data or not data2:
            raise RuntimeError("empty response")
        return data + data2
    except Exception as e:
        print("API HuggingFace inacessivel ou mal formada: %s" % e, file=sys.stderr)
        sys.exit(1)

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--num-models", type=int, default=5)
    ap.add_argument("--variation", type=float, default=0.15)
    ap.add_argument("--seed", type=int, default=42)
    ap.add_argument("--output-dir", default="models")
    args = ap.parse_args()

    sample = fetch_sample()
    digest = hashlib.sha256(sample).hexdigest()
    rng = random.Random(args.seed + int(digest[:8], 16))

    os.makedirs(args.output_dir, exist_ok=True)
    base = [0.0]*WEIGHT_COUNT
    base[0:6] = [100.0,320.0,330.0,500.0,900.0,0.0]
    for i in range(6, 6+6*64):
        base[i] = rng.uniform(-5.0,5.0)
    for i in range(390, WEIGHT_COUNT):
        base[i] = rng.uniform(0.5,2.0)

    for mi in range(args.num_models):
        vals = list(base)
        for i in range(WEIGHT_COUNT):
            vals[i] += rng.gauss(0.0, max(1e-6, abs(base[i] if base[i]!=0 else 1.0)*args.variation))
        path = os.path.join(args.output_dir, "model_%d.bin" % mi)
        with open(path,"wb") as f:
            f.write(struct.pack("<III", 0x4245414D, 1, WEIGHT_COUNT))
            for v in vals:
                f.write(struct.pack("<f", float(v)))
        print("wrote", path)

if __name__=="__main__":
    main()