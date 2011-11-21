
#include <sst_config.h>
#include <sst/core/serialization/element.h>
#include <sst/core/component.h>
#include <sst/core/configGraph.h>

#include <util.h>
#include <python/swig/pyobject.hh>
#include <debug.h>
#include <sst/core/sdl.h>
#include <factory.h>
#include <cpu/base.hh>

struct LinkInfo {
    std::string compName;
    std::string portName;
    int portNum;
};

typedef std::pair< LinkInfo, LinkInfo>      link_t;
typedef std::map< std::string, link_t >     linkMap_t;

static void printLinkMap( linkMap_t& );
static void connectAll( objectMap_t&, linkMap_t& );

objectMap_t buildConfig( M5* comp, std::string name, std::string configFile, SST::Params& params )
{
    objectMap_t     objectMap;
    linkMap_t       linkMap;

    DBGC( 2, "name=`%s` file=`%s`\n", name.c_str(), configFile.c_str() );

    SST::sdl_parser sdl = SST::sdl_parser( configFile );
    SST::ConfigGraph& graph = *sdl.createConfigGraph();

    Factory factory( comp );

    SST::ConfigLinkMap_t::iterator lmIter;

    for ( lmIter = graph.getLinkMap().begin(); 
                lmIter != graph.getLinkMap().end(); ++lmIter ) {
        SST::ConfigLink& tmp = *(*lmIter).second;
        DBGC(2,"key=%s name=%s\n",(*lmIter).first.c_str(), tmp.name.c_str());

        LinkInfo l0,l1;
        l0.compName = graph.getComponentMap()[ tmp.component[0] ]->name.c_str();
        l1.compName = graph.getComponentMap()[ tmp.component[1] ]->name.c_str();

        l0.portName = tmp.port[0];
        l1.portName = tmp.port[1];

        l0.portNum= -1;
        l1.portNum = -1;

        linkMap[ tmp.name  ] =  std::make_pair( l0, l1 ); 
    } 
    //printLinkMap( linkMap );

    SST::ConfigComponentMap_t::iterator iter; 

    for ( iter = graph.getComponentMap().begin(); 
            iter != graph.getComponentMap().end(); ++iter ) {
        SST::ConfigComponent& tmp = *(*iter).second;
        DBGC(2,"id=%d %s %s\n",(*iter).first, tmp.name.c_str(), 
                                                    tmp.type.c_str());

        SST::Params tmpParams = params.find_prefix_params( tmp.name + "." );
        tmp.params.insert( tmpParams.begin(), tmpParams.end() );

        objectMap[ tmp.name.c_str() ]  = factory.createObject( 
                        name + "." + tmp.name, tmp.type, tmp.params );
    }

    connectAll( objectMap, linkMap );

    return objectMap;
}

void printLinkMap( linkMap_t& map  )
{
    for ( linkMap_t::iterator iter=map.begin(); iter != map.end(); ++iter ) {
        printf("link=%s %s<->%s\n",iter->first.c_str(),
            iter->second.first.compName.c_str(), 
            iter->second.second.compName.c_str());
    } 
}

void connectAll( objectMap_t& objMap, linkMap_t& linkMap  )
{
    linkMap_t::iterator iter; 
    for ( iter=linkMap.begin(); iter != linkMap.end(); ++iter ) {
        DBGC( 2,"connecting %s [%s %s %d]<->[%s %s %d]\n",iter->first.c_str(),
            iter->second.first.compName.c_str(), 
            iter->second.first.portName.c_str(), 
            iter->second.first.portNum, 
            iter->second.second.compName.c_str(),
            iter->second.second.portName.c_str(),
            iter->second.second.portNum);

        std::string portName1 = iter->second.first.portName;
        std::string portName2 = iter->second.second.portName;
        SimObject* obj1 = objMap[iter->second.first.compName];
        SimObject* obj2 = objMap[iter->second.second.compName];


        // this is a hack to allow M5 full system mode to work
        if ( ( ! portName1.compare(0,4,"pic." ) ) )
        {
            DBGC( 2, "PIC portName=%s cpuId=%d\n",portName1.c_str(),
                            ((BaseCPU*)obj1)->cpuId());
            //obj1 = ((BaseCPU*)obj1)->getInterruptController();
            portName1 = portName1.substr(4); 
        }

        if ( ( ! portName2.compare(0,4,"pic." ) ) ) 
        {
            DBGC( 2, "PIC portName=%s cpuId=%d\n",portName2.c_str(),
                            ((BaseCPU*)obj2)->cpuId());
            //obj2 = ((BaseCPU*)obj2)->getInterruptController();
            portName2 = portName2.substr(4); 
        }
        
        if ( ! connectPorts( obj1,
                        portName1,
                        iter->second.first.portNum,
                        obj2,
                        portName2,
                        iter->second.second.portNum) ) 
        {
            printf("connectPorts failed\n");
            exit(1);
        }
    } 
}
