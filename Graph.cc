
#include <stdio.h>

#include <fstream>
#include <sstream>
#include <iostream>
#include <string>

#include "pcre.h"

namespace Graph {

    void cleanup( VertexList *list ) {
        VertexList *next;
        while ( list ) {
            next = list->next;
            delete list;
            list = next;
        }
    }

    VertexList::VertexList( Vertex *v, VertexList *next = 0 )
    : vertex(v), next(next) { }

    Vertex::Vertex()
    : next(0), edge(0), parent(0), data(0)
    { }

    Vertex::Vertex( void *data )
    : next(0), edge(0), parent(0), data(data)
    { }

    Edge::Edge( Vertex *vertex )
    : vertex(vertex), next(0), weight(0) {}

    void Vertex::visit() { }
    void Vertex::reset() {
        discovered = false;
        explored   = false;
    }

    Edge *
    Vertex::connect( Vertex *v ) {
        Edge *e = new Edge( v );
        e->next = edge;
        edge = e;
        return e;
    }

    Graph::Graph() : vertices(0) {
        root = new Vertex;
        add( root );

        for ( Vertex *v = vertices ; v ; v = v->next ) {
            if ( v == root ) continue;
        }
    }

    void Graph::add( Vertex *vertex ) {
        vertex->next = vertices;
        vertices = vertex;
    }

    void Graph::reset() {
        acyclic = true;
        Vertex *v = vertices;
        while ( v != 0 ) {
            v->reset();
            v = v->next;
        }
    }

    void Graph::DFS() {
        reset();
        DFS( root );
    }

    void Graph::DFS( Vertex *u ) {
        u->discovered = true;
        u->visit();
        for ( Edge *edge = u->edge ; edge != 0 ; edge = edge->next ) {
            Vertex *v = edge->vertex;
            if ( v->discovered ) {
                if ( v->explored == false ) {
                    fprintf( stderr, "back edge found in UEC");
                    acyclic = false;
                }
                continue;
            }
            v->parent = u;
            DFS( v );
        }
        u->explored = true;
    }

    VertexList *
    Graph::TSort() {
        reset();
        return TSort(root, 0);
    }

    VertexList *
    Graph::TSort( Vertex *u, VertexList *list ) {
        if ( u == 0 ) {
            fprintf( stderr, "null vertex in tsort (error)" );
            return 0;
        }
        u->discovered = true;
        u->visit();
        for ( Edge *edge = u->edge ; edge != 0 ; edge = edge->next ) {
            Vertex *v = edge->vertex;
            if ( v->discovered ) {
                if ( v->explored == false ) {
                    fprintf( stderr, "back edge found in UEC");
                    acyclic = false;
                }
                continue;
            }
            v->parent = u;
            list = TSort( v, list );
        }
        u->explored = true;
        return new VertexList( u, list );
    }

    Graph::~Graph() {
        Vertex *v = vertices;
        while ( v ) {
            Vertex *u = v->next;
            delete v;
            v = u;
        }
    }

    bool
    Initialize( Tcl_Interp *interp ) {
        return true;
    }

}
