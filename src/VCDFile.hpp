#include <map>
#include <string>
#include <vector>
#include "VCDTypes.hpp"
#include "VCDValue.hpp"
#ifndef VCDFile_HPP
#define VCDFile_HPP
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
/*!
@brief Top level object to represent a single VCD file.
*/
class VCDFile {
public:
    VCDFile(){ }
    ~VCDFile(){
        // Delete signals and scopes.
        for (VCDScope * scope : this -> scopes) {
            for (VCDSignal * signal : scope -> signals)
                delete signal;
            delete scope;
        }
        for(auto hash_val = this -> val_map.begin(); hash_val != this -> val_map.end(); ++hash_val) {
            for(auto vals = hash_val -> second -> begin(); vals != hash_val -> second -> end(); ++vals) {
                delete (*vals) -> value;
                delete *vals;
            }
            delete hash_val -> second;
        }
    }
    //! Timescale of the VCD file.
    VCDTimeUnit time_units;
    //! Multiplier of the VCD file time units.
    VCDTimeRes  time_resolution;
    //! Date string of the VCD file.
    std::string date;
    //! Version string of the simulator which generated the VCD.
    std::string version;
    //! Root scope node of the VCD signals
    VCDScope * root_scope;
    /*!
    @brief Add a new scope object to the VCD file
    @param s in - The VCDScope object to add to the VCD file.
    */
    void add_scope( VCDScope * s);
    /*!
    @brief Add a new signal to the VCD file
    @param s in - The VCDSignal object to add to the VCD file.
    */
    void add_signal( VCDSignal * s);
    /*!
    @brief Add a new signal value to the VCD file, tagged by time.
    @param time_val in - A signal value, tagged by the time it occurs.
    @param hash in - The VCD hash value representing the signal.
    */
    void add_signal_value( VCDTimedValue * time_val, VCDSignalHash   hash); 
    /*!
    @brief Get the value of a particular signal at a specified time.
    @note The supplied time value does not need to exist in the
    vector returned by get_timestamps().
    @param hash in - The hashcode for the signal to identify it.
    @param time in - The time at which we want the value of the signal.
    @returns A pointer to the value at the supplie time, or nullptr if
    no such record can be found.
    */
    VCDValue * get_signal_value_at ( VCDSignalHash hash, VCDTime       time) {
        if(this -> val_map.find(hash) == this -> val_map.end()) {
            return nullptr;
        }
        VCDSignalValues * vals = this -> val_map[hash];
        if(vals -> size() == 0) {
            return nullptr;
        }
        VCDValue * tr = nullptr;
        for(auto it = vals -> begin(); it != vals -> end(); ++ it) { 
            if((*it) -> time <= time) {
                tr = (*it) -> value;
            } else {
                break;
            }
        }
        return tr;
    }
    VCDScope * get_scope( VCDScopeName name) {
        return nullptr;
    } 
    std::vector<VCDTime>* get_timestamps() {
        return &this -> times;
    } 
    std::vector<VCDScope*>* get_scopes() {
        return &this -> scopes;
    } 
    std::vector<VCDSignal*>* get_signals(){
        return &this -> signals;
    } 
    void add_timestamp( VCDTime time) {
        this -> times.push_back(time);
    }
    void done(void);
protected:
    //! Flat vector of all signals in the file.
    std::vector<VCDSignal*> signals;
    //! Flat mao of all scope objects in the file, keyed by name.
    std::vector<VCDScope*>  scopes;
    //! Vector of time values present in the VCD file - sorted, asc
    std::vector<VCDTime>    times;
    //! Map of hashes onto vectors of times and signal values.
    std::map<VCDSignalHash, VCDSignalValues*> val_map;
};
#endif
