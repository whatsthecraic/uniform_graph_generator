---
Uniform Graph Generator (UGG)
---

This is a small utility to generate synthetic graphs with a uniform 
node degree distribution. The graphs are produced in the same 
format expected by the [Graphalytics driver](https://github.com/ldbc/ldbc_graphalytics).
The tool yields three outputs: a property file (.properties) with a sequence of pairs 
`key = value`, a vertex file (.v) with the sorted list of all vertices 
generated and an edge file (.e) with the sorted list of all edges generated.
All created files are in plain text format. The tool can only create unweighted
simple graphs.

#### Requirements
- O.S. Linux
- CMake v 3.14 or newer
- A C++17 capable compiler

#### How to build
```
git clone https://github.com/whatsthecraic/uniform_graph_generator
cd uniform_graph_generator
git submodule update --init
mkdir build && cd build
cmake ../ -DCMAKE_BUILD_TYPE=Release
make -j
```

The final artifact is the executable `ugg`.

#### Usage

Type `./ugg -h` for the full list of options. The basic pattern is :
```
  ./ugg -V <num_vertices> -E <num_edges> -o <output_prefix> [-m <max_vertex_id>]
```

For instance, to create a uniform graph with the same number of vertices and edges of 
[graph500-24](https://www.graphalytics.org/datasets), run:

```
  ./ugg -V 8870942 -E 260379520 -m 1.9 -o uniform-24 
```







