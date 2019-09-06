#include <cstdlib>
#include <iostream>
#include <random>
#include <string>

#include "lib/common/error.hpp"
#include "lib/common/quantity.hpp"
#include "lib/cxxopts.hpp"
#include "lib/cuckohash.hpp"

using namespace common;
using namespace std;

// globals
uint64_t g_max_vertex_id; // the maximum vertex id to assign to the nodes in the graph
uint64_t g_num_edges; // the total number of edges to create
uint64_t g_num_vertices; // number of vertices to create
string g_output_prefix; // path where to save the generated files
uint64_t g_seed = std::random_device{}(); // the seed to use for the random generator

// function prototypes
static void parse_command_line_arguments(int argc, char* argv[]);

// entry point
int main(int argc, char* argv[]) {
    try {
        parse_command_line_arguments(argc, argv);
    } catch (common::Error& e){
        cerr << e << endl;
        cerr << "Type `" << argv[0] << " --help' to check how to run the program\n";
        cerr << "Program terminated" << endl;
        return 1;
    }

    return 0;
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
        g_num_edges *= g_num_vertices;
    }

    if(parsed_args.count("output") == 0 || parsed_args["output"].as<string>().empty()){
        ERROR("Missing mandatory argument --output");
    }
    g_output_prefix = parsed_args["output"].as<string>();

    if(parsed_args.count("max_vertex_id") > 0){
        double exp_factor =  parsed_args["max_vertex_id"].as<double>();
        if(exp_factor < 1){ ERROR("Expansion factor (max_vertex_id) is less than 1: " << exp_factor); }
        g_max_vertex_id = exp_factor * g_num_vertices;
        if(g_max_vertex_id < g_num_vertices) g_max_vertex_id = g_num_vertices; // error in the mult with doubles
    } else {
        g_max_vertex_id = g_num_vertices;
    }

    if(parsed_args.count("seed") > 0){
        g_seed = parsed_args["seed"].as<uint64_t>();
    }

    cout << "Number of vertices to create: " << g_num_vertices << "\n";
    cout << "Number of edges to create: " << g_num_edges << "\n";
    cout << "Max vertex id: " << g_max_vertex_id << "\n";
    cout << "Output prefix: " << g_output_prefix << "\n";
    cout << "Seed for the random generator:  " << g_seed << "\n";
}