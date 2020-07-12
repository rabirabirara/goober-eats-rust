#include "provided.h"
#include "ExpandableHashMap.h"
#include "support.h"
#include <string>
#include <vector>
#include <functional>
#include <iostream>
#include <fstream>
#include <sstream>

using namespace std;

unsigned int hasher(const GeoCoord& g)
{
    return std::hash<std::string>()(g.latitudeText + g.longitudeText);
}

StreetSegment reverseSegment(const StreetSegment& street) {
    return StreetSegment(street.end, street.start, street.name);
}

class StreetMapImpl
{
public:
    StreetMapImpl();
    ~StreetMapImpl();
    bool load(string mapFile);
    bool getSegmentsThatStartWith(const GeoCoord& gc, vector<StreetSegment>& segs) const;
private:
    ExpandableHashMap<GeoCoord, vector<StreetSegment> > m_streets;
};

StreetMapImpl::StreetMapImpl()  // default load factor: 0.5
{}

StreetMapImpl::~StreetMapImpl()
{}

bool StreetMapImpl::load(string mapFile)
{
    ifstream infile(mapFile);
    if (!infile) {
        cerr << "Failed to retrieve data from " << mapFile << "!";
        return false;
    }

    string streetName;
    int streetCount;
    string coords;
    string coordLat1, coordLon1, coordLat2, coordLon2;
    
    while (getline(infile, streetName)) 
    {
        if (!(infile >> streetCount))
            cerr << "inFile failed to retrieve streetCount.";
        infile.ignore(1000, '\n');

        for (int i = 0; i != streetCount; i++) {
            getline(infile, coords);
            istringstream iss(coords);
            iss >> coordLat1 >> coordLon1 >> coordLat2 >> coordLon2;
            
            GeoCoord start(coordLat1, coordLon1);
            GeoCoord end(coordLat2, coordLon2);
            
            StreetSegment segment(start, end, streetName);

            vector<StreetSegment>* startVec = m_streets.find(start);
            vector<StreetSegment>* endVec = m_streets.find(end);

            if (startVec == nullptr) { // first time the GeoCoord start has been used
                vector<StreetSegment> segs;
                segs.push_back(segment);
                m_streets.associate(start, segs);
            }
            else {
                startVec->push_back(segment);
            }

            if (endVec == nullptr) {
                vector<StreetSegment> segs;
                segs.push_back(reverseSegment(segment));
                m_streets.associate(end, segs);
            }
            else {
                endVec->push_back(reverseSegment(segment));
            }
        }
    }

    return true;
}

bool StreetMapImpl::getSegmentsThatStartWith(const GeoCoord& gc, vector<StreetSegment>& segs) const
{
    const vector<StreetSegment>* ptr = m_streets.find(gc);
    if (ptr == nullptr)
        return false;
    else {
        auto it = ptr->begin();
        auto jt = ptr->end();
        segs.assign(it, jt);
        return true;
    }
}

//******************** StreetMap functions ************************************

// These functions simply delegate to StreetMapImpl's functions.
// You probably don't want to change any of this code.

StreetMap::StreetMap()
{
    m_impl = new StreetMapImpl;
}

StreetMap::~StreetMap()
{
    delete m_impl;
}

bool StreetMap::load(string mapFile)
{
    return m_impl->load(mapFile);
}

bool StreetMap::getSegmentsThatStartWith(const GeoCoord& gc, vector<StreetSegment>& segs) const
{
   return m_impl->getSegmentsThatStartWith(gc, segs);
}
