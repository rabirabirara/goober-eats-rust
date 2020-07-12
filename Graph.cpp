#include "Graph.h"
#include "provided.h"
#include <vector>
#include <string> // for implementing the swap of two coords/requests, because an assignment operator isn't defined
#include <algorithm>
#include <utility>
#include <unordered_set>
#include <unordered_map>
using namespace std;

// Both structs defined in Graph.h
struct Vertex;
struct Edge;

void Structure::makeTour(const unordered_set<Vertex *, HashVertices, CompareVertices> *verts)
{
    Vertex *current;

    // Double iterator initialization.
    for (pair<unordered_set<Vertex *, HashVertices, CompareVertices>::const_iterator,
              unordered_set<Vertex *, HashVertices>::const_iterator>
             i(verts->cbegin(), verts->cbegin());
         ; ++i.first)
    {
        current = (*(i.first));
        if (i.first == i.second)
            i.second++;
        if (i.second == verts->end())
            return;
        connect(current, (*(i.second)));
    }
    connect(current, *verts->begin());
}

void Structure::setTip(Vertex *tip)
{
    tip->makeTip();
    m_tip = tip;
}

double Structure::distance(const Vertex *one, const Vertex *two)
{
    return (distanceEarthMiles(one->coord, two->coord));
}

Vertex *Structure::getTip()
{
    return m_tip;
}
Vertex *Structure::getRoot()
{
    return m_root;
}
bool Structure::areAdjacent(Vertex *one, Vertex *two) const
{
    vector<Edge> primary = m_findAdj.find(one)->second;
    for (auto it : primary)
    {
        if (it.thisContains(two))
            return true;
    }
    return false;
}

void Structure::connect(Vertex *one, Vertex *two)
{
    Edge e1(one, two);
    Edge e2(two, one);
    m_edges.insert(e1);
    m_edges.insert(e2);
    m_tabuToDrop.insert(e1);
    m_tabuToDrop.insert(e2);
    auto found1 = m_findAdj.find(one);
    auto found2 = m_findAdj.find(two);
    if (found1 == m_findAdj.end())
    {
        vector<Edge> v1;
        v1.push_back(e1);
        m_findAdj[one] = v1;
    }
    else
    {
        found1->second.push_back(e1);
    }
    if (m_findAdj.find(two) == m_findAdj.end())
    {
        vector<Edge> v2;
        v2.push_back(e2);
        m_findAdj[two] = v2;
    }
    else
    {
        found2->second.push_back(e2);
    }
}

void Structure::detach(Vertex *one, Vertex *two)
{ // properly implement this.  change map associations and set presence.
    Edge e1(one, two);
    Edge e2(two, one);
    m_edges.erase(e1);
    m_edges.erase(e2);
    m_tabuToAdd.insert(e1);
    m_tabuToAdd.insert(e2);
    auto found1 = m_findAdj.find(one);
    auto found2 = m_findAdj.find(two);
    if (found1 != m_findAdj.end())
    {
        for (auto it = found1->second.begin(); it != found1->second.end(); ++it)
        {
            if (it->two == two)
            {
                found1->second.erase(it);
            }
        }
    }
    if (found2 != m_findAdj.end())
    {
        for (auto jt = found2->second.begin(); jt != found2->second.end(); ++jt)
        {
            if (jt->two == one)
            {
                found2->second.erase(jt);
            }
        }
    }
}

// Finds random adjacent node, regardless of Flower position.
Vertex *Structure::findAdj(Vertex *vert) const
{
    if (m_findAdj.find(vert) == m_findAdj.end())
    {
        cerr << "findAdj() did not find any edges connecting this vertex to the tour.";
        return nullptr;
    }
    vector<Edge> edges = m_findAdj.find(vert)->second;
    int random = rand() % edges.size();
    auto it = edges.begin();
    advance(it, random);
    return it->notThis(vert);
}

// Doesn't randomize which adjacent cycle node to choose.
Vertex *Structure::findAdjCycle(Vertex *cycle) const
{
    auto iterCycle = m_findAdj.find(cycle);
    if (iterCycle == m_findAdj.end())
    {
        cerr << "This cycle node was not found in the map of nodes to adjacent edges.";
        return nullptr;
    }
    // vector<Edge> edges = m_findAdj.find(cycle)->second;
    for (auto it : iterCycle->second)
    {
        if (it.thisContains(cycle) && it.notThis(cycle)->isCycle())
        {
            return it.notThis(cycle);
        }
    }
}

// Doesn't randomize node chosen on subpath.
Vertex *Structure::findAdjStem(Vertex *stem) const
{
    auto iterStem = m_findAdj.find(stem);
    if (iterStem == m_findAdj.end())
    {
        cerr << "This stem node was not found in the map of nodes to adjacent edges.";
        return nullptr;
    }
    // vector<Edge> edges = m_findAdj.find(stem)->second;
    for (auto it : iterStem->second)
    {
        if (it.thisContains(stem) && isOnSubpath(stem, it.notThis(stem)))
        {
            return it.notThis(stem);
        }
    }
}

// Check this code.  May have non-terminating while-loop.
bool Structure::isOnSubpath(Vertex *vert, Vertex *sub) const
{
    if (vert == m_root)
        return true;
    Vertex *current = m_tip; // Check each node from tip to vert.  if sub encountered first, return true.  else false.
    vector<Edge> edges;
    while (true)
    {
        if (current == m_root)
            return false;
        edges = m_findAdj.find(current)->second;
        for (auto it : edges)
        {
            if (it.thisContains(current))
            {
                if (it.notThis(current) == vert)
                    return false;
                if (it.notThis(current) == sub)
                    return true;
                current = it.notThis(current);
            }
        }
    }
    cerr << "isOnSubpath() did not successfuly terminate search for subpath nodes.";
}

// Start at root.  If root found, take adjacent nodes of root.  Append to vector
// of subroots and turn into a pair; return pair.
pair<Vertex *, Vertex *> Structure::findSubroots()
{
    auto iterRoot = m_findAdj.find(m_root);
    vector<Vertex *> subroots;
    if (iterRoot != m_findAdj.end())
    {
        for (auto jt : iterRoot->second)
        {
            if (jt.notThis(m_root)->isCycle())
                subroots.push_back(jt.notThis(m_root));
        }
        pair<Vertex *, Vertex *> subrootPair(subroots[0], subroots[1]);
        return subrootPair;
    }
    cerr << "findSubroots() failed to find adjacent nodes to the root.";
    return pair<Vertex *, Vertex *>(nullptr, nullptr);
}
