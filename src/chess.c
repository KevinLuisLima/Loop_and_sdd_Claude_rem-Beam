#include "chess.h"
#include "eval.h"
#include "weights.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <math.h>
#include <time.h>

#define EMPTY_SQ 12

static Color col(int pc){ return pc<6?WHITE:BLACK; }
Color color_of(int piece){ return piece<6?WHITE:BLACK; }
int type_of(int piece){ return piece%6; }

void pos_init_start(Position *p){
    memset(p,0,sizeof(*p));
    p->ep=-1;
    char fen[]="rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
    pos_from_fen(p,fen);
}

static void clear_pos(Position *p){ memset(p,0,sizeof(*p)); for(int i=0;i<64;i++) p->board[i]=12; p->ep=-1; }

static void rebuild_occ(Position *p){
    p->occ[0]=p->occ[1]=p->occ[2]=0;
    for(int pc=0;pc<12;pc++) p->occ[col(pc)] |= p->bb[pc];
    p->occ[2]=p->occ[0]|p->occ[1];
}

int pos_from_fen(Position *p, const char *fen){
    if(!p||!fen) return 0;
    clear_pos(p);
    p->valid=false;
    /* work on copy */
    char buf[512]; strncpy(buf,fen,511); buf[511]=0;
    char *tok = strtok(buf," ");
    if(!tok) return 0;
    int rank=7, file=0;
    for(char *c=tok; *c; c++){
        if(*c=='/'){ rank--; file=0; if(rank<0) return 0; continue; }
        if(isdigit((unsigned char)*c)){ file += *c-'0'; if(file>8) return 0; continue; }
        int piece=-1;
        switch(*c){
            case 'P': piece=WP; break; case 'N': piece=WN; break; case 'B': piece=WB; break;
            case 'R': piece=WR; break; case 'Q': piece=WQ; break; case 'K': piece=WK; break;
            case 'p': piece=BP; break; case 'n': piece=BN; break; case 'b': piece=BB; break;
            case 'r': piece=BR; break; case 'q': piece=BQ; break; case 'k': piece=BK; break;
            default: return 0;
        }
        if(rank<0||file>7) return 0;
        int s=rank*8+file;
        p->board[s]=piece;
        p->bb[piece] |= (1ULL<<s);
        file++;
    }
    rebuild_occ(p);
    tok=strtok(NULL," ");
    if(!tok) p->side=WHITE;
    else if(tok[0]=='w') p->side=WHITE;
    else if(tok[0]=='b') p->side=BLACK;
    else return 0;

    tok=strtok(NULL," ");
    p->castling=0;
    if(tok && strcmp(tok,"-")!=0){
        for(char *c=tok;*c;c++){
            switch(*c){ case 'K': p->castling|=1; break; case 'Q': p->castling|=2; break;
            case 'k': p->castling|=4; break; case 'q': p->castling|=8; break; default: break;}
        }
    }
    tok=strtok(NULL," ");
    p->ep=-1;
    if(tok && strcmp(tok,"-")!=0){
        if(strlen(tok)<2) return 0;
        int f=tok[0]-'a', r=tok[1]-'1';
        if(f<0||f>7||r<0||r>7) return 0;
        p->ep=r*8+f;
    }
    tok=strtok(NULL," ");
    p->halfmove = tok? atoi(tok):0;
    tok=strtok(NULL," ");
    p->fullmove = tok? atoi(tok):1;

    int wk=0,bk=0;
    for(int s=0;s<64;s++){
        if(p->board[s]==WK) wk++;
        if(p->board[s]==BK) bk++;
    }
    if(wk!=1 || bk!=1) return 0;
    p->valid=true;
    return 1;
}

int pos_to_fen(const Position *p, char *buf, size_t sz){
    if(!p||!buf) return 0;
    size_t pos=0;
    for(int r=7;r>=0;r--){
        int empty=0;
        for(int f=0;f<8;f++){
            int sqq=r*8+f;
            int pc=p->board[sqq];
            if(pc==12){ empty++; continue; }
            if(empty){
                if(pos+1>=sz) return 0;
                buf[pos++]='0'+empty; empty=0;
            }
            char ch;
            switch(pc){ case WP:ch='P';break;case WN:ch='N';break;case WB:ch='B';break;case WR:ch='R';break;case WQ:ch='Q';break;case WK:ch='K';break;
                case BP:ch='p';break;case BN:ch='n';break;case BB:ch='b';break;case BR:ch='r';break;case BQ:ch='q';break;case BK:ch='k';break; default: ch='?';}
            if(pos+1>=sz) return 0;
            buf[pos++]=ch;
        }
        if(empty){
            if(pos+1>=sz) return 0;
            buf[pos++]='0'+empty;
        }
        if(r>0){ if(pos+1>=sz) return 0; buf[pos++]='/'; }
    }
    /* side */
    if(pos+4>=sz) return 0;
    buf[pos++]=' '; buf[pos++]= (p->side==WHITE?'w':'b'); buf[pos++]=' ';
    /* castling */
    int any=0;
    if(p->castling&1){buf[pos++]='K';any=1;}
    if(p->castling&2){buf[pos++]='Q';any=1;}
    if(p->castling&4){buf[pos++]='k';any=1;}
    if(p->castling&8){buf[pos++]='q';any=1;}
    if(!any) buf[pos++]='-';
    /* ep */
    buf[pos++]=' ';
    if(p->ep>=0){
        int f=p->ep%8, r=p->ep/8;
        buf[pos++]='a'+f; buf[pos++]='1'+r;
    } else buf[pos++]='-';
    buf[pos]=0;
    return 1;
}

/* attacks */
static int attacked_by_piece(Position *p, int sq, int color){
    int r=sq/8, f=sq%8;
    /* pawns */
    if(color==WHITE){
        /* white pawn attacks up-left/up-right from square? attack squares for white: sq-7, sq-9 from white piece; equivalently white attacks sq from sq-9/sq-7? We'll check attackers: white pawn can be at sq-7 or sq-9 (one rank below) */
        if(r>0){
            if(f>0 && p->board[sq-9]==WP) return 1;
            if(f<7 && p->board[sq-7]==WP) return 1;
        }
    } else {
        if(r<7){
            if(f>0 && p->board[sq+7]==BP) return 1;
            if(f<7 && p->board[sq+9]==BP) return 1;
        }
    }
    /* knights */
    static const int kn[8]={-17,-15,-10,-6,6,10,15,17};
    for(int i=0;i<8;i++){
        int a=sq+kn[i];
        if(a<0||a>63) continue;
        if(abs((a%8)-f)>2) continue;
        int piece=(color==WHITE)?WN:BN;
        if(p->board[a]==piece) return 1;
    }
    /* king */
    for(int dr=-1;dr<=1;dr++) for(int df=-1;df<=1;df++){
        if(dr==0&&df==0) continue;
        int nr=r+dr, nf=f+df;
        if(nr<0||nr>7||nf<0||nf>7) continue;
        int a=nr*8+nf;
        int piece=(color==WHITE)?WK:BK;
        if(p->board[a]==piece) return 1;
    }
    /* sliders */
    /* bishops/queens diagonals */
    int bdirs[4][2]={{1,1},{1,-1},{-1,1},{-1,-1}};
    for(int d=0;d<4;d++){
        int cr=r, cf=f;
        while(1){
            cr+=bdirs[d][0]; cf+=bdirs[d][1];
            if(cr<0||cr>7||cf<0||cf>7) break;
            int a=cr*8+cf;
            int pc=p->board[a];
            if(pc==12){ continue; }
            if(pc==(color==WHITE?WB:BB) || pc==(color==WHITE?WQ:BQ)) return 1;
            break;
        }
    }
    /* rooks/queens orthog */
    int rdirs[4][2]={{1,0},{-1,0},{0,1},{0,-1}};
    for(int d=0;d<4;d++){
        int cr=r, cf=f;
        while(1){
            cr+=rdirs[d][0]; cf+=rdirs[d][1];
            if(cr<0||cr>7||cf<0||cf>7) break;
            int a=cr*8+cf;
            int pc=p->board[a];
            if(pc==12){ continue; }
            if(pc==(color==WHITE?WR:BR) || pc==(color==WHITE?WQ:BQ)) return 1;
            break;
        }
    }
    return 0;
}

int is_attacked(const Position *p, int sq, int color){
    Position tmp=*p;
    return attacked_by_piece(&tmp, sq, color);
}

int in_check(const Position *p, int color){
    /* find king */
    for(int s=0;s<64;s++){
        int pc=p->board[s];
        if(pc==(color==WHITE?WK:BK)){
            return is_attacked(p,s,color^1);
        }
    }
    return 0;
}

/* make/unmake */
static void apply_move(Position *p, const Move *m){
    int from=m->from, to=m->to;
    int piece=p->board[from];
    int captured = (m->flags==2)? (p->side==WHITE?BP:WP) : p->board[to];
    /* handle ep captured */
    if(m->flags==2){
        int capsq = (p->side==WHITE)? to-8 : to+8;
        p->bb[captured] &= ~(1ULL<<capsq);
        p->board[capsq]=12;
    }
    /* move piece */
    p->bb[piece] &= ~(1ULL<<from);
    p->bb[piece] |= (1ULL<<to);
    if(captured!=12 && m->flags!=2){
        int cpiece=captured;
        p->bb[cpiece] &= ~(1ULL<<to);
    }
    p->board[to]=piece;
    p->board[from]=12;
    /* promotion */
    if(m->prom>0){
        int newp;
        if(p->side==WHITE){
            newp = (m->prom==1?WQ:(m->prom==2?WR:(m->prom==3?WB:WN)));
        } else {
            newp = (m->prom==1?BQ:(m->prom==2?BR:(m->prom==3?BB:BN)));
        }
        /* remove pawn, add new */
        p->bb[piece] &= ~(1ULL<<to);
        p->bb[newp] |= (1ULL<<to);
        p->board[to]=newp;
    }
    /* castling rook */
    if(m->flags==3 || m->flags==4){
        /* king side or queen side */
        if(p->side==WHITE){
            if(m->to==6){ /* king side */
                p->bb[WR] &= ~(1ULL<<7); p->bb[WR] |= (1ULL<<5);
                p->board[7]=12; p->board[5]=WR;
            } else {
                p->bb[WR] &= ~(1ULL<<0); p->bb[WR] |= (1ULL<<3);
                p->board[0]=12; p->board[3]=WR;
            }
        } else {
            if(m->to==62){
                p->bb[BR] &= ~(1ULL<<63); p->bb[BR] |= (1ULL<<61);
                p->board[63]=12; p->board[61]=BR;
            } else {
                p->bb[BR] &= ~(1ULL<<56); p->bb[BR] |= (1ULL<<59);
                p->board[56]=12; p->board[59]=BR;
            }
        }
    }
    /* rights update: simplified: clear rights if king/rook moved */
    if(piece==WK){ p->castling &= ~3; }
    if(piece==BK){ p->castling &= ~12; }
    if(from==7 || to==7) p->castling &= ~1;
    if(from==0 || to==0) p->castling &= ~2;
    if(from==63 || to==63) p->castling &= ~4;
    if(from==56 || to==56) p->castling &= ~8;
    if(p->ep>=0) p->ep=-1; /* reset; set below? ep already set before move? We'll handle in make_move outer */
    rebuild_occ(p);
}

void make_move(Position *p, const Move *m){
    /* Save? caller handles undo via copy; we just update p */
    /* en passant target for next move */
    int ep_sq=-1;
    int piece=p->board[m->from];
    if(type_of(piece)==0 && abs(m->to-m->from)==16){
        ep_sq = (m->from+m->to)/2;
    }
    /* halfmove */
    int isCapture = (p->board[m->to]!=12) || (m->flags==2);
    int isPawn = (type_of(piece)==0);
    int saved_half=p->halfmove;
    /* store undo in move? move has captured */
    /* We'll just mutate; unmake not used in search? We need unmake. Implement via position copy in search instead. */
    apply_move(p,m);
    p->side ^= 1;
    if(isCapture||isPawn) p->halfmove=0; else p->halfmove=saved_half+1;
    p->ep=ep_sq;
    if(p->side==WHITE) p->fullmove++;
    /* store saved fields? We'll not support unmake; search will copy position */
}

void unmake_move(Position *p, const Move *m, const Position *saved){
    (void)m;
    *p=*saved;
}

/* generate pseudo moves */
static int addm(Move*moves,int*n,int from,int to,int prom,int flags){
    moves[*n].from=from; moves[*n].to=to; moves[*n].prom=prom; moves[*n].flags=flags; moves[*n].captured=0; (*n)++;
    return 1;
}

int generate_pseudo_moves(const Position *p, Move *moves){
    int n=0;
    int color=p->side;
    int opp=color^1;
    /* helper to iterate pieces by scanning board */
    for(int s=0;s<64;s++){
        int pc=p->board[s];
        if(pc==12 || (int)color_of(pc)!=color) continue;
        int t=type_of(pc);
        int r=s/8, f=s%8;
        if(t==0){ /* pawn */
            int dir = color==WHITE?1:-1;
            int promRank = color==WHITE?7:0;
            int startRank = color==WHITE?1:6;
            /* forward 1 */
            int to=s+dir*8;
            if(to>=0&&to<64 && (to/8)==(r+dir) && p->board[to]==12){
                if(to/8==promRank){ for(int pr=1;pr<=4;pr++) addm(moves,&n,s,to,pr,0); }
                else addm(moves,&n,s,to,0,0);
                /* forward 2 */
                if(r==startRank){
                    int to2=s+dir*16;
                    if(p->board[to2]==12) addm(moves,&n,s,to2,0,1);
                }
            }
            /* captures */
            for(int df=-1;df<=1;df+=2){
                int nf=f+df;
                if(nf<0||nf>7) continue;
                int to=s+dir*8+df;
                if(to<0||to>63) continue;
                if((to/8)!=(r+dir)) continue;
                if(abs((to%8)-f)!=1) continue;
                if(p->board[to]!=12 && color_of(p->board[to])==(Color)opp){
                    if(to/8==promRank){ for(int pr=1;pr<=4;pr++) addm(moves,&n,s,to,pr,0);}
                    else addm(moves,&n,s,to,0,0);
                }
                if(p->ep==to && p->board[to]==12){
                    addm(moves,&n,s,to,0,2);
                }
            }
        } else if(t==1){ /* knight */
            static const int off[8]={-17,-15,-10,-6,6,10,15,17};
            for(int i=0;i<8;i++){
                int to=s+off[i];
                if(to<0||to>63) continue;
                if(abs((to%8)-f)>2) continue;
                if(p->board[to]!=12 && color_of(p->board[to])==(Color)color) continue;
                addm(moves,&n,s,to,0,0);
            }
        } else if(t==5){ /* king */
            for(int dr=-1;dr<=1;dr++) for(int df=-1;df<=1;df++){
                if(dr==0&&df==0) continue;
                int nr=r+dr, nf=f+df;
                if(nr<0||nr>7||nf<0||nf>7) continue;
                int to=nr*8+nf;
                if(p->board[to]!=12 && color_of(p->board[to])==(Color)color) continue;
                addm(moves,&n,s,to,0,0);
            }
            /* castling */
            if(color==WHITE && s==4){
                if((p->castling&1) && p->board[5]==12 && p->board[6]==12 && !is_attacked(p,4,BLACK)&&!is_attacked(p,5,BLACK)&&!is_attacked(p,6,BLACK)) addm(moves,&n,4,6,0,3);
                if((p->castling&2) && p->board[3]==12 && p->board[2]==12 && !is_attacked(p,4,BLACK)&&!is_attacked(p,3,BLACK)&&!is_attacked(p,2,BLACK)) addm(moves,&n,4,2,0,4);
            } else if(color==BLACK && s==60){
                if((p->castling&4) && p->board[61]==12 && p->board[62]==12 && !is_attacked(p,60,WHITE)&&!is_attacked(p,61,WHITE)&&!is_attacked(p,62,WHITE)) addm(moves,&n,60,62,0,3);
                if((p->castling&8) && p->board[59]==12 && p->board[58]==12 && !is_attacked(p,60,WHITE)&&!is_attacked(p,59,WHITE)&&!is_attacked(p,58,WHITE)) addm(moves,&n,60,58,0,4);
            }
        } else if(t==2||t==4||t==3){ /* sliders */
            /* directions */
            int dirs[8][2];
            int ndir=0;
            if(t==2||t==4){ dirs[ndir][0]=1;dirs[ndir][1]=1;ndir++; dirs[ndir][0]=1;dirs[ndir][1]=-1;ndir++; dirs[ndir][0]=-1;dirs[ndir][1]=1;ndir++; dirs[ndir][0]=-1;dirs[ndir][1]=-1;ndir++; }
            if(t==3||t==4){ dirs[ndir][0]=1;dirs[ndir][1]=0;ndir++; dirs[ndir][0]=-1;dirs[ndir][1]=0;ndir++; dirs[ndir][0]=0;dirs[ndir][1]=1;ndir++; dirs[ndir][0]=0;dirs[ndir][1]=-1;ndir++; }
            for(int d=0;d<ndir;d++){
                int cr=r, cf=f;
                while(1){
                    cr+=dirs[d][0]; cf+=dirs[d][1];
                    if(cr<0||cr>7||cf<0||cf>7) break;
                    int to=cr*8+cf;
                    if(p->board[to]!=12){
                        if(color_of(p->board[to])!=(Color)color) addm(moves,&n,s,to,0,0);
                        break;
                    }
                    addm(moves,&n,s,to,0,0);
                }
            }
        }
    }
    return n;
}

int filter_legal(Position *p, Move *moves, int n){
    int out=0;
    for(int i=0;i<n;i++){
        Position saved=*p;
        make_move(p,&moves[i]);
        int kingSq=-1;
        for(int s=0;s<64;s++) if(p->board[s]==(saved.side==WHITE?WK:BK)){kingSq=s;break;}
        if(kingSq>=0 && !is_attacked(p, kingSq, p->side)){
            moves[out++]=moves[i];
        }
        *p=saved;
    }
    return out;
}

/* count legal moves helper */
int count_legal_moves(Position *p){
    Move m[256];
    int n=generate_pseudo_moves(p,m);
    n=filter_legal(p,m,n);
    return n;
}

/* make wrapper with undo via copy not needed */
/* filter uses saved side */
/* Note: make_move assumes captured info; unmake via caller copy */

/* Perft */
static long long perft_rec(Position p, int depth){
    if(depth==0) return 1;
    Move moves[512];
    int n=generate_pseudo_moves(&p,moves);
    n=filter_legal(&p,moves,n);
    if(depth==1) return n;
    long long nodes=0;
    for(int i=0;i<n;i++){
        Position saved=p;
        make_move(&p,&moves[i]);
        nodes += perft_rec(p, depth-1);
        p=saved;
    }
    return nodes;
}

long long perft(Position p, int depth){
    return perft_rec(p,depth);
}

static int sort_moves(Move *m, int n, const Position *p){
    for(int i=1;i<n;i++){
        Move key=m[i];
        int iscap = (p->board[key.to]!=12)?1:0;
        int j=i-1;
        while(j>=0){
            Move prev=m[j];
            int pc = (p->board[prev.to]!=12)?1:0;
            if(pc>=iscap) break;
            m[j+1]=m[j]; j--;
        }
        m[j+1]=key;
    }
    return n;
}

static long long negamax(Position *p, const float *weights, int depth, long long alpha, long long beta, clock_t start, long budget, int *timeout){
    if(*timeout) return 0;
    clock_t now=clock();
    if((now-start)>budget){ *timeout=1; return 0; }
    Move moves[512];
    int n=generate_pseudo_moves(p,moves);
    n=filter_legal(p,moves,n);
    if(n==0){
        if(in_check(p,p->side)) return -1000000 + (10-depth);
        return 0;
    }
    if(depth<=0){
        return eval_position(p, weights);
    }
    sort_moves(moves,n,p);
    long long best=-1000000000LL;
    for(int i=0;i<n;i++){
        Position saved=*p;
        make_move(p,&moves[i]);
        long long score = -negamax(p, weights, depth-1, -beta, -alpha, start, budget, timeout);
        *p=saved;
        if(*timeout) return 0;
        if(score>best) best=score;
        if(score>alpha) alpha=score;
        if(alpha>=beta) break;
    }
    return best;
}

Move find_best_move(Position *p, const float *weights, int time_ms, int max_depth){
    Move best; best.from=-1; best.to=-1; best.prom=0; best.flags=0;
    Move moves[512];
    int n=generate_pseudo_moves(p,moves);
    n=filter_legal(p,moves,n);
    if(n==0) return best;
    if(n==1){ best=moves[0]; return best; }
    best=moves[0];
    clock_t start=clock();
    long budget = (long)time_ms * CLOCKS_PER_SEC / 1000;
    int timeout=0;
    for(int depth=1; depth<=max_depth && !timeout; depth++){
        Move iterBest=moves[0];
        long long bestScore=-1000000000LL;
        long long alpha=-1000000000LL, beta=1000000000LL;
        int localTimeout=0;
        for(int i=0;i<n;i++){
            Position saved=*p;
            make_move(p,&moves[i]);
            long long score = -negamax(p, weights, depth-1, -beta, -alpha, start, budget, &localTimeout);
            *p=saved;
            if(localTimeout){ timeout=1; break; }
            if(score>bestScore){ bestScore=score; iterBest=moves[i]; }
            if(score>alpha) alpha=score;
        }
        if(!timeout){
            best=iterBest;
        }
    }
    return best;
}
