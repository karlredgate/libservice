
#ifndef _OPLGEN_H_
#define _OPLGEN_H_

namespace Graph {
    class Vertex;
    class VertexList;
    class Graph {
    public:
        Vertex *root;
        Vertex *vertices;
        bool acyclic;
        Graph() : vertices(0) {}
        Graph( Vsa * );
        void add( Vertex * );
        void reset();
        void DFS( Vertex * );
        void DFS();
        VertexList *TSort( Vertex *, VertexList * );
        VertexList *TSort();
        ~Graph();
    };
    class Vertex;
    class Edge {
    public:
        Vertex *vertex;
        Edge *next;
        int weight;
        Edge( Vertex * );
        ~Edge() { if (next) delete next; }
    };
    class Edge;
    class Vertex {
    public:
        Vertex *next;
        Edge *edge;
        Vertex *parent;
        void *data;
        uint16_t id;
        bool discovered, explored;
    
        Vertex();
        Vertex( void * );
        virtual void visit();
        void reset();
        Edge * connect( Vertex * );
        ~Vertex() { if ( edge ) delete edge; }
    };
    class VertexList {
    public:
        Vertex *vertex;
        VertexList *next;
        VertexList( Vertex *, VertexList * );
        ~VertexList() { }
    };
    
    bool Initialize(Tcl_Interp *);
    
}
#endif
