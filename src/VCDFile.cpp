
#include <iostream>
#include <list>

#include "VCDFile.hpp"
#define DOLLAR "$"

bool filterENARDY=true;

typedef struct {
    std::list<std::string> name;
    bool isRdy;
    bool isEna;
    bool used;
} MapNameItem;
std::map<std::string, MapNameItem *> mapName;
std::map<std::string, bool> actionMethod;

std::map<VCDScope *, std::string> scopeName;
std::map<std::string, std::string> mapValue;
std::map<std::string, std::string> currentValue, currentCycle;

static bool inline endswith(std::string str, std::string suffix)
{
    int skipl = str.length() - suffix.length();
    return skipl >= 0 && str.substr(skipl) == suffix;
}
static bool inline startswith(std::string str, std::string suffix)
{
    return str.substr(0, suffix.length()) == suffix;
}
static inline std::string autostr(uint64_t X, bool isNeg = false)
{
  char Buffer[21];
  char *BufPtr = std::end(Buffer);
  if (X == 0) *--BufPtr = '0';  // Handle special case...
  while (X) {
    *--BufPtr = '0' + char(X % 10);
    X /= 10;
  }
  if (isNeg) *--BufPtr = '-';   // Add negative sign...
  return std::string(BufPtr, std::end(Buffer));
}
std::string baseMethodName(std::string pname)
{
    int ind = pname.find("__ENA[");
    if (ind == -1)
        ind = pname.find("__RDY[");
    if (ind == -1)
        ind = pname.find("__ENA(");
    if (ind == -1)
        ind = pname.find("__RDY(");
    if (ind > 0)
        pname = pname.substr(0, ind) + pname.substr(ind + 5);
    if (endswith(pname, "__ENA") || endswith(pname, "__RDY"))
        pname = pname.substr(0, pname.length()-5);
    return pname;
}
std::string getRdyName(std::string basename)
{
    std::string base = baseMethodName(basename), sub;
    if (endswith(base, "]")) {
        int ind = base.rfind("[");
        sub = base.substr(ind);
        base = base.substr(0, ind);
    }
    else if (endswith(base, ")")) {
        int ind = base.rfind("(");
        sub = base.substr(ind);
        base = base.substr(0, ind);
    }
    return base + "__RDY" + sub;
}

std::string getEnaName(std::string basename)
{
    std::string base = baseMethodName(basename), sub;
    if (endswith(base, "]")) {
        int ind = base.rfind("[");
        sub = base.substr(ind);
        base = base.substr(0, ind);
    }
    else if (endswith(base, ")")) {
        int ind = base.rfind("(");
        sub = base.substr(ind);
        base = base.substr(0, ind);
    }
    return base + "__ENA" + sub;
}

bool isRdyName(std::string name)
{
    std::string rname = getRdyName(name);
    return name == rname;
}

bool isEnaName(std::string name)
{
    std::string rname = getEnaName(name);
    return name == rname;
}

std::string trimSpace(std::string arg)
{
    int beg = 0, end = arg.length();
    while (arg[beg] == ' ')
        beg++;
    while (arg[end-1] == ' ')
        end--;
    return arg.substr(beg, end);
}

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
        mapName[s->hash] = new MapNameItem({{}, false, false, false});
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
                int slash = namei->rfind("/");
                int dollar = namei->find(DOLLAR);
                if (dollar > 0 && (slash == -1 || dollar < slash) && itemi->second->name.size() != 1) {
                    namei = itemi->second->name.erase(namei);
                    continue;
                }
                bool isRdy = isRdyName(*namei);
                bool isEna = isEnaName(*namei);
                itemi->second->isRdy |= isRdy;
                itemi->second->isEna |= isEna;
                if (isEna)
                    actionMethod[baseMethodName(*namei)] = true;
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
//printf("[%s:%d] timeval %x type %d: ", __FUNCTION__, __LINE__, timeval, time_val->value->get_type());
//printf(" VAL %s\n", val.c_str());
    if (timeval != 0 || (val.find_first_not_of("0") != std::string::npos
                      && val.find_first_not_of("_") != std::string::npos)) {
        if (lasttime != timeval) {
            bool found = false;
            auto header = [&](void) -> void {
                if (!found)
                    printf("--------------------------------------------------- %d ----------------------\n", lasttime);
                found = true;
            };
            for (auto item: currentValue) {
                 std::string rdyName = getRdyName(item.first);
                 if (isEnaName(item.first) && item.second == "1" && currentValue[rdyName] == "1") {
                     currentCycle[item.first] = "";
                     currentCycle[rdyName] = "";
                     std::string methodName = baseMethodName(item.first);
                     std::string sep;
                     header();
                     printf("%s(", methodName.c_str());
                     methodName += "$";
                     for (auto param: currentValue)
                          if (startswith(param.first, methodName)) {
                              currentCycle[param.first] = "";
                              printf("%s%s=%s", sep.c_str(), param.first.substr(methodName.length()).c_str(), param.second.c_str());
                              sep = ", ";
                          }
                     printf(") -----------\n");
                 }
            }
            for (auto item: currentCycle) {
                std::string name = item.first, value = item.second;
                if (value != "") {
                    if (isRdyName(item.first)) {
                        if (lasttime > 0 && value == "1") {
                            int len = 50 - name.length();
                            if (len > 0 && value == "1")
                                 name += std::string("                                                                             ").substr(0, len) + value;
                            header();
                            printf(" %50s %s", " ", name.c_str());
                            if (value.length() > 1)
                                printf(" = %8s", value.c_str());
                            printf("\n");
                        }
                    }
                    else if (isEnaName(item.first)) {
                        if (value == "1") {
                        std::string prefix = " ";
                        if (value == "1")
                            prefix = value;
                        header();
                        printf("%s %50s", prefix.c_str(), name.c_str());
                        if (value.length() > 1)
                            printf(" = %8s", value.c_str());
                        printf("\n");
                        }
                    }
                    else {
                        int ind = name.rfind(DOLLAR);
                        if (ind == -1 || !actionMethod[name.substr(0, ind)]) {
                            header();
                            printf("  %50s = %8s\n", name.c_str(), value.c_str());
                        }
                    }
                }
            }
            currentCycle.clear();
            lasttime = timeval;
        }
        std::string sep;
        auto nameList = mapName[hash];
        for (auto item: nameList->name) {   // maintain 'current value of signal'
             currentValue[item] = val;
             currentCycle[item] = val;
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
