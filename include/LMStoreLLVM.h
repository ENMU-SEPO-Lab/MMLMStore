#ifndef AVAILSET_h
#define AVAILSET_h

#include "llvm/Pass.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/Casting.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"
#include "llvm/IR/Constants.h"
#include "llvm/Support/Debug.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/ADT/STLExtras.h"
#include <string>
#include <sstream>
#include <set>
#include <stdio.h>
#include "llvm/IR/Operator.h"
#include "llvm/ADT/Statistic.h"
#include "iostream"
#include "llvm/IR/InstIterator.h"

using namespace llvm;
using namespace std;


unsigned timestamp = 0;
unsigned lmstorage_size = 16384; // in bytes
unsigned num_lmref_entries = 64;
unsigned LMRef_index = 0;
unsigned blockSID = 0;
int section_num = 0;
set<string> sections;
map<string,string> globVar;

struct Block
{
    int lmref_index;
    unsigned numberOfEdges = 0;
    std::set<string> edges;
};

std::map<string, Block> BIG;

enum DataType
{
    isInvalid,
    isIntegerTy,
    isFloatTy,
    isDoubleTy,
    isArrayTy,
    isStructTy
};

struct variable_t
{
    string varName;
    DataType type = isInvalid;
    unsigned size = 0;
    DataType elementType = isInvalid;
    unsigned numElements = 0;
    int offset = 0;
    unsigned num_reads = 0;
    unsigned num_writes = 0;
};

struct LMStoreBlock_t
{
    string funcName;
    unsigned block_size = 0;
    unsigned num_accesses = 0;
    bool bypass = false;
    std::map<string, variable_t> variables; 
    string access_starting_addr;
    string access_segment;
    string access_section;
    string no_read_alloca_starting_addr;
    string no_read_alloca_segment;
    string no_read_alloca_section;
    string read_alloca_starting_addr;
    string read_alloca_segment;
    string read_alloca_section;
    string no_write_dealloca_starting_addr;
    string no_write_dealloca_segment;
    string no_write_dealloca_section;
    string write_dealloca_starting_addr;
    string write_dealloca_segment;
    string write_dealloca_section;
};

std::map<string, LMStoreBlock_t> lmstore_blocks;

struct node_t
{
    unsigned numberOfCalls {0};
    std::set<string> edges;
    LMStoreBlock_t block;
    std::map<string, LMStoreBlock_t> lmstoreBlock;
};

struct node
{
    string func_name;
    std::vector<string> edges;
    std::set<string> variables;
};

LMStoreBlock_t GBlock;
std::map<string, node>callGraph;

std::map<unsigned, LMStoreBlock_t> blockDescriptor;

std::map<string, node_t> CallGraph;

#endif