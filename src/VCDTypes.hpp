
#include <map>
#include <utility>
#include <string>
#include <vector>
#include <set>
#include <stack>

#ifndef VCDTypes_HPP
#define VCDTypes_HPP

//! Compressed hash representation of a signal.
typedef std::string VCDSignalHash;

//! Represents a single instant in time in a trace
typedef double VCDTime;

//! Specifies the timing resoloution along with VCDTimeUnit
typedef unsigned VCDTimeRes;

//! Width in bits of a signal.
typedef unsigned VCDSignalSize;

//! Represents the four-state signal values of a VCD file.
typedef enum {
    VCD_0 = 0,  //!< Logic zero
    VCD_1 = 1,  //!< Logic one
    VCD_X = 2,  //!< Unknown / Undefined
    VCD_Z = 3   //!< High Impedence.
} VCDBit;

//! A vector of VCDBit values.
typedef std::vector<VCDBit> VCDBitVector;

//! Typedef to identify a real number as stored in a VCD.
typedef double VCDReal;

//! Describes how a signal value is represented in the VCD trace.
typedef enum {
    VCD_SCALAR, //!< Single VCDBit
    VCD_VECTOR, //!< Vector of VCDBit
    VCD_REAL    //!< IEEE Floating point (64bit).
} VCDValueType;

// Forward declaration of class.
class VCDValue;

//! A signal value tagged with times.
typedef struct {
    VCDTime     time;
    VCDValue  * value;
} VCDTimedValue;

//! A vector of tagged time/value pairs, sorted by time values.
typedef std::vector<VCDTimedValue*> VCDSignalValues;

//! Variable types of a signal in a VCD file.
typedef enum {
    VCD_VAR_EVENT,
    VCD_VAR_INTEGER,
    VCD_VAR_PARAMETER,
    VCD_VAR_REAL,
    VCD_VAR_REALTIME,
    VCD_VAR_REG,
    VCD_VAR_SUPPLY0,
    VCD_VAR_SUPPLY1,
    VCD_VAR_TIME,
    VCD_VAR_TRI,
    VCD_VAR_TRIAND,
    VCD_VAR_TRIOR,
    VCD_VAR_TRIREG,
    VCD_VAR_TRI0,
    VCD_VAR_TRI1,
    VCD_VAR_WAND,
    VCD_VAR_WIRE,
    VCD_VAR_WOR
} VCDVarType;

//! Represents the possible time units a VCD file is specified in.
typedef enum {
    TIME_S,     //!< Seconds
    TIME_MS,    //!< Milliseconds
    TIME_US,    //!< Microseconds
    TIME_NS,    //!< Nanoseconds
    TIME_PS,    //!< Picoseconds
} VCDTimeUnit;

//! Represents the type of SV construct who's scope we are in.
typedef enum {
    VCD_SCOPE_BEGIN,
    VCD_SCOPE_FORK,
    VCD_SCOPE_FUNCTION,
    VCD_SCOPE_MODULE,
    VCD_SCOPE_TASK,
    VCD_SCOPE_ROOT
} VCDScopeType;

// Typedef over vcdscope to make it available to VCDSignal struct.
typedef struct vcdscope VCDScope;

//! Represents a single signal reference within a VCD file
typedef struct {
    VCDSignalHash       hash;
    std::string         reference;
    VCDScope          * scope;
    VCDSignalSize       size;
    VCDVarType          type;
} VCDSignal;

//! Represents a scope type, scope name pair and all of it's child signals.
struct vcdscope {
    std::string               name;     //!< The short name of the scope
    VCDScopeType              type;     //!< Construct type
    VCDScope                * parent;   //!< Parent scope object
    std::vector<VCDScope*>    children; //!< Child scope objects.
    std::vector<VCDSignal*>   signals;  //!< Signals in this scope.
};

/*!
@brief Represents a single value found in a VCD File.
@details Can contain a single bit (a scalar), a bti vector, or an
IEEE floating point number.
*/
class VCDValue {
    //! The type of value this instance stores.
    VCDValueType    type;
    //! The actual value stored, as identified by type.
    union valstore {
        VCDBit         val_bit;   //!< Value as a bit
        VCDBitVector * val_vector;//!< Value as a bit vector
        VCDReal        val_real;  //!< Value as a real number (double).
    } value;
    //! Convert a VCDBit to a single char
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

public:
    VCDValue (VCDBit  value) {
        this->type = VCD_SCALAR;
        this->value.val_bit = value;
    }
    VCDValue (VCDBitVector *  value) {
        this->type = VCD_VECTOR;
        this->value.val_vector= value;
    }
    VCDValue (VCDReal value) {
        this->type = VCD_REAL;
        this->value.val_real = value;
    }
    VCDValueType   get_type() {
        return this->type;
    }
    VCDBit       get_value_bit() {
        return this->value.val_bit;
    }
    VCDBitVector * get_value_vector() {
        return this->value.val_vector;
    }
    VCDReal      get_value_real() {
        return this->value.val_real;
    }
    ~VCDValue () {
        if(this->type == VCD_VECTOR)
            delete this->value.val_vector;
    }
};

/*!
@brief Top level object to represent a single VCD file.
*/
class VCDFile {
///protected:
    //! Flat vector of all signals in the file.
    std::vector<VCDSignal*> signals;
    //! Flat mao of all scope objects in the file, keyed by name.
    std::vector<VCDScope*>  scopes;
    //! Vector of time values present in the VCD file - sorted, asc
    std::vector<VCDTime>    times;
    //! Map of hashes onto vectors of times and signal values.
    std::map<VCDSignalHash, VCDSignalValues*> val_map;
public:
    VCDFile(){ }
    ~VCDFile(){
        // Delete signals and scopes.
        for (VCDScope * scope : this->scopes) {
            for (VCDSignal * signal : scope->signals)
                delete signal;
            delete scope;
        }
        for(auto hash_val = this->val_map.begin(); hash_val != this->val_map.end(); ++hash_val) {
            for(auto vals = hash_val->second->begin(); vals != hash_val->second->end(); ++vals) {
                delete (*vals)->value;
                delete *vals;
            }
            delete hash_val->second;
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
    VCDScope * get_scope( std::string  name) {
        return nullptr;
    } 
    std::vector<VCDTime>* get_timestamps() {
        return &this->times;
    } 
    std::vector<VCDScope*>* get_scopes() {
        return &this->scopes;
    } 
    std::vector<VCDSignal*>* get_signals(){
        return &this->signals;
    } 
    void add_timestamp( VCDTime time) {
        this->times.push_back(time);
    }
};

/*!
@brief Class for parsing files containing CSP notation.
*/
class VCDFileParser {
    //! Utility function for starting parsing.
    void scan_begin ();
    //! Utility function for stopping parsing.
    void scan_end   ();

public:
    //! Create a new parser/
    VCDFileParser();
    ~VCDFileParser() {}
    
    /*!
    @brief Parse the suppled file.
    @returns A handle to the parsed VCDFile object or nullptr if parsing
    fails.
    */
    VCDFile * parse_file(const std::string & filepath);
    
    //! The current file being parsed.
    std::string filepath;
    
    //! Should we debug tokenising?
    bool trace_scanning;

    //! Should we debug parsing of tokens?
    bool trace_parsing;

    //! Reports errors to stderr.
    //void error(const VCDParser::location & l, const std::string & m);

    //! Reports errors to stderr.
    void error(const std::string & m);

    //! Current file being parsed and constructed.
    VCDFile * fh;
    
    //! Current stack of scopes being parsed.
    std::stack<VCDScope*> scopes;
};

#define YY_DECL \
    VCDParser::parser::symbol_type yylex (VCDFileParser & driver)
#endif
