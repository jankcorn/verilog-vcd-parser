
#include <iostream>
#include <list>

#include "VCDFile.hpp"

bool filterENARDY=true;

typedef struct {
    std::list<std::string> name;
    bool isRdy;
    bool isEna;
    bool used;
} MapNameItem;
std::map<std::string, MapNameItem *> mapName;

std::map<VCDScope *, std::string> scopeName;
std::map<std::string, std::string> mapValue;

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

void VCDFile::add_scope( VCDScope * s)
{
    std::string parent, name = s->name;
    if (s->parent)
        parent = scopeName[s->parent];
    if (parent != "")
        parent += "/";
    if (s->type == VCD_SCOPE_ROOT || name == "TOP" || name == "VsimTop")
        name = "";
    if (startswith(name, "DUT__"))
        parent = "";
    scopeName[s] = parent + name;
//printf("[%s:%d] scope %p parent %p/%s type %d name %s\n", __FUNCTION__, __LINE__, s, s->parent, parent.c_str(), s->type, s->name.c_str());
    this -> scopes.push_back(s);
}

void VCDFile::add_signal( VCDSignal * s)
{
    this -> signals.push_back(s);
    if (s->type != VCD_VAR_WIRE)
        printf("[%s:%d] hash %s ref %s scope %p size %x type %d\n", __FUNCTION__, __LINE__, s->hash.c_str(), s->reference.c_str(), s->scope, s->size, s->type);
    std::string parent;
    if (s->scope)
        parent = scopeName[s->scope] + "/";
    parent += s->reference;    // combine parent and node names
    if (mapName.find(s->hash) == mapName.end())
        mapName[s->hash] = new MapNameItem({{}, false, false});
    mapName[s->hash]->name.push_back(parent);
    // Add a timestream entry
    if(val_map.find(s -> hash) == val_map.end()) {
        // Values will be populated later.
        val_map[s -> hash] = new VCDSignalValues();
    }
}

void VCDFile::add_signal_value( VCDTimedValue * time_val, VCDSignalHash   hash)
{
    static bool first = true;
    std::string val;
    static int lasttime = 0;
    int timeval = time_val->time;

    if (first) {
        first = false;
        for (auto itemi = mapName.begin(), iteme = mapName.end(); itemi != iteme; itemi++) {
            for (auto namei = itemi->second->name.begin(), namee = itemi->second->name.end(); namei != namee;) {
                bool isRdy = namei->find("__RDY") != std::string::npos;
                bool isEna = namei->find("__ENA") != std::string::npos;
                if ((*namei).find("$") != std::string::npos && itemi->second->name.size() != 1 && (isRdy || isEna)) {
                    namei = itemi->second->name.erase(namei);
                    continue;
                }
                itemi->second->isRdy |= isRdy;
                itemi->second->isEna |= isEna;
                namei++;
            }
        }
    }
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
                     nameList->used = true;
                     int len = 50 - item.length();
                     if (len > 0 && val == "1")
                          item += std::string("                                                                             ").substr(0, len) + val;
                     printf("%s %50s %s", sep.c_str(), " ", item.c_str());
                     sep = "\n";
break;
                 }
            }
            if (sep != "") {
                if (val.length() > 1)
                    printf(" = %8s\n", val.c_str());
                else
                    printf("\n");
            }
        }
        else {
            std::string prefix = " ";
            if (val == "1")
                prefix = val;
            for (auto item: nameList->name) {
                 nameList->used = true;
                 printf("%s%s %50s", sep.c_str(), prefix.c_str(), item.c_str());
                 sep = "\n";
break;
            }
            if (sep != "") {
                if (val.length() > 1)
                    printf(" = %8s\n", val.c_str());
                else
                    printf("\n");
            }
        }
    }
    this -> val_map[hash] -> push_back(time_val);
}

void VCDFile::done(void)
{
if (0)
    for (auto item: mapName) {
        if (item.second->used) {
            std::string start = "*";
            for (auto name: item.second->name) {
                if (item.second->isRdy)
                    printf("%s%50s %s\n", start.c_str(), " ", name.c_str());
                else
                    printf("%s%50s\n", start.c_str(), name.c_str());
                start = " ";
            }
        }
    }
}
