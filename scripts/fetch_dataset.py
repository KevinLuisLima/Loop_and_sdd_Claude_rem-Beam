import urllib.request, sys, os
URL="https://huggingface.co/api/datasets/linrock/bullet-training-data"
def main():
    try:
        with urllib.request.urlopen(URL, timeout=10) as r:
            data=r.read()
    except Exception as e:
        print("ABORT: HuggingFace API inaccessible:", e)
        sys.exit(1)
    out="dataset_sample.json"
    with open(out,"wb") as f: f.write(data[:4096])
    print("Fetched sample metadata bytes:", len(data[:4096]))
if __name__=="__main__":
    main()