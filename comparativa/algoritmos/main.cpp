#include "utils.hpp"
#include <iostream>
#include <iomanip>
#include <map>
#include <sstream>

using namespace std;

static map<string,string> parse_args(int argc, char** argv){
    map<string,string> A;
    for(int i=1;i<argc;i++){
        string a=argv[i];
        auto p=a.find('=');
        if(p==string::npos) A[a]="1";
        else A[a.substr(0,p)] = a.substr(p+1);
    }
    return A;
}

static void print_usage(){
    cerr <<
    "USO:\n"
    "  Generar grid:\n"
    "    --mode=gen_grid --rows=R --cols=C --out=graph.bin [--diag8] [--wmin=1] [--wmax=1] [--seed=42]\n"
    "  Generar ER (Erdos-Renyi):\n"
    "    --mode=gen_er --N=N --M=M --out=graph.bin [--undirected] [--wmin=1] [--wmax=10] [--seed=42]\n"
    "  Ejecutar:\n"
    "    --mode=run --in=graph.bin --s=S --t=T --algos=dijkstra,astar,bmssp,dstar [--B=1e9]\n"
    "\n"
    "Salida (CSV): algo,N,M,s,t,time_ms,path_len\n";
}

int main(int argc, char** argv){
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    if(argc < 2){
        print_usage();
        return 1;
    }

    auto A = parse_args(argc, argv);
    string mode = A.count("--mode") ? A["--mode"] : "";

    try{
        if(mode == "gen_grid"){
            if(!A.count("--rows") || !A.count("--cols") || !A.count("--out")){
                print_usage(); return 1;
            }
            int rows = stoi(A["--rows"]);
            int cols = stoi(A["--cols"]);
            bool diag8 = A.count("--diag8")>0;
            float wmin = A.count("--wmin")? stof(A["--wmin"]) : 1.0f;
            float wmax = A.count("--wmax")? stof(A["--wmax"]) : 1.0f;
            unsigned seed = A.count("--seed")? (unsigned)stoul(A["--seed"]) : 42u;
            string out = A["--out"];

            CSR g = gen_grid(rows, cols, diag8, wmin, wmax, seed);
            save_csr_bin(g, out);
            cerr << "OK grid " << rows << "x" << cols
                 << " diag8=" << (diag8?"yes":"no")
                 << " -> " << out << "\n";
            return 0;

        } else if(mode == "gen_er"){
            if(!A.count("--N") || !A.count("--M") || !A.count("--out")){
                print_usage(); return 1;
            }
            int N = stoi(A["--N"]);
            long long M = stoll(A["--M"]);
            bool directed = !(A.count("--undirected")>0);
            float wmin = A.count("--wmin")? stof(A["--wmin"]) : 1.0f;
            float wmax = A.count("--wmax")? stof(A["--wmax"]) : 10.0f;
            unsigned seed = A.count("--seed")? (unsigned)stoul(A["--seed"]) : 42u;
            string out = A["--out"];

            CSR g = gen_er(N, M, wmin, wmax, seed, directed);
            save_csr_bin(g, out);
            cerr << "OK ER N=" << N << " M=" << M
                 << " directed=" << (directed?"yes":"no")
                 << " -> " << out << "\n";
            return 0;

        } else if(mode == "run"){
            if(!A.count("--in") || !A.count("--s") || !A.count("--t")){
                print_usage(); return 1;
            }
            string in = A["--in"];
            int s = stoi(A["--s"]);
            int t = stoi(A["--t"]);
            float B = A.count("--B") ? stof(A["--B"]) : 1e30f;

            // Lista de algoritmos (1 o varios separados por coma)
            vector<string> algos;
            if(A.count("--algos")){
                string list = A["--algos"];
                stringstream ss(list); string x;
                while(getline(ss,x,',')) if(!x.empty()) algos.push_back(x);
            } else {
                algos = {"dijkstra"}; // por defecto
            }

            CSR g = load_csr_bin(in);
            cout << "algo,N,M,s,t,time_ms,path_len\n";

            for(const string& algo : algos){
                vector<int> parent(g.N, -1);
                Timer T; T.start();
                bool ok = false;

                if(algo=="dijkstra"){
                    ok = dijkstra_run(g, s, t, parent);
                } else if(algo=="astar"){
                    ok = astar_run(g, s, t, parent);
                } else if(algo=="bmssp"){
                    ok = bmssp_run(g, s, t, B, parent);
                } else if(algo=="dstar"){
                    ok = dstar_lite_run_static(g, s, t, parent);
                } else {
                    cerr << "Algoritmo desconocido: " << algo << "\n";
                    continue;
                }

                double ms = T.ms();
                int plen = ok ? path_length(s, t, parent) : 0;

                cout << algo << ","
                     << g.N << ","
                     << g.M << ","
                     << s << ","
                     << t << ","
                     << fixed << setprecision(3) << ms << ","
                     << plen << "\n";
            }
            return 0;

        } else {
            print_usage();
            return 1;
        }
    } catch(const exception& e){
        cerr << "Error: " << e.what() << "\n";
        return 2;
    }
}
