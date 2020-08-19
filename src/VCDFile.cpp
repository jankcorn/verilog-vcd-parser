
#include <iostream>

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


/*!
@brief Add a new scope object to the VCD file
*/
void VCDFile::add_scope(
    VCDScope * s
){
    std::string stype;
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
        break;
    default:
        stype = "UNK_" + autostr(s->type);
        break;
    }
printf("[%s:%d] type %s name %s\n", __FUNCTION__, __LINE__, stype.c_str(), s->name.c_str());
    this -> scopes.push_back(s);
}


std::map<std::string, std::string> mapName;
/*!
@brief Add a new signal object to the VCD file
*/
void VCDFile::add_signal(
    VCDSignal * s
){
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
printf("[%s:%d] hash %s ref %s scope %p size %x type %s\n", __FUNCTION__, __LINE__, s->hash.c_str(), s->reference.c_str(), s->scope, s->size, stype.c_str());
if (mapName[s->hash] != "" && mapName[s->hash] != s->reference) {
//printf("[%s:%d] ERROR: hash %s was %s now %s\n", __FUNCTION__, __LINE__, s->hash.c_str(), mapName[s->hash].c_str(), s->reference.c_str());
}
mapName[s->hash] = s->reference;
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
    std::string name = mapName[hash];

    std::string val;
    switch (time_val->value->get_type()) {
    case VCD_SCALAR:
        val = VCDBit2Char(time_val->value->get_value_bit());
        break;
    case VCD_VECTOR:
        for (auto item: *time_val->value->get_value_vector())
            val += VCDBit2Char(item);
        break;
    case VCD_REAL:
        val = "REAL";
        break;
    default:
printf("[%s:%d]ERRRRROROR\n", __FUNCTION__, __LINE__);
    }
printf("[%s:%d] name %s val %s time %d\n", __FUNCTION__, __LINE__, name.c_str(), val.c_str(), (int)time_val->time);
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
