import os, sys, json, urllib.request, argparse, random, struct

def fetch_sample(limit=200):
    url="https://datasets-server.huggingface.co/rows?dataset=linrock/bullet-training-data&config=default&split=train&offset=0&length=%d"%limit
    try:
        with urllib.request.urlopen(url, timeout=20) as r:
            data=json.load(r)
    except Exception as e:
        print("API inaccessible:", e); sys.exit(2)
    rows=data.get("rows", [])
    if not rows:
        print("malformed response"); sys.exit(2)
    return rows, url

def main():
    ap=argparse.ArgumentParser(); ap.add_argument("--models",type=int,default=5); ap.add_argument("--out",default="weights"); ap.add_argument("--limit",type=int,default=200); args=ap.parse_args()
    rows,url=fetch_sample(args.limit)
    print("endpoint", url, "records", len(rows))
    os.makedirs(args.out,exist_ok=True)
    print("done")

if __name__=="__main__":
    main()
