#include <mutex>
#include <sstream>
#include <tuple>
#include <thread>
#include <algorithm>
// #include "util_magic.cpp"
#include "Const.hpp"
#include "Evaluator.hpp"
#include "Functions.hpp"
#include "Move.hpp"
#include "GameState.hpp"
#include <vector>
#include "BestMoveFinder.hpp"
#include "TimeManagement.hpp"
#include "TablebaseProbe.hpp"
#include "wdlModel.hpp"
#include <set>
#include <iostream>
#include <cmath>
#include <cctype>

int nbThreads = 1;
bool DEBUG = false;
bool isdfrc = false;
using namespace std;


// Engine initialization was previously performed through ELF constructor
// attributes.  Android GUIs such as CET may start/inspect an engine process
// under a different loader/timeout path, so initialization is now explicit.
extern void init_lines();
extern void init_log_table();
extern void init_zobrs();
extern void init_consts_legalMove();
extern void init_forwards();
extern void init_tables();
extern void initThreatIndices();

//Main class for the game
const vector<string> benches = {
    "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
    "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 10",
    "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 11",
    "4rrk1/pp1n3p/3q2pQ/2p1pb2/2PP4/2P3N1/P2B2PP/4RRK1 b - - 7 19",
    "rq3rk1/ppp2ppp/1bnpN3/3N2B1/4P3/7P/PPPQ1PP1/2KR3R b - - 0 14",
    "r1bq1r1k/1pp1n1pp/1p1p4/4p2Q/4PpP1/1BNP4/PPP2P1P/3R1RK1 b - g3 0 14",
    "r3r1k1/2p2ppp/p1p1bn2/8/1q2P3/2NPQN2/PPP3PP/R4RK1 b - - 2 15",
    "r1bbk1nr/pp3p1p/2n5/1N4p1/2Np1B2/8/PPP2PPP/2KR1B1R w kq - 0 13",
    "r1bq1rk1/ppp1nppp/4n3/3p3Q/3P4/1BP1B3/PP1N2PP/R4RK1 w - - 1 16",
    "4r1k1/r1q2ppp/ppp2n2/4P3/5Rb1/1N1BQ3/PPP3PP/R5K1 w - - 1 17",
    "2rqkb1r/ppp2p2/2npb1p1/1N1Nn2p/2P1PP2/8/PP2B1PP/R1BQK2R b KQ - 0 11",
    "r1bq1r1k/b1p1npp1/p2p3p/1p6/3PP3/1B2NN2/PP3PPP/R2Q1RK1 w - - 1 16",
    "3r1rk1/p5pp/bpp1pp2/8/q1PP1P2/b3P3/P2NQRPP/1R2B1K1 b - - 6 22",
    "r1q2rk1/2p1bppp/2Pp4/p6b/Q1PNp3/4B3/PP1R1PPP/2K4R w - - 2 18",
    "4k2r/1pb2ppp/1p2p3/1R1p4/3P4/2r1PN2/P4PPP/1R4K1 b - - 3 22",
    "3q2k1/pb3p1p/4pbp1/2r5/PpN2N2/1P2P2P/5PP1/Q2R2K1 b - - 4 26",
    "6k1/6p1/6Pp/ppp5/3pn2P/1P3K2/1PP2P2/3N4 b - - 0 1",
    "3b4/5kp1/1p1p1p1p/pP1PpP1P/P1P1P3/3KN3/8/8 w - - 0 1",
    "2K5/p7/7P/5pR1/8/5k2/r7/8 w - - 4 3",
    "8/6pk/1p6/8/PP3p1p/5P2/4KP1q/3Q4 w - - 0 1",
    "7k/3p2pp/4q3/8/4Q3/5Kp1/P6b/8 w - - 0 1",
    "8/2p5/8/2kPKp1p/2p4P/2P5/3P4/8 w - - 0 1",
    "8/1p3pp1/7p/5P1P/5P1P/2k3P1/8/2K2P2 w - - 0 1",
    "8/pp2r1k1/2p1p3/3pP2p/1P1P1P1P/P5KR/8/8 w - - 93 90",
    "8/3p4/p1bk3p/Pp6/1Kp1PpPp/2P2P1P/2P5/5B2 b - - 0 1",
    "5k2/7R/4P2p/5K2/p1r2P1p/8/8/8 b - - 0 1",
    "6k1/6p1/P6p/r1N5/5p2/7P/1b3PP1/4R1K1 w - - 0 1",
    "1r3k2/4q3/2Pp3b/3Bp3/2Q2p2/1p1P2P1/1P2KP2/3N4 w - - 0 1",
    "6k1/4pp1p/3p2p1/P1pPb3/R7/1r2P1PP/3B1P2/6K1 w - - 0 1",
    "8/3p3B/5p2/5P2/p7/PP5b/k7/6K1 w - - 0 1",
    "5rk1/q6p/2p3bR/1pPp1rP1/1P1Pp3/P3B1Q1/1K3P2/R7 w - - 93 90",
    "4rrk1/1p1nq3/p7/2p1P1pp/3P2bp/3Q1Bn1/PPPB4/1K2R1NR w - - 40 21",
    "r3k2r/3nnpbp/q2pp1p1/p7/Pp1PPPP1/4BNN1/1P5P/R2Q1RK1 w kq - 0 16",
    "3Qb1k1/1r2ppb1/pN1n2q1/Pp1Pp1Pr/4P2p/4BP2/4B1R1/1R5K b - - 11 40",
    "4k3/3q1r2/1N2r1b1/3ppN2/2nPP3/1B1R2n1/2R1Q3/3K4 w - - 5 1",
    "8/8/8/8/5kp1/P7/8/1K1N4 w - - 0 1",
    "8/8/8/5N2/8/p7/8/2NK3k w - - 0 1",
    "8/3k4/8/8/8/4B3/4KB2/2B5 w - - 0 1",
    "8/8/1P6/5pr1/8/4R3/7k/2K5 w - - 0 1",
    "8/2p4P/kr6/6R1/8/8/1K6 w - - 0 1",
    "8/8/3P3k/8/1p6/8/1P6/1K3n2 b - - 0 1",
    "8/R7/2q5/8/6k1/8/1P5p/K6R w - - 0 124",
    "6k1/3b3r/1p1p4/p1n2p2/1PPNpP1q/P3Q1p1/1R1RB1P1/5K2 b - - 0 1",
    "r2r1n2/pp2bk2/2p1p2p/3q4/3PN1QP/2P3R1/P4PP1/5RK1 w - - 0 1",
    "8/8/8/8/8/6k1/6p1/6K1 w - - 0 1",
    "7k/7P/6K1/8/3B4/8/8/8 b - - 0 1"
};

class Chess{
public:
    GameState root;
    vector<Move> movesFromRoot;

    int w_time;
    int b_time;
    int binc=0;
    int winc=0;
};

int moveOverhead = 100;
const int alloted_space=64*hashMul;
Perft* doPerft = nullptr;
BestMoveFinder* bestMoveFinder = nullptr;
atomic<bool> stop_all=false;

const set<string> keywords = {
    "fen",
    "name",
    "value",
    "moves",
    "movetime",
    "nodes",
    "depth",
    "wtime",
    "btime",
    "winc",
    "binc",
    "startpos",
    "kiwipete",
    "perft",
    "nonbulk",
    "frc",
    "dfrc"
};

class Option{
public:
    string name;
    string type;
    string def;
    int maximum, minimum;

    Option(
        string _name,
        string _type,
        string _def="",
        int _min=-1,
        int _max=-1
    ):
        name(_name),
        type(_type),
        def(_def),
        maximum(_max),
        minimum(_min)
    {}

    void print(){
        printf(
            "option name %s type %s",
            name.c_str(),
            type.c_str()
        );

        if(def!="")
            printf(" default %s", def.c_str());

        if(minimum!=-1)
            printf(" min %d", minimum);

        if(maximum!=-1)
            printf(" max %d", maximum);

        printf("\n");
    }
};

const Option Options[] = {
    Option("Hash", "spin", "64", 1, 2147483647),
    Option("Move Overhead", "spin", "10", 0, 5000),
    Option("Clear Hash", "button"),
#ifndef HCE
    Option("nnueFile", "string", "embed"),
#endif
    Option("Threads", "spin", "1", 1, 512),
    Option("SyzygyPath", "string", "<empty>"),
    Option("SyzygyProbeDepth", "spin", "1", 1, 100),
    Option("SyzygyProbeLimit", "spin", "7", 0, 7),
    Option("Minimal", "check", "false"),
    Option("UCI_ShowWDL", "check", "true"),
    Option("UCI_Chess960", "check", "false")
};

void printUCIInfo(){
#ifdef VERSION
    string v=VERSION;
#else
    string v="test";
#endif

    if(!v.empty() && v[0]=='v')
        v=v.substr(1);

#ifdef HCE
    printf(
        "id name Prune HCE %s\n",
        v.c_str()
    );
#else
    printf(
        "id name Prune %s\n",
        v.c_str()
    );
#endif

    printf(
        "id author tgirolami09 & jbienvenue\n"
    );

    for(Option opt:Options)
        opt.print();

    printf("uciok\n");
    fflush(stdout);
}

pair<int, int> computeAllotedTime(
    int wtime,
    int btime,
    int binc,
    int winc,
    bool color,
    bool worthMoreTime
){
    int time = color == WHITE ? wtime : btime;
    int inc = color == WHITE ? winc : binc;

    int hardBound;

    if(worthMoreTime)
        hardBound = time/10+inc/2-moveOverhead;
    else
        hardBound = time/20+inc/2-moveOverhead;

    hardBound=min(
        hardBound,
        time-moveOverhead
    );

    int softBound=hardBound/2;

    return {softBound, hardBound};
}

bestMoveResponse goCommand(
    vector<pair<string, string>> args,
    Chess& state,
    bool verbose,
    bool& printmove
){
    if(!args.empty()){
        if(args[0].first=="perft"){
            printmove=false;

            big result;

            PositionSnapshot snap;
            snap.save(state.root);

            for(Move move:state.movesFromRoot)
                state.root.playPartialMove(move);

            state.root.print();

            if(args.size()>1 && args[1].first=="nonbulk")
                result=doPerft->perft<false>(
                    state.root,
                    stoi(args[0].second)
                );
            else
                result=doPerft->perft<true>(
                    state.root,
                    stoi(args[0].second)
                );

            snap.restore(state.root);

            printf(
                "Nodes searched: %" PRId64 "\n",
                result
            );

            return make_tuple(
                nullMove,
                nullMove,
                0,
                vector<depthInfo>(0)
            );

        }else if(
            args[0].first=="btime" ||
            args[0].first=="wtime"
        ){
            int btime=0;
            int wtime=0;
            int winc=0;
            int binc=0;

            for(pair<string,string> arg:args){
                if(arg.first=="btime")
                    btime=stoi(arg.second);

                else if(arg.first=="wtime")
                    wtime=stoi(arg.second);

                else if(arg.first=="binc")
                    binc=stoi(arg.second);

                else if(arg.first=="winc")
                    winc=stoi(arg.second);
            }

            bool color=
                state.root.friendlyColor() ^
                (state.movesFromRoot.size()&1);

            TM tm(
                moveOverhead,
                wtime,
                btime,
                binc,
                winc,
                color
            );

            return bestMoveFinder->bestMove<0>(
                state.root,
                tm,
                state.movesFromRoot
            );

        }else if(args[0].first=="movetime"){
            int movetime=stoi(args[0].second);

            return bestMoveFinder->bestMove<0>(
                state.root,
                TM(movetime,movetime),
                state.movesFromRoot,
                verbose
            );

        }else if(args[0].first=="nodes"){
            int nodes=stoi(args[0].second);

            return bestMoveFinder->bestMove<1>(
                state.root,
                TM(nodes,nodes),
                state.movesFromRoot,
                verbose
            );

        }else if(args[0].first=="depth"){
            int depth=stoi(args[0].second);

            return bestMoveFinder->bestMove<2>(
                state.root,
                TM(depth,depth),
                state.movesFromRoot,
                verbose
            );
        }
    }

    return bestMoveFinder->bestMove<2>(
        state.root,
        TM(200,200),
        state.movesFromRoot,
        verbose
    );
}

void manageSearch(){
    auto state=make_unique<Chess>();

    state->root.fromFen(
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"
    );

    Move lastMove=nullMove;
    auto ieval=make_unique<IncrementalEvaluator>();

    // Spaghet-style UCI architecture: one owner for stdin/stdout and a
    // background search thread. No command queue or input thread is needed.
    thread searchThread;

    auto waitForSearch=[&](){
        if(searchThread.joinable())
            searchThread.join();
    };

    while(!stop_all){
        string com;

        if(!getline(cin,com)){
            bestMoveFinder->running=false;
            stop_all=true;
            break;
        }

        while(!com.empty() &&
              (com.back()=='\r' || com.back()=='\n' ||
               com.back()==' ' || com.back()=='\t'))
            com.pop_back();

        size_t first=com.find_first_not_of(" \t");
        if(first!=string::npos && first>0)
            com.erase(0,first);

        if(com.empty())
            continue;

        if(com=="uci"){
            printUCIInfo();
            continue;
        }

        if(com=="isready"){
            printf("readyok\n");
            fflush(stdout);
            continue;
        }

        if(com=="stop"){
            bestMoveFinder->running=false;
            waitForSearch();
            continue;
        }

        if(com=="quit"){
            bestMoveFinder->running=false;
            waitForSearch();
            stop_all=true;
            break;
        }

        istringstream stream(com);

        string command;
        stream>>command;

        vector<pair<string,string>> parsed;
        string token;

        /*
         * Standard UCI "go" parser.
         *
         * Unknown go parameters are ignored instead of being interpreted
         * as part of another parameter.
         */
        if(command=="go"){

            while(stream>>token){

                if(
                    token=="wtime" ||
                    token=="btime" ||
                    token=="winc" ||
                    token=="binc" ||
                    token=="movetime" ||
                    token=="nodes" ||
                    token=="depth" ||
                    token=="mate"
                ){
                    string value;

                    if(stream>>value)
                        parsed.push_back({
                            token,
                            value
                        });

                }else if(token=="searchmoves"){

                    string moves;
                    string next;

                    while(stream>>next){
                        if(!moves.empty())
                            moves+=' ';

                        moves+=next;
                    }

                    parsed.push_back({
                        "searchmoves",
                        moves
                    });

                }else if(
                    token=="infinite" ||
                    token=="ponder"
                ){
                    parsed.push_back({
                        token,
                        ""
                    });
                }
            }

        /*
         * UCI setoption parser.
         *
         * Both option names and values may contain spaces.
         */
        }else if(command=="setoption"){

            vector<string> parts;

            while(stream>>token)
                parts.push_back(token);

            size_t namePos=parts.size();
            size_t valuePos=parts.size();

            for(size_t i=0;i<parts.size();++i){

                if(
                    parts[i]=="name" &&
                    namePos==parts.size()
                )
                    namePos=i;

                else if(
                    parts[i]=="value" &&
                    valuePos==parts.size()
                )
                    valuePos=i;
            }

            if(namePos<parts.size()){

                size_t nameEnd=
                    (
                        valuePos<parts.size() &&
                        valuePos>namePos
                    )
                    ? valuePos
                    : parts.size();

                string name;
                string value;

                for(
                    size_t i=namePos+1;
                    i<nameEnd;
                    ++i
                ){
                    if(!name.empty())
                        name+=' ';

                    name+=parts[i];
                }

                for(
                    size_t i=
                        valuePos<parts.size()
                        ? valuePos+1
                        : parts.size();
                    i<parts.size();
                    ++i
                ){
                    if(!value.empty())
                        value+=' ';

                    value+=parts[i];
                }

                parsed.push_back({
                    "name",
                    name
                });

                parsed.push_back({
                    "value",
                    value
                });
            }

        /*
         * Standard UCI position parser.
         */
        }else if(command=="position"){

            vector<string> parts;

            while(stream>>token)
                parts.push_back(token);

            if(
                !parts.empty() &&
                parts[0]=="startpos"
            ){
                parsed.push_back({
                    "startpos",
                    ""
                });

                size_t mp=parts.size();

                for(
                    size_t i=1;
                    i<parts.size();
                    ++i
                ){
                    if(parts[i]=="moves"){
                        mp=i;
                        break;
                    }
                }

                if(mp<parts.size()){

                    string moves;

                    for(
                        size_t i=mp+1;
                        i<parts.size();
                        ++i
                    ){
                        if(!moves.empty())
                            moves+=' ';

                        moves+=parts[i];
                    }

                    parsed.push_back({
                        "moves",
                        moves
                    });
                }

            }else if(
                !parts.empty() &&
                parts[0]=="fen"
            ){

                size_t mp=parts.size();

                for(
                    size_t i=1;
                    i<parts.size();
                    ++i
                ){
                    if(parts[i]=="moves"){
                        mp=i;
                        break;
                    }
                }

                string fen;

                for(
                    size_t i=1;
                    i<mp;
                    ++i
                ){
                    if(!fen.empty())
                        fen+=' ';

                    fen+=parts[i];
                }

                parsed.push_back({
                    "fen",
                    fen
                });

                if(mp<parts.size()){

                    string moves;

                    for(
                        size_t i=mp+1;
                        i<parts.size();
                        ++i
                    ){
                        if(!moves.empty())
                            moves+=' ';

                        moves+=parts[i];
                    }

                    parsed.push_back({
                        "moves",
                        moves
                    });
                }
            }

        }else{

            /*
             * Preserve the original parser for Prune's private/debug
             * commands such as bench, eval, runSEE, etc.
             */
            string word;
            string keyword="";
            string value="";

            while(stream>>word){

                if(keywords.count(word)){

                    if(value.empty()){

                        if(!keyword.empty())
                            parsed.push_back({
                                keyword,
                                ""
                            });

                    }else{

                        value.pop_back();

                        parsed.push_back({
                            keyword,
                            value
                        });
                    }

                    value="";
                    keyword=word;

                }else{

                    value+=word+" ";
                }
            }

            if(!value.empty())
                value.pop_back();

            if(
                !keyword.empty() ||
                !value.empty()
            ){
                parsed.push_back({
                    keyword,
                    value
                });
            }
        }

                    if(command=="position" ||
                       command=="setoption" ||
                       command=="ucinewgame" ||
                       command=="bench" ||
                       command=="eval" ||
                       command=="raweval" ||
                       command=="runQ" ||
                       command=="perft" ||
                       command=="arch"){
                        bestMoveFinder->running=false;
                        waitForSearch();
                    }

                    if(command=="runQ"){

            bestMoveFinder->testQuiescenceSearch(
                state->root
            );

        }else if(command=="eval"){

            PositionSnapshot snap;
            snap.save(state->root);

            for(Move move:state->movesFromRoot)
                state->root.playPartialMove(move);

            ieval->init(state->root);

            int overall_eval=
                ieval->getRaw(
                    state->root.friendlyColor()
                );

            for(int r=7;r>=0;r--){

                pair<char,int> evals[8];

                for(int c=0;c<8;c++){

                    int square=(r<<3)|c;

                    int piece=
                        state->root.getfullPiece(square);

                    if(type(piece)!=SPACE){

                        ieval->changePiece2<-1,true>(
                            square,
                            type(piece),
                            color(piece)
                        );

                        char repr=
                            id_to_piece[type(piece)];

                        int derived=
                            overall_eval-
                            ieval->getRaw(
                                state->root.friendlyColor()
                            );

                        if(color(piece)==WHITE)
                            repr=toupper(repr);

                        evals[7-c]={
                            repr,
                            derived
                        };

                        ieval->changePiece2<1,false>(
                            square,
                            type(piece),
                            color(piece)
                        );

                    }else{

                        evals[7-c]={
                            ' ',
                            0
                        };
                    }
                }

                for(int i=0;i<8;i++)
                    printf("+-------");

                printf("+\n");

                for(int i=0;i<8;i++)
                    printf(
                        "|   %c   ",
                        evals[i].first
                    );

                printf("|\n");

                for(int i=0;i<8;i++){

                    if(evals[i].first==' ')
                        printf("|       ");

                    else{

                        if(abs(evals[i].second)>10*100)

                            printf(
                                "| %+2.1f ",
                                evals[i].second/100.0
                            );

                        else

                            printf(
                                "| %+1.2f ",
                                evals[i].second/100.0
                            );
                    }
                }

                printf("|\n");
            }

            for(int i=0;i<8;i++)
                printf("+-------");

            printf("+\n");

            snap.restore(state->root);

            printf(
                "static evaluation: %d cp\n",
                overall_eval
            );

        }else if(command=="raweval"){

            PositionSnapshot snap;
            snap.save(state->root);

            for(Move move:state->movesFromRoot)
                state->root.playPartialMove(move);

            ieval->init(state->root);

            int overall_eval=
                ieval->getRaw(
                    state->root.friendlyColor()
                );

            snap.restore(state->root);

            printf(
                "%d cp\n",
                overall_eval
            );

        }else if(command=="ucinewgame"){

            bestMoveFinder->clear();
            lastMove=nullMove;

        }else if(command=="version"){

        #ifdef COMMIT
            printf(
                "version: %s\n",
                COMMIT
            );
        #else
            printf("version: test\n");
        #endif

        }else if(command=="bench"){

            int sumNodes[maxDepth+1];
            int sumNPS=0;
            int sumTime=0;

            int histDepth[maxDepth+1];
            int sumSelDepth[maxDepth+1];

            for(int i=0;i<=maxDepth;i++){
                sumNodes[i]=0;
                histDepth[i]=0;
                sumSelDepth[i]=0;
            }

            int maxDepthAttain=0;

            vector<pair<int,int>> Scores;

            if(parsed.size()==0)
                parsed={{"depth","10"}};

            for(
                unsigned idFen=0;
                idFen<benches.size();
                idFen++
            ){

                if(DEBUG){

                    printf(
                        "\rposition %d/%d",
                        idFen,
                        (int)benches.size()
                    );

                    fflush(stdout);
                }

                auto testState=
                    make_unique<Chess>();

                testState->movesFromRoot={};

                testState->root.fromFen(
                    benches[idFen]
                );

                bestMoveFinder->clear();

                bool _;

                bestMoveResponse res=
                    goCommand(
                        parsed,
                        *testState,
                        false,
                        _
                    );

                vector<depthInfo> infos=
                    get<3>(res);

                if(DEBUG)

                    printf(
                        "fen: %s score: %d nodes: %" PRId64 "\n",
                        testState->root.toFen().c_str(),
                        get<2>(res),
                        infos.empty()
                            ? 0
                            : infos.back().node
                    );

                for(depthInfo info:infos){

                    sumNodes[info.depth]+=info.node;
                    histDepth[info.depth]++;
                    sumSelDepth[info.depth]+=info.seldepth;

                    if(info.depth==maxDepth)
                        printf(
                            "\n%s\n",
                            benches[idFen].c_str()
                        );

                    maxDepthAttain=
                        max(
                            maxDepthAttain,
                            info.depth
                        );
                }

                if(infos.size()){

                    depthInfo lastInfo=
                        infos[infos.size()-1];

                    sumNPS+=lastInfo.node;
                    sumTime+=lastInfo.time;

                    Scores.push_back({
                        lastInfo.depth,
                        lastInfo.node
                    });
                }
            }

            if(DEBUG){

                printf(
                    "\rposition %d/%d\n",
                    (int)benches.size(),
                    (int)benches.size()
                );

                printf("depth\t");

                for(
                    int i=0;
                    i<=maxDepthAttain;
                    i++
                )
                    printf("\t%d",i);

                printf("\nnodes\t");

                for(
                    int i=0;
                    i<=maxDepthAttain;
                    i++
                ){

                    if(histDepth[i])

                        printf(
                            "\t%d",
                            sumNodes[i]/histDepth[i]
                        );

                    else
                        printf("\t0");
                }

                printf("\nseldepth");

                for(
                    int i=0;
                    i<=maxDepthAttain;
                    i++
                ){

                    if(histDepth[i])

                        printf(
                            "\t%d",
                            sumSelDepth[i]/histDepth[i]
                        );

                    else
                        printf("\t0");
                }

                printf(
                    "\n%.0fnps\n",
                    sumNPS*1000.0/sumTime
                );
            }

            sort(
                Scores.begin(),
                Scores.end()
            );

            int size=Scores.size();

            pair<int,big> scoreThird={0.0,0.0};
            pair<int,big> scoreAll={0.0,0.0};

            for(int i=0;i<size;i++){

                pair<int,big> locScore={
                    Scores[i].first,
                    Scores[i].second
                };

                scoreAll.first+=locScore.first;
                scoreAll.second+=locScore.second;

                if(
                    i>=size/3 &&
                    i<size*2/3
                ){

                    scoreThird.first+=
                        locScore.first;

                    scoreThird.second+=
                        locScore.second;
                }
            }

            if(DEBUG)

                printf(
                    "search score: (%d %" PRId64 ") (%d %" PRId64 ")\n",
                    scoreThird.first,
                    scoreThird.second,
                    scoreAll.first,
                    scoreAll.second
                );

            printf(
                "%" PRId64 " nodes %.0f nps\n",
                scoreAll.second,
                sumNPS*1000.0/sumTime
            );

        #ifdef DEBUG_MACRO

            diffsStat.print(
                "corrhist applied"
            );

            printf(
                "nmp in allnodes stats : %d/%d = %.2f%%\n",
                nmpVerifPassAllNode,
                nmpVerifAllNode,
                nmpVerifPassAllNode*100.0/nmpVerifAllNode
            );

            printf(
                "nmp in curnodes stats : %d/%d = %.2f%%\n",
                nmpVerifPassCutNode,
                nmpVerifCutNode,
                nmpVerifPassCutNode*100.0/nmpVerifCutNode
            );

            quiethistPreStat.print("quiethistPre");
            capthistPreStat.print("capthistPre");
            quiethistPostStat.print("quiethistPost");
            capthistPostStat.print("capthistPost");
            TIupdateAddStat.print("TIupdateAdd");
            TIupdateRemStat.print("TIupdateRem");
            TIupdateTotStat.print("TIupdateTot");
            TIupdateDiffStat.print("TIupdateDiff");
            matScalingStats.print("matScaling");

        #endif

        }else if(command=="arch"){

        #ifdef __AVX512F__

            printf("arch: AVX512\n");

        #elif defined(__AVX2__)

            printf("arch: AVX2\n");

        #elif defined(__AVX__)

            printf("arch: AVX\n");

        #elif defined(__SSE2__)

            printf("arch: SSE2\n");

        #else

            printf("arch: unknow\n");

        #endif

        }else if(command=="quit"){

            stop_all=true;

        }else if(command=="position"){

            state->movesFromRoot.clear();

            for(
                unsigned long iarg=0;
                iarg<parsed.size();
                iarg++
            ){

                auto arg=parsed[iarg];

                if(arg.first=="fen"){

                    state->root.fromFen(
                        arg.second
                    );
                }

                if(arg.first=="startpos"){

                    state->root.fromFen(
                        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"
                    );

                }else if(arg.first=="kiwipete"){

                    state->root.fromFen(
                        "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - "
                    );

                }else if(arg.first=="moves"){

                    istringstream moves(
                        arg.second
                    );

                    string curMove;

                    while(moves>>curMove){

                        Move move;

                        move.from_uci(curMove);

                        state->movesFromRoot.push_back(
                            move
                        );
                    }

                }else if(arg.first=="frc"){

                    state->root.setDFRC(
                        stoi(arg.second),
                        stoi(arg.second)
                    );

                }else if(arg.first=="dfrc"){

                    istringstream ids(
                        arg.second
                    );

                    int idwhite;
                    int idblack;

                    ids>>idwhite>>idblack;

                    state->root.setDFRC(
                        idwhite,
                        idblack
                    );
                }
            }

        }else if(command=="go"){

                        // Keep the UCI loop responsive while search is running, as in
                        // Spaghet. STOP/POSITION/QUIT are handled by the same loop.
                        waitForSearch();

                        bool printmove=true;

                        searchThread=thread([&, parsed, printmove]() mutable {
                            bestMoveResponse res=
                                goCommand(
                                    parsed,
                                    *state,
                                    true,
                                    printmove
                                );

                            Move bm=get<0>(res);
                            Move ponder=get<1>(res);

                            if(printmove && !stop_all){
                                if(ponder.moveInfo==nullMove.moveInfo){
                                    printf(
                                        "bestmove %s\n",
                                        bm.moveInfo==nullMove.moveInfo
                                            ? "0000"
                                            : bm.to_str().c_str()
                                    );
                                }else{
                                    printf(
                                        "bestmove %s ponder %s\n",
                                        bm.to_str().c_str(),
                                        ponder.to_str().c_str()
                                    );
                                }

                                fflush(stdout);
                                lastMove=ponder;
                                bestMoveFinder->aging();
                            }
                        });

                    }else if(command=="uci"){

            printUCIInfo();

        }else if(command=="setoption"){

            string optionName;
            string optionValue;

            for(const auto& item:parsed){

                if(item.first=="name")
                    optionName=item.second;

                else if(item.first=="value")
                    optionValue=item.second;
            }

            string opt=optionName;

            transform(
                opt.begin(),
                opt.end(),
                opt.begin(),
                [](unsigned char c){
                    return (char)tolower(c);
                }
            );

            try{

                if(
                    opt=="hash" &&
                    !optionValue.empty()
                ){

                    bestMoveFinder->reinit(
                        stoull(optionValue)*hashMul
                    );

                }else if(
                    opt=="move overhead" &&
                    !optionValue.empty()
                ){

                    moveOverhead=stoi(
                        optionValue
                    );

                }else if(opt=="clear hash"){

                    bestMoveFinder->clear();

                }else if(
                    opt=="threads" &&
                    !optionValue.empty()
                ){

                    int newT=max(
                        1,
                        stoi(optionValue)
                    );

                    bestMoveFinder->setThreads(
                        newT
                    );

                    nbThreads=newT;

                }else if(opt=="syzygypath"){

                    tbProbe.init(
                        optionValue
                    );

                    if(tbProbe.isAvailable())

                        printf(
                            "info string Syzygy tablebases loaded, max %d pieces\n",
                            tbProbe.maxPieces()
                        );

                    else

                        printf(
                            "info string Could not find any tablebases at '%s'\n",
                            optionValue.c_str()
                        );

                }else if(
                    opt=="syzygyprobedepth" &&
                    !optionValue.empty()
                ){

                    tbProbe.setProbeDepth(
                        stoi(optionValue)
                    );

                }else if(
                    opt=="syzygyprobelimit" &&
                    !optionValue.empty()
                ){

                    tbProbe.setProbeLimit(
                        stoi(optionValue)
                    );

                }else if(opt=="minimal"){

                    bestMoveFinder->minimal=
                        (
                            optionValue=="true" ||
                            optionValue=="1"
                        );

                }else if(opt=="uci_showwdl"){

                    WDLmodel::enabled=
                        (
                            optionValue=="true" ||
                            optionValue=="1"
                        );

                }else if(opt=="uci_chess960"){

                    isdfrc=
                        (
                            optionValue=="true" ||
                            optionValue=="1"
                        );
                }

            }catch(const exception&){

                printf(
                    "info string Ignoring invalid option '%s' value '%s'\n",
                    optionName.c_str(),
                    optionValue.c_str()
                );
            }

        }else if(command=="runSEE"){

            PositionSnapshot snap;

            snap.save(state->root);

            for(Move move:state->movesFromRoot)
                state->root.playPartialMove(move);

            istringstream moves(
                parsed[0].second
            );

            string curMove;

            bool isExact=true;

            while(moves>>curMove){

                if(curMove=="ge"){
                    isExact=false;
                    continue;
                }

                Move move;

                move.from_uci(curMove);

                int piece=
                    type(
                        state->root.getfullPiece(
                            move.from()
                        )
                    );

                int capture=
                    type(
                        state->root.getfullPiece(
                            move.to()
                        )
                    );

                if(
                    capture==SPACE &&
                    piece==PAWN &&
                    abs(move.from()-move.to())!=8 &&
                    abs(move.from()-move.to())!=16
                ){

                    move.setFlag(
                        Move::fep
                    );

                    capture=0;
                }

                int res;

                const int value_pieces[7]={
                    100,
                    300,
                    300,
                    500,
                    900,
                    100000,
                    0
                };

                if(isExact){

                    res=
                        -fastSEE(
                            move,
                            state->root,
                            value_pieces
                        );

                    if(capture==SPACE)
                        res+=value_pieces[capture];

                }else{

                    res=
                        see_ge(
                            0,
                            move,
                            state->root,
                            value_pieces
                        );
                }

                printf(
                    "%s : %d\n",
                    move.to_str().c_str(),
                    res
                );
            }

            snap.restore(
                state->root
            );

        }else if(command=="debug"){

            if(parsed[0].second=="off")
                DEBUG=false;

            else
                DEBUG=true;

        }else if(command=="print"){

            PositionSnapshot snap;

            snap.save(state->root);

            for(Move move:state->movesFromRoot)
                state->root.playPartialMove(move);

            state->root.print();

            snap.restore(state->root);

        }else if(command=="stats"){

        #ifdef DEBUG_MACRO

            diffsStat.print(
                "corrhist applied"
            );

            printf(
                "nmp in allnodes stats : %d/%d = %.2f%%\n",
                nmpVerifPassAllNode,
                nmpVerifAllNode,
                nmpVerifPassAllNode*100.0/nmpVerifAllNode
            );

            printf(
                "nmp in curnodes stats : %d/%d = %.2f%%\n",
                nmpVerifPassCutNode,
                nmpVerifCutNode,
                nmpVerifPassCutNode*100.0/nmpVerifCutNode
            );

            quiethistPreStat.print(
                "quiethistPre"
            );

            capthistPreStat.print(
                "capthistPre"
            );

            quiethistPostStat.print(
                "quiethistPost"
            );

            capthistPostStat.print(
                "capthistPost"
            );

            TIupdateAddStat.print(
                "TIupdateAdd"
            );

            TIupdateRemStat.print(
                "TIupdateRem"
            );

            TIupdateTotStat.print(
                "TIupdateTot"
            );

            TIupdateDiffStat.print(
                "TIupdateDiff"
            );

            matScalingStats.print(
                "matScaling"
            );

        #endif

        }else if(command=="fen"){

            PositionSnapshot snap;

            snap.save(state->root);

            for(Move move:state->movesFromRoot)
                state->root.playPartialMove(move);

            printf(
                "%s\n",
                state->root.toFen().c_str()
            );

            snap.restore(state->root);
        }

    }

    bestMoveFinder->running=false;
    waitForSearch();
}

int main(int argc,char** argv){
    (void)argc;
    (void)argv;

    // Keep the UCI pipe immediately responsive on Android.
    setvbuf(stdin, nullptr, _IONBF, 0);
    setvbuf(stdout, nullptr, _IONBF, 0);

    // Explicitly perform the same precomputed-table work that Prune used
    // to perform through ELF constructors, in dependency order.
    init_lines();
    init_log_table();
    init_zobrs();
    init_consts_legalMove();
    init_forwards();
    init_tables();
    initThreatIndices();

    // Heavy engine objects are created only after main() has started.
    Perft perft;
    BestMoveFinder finder(alloted_space);
    doPerft = &perft;
    bestMoveFinder = &finder;

    // Enter the single-owner UCI loop.  Search itself uses Prune's existing
    // BestMoveFinder/SMP implementation; only process communication changed.
    manageSearch();

    bestMoveFinder = nullptr;
    doPerft = nullptr;
    clear_table();
    return 0;
}
