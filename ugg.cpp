#include <cassert>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <random>
#include <string>

#include "lib/common/error.hpp"
#include "lib/common/filesystem.hpp"
#include "lib/common/quantity.hpp"
#include "lib/cxxopts.hpp"
#include "lib/cuckohash.hpp"

using namespace common;
using namespace std;

// data structures
using graph_raw_t = pair</* vertices */ vector<uint64_t>, /* edges */ vector<pair<uint64_t, uint64_t>>>;
struct Edge {
    const uint64_t m_source;
    const uint64_t m_destination;

    Edge(uint64_t source, uint64_t destination) : m_source(min(source, destination)), m_destination(max(source,destination)){ }

    // Check whether the two edges are equal
    bool operator==(const Edge& e) const noexcept { return e.m_source == m_source && e.m_destination == m_destination; }
    bool operator!=(const Edge& e) const noexcept { return !(*this == e); }
};
namespace std {
template<> struct hash<::Edge>{ // hash function
    size_t operator()(const Edge& e) const { return hash<uint64_t>{}(e.m_source) ^ hash<uint64_t>{}(e.m_destination); }
};
} // namespace std

// globals
double g_exp_factor_vertex_id; // the maximum vertex id to assign to the nodes in the graph
uint64_t g_num_edges; // the total number of edges to create
uint64_t g_num_vertices; // number of vertices to create
string g_output_prefix; // path where to save the generated files
uint64_t g_seed = std::random_device{}(); // the seed to use for the random generator

// function prototypes
static void parse_command_line_arguments(int argc, char* argv[]);
static vector<Edge> make_edges();
static vector<uint64_t> make_vertices();
static void save(const vector<uint64_t>& vertices, const vector<Edge>& edges);

// entry point
int main(int argc, char* argv[]) {
    try {
        parse_command_line_arguments(argc, argv);

        cout << "Generating the list of edges ... " << endl;
        vector<Edge> edges = make_edges();
        cout << "Generating the list of vertices ..." << endl;
        vector<uint64_t> vertices = make_vertices();

        save(vertices, edges);
    } catch (common::Error& e){
        cerr << e << endl;
        cerr << "Type `" << argv[0] << " --help' to check how to run the program\n";
        cerr << "Program terminated" << endl;
        return 1;
    }

    cout << "Done\n";

    return 0;
}

static vector<Edge> make_edges(){
    cuckoohash_map<Edge, bool> edges_created;

    int num_threads = std::thread::hardware_concurrency();
    auto create_edges = [=, &edges_created](int thread_id){
        std::mt19937_64 random_generator { g_seed + thread_id };
        uniform_int_distribution<uint64_t> uniform_distribution {0, g_num_vertices -1}; // [a, b]
        const uint64_t num_edges_to_create = g_num_edges / num_threads + (thread_id < (g_num_edges % num_threads));

        uint64_t num_edges_created_insofar = 0;
        while(num_edges_created_insofar < num_edges_to_create){
            Edge edge { uniform_distribution(random_generator), uniform_distribution(random_generator) };
            if(edge.m_source == edge.m_destination) continue; // try again
            if(edges_created.insert(edge, true)){
                num_edges_created_insofar++;
            }
        }
    };

    vector<thread> threads;
    threads.reserve(num_threads);
    for(int thread_id = 0; thread_id < num_threads; thread_id++){
        threads.emplace_back(create_edges, thread_id);
    }
    for(auto& t: threads) t.join();

    auto lst_edges = edges_created.lock_table();
    vector<Edge> edges;
    edges.reserve(lst_edges.size());
    for(auto& it_edge : lst_edges){
        edges.push_back(it_edge.first);
    }
    assert(edges.size() == g_num_edges && "The number of edges created does not match what the user requested");

//    lst_edges.unlock(); // not sure whether it's necessary
    return edges;
}

static vector<uint64_t> make_vertices(){
    vector<uint64_t> vertices;
    vertices.reserve(g_num_vertices);
    vertices.push_back(1); // the first vertex is always 1
    uint64_t vertex_id = 1;
    while(vertices.size() < g_num_vertices){
        if(vertex_id >= g_exp_factor_vertex_id * vertices.size()){
            vertices.push_back(vertex_id +1 /* because we started from 1 */);
        }
        vertex_id++;
    }

    return vertices;
}

static void save(const vector<uint64_t>& vertices, const vector<Edge>& edges){
    string basedir = filesystem::directory(g_output_prefix);
    filesystem::mkdir(basedir);
}

static void parse_command_line_arguments(int argc, char* argv[]){
    using namespace cxxopts;

    Options options(argv[0], "Uniform Graph Generator (ugg): create a uniform undirected graph");
    options.custom_help(" -V <num_vertices> -E <num_edges> -o <output_prefix> [-m <max_vertex_id>]");
    options.add_options()
       ("E, num_edges", "The total number of edges in the graph. If the value provided is less than the number of vertices, then it assumes that the given quantity is the average number of edges per vertex", value<ComputerQuantity>())
       ("h, help", "Show this help menu")
       ("m, max_vertex_id", "The expansion factor for the maximum vertex id to assign to the vertices/nodes in the graph. Node IDs will be in the domain  [0, max_vertex_id * num_vertices)", value<double>())
       ("o, output", "The prefix path where to save the created graph", value<string>())
       ("V, num_vertices", "The number of vertices to generate in the graph", value<ComputerQuantity>())
       ("seed", "Seed to initialise the random generator", value<uint64_t>())
   ;

    auto parsed_args = options.parse(argc, argv);

    if(parsed_args.count("help") > 0){
        cout << options.help() << endl;
        exit(EXIT_SUCCESS);
    }

    if(parsed_args.count("num_vertices") == 0){ ERROR("Missing mandatory argument --num_vertices"); }
    g_num_vertices = parsed_args["num_vertices"].as<ComputerQuantity>();
    if(g_num_vertices == 0){ ERROR("No vertices to generate"); }

    if(parsed_args.count("num_edges") == 0){ ERROR("Missing mandatory argument --num_edges"); }
    g_num_edges = parsed_args["num_edges"].as<ComputerQuantity>();
    if(g_num_edges == 0){ ERROR("No edges to generate"); }
    if(g_num_edges < g_num_vertices){
        cout << "Assuming to create " << g_num_edges << " on average per vertex\n\n";
        g_num_edges *= g_num_vertices /2; /* because the graph is undirected */
    }

    if(parsed_args.count("output") == 0 || parsed_args["output"].as<string>().empty()){
        ERROR("Missing mandatory argument --output");
    }
    g_output_prefix = parsed_args["output"].as<string>();

    if(parsed_args.count("max_vertex_id") > 0){
        double exp_factor =  parsed_args["max_vertex_id"].as<double>();
        if(exp_factor < 1){ ERROR("Expansion factor (max_vertex_id) is less than 1: " << exp_factor); }
        g_exp_factor_vertex_id = exp_factor;
    } else {
        g_exp_factor_vertex_id = 1.0;
    }

    if(parsed_args.count("seed") > 0){
        g_seed = parsed_args["seed"].as<uint64_t>();
    }

    cout << "Number of vertices to create: " << g_num_vertices << "\n";
    cout << "Number of edges to create: " << g_num_edges << "\n";
    cout << "Max vertex id: " << (uint64_t) ceil(g_exp_factor_vertex_id * (g_num_vertices -1)) +1 << " (exp factor: " << g_exp_factor_vertex_id << ")\n";
    cout << "Output prefix: " << g_output_prefix << "\n";
    cout << "Seed for the random generator:  " << g_seed << "\n";
    cout << endl;
}