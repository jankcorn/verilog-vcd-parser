
#include <iostream>
#include <list>

#include "VCDFile.hpp"
        
        
//! Instance a new VCD file container.
VCDFile::VCDFile(){

}
        
//! Destructor
VCDFile::~VCDFile(){

    // Delete signals and scopes.

    for (VCDScope * scope : this -> scopes) {
    
        for (VCDSignal * signal : scope -> signals) {
            delete signal;
        }
        
        delete scope;
    }

    // Delete signal values.
    
    for(auto hash_val = this -> val_map.begin();
             hash_val != this -> val_map.end();
             ++hash_val)
    {
        for(auto vals = hash_val -> second -> begin();
                 vals != hash_val -> second -> end();
                 ++vals)
        {
            delete (*vals) -> value;
            delete *vals;
        }

        delete hash_val -> second;
    }

}


std::map<VCDScope *, std::string> scopeName;
/*!
@brief Add a new scope object to the VCD file
*/
void VCDFile::add_scope( VCDScope * s)
{
    std::string stype;
    std::string parent, name = s->name;
    switch (s->type) {
    case VCD_SCOPE_BEGIN:
        stype = "BEGIN";
        break;
    case VCD_SCOPE_FORK:
        stype = "FORK";
        break;
    case VCD_SCOPE_FUNCTION:
        stype = "FUNCTION";
        break;
    case VCD_SCOPE_MODULE:
        stype = "MODULE";
        break;
    case VCD_SCOPE_TASK:
        stype = "TASK";
        break;
    case VCD_SCOPE_ROOT:
        stype = "ROOT";
        name = "";
        break;
    default:
        stype = "UNK_" + autostr(s->type);
        break;
    }
    if (s->parent)
        parent = scopeName[s->parent];
    if (parent != "")
        parent += "/";
    if (name == "TOP" || name == "VsimTop")
        name = "";
    if (startswith(name, "DUT__"))
        parent = "";
    scopeName[s] = parent + name;
//printf("[%s:%d] scope %p parent %p/%s type %s name %s\n", __FUNCTION__, __LINE__, s, s->parent, parent.c_str(), stype.c_str(), s->name.c_str());
    this -> scopes.push_back(s);
}


typedef struct {
    std::list<std::string> name;
    bool isRdy;
} MapNameItem;
std::map<std::string, MapNameItem *> mapName;
/*!
@brief Add a new signal object to the VCD file
*/
void VCDFile::add_signal( VCDSignal * s)
{
    this -> signals.push_back(s);
    std::string stype;
    switch(s->type) {
    case VCD_VAR_EVENT:
        stype = "EVENT";
        break;
    case VCD_VAR_INTEGER:
        stype = "INTEGER";
        break;
    case VCD_VAR_PARAMETER:
        stype = "PARAMETER";
        break;
    case VCD_VAR_REAL:
        stype = "REAL";
        break;
    case VCD_VAR_REALTIME:
        stype = "REALTIME";
        break;
    case VCD_VAR_REG:
        stype = "REG";
        break;
    case VCD_VAR_SUPPLY0:
        stype = "SUPPLY0";
        break;
    case VCD_VAR_SUPPLY1:
        stype = "SUPPLY1";
        break;
    case VCD_VAR_TIME:
        stype = "TIME";
        break;
    case VCD_VAR_TRI:
        stype = "TRI";
        break;
    case VCD_VAR_TRIAND:
        stype = "TRIAND";
        break;
    case VCD_VAR_TRIOR:
        stype = "TRIOR";
        break;
    case VCD_VAR_TRIREG:
        stype = "TRIREG";
        break;
    case VCD_VAR_TRI0:
        stype = "TRI0";
        break;
    case VCD_VAR_TRI1:
        stype = "TRI1";
        break;
    case VCD_VAR_WAND:
        stype = "WAND";
        break;
    case VCD_VAR_WIRE:
        stype = "WIRE";
        break;
    case VCD_VAR_WOR:
        stype = "WOR";
        break;
    default:
        stype = "UNK_" + autostr(s->type);
        break;
    };
if (stype != "WIRE")
printf("[%s:%d] hash %s ref %s scope %p size %x type %s\n", __FUNCTION__, __LINE__, s->hash.c_str(), s->reference.c_str(), s->scope, s->size, stype.c_str());
    std::string parent;
    if (s->scope)
        parent = scopeName[s->scope] + "/";
    parent += s->reference;    // combine parent and node names
    if (mapName.find(s->hash) == mapName.end())
        mapName[s->hash] = new MapNameItem({{}, false});
    bool isRdy = s->reference.find("__RDY") != std::string::npos;
    if (parent.find("$") == std::string::npos || mapName[s->hash]->name.size() == 0) {  // don't push internal wire names
        mapName[s->hash]->isRdy |= isRdy;
        mapName[s->hash]->name.push_back(parent);
    }
    // Add a timestream entry
    if(val_map.find(s -> hash) == val_map.end()) {
        // Values will be populated later.
        val_map[s -> hash] = new VCDSignalValues();
    }
}


/*!
*/
VCDScope * VCDFile::get_scope(
    VCDScopeName name
){
        return nullptr;
}


/*!
@brief Add a new signal value to the VCD file, tagged by time.
*/
    static char VCDBit2Char(VCDBit b) {
        switch(b) {
            case(VCD_0):
                return '0';
            case(VCD_1):
                return '1';
            case(VCD_Z):
                return 'Z';
            case(VCD_X):
            default:
                return 'X';
        }
    }
std::map<std::string, std::string> mapValue;
void VCDFile::add_signal_value(
    VCDTimedValue * time_val,
    VCDSignalHash   hash
){
    std::string val;
    static int lasttime = 0;
    int timeval = time_val->time;
    switch (time_val->value->get_type()) {
    case VCD_SCALAR:
        val = VCDBit2Char(time_val->value->get_value_bit());
        break;
    case VCD_VECTOR:
        for (auto item: *time_val->value->get_value_vector())
            val += VCDBit2Char(item);
        if (val.find("X") == std::string::npos && val.find("Z") == std::string::npos) {
            int len = val.length() / 4;
            int addLen = val.length() - len * 4;
            std::string temp = std::string("00000000").substr(0, addLen) + val;
            val = "";
            unsigned i = 0;
            int data = 0;
            while (i < temp.length()) {
                if (i && (i % 32) == 0)
                    val += "_";
                data = (data << 1) + (temp[i] - '0');
                i++;
                if ((i % 4) == 0) {
                    val += "0123456789abcdef"[data];
                    data = 0;
                }
            }
        }
        break;
    case VCD_REAL:
        val = "REAL";
        break;
    default:
printf("[%s:%d]ERRRRROROR\n", __FUNCTION__, __LINE__);
    }
    if (timeval != 0 || (val.find_first_not_of("0") != std::string::npos
                      && val.find_first_not_of("_") != std::string::npos)) {
        if (lasttime != timeval) {
            printf("--------------------------------------------------- %d ----------------------\n", timeval);
            lasttime = timeval;
        }
        std::string sep;
        auto nameList = mapName[hash];
        if (nameList->isRdy) {
        for (auto item: nameList->name) {
             if (timeval != 0 || item.find("__RDY") == std::string::npos) {
                 printf("%s %50s %s", sep.c_str(), " ", item.c_str());
                 sep = "\n";
             }
        }
        if (sep != "")
            printf(" = %8s\n", val.c_str());
        }
        else {
        for (auto item: nameList->name) {
             printf("%s %50s", sep.c_str(), item.c_str());
             sep = "\n";
        }
        printf(" = %8s\n", val.c_str());
        }
    }
    std::string name;
    if (0)
    if (!startswith(name, "CLK")
     && name != "waitForEnq"
     && name != "buffer[:]"
     && name != "remain[:]"
     && name != "v[:]"
     && name != "indication$enq$len[:]"
     && !startswith(name, "in$enq")
     && !startswith(name, "out$enq")
     && !startswith(name, "beat")) {
        static int lastTime = -1;
        int thisTime = (int) time_val->time;
        if (thisTime) {
            if (thisTime != lastTime) {
                for (auto item: mapValue)
                     printf(" %s = %s", item.first.c_str(), item.second.c_str());
                printf("\n[%5d]", thisTime);
                mapValue.clear();
            }
            mapValue[name] = val;
            //printf(" %s = %s", name.c_str(), val.c_str());
        }
        lastTime = thisTime;
    }
    this -> val_map[hash] -> push_back(time_val);
}


/*!
*/
std::vector<VCDTime>* VCDFile::get_timestamps(){
    return &this -> times;
}


/*!
*/
std::vector<VCDScope*>* VCDFile::get_scopes(){
    return &this -> scopes;
}


/*!
*/
std::vector<VCDSignal*>* VCDFile::get_signals(){
    return &this -> signals;
}


/*!
*/
void VCDFile::add_timestamp(
    VCDTime time
){
    this -> times.push_back(time);
}

/*!
*/
VCDValue * VCDFile::get_signal_value_at (
    VCDSignalHash hash,
    VCDTime       time
){
    if(this -> val_map.find(hash) == this -> val_map.end()) {
        return nullptr;
    }
    
    VCDSignalValues * vals = this -> val_map[hash];

    if(vals -> size() == 0) {
        return nullptr;
    }

    VCDValue * tr = nullptr;

    for(auto it = vals -> begin();
             it != vals -> end();
             ++ it) {

        if((*it) -> time <= time) {
            tr = (*it) -> value;
        } else {
            break;
        }
    }

    return tr;
}
