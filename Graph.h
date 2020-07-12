#include "provided.h"
#include <vector>
#include <string> // for implementing the swap of two coords/requests, because an assignment operator isn't defined
#include <algorithm>
#include <functional>
#include <utility>
#include <unordered_set>
#include <unordered_map>
using namespace std;

enum Flower
{
    STEM,
    CYCLE
};

struct Vertex
{
    Vertex(GeoCoord g)
        : coord(g), isTip(false), isRoot(false), stem_cycle(CYCLE)
    {
    }
    Vertex(GeoCoord g, bool base)
        : coord(g), isBase(base), isTip(false), isRoot(false), stem_cycle(CYCLE)
    {
    }

    bool operator==(const Vertex *other)
    {
        return (coord == other->coord && stem_cycle == other->stem_cycle);
    }

    Vertex *makeRoot()
    {
        isRoot = true;
        stem_cycle = true;
        return this;
    }
    Vertex *makeTip()
    {
        isTip = true;
        stem_cycle = true;
        return this;
    }
    bool isStem() const
    {
        return (stem_cycle == STEM ? true : false);
    }
    bool isCycle() const
    {
        return (stem_cycle == CYCLE ? true : false);
    }
    bool isTip() const { return isTip; }
    bool isRoot() const { return isRoot; }
    // bool isSubRoot() {
    //     return (!stem_cycle && isNextTo(this, getRoot()));
    // }

    bool isBase; // In case a tour has a designated return point.
    bool isTip;
    bool isRoot;
    Flower stem_cycle; // stem is true, cycle is false
    GeoCoord coord;
};

struct Edge
{
    Edge(Vertex *first, Vertex *second)
        : one(first), two(second)
    {
    }

    bool operator==(const Edge &other)
    {
        // return ((one == other.one) && (two == other.two) || (one == other.two) && (two = other.one));
        return (other.thisContains(one) && other.thisContains(two));
    }

    bool thisContains(Vertex *vert)
    {
        return (one == vert || two == vert);
    }
    Vertex *notThis(Vertex *vert)
    {
        return (one == vert ? two : one);
    }

    Vertex *one;
    Vertex *two;
};

/*
struct HashVertices {
    size_t operator()(const pair<Vertex*, Vertex*>& pair) const {
        return std::hash<std::string>()(
            pair.first->coord.latitudeText + pair.first->coord.longitudeText +
            pair.second->coord.latitudeText + pair.second->coord.longitudeText);
    }
};
*/

struct HashVertices
{
    std::size_t operator()(const Vertex *vert) const
    {
        return std::hash<std::string>()(
            vert->coord.latitudeText + vert->coord.longitudeText);
    }
};

struct CompareVertices
{
    std::size_t operator()(Vertex const *one, Vertex const *two) const
    {
        return one->coord == two->coord;
    }
};

struct HashEdges
{
    std::size_t operator()(const Edge &edge) const
    {
        return std::hash<std::string>()(edge.one->coord.latitudeText + edge.one->coord.longitudeText +
                                        edge.two->coord.latitudeText + edge.two->coord.longitudeText);
    }
};

class NodeList
{
    unordered_set<Vertex *, HashVertices, CompareVertices> m_vertices; // pointer or not pointer?

public:
    // ONLY inserts vertices.
    NodeList(GeoCoord depot, vector<DeliveryRequest> deliveries)
    {
        // m_vertices = new unordered_set<Vertex*, HashVertices, CompareVertices>;   // if pointer
        Vertex *current = new Vertex(depot, true);
        m_vertices.insert(current);
        for (auto it : deliveries)
        {
            Vertex *next = new Vertex(it.location);
            m_vertices.insert(next);
            // current = next;
        }
        // Vertex end(depot);   // not sure why I created this new vertex.
    }
    // should be done only when all the tours are not needed.  figure it out, how to track Structure count.
    ~NodeList()
    {
        for (auto it = m_vertices->begin(); it != m_vertices->end(); ++it)
        {
            delete *it;
        }
        m_vertices->clear();
        delete m_vertices;
    }

    int getSize() const
    {
        return m_vertices->size();
    }

    const unordered_set<Vertex *, HashVertices, CompareVertices> *getNodeList() const
    {
        return m_vertices;
    }

    // Need to find a random stem (including root) that isn't the tip.
    Vertex *findRandStem() const
    {
        int stemCount = 0;
        vector<Vertex *> stems;
        for (auto it = getNodeList()->begin(); it != getNodeList()->end(); ++it)
        {
            if ((*it)->isStem() && (!(*it)->isTip()))
            {
                stems.push_back(*it);
                stemCount++;
            }
        }
        if (stemCount == 0)
        { // prevent divison by zero.
            cerr << "Structure error: Stem is degenerate. Fix call.";
            return nullptr;
        }
        int choice = rand() % stemCount;
        return stems[choice];
    }
    Vertex *findRandCycle() const
    {
        int cycleCount = 0;
        vector<Vertex *> cycles;
        for (auto it = getNodeList()->begin(); it != getNodeList()->end(); ++it)
        {
            if ((*it)->isCycle())
            {
                cycles.push_back(*it);
                cycleCount++;
            }
        }
        if (cycleCount == 0)
        {
            cerr << "Structure error: Cycle is empty.";
            return nullptr;
        }
        int choice = rand() % cycleCount;
        return cycles[choice];
    }
};

// A structure has edges.  It's called a structure because
// a structure describes relationships between nodes.
class Structure
{
    unordered_set<Edge, HashEdges> m_edges;
    unordered_map<Vertex *, vector<Edge>, HashVertices> m_findAdj;

    unordered_set<Edge> m_tabuToAdd;
    unordered_set<Edge> m_tabuToDrop;

    NodeList *m_nodelist;
    Vertex *m_tip;
    Vertex *m_root;

public:
    Structure(NodeList *nodes)
        : m_nodelist(nodes), m_tip(nullptr), m_root(nullptr)
    {
    }
    Structure(unordered_set<Edge, HashEdges> edges)
        : m_edges(edges), m_tip(nullptr), m_root(nullptr)
    {
    }

    Structure &operator=(const Structure &other)
    {
        if (this != &other)
        {
            m_edges = other.m_edges;
            m_tip = other.m_tip;
            m_root = other.m_root;
        }
        return *this;
    }

    void makeTour(const unordered_set<Vertex *, HashVertices, CompareVertices> *verts);
    void setTip(Vertex *tip);
    double distance(const Vertex *one, const Vertex *two);
    /*
    void convert() {
        int random = rand() % m_nodelist->size;
        auto it = getNodeList().begin();
        advance(it, random);
        setTip(*it);
    }
    */
    /*
    void ejectCycle() {
        Vertex* cycle = m_nodelist->findRandCycle();
        connect(m_tip, cycle);
        Vertex* adj = findAdjCycle();
        detach(cycle, adj);
        setTip(adj);
    }
    */
    /*
    void ejectStem() {
        Vertex* stem = m_nodelist->findRandStem();
        connect(m_tip, stem);
        Vertex* adj = findAdjStem();
        detach(stem, adj);
        setTip(adj);
    }
    */

    /*
    void recordTrial() {
        makeTrial();
        //traverse the tour and record the edges and order
    }
    void makeTrial() {
        pair<Vertex*, Vertex*> subroots = findSubroots();
        if (distance(m_tip, subroots.first) < distance(m_tip, subroots.second)) {
            connect(m_tip, subroots.first);
            detach(subroots.first, m_root);
        }
        else  {
            connect(m_tip, subroots.second);
            detach(subroots.second, m_root);
        }
    }
    */

    Vertex *getTip();
    Vertex *getRoot();

    bool areAdjacent(Vertex *one, Vertex *two) const;

    void connect(Vertex *one, Vertex *two);
    void detach(Vertex *one, Vertex *two);

    void connect(const Edge &toAdd)
    {
        connect(toAdd.one, toAdd.two);
    }
    void detach(const Edge &toRemove)
    {
        detach(toRemove.one, toRemove.two);
    }
    Edge reverseEdge(const Edge &seg) const
    {
        return Edge(seg.two, seg.one);
    }
    bool inThisTour(const Edge &target) const
    {
        return (m_edges.find(target) != m_edges.end());
    } // (i j) must not be in last tour. (j k) must be in last tour.
    bool isTabuToAdd(const Edge &segment) const
    {
        return (m_tabuToAdd.find(segment) == m_tabuToAdd.end());
    }
    bool isTabuToDrop(const Edge &segment) const
    {
        return (m_tabuToAdd.find(segment) == m_tabuToDrop.end());
    }
    void resetTabu();
    Vertex *findAdj(Vertex *vert) const;
    Vertex *findAdjCycle(Vertex *cycle) const;
    Vertex *findAdjStem(Vertex *stem) const;
    bool isOnSubpath(Vertex *vert, Vertex *sub) const;
    pair<Vertex *, Vertex *> findSubroots();
};
