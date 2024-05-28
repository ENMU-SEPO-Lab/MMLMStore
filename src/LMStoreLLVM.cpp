#include "../include/LMStoreLLVM.h"

namespace {
  struct LMStorePass: public ModulePass {
    static char ID;
    LMStorePass(): ModulePass(ID) {}

    virtual bool runOnModule(Module & M);
    DataType typeCheck(llvm::Type * type);
    void DefUse(Function & F, Module & M); //create global variable
    bool allocaInsGen(Module & M, Instruction & I, string section,
      Twine varName);
    bool deallocaInsGen(Module & M, Instruction & I, string section,
      Twine varName);
    void incBlockSID();
    void incLMRefIndex();
    void blockDescriptorGen(LMStoreBlock_t block);
    bool GlbVarGen(Module & M, string func_name, LMStoreBlock_t block);
    bool codeGen(Function & F, Module & M);
    bool GlobVarGen(Module & M);
    bool dump(Function & F, Module & M);
    void iniCallGraph(Function & F);
    void printEdges();
    bool isSafe(std::set < string > edges, int lmref_index);
    void LMRefMappingEntry();
    void graphGen(string block_id, string edge);
    void Fini();
  };
}

void LMStorePass::incBlockSID() {
  blockSID += 1;
}

void LMStorePass::incLMRefIndex() {
  LMRef_index += 1;
}

void LMStorePass::blockDescriptorGen(LMStoreBlock_t block) {
  char buffer[12];
  int init = 0;
  unsigned int offset = 0;

  incBlockSID();
  incLMRefIndex();

  blockDescriptor[blockSID].funcName = block.funcName;
  //blockDescriptor[blockSID].num_accesses = 0;
  /*
   * compute  an unique virtual address for a block section
   */

  std::stringstream ss;
  //ss<< std::hex << LMRef_index; // int decimal_value
  ss << std::hex << BIG[blockDescriptor[blockSID].funcName].lmref_index; // int decimal_value
  std::string LMRefIndx(ss.str());
  if (LMRefIndx.length() < 2)
    LMRefIndx = "0" + LMRefIndx;

  snprintf(buffer, sizeof(buffer), "0x72%s%#04x", LMRefIndx.c_str(), init);
  std::string access_starting_addr = buffer;

  blockDescriptor[blockSID].access_starting_addr = access_starting_addr;
  blockDescriptor[blockSID].access_segment = block.access_segment;
  blockDescriptor[blockSID].access_section = block.access_section;

  std::map < string, variable_t > ::iterator it = block.variables.begin();

  while (it != block.variables.end()) {
    unsigned long long b;
    istringstream iss(access_starting_addr);
    iss >> hex >> b;
    b += offset;
    ss << std::hex << b;

    unsigned size = it -> second.size;
    blockDescriptor[blockSID].block_size += size;

    blockDescriptor[blockSID].variables[ss.str()].offset = offset;
    blockDescriptor[blockSID].variables[ss.str()].size = size;
    blockDescriptor[blockSID].variables[ss.str()].num_reads = it -> second.num_reads;
    blockDescriptor[blockSID].variables[ss.str()].num_writes = it -> second.num_writes;

    offset += size;
    ++it;
  }

  std::stringstream ssBlockSID;
  ssBlockSID << std::hex << blockSID; // int blockSID
  std::string blkSID(ssBlockSID.str());
  if (blkSID.length() < 2)
    blkSID = "0" + blkSID;

  std::stringstream ssBlockSize;
  ssBlockSize << std::hex << blockDescriptor[blockSID].block_size;
  std::string blkSize(ssBlockSize.str());
  if (blkSize.length() < 2)
    blkSize = "0" + blkSize;

  snprintf(buffer, sizeof(buffer), "0x71%s%s%s", LMRefIndx.c_str(),
    blkSID.c_str(), blkSize.c_str());
  std::string no_read_alloca_starting_addr = buffer;
  blockDescriptor[blockSID].no_read_alloca_starting_addr = no_read_alloca_starting_addr;
  blockDescriptor[blockSID].no_read_alloca_segment = block.no_read_alloca_segment;
  blockDescriptor[blockSID].no_read_alloca_section = block.no_read_alloca_section;

  snprintf(buffer, sizeof(buffer), "0x73%s%s%s", LMRefIndx.c_str(),
    blkSID.c_str(), blkSize.c_str());
  std::string no_write_dealloca_starting_addr = buffer;
  blockDescriptor[blockSID].no_write_dealloca_starting_addr = no_write_dealloca_starting_addr;
  blockDescriptor[blockSID].no_write_dealloca_segment = block.no_write_dealloca_segment;
  blockDescriptor[blockSID].no_write_dealloca_section = block.no_write_dealloca_section;

  snprintf(buffer, sizeof(buffer), "0x74%s%s%s", LMRefIndx.c_str(),
    blkSID.c_str(), blkSize.c_str());
  std::string read_alloca_starting_addr = buffer;
  blockDescriptor[blockSID].read_alloca_starting_addr = read_alloca_starting_addr;
  blockDescriptor[blockSID].read_alloca_segment = block.read_alloca_segment;
  blockDescriptor[blockSID].read_alloca_section = block.read_alloca_section;

  snprintf(buffer, sizeof(buffer), "0x75%s%s%#02x", LMRefIndx.c_str(),
    blkSID.c_str(), init);
  std::string write_dealloca_starting_addr = buffer;
  blockDescriptor[blockSID].write_dealloca_starting_addr = write_dealloca_starting_addr;
  blockDescriptor[blockSID].write_dealloca_segment = block.write_dealloca_segment;
  blockDescriptor[blockSID].write_dealloca_section = block.write_dealloca_section;
}

bool LMStorePass::GlbVarGen(Module & M, string func_name,
  LMStoreBlock_t block) {
  std::map < string, variable_t > ::iterator it = block.variables.begin();
  string section = block.access_section;
  string target_func = "M";

  while (it != block.variables.end()) {
    string varName = func_name + "." + it -> first;
    DataType type = it -> second.type;
    switch (type) {
    case isIntegerTy: {
      Type * I32Ty = IntegerType::get(M.getContext(), 32);
      GlobalVariable * gvI32Ty = new GlobalVariable(M,
        I32Ty,
        false,
        GlobalValue::ExternalLinkage,
        /*Initializer=*/
        0,
        varName.c_str());
      gvI32Ty -> setSection(section.c_str());
      gvI32Ty -> setAlignment(it -> second.size);
      GlobalVariable * G = M.getNamedGlobal(it -> first);
      ConstantInt * const_int32_4;
      const ConstantInt * data;
      if (func_name == target_func)
        if ((data = dyn_cast < ConstantInt > (G -> getInitializer())) &&
          (func_name == target_func)) {
          const_int32_4 = ConstantInt::get(M.getContext(),
            APInt(32,
              StringRef(to_string(data -> getSExtValue())),
              10));
          gvI32Ty -> setInitializer(const_int32_4);
          break;
        }
      const_int32_4 = ConstantInt::get(M.getContext(),
        APInt(32, StringRef("0"), 10));
      gvI32Ty -> setInitializer(const_int32_4);
    }
    break;
    case isFloatTy: {
      Type * F32Ty = Type::getFloatTy(M.getContext());
      GlobalVariable * gvF32Ty = new GlobalVariable(M,
        F32Ty,
        false,
        GlobalValue::ExternalLinkage,
        /*Initializer=*/
        0,
        varName.c_str());
      gvF32Ty -> setSection(section.c_str());
      gvF32Ty -> setAlignment(it -> second.size);
      GlobalVariable * G = M.getNamedGlobal(it -> first);
      const ConstantFP * data;
      ConstantFP * const_float_4;

      if (func_name == target_func)
        if (data = dyn_cast < ConstantFP > (G -> getInitializer())) {
          const_float_4 = ConstantFP::get(M.getContext(),
            APFloat((data -> getValueAPF()).convertToFloat()));
          gvF32Ty -> setInitializer(const_float_4);
          break;
        }

		const_float_4 = ConstantFP::get(M.getContext(), APFloat(0.000000e+00f));
      gvF32Ty -> setInitializer(const_float_4);
    }
    break;
    case isDoubleTy: {
      Type * D32Ty = Type::getDoubleTy(M.getContext());
      GlobalVariable * gvD32Ty = new GlobalVariable(M,
        D32Ty,
        false,
        GlobalValue::ExternalLinkage,
        /*Initializer=*/
        0,
        varName.c_str());
      gvD32Ty -> setSection(section.c_str());
      gvD32Ty -> setAlignment(it -> second.size);
      GlobalVariable * G = M.getNamedGlobal(it -> first);
      const ConstantFP * data;
      ConstantFP * const_Double_8;

      if (func_name == target_func)
        if (data = dyn_cast < ConstantFP > (G -> getInitializer())) {
          const_Double_8 = ConstantFP::get(M.getContext(),
            APFloat((data -> getValueAPF()).convertToDouble()));
          gvD32Ty -> setInitializer(const_Double_8);
          break;
        }

      const_Double_8 = ConstantFP::get(M.getContext(), APFloat(0.000000e+00));
      gvD32Ty -> setInitializer(const_Double_8);
    }
    break;
    case isArrayTy: {
      DataType elementType = it -> second.elementType;
      unsigned numElements = it -> second.numElements;
      switch (elementType) {
      case isIntegerTy: {
        ArrayType * ArrayTy_1 =
          ArrayType::get(IntegerType::get(M.getContext(),
            32), numElements);
        GlobalVariable * gvar_array_myarr =
          new GlobalVariable(M,
            /*Type=*/
            ArrayTy_1,
            /*isConstant=*/
            false,
            /*Linkage=*/
            GlobalValue::ExternalLinkage,
            /*Initializer=*/
            0,
            varName.c_str());
        gvar_array_myarr -> setSection(section.c_str());
        gvar_array_myarr -> setAlignment(4);
        GlobalVariable * G = M.getNamedGlobal(it -> first);
        const ConstantDataArray * data;

        if (func_name == target_func)
          if (data = dyn_cast < ConstantDataArray > (G -> getInitializer())) {

            // Constant Definitions
            std::vector < Constant * > const_array_3_elems;
            for (int k = 0, ed = data -> getNumElements(); k < ed; ++k) {
              const_array_3_elems.push_back(
                ConstantInt::get(M.getContext(),
                  APInt(32, StringRef(to_string(
                      data -> getElementAsInteger(k))),
                    10)));
            }
            // Global Variable Definitions
            Constant * const_array_14 = ConstantArray::get(ArrayTy_1,
              const_array_3_elems);
            gvar_array_myarr -> setInitializer(const_array_14);
            break;
          }
        ConstantAggregateZero * const_array_14 =
          ConstantAggregateZero::get(ArrayTy_1);
        gvar_array_myarr -> setInitializer(const_array_14);
      }
      break;
      case isFloatTy: {
        ArrayType * ArrayTy_0 =
          ArrayType::get(Type::getFloatTy(M.getContext()),
            numElements);
        GlobalVariable * gvar_array_myarr =
          new GlobalVariable( /*Module=*/ M,
            /*Type=*/
            ArrayTy_0,
            /*isConstant=*/
            false,
            /*Linkage=*/
            GlobalValue::ExternalLinkage,
            /*Initializer=*/
            0,
            varName.c_str());
        gvar_array_myarr -> setSection(section.c_str());
        gvar_array_myarr -> setAlignment(4);
        GlobalVariable * G = M.getNamedGlobal(it -> first);
        const ConstantDataArray * data;

        if (func_name == target_func)
          if (data = dyn_cast < ConstantDataArray > (G -> getInitializer())) {
            // Constant Definitions
            std::vector < Constant * > const_array_3_elems;
            for (int k = 0, ed = data -> getNumElements(); k < ed; ++k)
              const_array_3_elems.push_back(
                ConstantFP::get(M.getContext(),
                  APFloat(data -> getElementAsFloat(k))));

            // Global Variable Definitions
            Constant * const_array_14 = ConstantArray::get(ArrayTy_0,
              const_array_3_elems);
            gvar_array_myarr -> setInitializer(const_array_14);
            break;
          }
        ConstantAggregateZero * const_array_14 =
          ConstantAggregateZero::get(ArrayTy_0);
        gvar_array_myarr -> setInitializer(const_array_14);
      }
      break;
      case isDoubleTy: {
        ArrayType * ArrayTy_0 =
          ArrayType::get(Type::getDoubleTy(M.getContext()),
            numElements);
        GlobalVariable * gvar_array_myarr =
          new GlobalVariable( /*Module=*/ M,
            /*Type=*/
            ArrayTy_0,
            /*isConstant=*/
            false,
            /*Linkage=*/
            GlobalValue::ExternalLinkage,
            /*Initializer=*/
            0,
            varName.c_str());
        gvar_array_myarr -> setSection(section.c_str());
        gvar_array_myarr -> setAlignment(8);
        GlobalVariable * G = M.getNamedGlobal(it -> first);
        const ConstantDataArray * data;

        if (func_name == target_func)
          if (data = dyn_cast < ConstantDataArray > (G -> getInitializer())) {
            // Constant Definitions
            std::vector < Constant * > const_array_3_elems;
            for (int k = 0, ed = data -> getNumElements(); k < ed; ++k)
              const_array_3_elems.push_back(
                ConstantFP::get(M.getContext(),
                  APFloat(data -> getElementAsDouble(k))));

            // Global Variable Definitions
            Constant * const_array_14 =
              ConstantArray::get(ArrayTy_0, const_array_3_elems);
            gvar_array_myarr -> setInitializer(const_array_14);
            break;
          }
        ConstantAggregateZero * const_array_14 =
          ConstantAggregateZero::get(ArrayTy_0);
        gvar_array_myarr -> setInitializer(const_array_14);
      }
      break;
      default: {
        //assert(false);
      }
      }

    }
    break;
    case isStructTy: {}
    break;
    default: {
      //assert(false);
      errs() << "invalid type\n";
    }
    }
    it -> second.varName = varName.c_str();
    globVar.insert(std::pair < string, string > (
      it -> first, varName.c_str()));
    ++it;
  }
  return true;
}

bool LMStorePass::allocaInsGen(Module & M, Instruction & I, string section, Twine varName) {
  errs() << "section: " << section << "\n";
  GlobalVariable * gvar_int32_myvar1 = new GlobalVariable( /*Module=*/ M,
    /*Type=*/
    IntegerType::get(M.getContext(), 32),
    /*isConstant=*/
    false,
    /*Linkage=*/
    GlobalValue::ExternalLinkage,
    /*Initializer=*/
    0,
    /*Name=*/
    varName);
  gvar_int32_myvar1 -> setSection(section.c_str());
  gvar_int32_myvar1 -> setAlignment(4);
  ConstantInt * const_int32_2 = ConstantInt::get(M.getContext(), APInt(32, StringRef("0"), 10));
  gvar_int32_myvar1 -> setInitializer(const_int32_2);
  Instruction * pInst = & I;
  LoadInst * int32_9 = new LoadInst(gvar_int32_myvar1, "AI_" + varName, true, pInst);
  int32_9 -> setAlignment(4);
  return true;
}

bool LMStorePass::deallocaInsGen(Module & M, Instruction & I, string section, Twine varName) {
  GlobalVariable * gvar_int32_myvar1 = new GlobalVariable( /*Module=*/ M,
    /*Type=*/
    IntegerType::get(M.getContext(), 32),
    /*isConstant=*/
    false,
    /*Linkage=*/
    GlobalValue::ExternalLinkage,
    /*Initializer=*/
    0,
    /*Name=*/
    varName);
  gvar_int32_myvar1 -> setSection(section.c_str());
  gvar_int32_myvar1 -> setAlignment(4);
  ConstantInt * const_int32_2 = ConstantInt::get(M.getContext(), APInt(32, StringRef("0"), 10));
  gvar_int32_myvar1 -> setInitializer(const_int32_2);
  Instruction * pInst = & I;
  LoadInst * int32_9 = new LoadInst(gvar_int32_myvar1, "DI_" + varName, true, pInst);
  int32_9 -> setAlignment(4);
  return true;
}

bool LMStorePass::codeGen(Function & F, Module & M) {
  bool modified = false;
  map < string, LMStoreBlock_t > ::iterator it = lmstore_blocks.find(F.getName());

  unsigned currBlock_size = it -> second.block_size;
  if (currBlock_size > lmstorage_size) {
    it -> second.bypass = true;
    return modified;
  }

  if (currBlock_size == 0)
    it -> second.bypass = true;

  lmstorage_size -= it -> second.block_size;

  it -> second.funcName = (F.getName()).str();
  it -> second.access_segment = "." + (F.getName()).str() + "MemAccessSegment";
  it -> second.access_section = "." + (F.getName()).str() + "MemAccessSection";
  it -> second.no_read_alloca_segment = "." + (F.getName()).str() + "NoReadAllocaSegment";
  it -> second.no_read_alloca_section = "." + (F.getName()).str() + "NoReadAllocaSection";
  it -> second.read_alloca_segment = "." + (F.getName()).str() + "ReadAllocaSegment";
  it -> second.read_alloca_section = "." + (F.getName()).str() + "ReadAllocaSection";
  it -> second.no_write_dealloca_segment = "." + (F.getName()).str() + "NoWriteDeallocaSegment";
  it -> second.no_write_dealloca_section = "." + (F.getName()).str() + "NoWriteDeallocaSection";
  it -> second.write_dealloca_segment = "." + (F.getName()).str() + "WriteDeallocaSegment";
  it -> second.write_dealloca_section = "." + (F.getName()).str() + "WriteDeallocaSection";

  modified = GlbVarGen(M, (F.getName()).str(), it -> second);
  blockDescriptorGen(it -> second);

  bool first_block = false;

  for (BasicBlock & BB: F) {
    for (Instruction & I: BB) {
      if (first_block == false) {
        if (F.getName() == "main" && GBlock.bypass == false) {
          string section = ".MNoReadAllocaSection";
          Twine varName = "NoReadAlloca.M";
          first_block = allocaInsGen(M, I, section, varName);
        }

        if (currBlock_size != 0) {
          string section = it -> second.no_read_alloca_section;
          Twine varName = "NoReadAlloca." + F.getName();
          first_block = allocaInsGen(M, I, section, varName);
        }
      }
      if (auto * SI = dyn_cast < StoreInst > ( & I)) {
        for (User::op_iterator OI = I.op_begin(), E = I.op_end(); OI != E; ++OI) {
          Value * val = * OI;
          map < string, string > ::iterator it2;
          it2 = globVar.find(val -> getName());
          if (it2 != globVar.end())
            *
            OI = (Value * ) M.getGlobalVariable(it2 -> second);
        }
      } else if (auto * LI = dyn_cast < LoadInst > ( & I)) {
        for (User::op_iterator OI = I.op_begin(), E = I.op_end(); OI != E; ++OI) {
          Value * val = * OI;
          map < string, string > ::iterator it2;
          it2 = globVar.find(val -> getName());
          if (it2 != globVar.end())
            *
            OI = (Value * ) M.getGlobalVariable(it2 -> second);
        }
      } else if (auto * GI = dyn_cast < GetElementPtrInst > ( & I)) {
        for (User::op_iterator OI = I.op_begin(), E = I.op_end(); OI != E; ++OI) {
          Value * val = * OI;
          map < string, string > ::iterator it2;
          it2 = globVar.find(val -> getName());
          if (it2 != globVar.end())
            *
            OI = (Value * ) M.getGlobalVariable(it2 -> second);
        }
      } else if (auto * RI = dyn_cast < ReturnInst > ( & I)) {
        bool last_block;
        if (F.getName() == "main" && GBlock.bypass == false) {
          string section = ".MNoWriteDeallocaSection";
          Twine varName = "NoWriteDealloca.M";
          last_block = deallocaInsGen(M, I, section, varName);
        }

        if (currBlock_size != 0) {
          string section = it -> second.no_write_dealloca_section;
          Twine varName = "NoWriteDealloca." + F.getName();
          last_block = deallocaInsGen(M, I, section, varName);
        }
      }
    }
  }
  return true;
}

bool LMStorePass::dump(Function & F, Module & M) {
  map < string, LMStoreBlock_t > ::iterator it = lmstore_blocks.find(F.getName());
  it -> second.funcName = (F.getName()).str();
  it -> second.access_segment = "." + (F.getName()).str() + "MemAccessSegment";
  it -> second.access_section = "." + (F.getName()).str() + "MemAccessSection";
  it -> second.no_read_alloca_segment = "." + (F.getName()).str() + "NoReadAllocaSegment";
  it -> second.no_read_alloca_section = "." + (F.getName()).str() + "NoReadAllocaSection";
  it -> second.no_write_dealloca_segment = "." + (F.getName()).str() + "NoWriteDeallocaSegment";
  it -> second.no_write_dealloca_section = "." + (F.getName()).str() + "NoWriteDeallocaSection";

  bool first_block = false;
  for (BasicBlock & BB: F) {
    for (Instruction & I: BB) {
      if (first_block == false) {
        string section = "." + (F.getName()).str() + "NoReadAllocaBlock";
        Twine varName = "NoReadAlloca." + F.getName();
        first_block = allocaInsGen(M, I, section, varName);
      }
      if (auto * SI = dyn_cast < StoreInst > ( & I)) {
        for (User::op_iterator OI = I.op_begin(), E = I.op_end(); OI != E; ++OI) {
          Value * val = * OI;
          map < string, string > ::iterator it2;
          it2 = globVar.find(val -> getName());
          if (it2 != globVar.end())
            *
            OI = (Value * ) M.getGlobalVariable(it2 -> second);
        }
      } else if (auto * LI = dyn_cast < LoadInst > ( & I)) {
        for (User::op_iterator OI = I.op_begin(), E = I.op_end(); OI != E; ++OI) {
          Value * val = * OI;
          map < string, string > ::iterator it2;
          it2 = globVar.find(val -> getName());
          if (it2 != globVar.end())
            *
            OI = (Value * ) M.getGlobalVariable(it2 -> second);
        }
      } else if (auto * GI = dyn_cast < GetElementPtrInst > ( & I)) {
        for (User::op_iterator OI = I.op_begin(), E = I.op_end(); OI != E; ++OI) {
          Value * val = * OI;
          map < string, string > ::iterator it2;
          it2 = globVar.find(val -> getName());
          if (it2 != globVar.end())
            *
            OI = (Value * ) M.getGlobalVariable(it2 -> second);
        }
      } else if (auto * RI = dyn_cast < ReturnInst > ( & I)) {
        string section = "." + (F.getName()).str() + "NoWriteDeallocaBlock";
        Twine varName = "NoWriteDealloca." + F.getName();
        bool last_block = deallocaInsGen(M, I, section, varName);
      }
    }
  }

  first_block = false;
  for (BasicBlock & BB: F) {
    for (Instruction & I: BB) {
      if (first_block == false) {
        string section = "." + (F.getName()).str() + "DumpBlock";
        Twine varName = "Dump." + F.getName();
        first_block = allocaInsGen(M, I, section, varName);
      }
    }
  }

  return true;
}

DataType LMStorePass::typeCheck(llvm::Type * type) {
  DataType tyCode;
  if (type -> isIntegerTy())
    tyCode = isIntegerTy;
  else if (type -> isFloatTy())
    tyCode = isFloatTy;
  else if (type -> isDoubleTy())
    tyCode = isDoubleTy;
  else if (type -> isArrayTy())
    tyCode = isArrayTy;
  else if (type -> isStructTy())
    tyCode = isStructTy;
  else
    tyCode = isInvalid;

  return tyCode;
}

void LMStorePass::iniCallGraph(Function & F) {
  for (BasicBlock & BB: F) {
    for (Instruction & I: BB) {
      if (CallInst * call_inst = dyn_cast < CallInst > ( & I)) {
        Function * fn = call_inst -> getCalledFunction();
        string func_name = fn -> getName();
        if (std::find(callGraph[F.getName()].edges.begin(),
            callGraph[F.getName()].edges.end(), func_name) ==
          callGraph[F.getName()].edges.end()) {
          callGraph[F.getName()].edges.push_back(func_name);
        }
      }
    }
  }
}

void LMStorePass::DefUse(Function & F, Module & M) {
  LMStoreBlock_t block;
  block.funcName = (F.getName()).str();
  for (BasicBlock & BB: F) {
    for (Instruction & I: BB) {
      if (auto * AI = dyn_cast < AllocaInst > ( & I)) {
        Type * AllocaTy = AI -> getAllocatedType();
        string varName = AI -> getName();
        std::map < string, variable_t > ::iterator it =
          block.variables.find(varName);
        DataType tyCode = typeCheck(AllocaTy);
        if (varName.size() > 0 && it == block.variables.end() && tyCode != 0) {
          variable_t
          var;
          var.type = tyCode;
          if (tyCode == isArrayTy) {
            llvm::Type * Ty = cast < ArrayType > (AllocaTy) -> getElementType();
            var.elementType = typeCheck(Ty);
            var.numElements = cast < ArrayType > (AllocaTy) -> getNumElements();
          }
          var.size = M.getDataLayout().getTypeAllocSize(AllocaTy);
          block.block_size += M.getDataLayout().getTypeAllocSize(AllocaTy);
          block.variables.insert(std::pair < string, variable_t > (varName,
            var));
        }
      } else if (auto * LI = dyn_cast < LoadInst > ( & I)) {
        for (unsigned i = 0, e = I.getNumOperands(); i != e; i++) {
          string varName = I.getOperand(i) -> getName();
          if (varName.size() > 0)
            callGraph[F.getName()].variables.insert(varName);
          std::map < string, variable_t > ::iterator it =
            block.variables.find(varName);
          if (varName.size() > 0 && it != block.variables.end()) {
            it -> second.num_reads += 1;
            block.num_accesses += 1;
          }
        }
      } else if (auto * SI = dyn_cast < StoreInst > ( & I)) {
        for (unsigned i = 0, e = I.getNumOperands(); i != e; i++) {
          string varName = I.getOperand(i) -> getName();
          if (varName.size() > 0)
            callGraph[F.getName()].variables.insert(varName);
          std::map < string, variable_t > ::iterator it =
            block.variables.find(varName);
          if (varName.size() > 0 && it != block.variables.end()) {
            it -> second.num_writes += 1;
            block.num_accesses += 1;
          }
        }
      } else if (auto * GI = dyn_cast < GetElementPtrInst > ( & I)) {
        for (User::op_iterator OI = I.op_begin(), E = I.op_end(); OI != E; ++OI) {
          Value * val = * OI;
          string varName = val -> getName();
          if (varName.size() > 0)
            callGraph[F.getName()].variables.insert(varName);
        }
      } else if (CallInst * call_inst = dyn_cast < CallInst > ( & I)) {
        Function * fn = call_inst -> getCalledFunction();
        string fn_name = fn -> getName();
        if (std::find(callGraph[F.getName()].edges.begin(),
            callGraph[F.getName()].edges.end(), fn_name) ==
          callGraph[F.getName()].edges.end()) {
          callGraph[F.getName()].edges.push_back(fn_name);
        }
      }
    }
  }

  std::map < string, LMStoreBlock_t > ::iterator it = lmstore_blocks.begin();
  it = lmstore_blocks.find(F.getName());
  if (it != lmstore_blocks.end())
    lmstore_blocks.erase(it);

  lmstore_blocks.insert(std::pair < string, LMStoreBlock_t > (F.getName(), block));
}

void LMStorePass::Fini() {
  std::map < unsigned, LMStoreBlock_t > ::iterator it = blockDescriptor.begin();
  FILE * linkld;

  linkld = fopen("link.ld", "w");
  fprintf(linkld, "SECTIONS\n{\n");

  while (it != blockDescriptor.end()) {
    if (it -> second.block_size > 0 && it -> second.bypass == false) {
      std::string address = it -> second.access_starting_addr;
      string segment = it -> second.access_segment;
      string section = it -> second.access_section;
      fprintf(linkld, "\t%s %s : {KEEP(*(%s))}\n",
        segment.c_str(),
        address.c_str(),
        section.c_str()
      );

      address = it -> second.no_read_alloca_starting_addr;
      segment = it -> second.no_read_alloca_segment;
      section = it -> second.no_read_alloca_section;
      fprintf(linkld, "\t%s %s : {KEEP(*(%s))}\n",
        segment.c_str(),
        address.c_str(),
        section.c_str()
      );
      address = it -> second.read_alloca_starting_addr;
      segment = it -> second.read_alloca_segment;
      section = it -> second.read_alloca_section;
      fprintf(linkld, "\t%s %s : {KEEP(*(%s))}\n",
        segment.c_str(),
        address.c_str(),
        section.c_str()
      );
      address = it -> second.no_write_dealloca_starting_addr;
      segment = it -> second.no_write_dealloca_segment;
      section = it -> second.no_write_dealloca_section;
      fprintf(linkld, "\t%s %s : {KEEP(*(%s))}\n",
        segment.c_str(),
        address.c_str(),
        section.c_str()
      );
      address = it -> second.write_dealloca_starting_addr;
      segment = it -> second.write_dealloca_segment;
      section = it -> second.write_dealloca_section;
      fprintf(linkld, "\t%s %s : {KEEP(*(%s))}\n",
        segment.c_str(),
        address.c_str(),
        section.c_str()
      );
    }
    ++it;
  }

  if (GBlock.block_size > 0 && GBlock.bypass == false) {
    char buffer[12];
    int init = 0;
    std::stringstream ss;
    ss << std::hex << 0; // int decimal_value
    std::string LMRefIndx(ss.str());
    if (LMRefIndx.length() < 2)
      LMRefIndx = "0" + LMRefIndx;

    std::stringstream ssBlockSize;
    ssBlockSize << std::hex << GBlock.block_size;
    std::string blkSize(ssBlockSize.str());
    if (blkSize.length() < 2)
      blkSize = "0" + blkSize;

    snprintf(buffer, sizeof(buffer), "0x72%s%#04x", "00", init);
    std::string access_starting_addr = buffer;
    string segment = ".MMemAccessSegment";
    string section = ".MMemAccessSection";
    fprintf(linkld, "\t%s %s : {KEEP(*(%s))}\n",
      segment.c_str(),
      access_starting_addr.c_str(),
      section.c_str()
    );
    snprintf(buffer, sizeof(buffer), "0x71%s%s%s", "00",
      "64", blkSize.c_str());
    access_starting_addr = buffer;
    segment = ".MNoReadAllocaSegment";
    section = ".MNoReadAllocaSection";
    fprintf(linkld, "\t%s %s : {KEEP(*(%s))}\n",
      segment.c_str(),
      access_starting_addr.c_str(),
      section.c_str()
    );

    snprintf(buffer, sizeof(buffer), "0x73%s%s%s", "00",
      "64", blkSize.c_str());
    access_starting_addr = buffer;
    segment = ".MNoWriteDeallocaSegment";
    section = ".MNoWriteDeallocaSection";
    fprintf(linkld, "\t%s %s : {KEEP(*(%s))}\n",
      segment.c_str(),
      access_starting_addr.c_str(),
      section.c_str()
    );
  }
  fprintf(linkld, "\n}");
  fclose(linkld);
}

void LMStorePass::graphGen(string block_id, string edge) {
  BIG[block_id].edges.insert(edge);
  BIG[block_id].numberOfEdges++;
}

bool LMStorePass::isSafe(std::set < string > edges, int lmref_index) {
  for (auto e: edges) {
    if (BIG[e].lmref_index == lmref_index)
      return false;
  }

  return true;
}

void LMStorePass::LMRefMappingEntry() {
  std::map < string, Block > ::iterator it = BIG.begin();
  while (it != BIG.end()) {
    std::set < string > edges(it -> second.edges);
    string block_id = it -> first;
    for (int lmref_index = 1; lmref_index < num_lmref_entries; ++lmref_index) {
      if (isSafe(edges, lmref_index)) {
        BIG[block_id].lmref_index = lmref_index;
        break;
      }
    }
    ++it;
  }
}

void LMStorePass::printEdges() {
  std::map < string, Block > ::iterator it = BIG.begin();
  while (it != BIG.end()) {
    std::set < string > edges(it -> second.edges);
    for (auto e: edges)
      errs() << e << ":" << BIG[e].lmref_index << "\t";
    errs() << "\n";
    ++it;
  }
}

bool LMStorePass::GlobVarGen(Module & M) {
  bool modified = false;
  BIG["M"].lmref_index = 0;
  for (auto & GV: M.getGlobalList()) {
    string varName = GV.getName();
    const ConstantDataArray * data;

    if (varName.size()) {
      bool alloc = true;
      variable_t
      var;
      Type * AllocaTy = GV.getValueType();
      DataType tyCode = typeCheck(AllocaTy);
      var.type = tyCode;
      if (var.elementType == 5)
        alloc = false;
      if (tyCode == isArrayTy) {
        llvm::Type * Ty = cast < ArrayType > (AllocaTy) -> getElementType();
        var.elementType = typeCheck(Ty);
        var.numElements = cast < ArrayType > (AllocaTy) -> getNumElements();
        if (var.elementType == 5)
          alloc = false;
      }
      var.size = M.getDataLayout().getTypeAllocSize(AllocaTy);
      if (tyCode == isArrayTy &&
        var.numElements > 20)
        alloc = false;

      if (alloc == true) {
        GBlock.block_size += M.getDataLayout().getTypeAllocSize(AllocaTy);
        GBlock.variables.insert(
          std::pair < string, variable_t > (varName,
            var)
        );
      } else {
        errs() << "and it is excluded\n";
      }
    }
  }

  if (GBlock.block_size > lmstorage_size) {
    GBlock.bypass = true;
    return modified;
  }

  lmstorage_size -= GBlock.block_size;

  GBlock.access_section = ".MMemAccessSection";
  GBlock.access_segment = ".MMemAccessSegment";
  modified = GlbVarGen(M, "M", GBlock);
  return modified;
}

bool LMStorePass::runOnModule(Module & M) {
  GlobalVariable * gvar_int32_i = new GlobalVariable(
    /*Module=*/
    M,
    /*Type=*/
    IntegerType::get(M.getContext(), 32),
    /*isConstant=*/
    false,
    /*Linkage=*/
    GlobalValue::ExternalLinkage,
    /*Initializer=*/
    0, // has initializer, specified below
    /*Name=*/
    "ryan_seraj"
  );
  gvar_int32_i -> setAlignment(4);

  // Constant Definitions
  ConstantInt * const_int32_2 = ConstantInt::get(M.getContext(), APInt(32, StringRef("10"), 10));

  // Global Variable Definitions
  gvar_int32_i -> setInitializer(const_int32_2);
  bool modified = false;

  modified = GlobVarGen(M);

  for (auto F = M.begin(); F != M.end(); F++) {
    iniCallGraph( * F);
    for (auto v: callGraph[F -> getName()].edges) {
      graphGen(F -> getName(), v);
      graphGen(v, F -> getName());
    }
  }
  LMRefMappingEntry();

  for (auto F = M.begin(); F != M.end(); F++) {
    DefUse( * F, M);
    modified = codeGen( * F, M);
  }

  Fini();
  return modified;
}

char LMStorePass::ID = 0;

// Register the pass so `opt -lmstore` runs it.
static RegisterPass < LMStorePass > X("lmstore", "lmstore pass");
