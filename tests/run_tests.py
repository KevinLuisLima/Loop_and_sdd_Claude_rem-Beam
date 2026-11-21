import subprocess, os, sys

BIN="./chess_engine"
tests=[]
def run(args):
    return subprocess.run([BIN]+args, capture_output=True, text=True)

def T(name, cond):
    tests.append((name, bool(cond)))

# perft initial
out=run(["--fen","rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1","--perft","5"]).stdout.strip()
T("perft init depth5", out=="4865609")

# kiwipete
out=run(["--fen","r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1","--perft","4"]).stdout.strip()
T("perft kiwipete depth4", out=="4085603")

# position3
out=run(["--fen","8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1","--perft","5"]).stdout.strip()
T("perft pos3 depth5", out=="674624")

# engine returns move
out=run(["--fen","rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1","--time","100"]).stdout.strip()
T("engine returns legal-ish move", len(out)==4 and out[1] in "12345678")

# invalid fen nonzero
r=run(["--fen","invalid fen","--time","10"])
T("invalid fen nonzero", r.returncode!=0)

# mate position no move?
out=run(["--fen","k7/8/1Q6/8/8/8/8/4K3 b - - 0 1","--time","100"]).stdout.strip()
T("engine outputs contingency on illegal? position with moves", len(out)>=4)

# sanitizer run
subprocess.run(["make","sanitize"], capture_output=True)
if os.path.exists("./chess_san"):
    r=subprocess.run(["./chess_san","--perft","4"], capture_output=True, text=True)
    T("sanitizer perft4", r.returncode==0)

passed=sum(1 for _,c in tests if c)
total=len(tests)
print(f"{passed}/{total} testes passaram")
sys.exit(0 if passed==total else 1)
PY
make test 2>&1 | tail -20