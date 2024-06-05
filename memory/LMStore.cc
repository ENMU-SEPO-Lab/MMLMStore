#include "LMStore.h"
#include <map>

using namespace std;
namespace mem
{

	// Maps physical <--> virtual addresse
	std::map<unsigned, string> mem_map;

	const misc::StringMap LMStore::BlockStateMap =
	{
		{ "I", BlockInvalid },
		{ "M", BlockModified },
		{ "E", BlockExclusive },
		{ "S", BlockShared }
	};

	LMStore::LMStore(unsigned BlockSize,
			unsigned LMRefSize,
			unsigned LMDirSize,
			unsigned NumberOfBlocks,
			unsigned lmsStoreSize)
		:
			BlockSize(BlockSize),
			LMRefSize(LMRefSize),
			LMDirSize(LMDirSize),
			NumberOfBlocks(NumberOfBlocks),
			LMStoreSize(lmsStoreSize)
	{

		LMDirIndex = 0;
		LMBIndex = 0;
		LMDirIndex = 0;

		// Allocate blocks, LMRef entries, and LMDir entries
		BlkDescriptor = misc::new_unique_array<Block_t>(NumberOfBlocks);
		LMRef = misc::new_unique_array<LMRef_t>(LMRefSize);
		LMDir = misc::new_unique_array<LMDir_t>(LMDirSize);
		LMStorage = misc::new_unique_array<Block_t>(LMDirSize);

		// Initialize blocks and LMRef structures
		initBlkDescriptor(NumberOfBlocks);
		initLMRef(LMRefSize);
		initLMDir(LMDirSize);
		initLMStorage(LMDirSize);

		L1Cache = nullptr;

		// EnabledOps = ReadAlloc | Access | WriteDealoc | NoReadAlloc | NoWriteDealloc
		EnabledOps = 0x1 | 0x2 | 0x4 | 0x8 | 0x10; // for debug purpose

		/*printf("LMStore has been created successfully! Enabled Ops 0x%x\n", 
			EnabledOps);*/
	}

	void LMStore::initBlkDescriptor(unsigned NumberOfBlocks)
	{
		for(unsigned  i = 0; i < NumberOfBlocks; ++i)
		{
			BlkDescriptor[i].BlockID = 0;
			BlkDescriptor[i].state = BlockInvalid;
			BlkDescriptor[i].size = 0;
		}
	}

	void LMStore::initLMRef(unsigned LMRefSize)
	{
		for(unsigned  i = 0; i < LMRefSize; ++i)
		{
			LMRef[i].LMDirIndex1 = i;
			LMRef[i].LMDirIndex2 = 0;
		}
	}

	void LMStore::initLMDir(unsigned LMDirSize)
	{
		for(unsigned  i = 0; i < LMDirSize; ++i)
		{
			LMDir[i].LMBIndex = 0;
			LMDir[i].BlockID = 0;
			LMDir[i].BlockSize = 0;
			LMDir[i].info = false;
			LMDir[i].state = BlockInvalid;
		}
	}

	void LMStore::initLMStorage(unsigned LMDirSize)
	{
		for(unsigned  i = 0; i < LMDirSize; ++i)
		{
			BlkDescriptor[i].BlockID = 0;
			BlkDescriptor[i].state = BlockInvalid;
			BlkDescriptor[i].size = 0;
		}
	}

	string LMStore::PhyToVirt(unsigned phyAddr) const
	{
		string virtAddr = "";
		std::map<unsigned, string>::iterator it = mem_map.find(phyAddr);
		if(it != mem_map.end())
			virtAddr = it->second;
		return virtAddr;
	}


	bool LMStore::ServesAddress(unsigned phyAddress) const
	{
		string VirtAddr = PhyToVirt(phyAddress);
		if (VirtAddr.empty())
			return false;

		/*std::string prefix = string(1, VirtAddr.at(0)) 
			+ string(1, VirtAddr.at(1)) 
			+ string(1, VirtAddr.at(2));*/
		std::string prefix = string(1, VirtAddr.at(0)) 
			+ string(1, VirtAddr.at(1));
		if ( (prefix.compare("71") == 0 && EnabledOps & 0x001) ||
		     (prefix.compare("72") == 0 && EnabledOps & 0x002) ||
		     (prefix.compare("73") == 0 && EnabledOps & 0x004) ||
		     (prefix.compare("74") == 0 && EnabledOps & 0x008) ||
		     (prefix.compare("75") == 0 && EnabledOps & 0x010) ||
		     (prefix.compare("76") == 0 && EnabledOps & 0x020) )
		{
			/*printf("LMStore::ServesAddress\tphyAddress 0x%x --> VirtAddr %s\n", 
				phyAddress, VirtAddr.c_str());*/
			return true;
		}
		else
			return false;

	}

	LMStore::OpCodeType LMStore::getOpCode(unsigned phyAddress)
	{
		string VirtAddr = PhyToVirt(phyAddress);
		const char *address = VirtAddr.c_str();
		LMStore::OpCodeType opcode = (OpCodeType)(address[1]-'0');
		/*cout << "opcode: " << opcode << "; address: " << address << endl;*/
		if (address[1]-'0' == 6)
			return LMStore::BlockAccess; // special case

		if (opcode == LMStore::BlockNoReadAlloc || 
		    opcode == LMStore::BlockAccess ||
		    opcode == LMStore::BlockNoWriteDealloc ||
		    opcode == LMStore::BlockReadAlloc ||
		    opcode == LMStore::BlockWriteDealloc )
			return opcode;
		else	
			return InvalidCode;
	}

	void LMStore::DecodeAddress(string VirtAddr,
			unsigned *OpCode,
			unsigned *LMRefIndex,
			unsigned *BlockID,
			unsigned *BlockSize,
			unsigned *offset)
	{
		std::string prefix = VirtAddr.substr(0, 1);

		LMStore::OpCodeType OP;
		unsigned lmrefidx = 0;
		unsigned blockid = 0;
		unsigned blocksz = 0;
		unsigned off = 0;

		// a memory request to LMStore
		OP = (OpCodeType)(VirtAddr[1]-'0');
		if (OP == BlockReadAlloc || OP == BlockNoReadAlloc)
		{
			/* LMStore block allocation wich consists of
			 * LMRef entry, blockSID, and block size.
			 * 7102020c
			 * 71010508
			 */
			// computing the LMRef index
			std::stringstream str1;
			str1 << VirtAddr.substr(2, 2);
			str1 >> std::hex >> lmrefidx;

			// computing the Block id
			std::stringstream str2;
			str2 << VirtAddr.substr(4, 2);
			str2 >> std::hex >> blockid;

			// computing the block_size
			std::stringstream str3;
			str3 <<  VirtAddr.substr(6, 2);
			str3 >> std::hex >> blocksz;
			printf("DecodeAddress: BlockAlloc VirtAddr %s,"
					"lmrefidx = %d, blockid = %d, blocksz = %d\n",
					VirtAddr.c_str(), lmrefidx, blockid, blocksz);
		}
		else if (OP == BlockWriteDealloc || OP == BlockNoWriteDealloc)
		{
			/* LMStore block deallocation which consists of
			 * an LMRef entry and blkSID.
			 * 73010100
			 */
			// computing the LMRef index
			std::stringstream str1;
			str1 << VirtAddr.substr(2, 2);
			str1 >> std::hex >> lmrefidx;

			// computing the Block id
			std::stringstream str2;
			str2 << VirtAddr.substr(4, 1);
			str2 >> std::hex >> blockid;
			/*printf("DecodeAddress: BlockDealloc VirtAddr %s,"
					"lmrefidx = %d, blockid = %d\n",
					VirtAddr.c_str(), lmrefidx, blockid);*/

			// computing the block_size
			std::stringstream str3;
			str3 <<  VirtAddr.substr(6, 2);
			str3 >> std::hex >> blocksz;
		}
		else if (OP == BlockAccess)
		{
			/* memory request to LMStore which consists of
			 * LMRef entry, offset and size of the data.
			 * 72020000
			 */
			// computing the LMRef index
			std::stringstream str1;
			str1 << VirtAddr.substr(2, 2);
			str1 >> std::hex >> lmrefidx;

			// computing the offset
			std::stringstream str2;
			str2 << VirtAddr.substr(4, 3);
			str2 >> std::hex >> off;
			/*printf("DecodeAddress: BlockAccess VirtAddr %s,"
					"lmrefidx = %d, off = %d\n",
					VirtAddr.c_str(), lmrefidx, off);*/
		}
		else
		{
			OP = InvalidCode; 
			//printf("DecodeAddress.InvalidCode VirtAddr %s\n", VirtAddr.c_str());
		}
		if (OpCode != nullptr)	*OpCode = OP;
		if (LMRefIndex != nullptr) *LMRefIndex = lmrefidx;
		if (BlockID != nullptr) *BlockID = blockid;
		if (BlockSize != nullptr) *BlockSize = blocksz;
		if (offset != nullptr) *offset = off;

		return;
	}


	bool LMStore::FindBlock(unsigned address, unsigned &blockstate)
	{
		unsigned OpCode = 0;
		unsigned LMRefIndex = 0;
		unsigned offset = 0;

		string VirtAddr = PhyToVirt(address);

		/*cout<<"findBlock: Phyaddr 0x" << std::hex << address
		    << " virtaddr " << VirtAddr << "\n";*/

		/*if (VirtAddr.empty())
		{
			cout<<"findBlock: VirtAddr.empty \n";
			return false;
		}

		std::string prefix = string(1, VirtAddr.at(0)) 
			+ string(1, VirtAddr.at(1)) 
			+ string(1, VirtAddr.at(2));
		if (prefix.compare("073") != 0)
		{
			cout<<"findBlock: Invalid prefix \n";
			return false;
		}*/

		DecodeAddress(VirtAddr,
				&OpCode,
				&LMRefIndex,
				nullptr,
				nullptr,
				&offset);
		unsigned LMDirIdx = LMRef[LMRefIndex].LMDirIndex1;
		blockstate = (unsigned) LMDir[LMDirIdx].state;

		cout<<"findBlock: BlockID-" << LMDir[LMDirIdx].BlockID 
		    << " BlockSize " << LMDir[LMDirIdx].BlockSize << "\n";
		
		if (blockstate == 0)
		{
			/*cout<<"findBlock: return false\n";*/
			return false; 
		}
		else
		{
			/*cout<<"findBlock: return true\n";*/
			return true;
		}
	}


	LMStore::Block_t* LMStore::getBlock(unsigned address)
	{
		unsigned LMRefIndex = 0;
		string VirtAddr = PhyToVirt(address);

		/*cout<<"getBlock: Phyaddr 0x" << std::hex << address
		    << " virtaddr " << VirtAddr << "\n";*/
		DecodeAddress(VirtAddr,
				nullptr,
				&LMRefIndex,
				nullptr,
				nullptr,
				nullptr);

		unsigned LMDirIndx = LMRef[LMRefIndex].LMDirIndex1;
		unsigned LMBIndx = LMDir[LMDirIndx].LMBIndex;

		return &LMStorage[LMBIndx];
	}

	unsigned LMStore::getBlockID(unsigned addr)
	{
		return addr;
	}

	unsigned LMStore::getBlockSize(unsigned address)
	{
		unsigned LMRefIndex = 0;
		unsigned BlockID = 0;
		unsigned BlockSize = 0;

		string VirtAddr = PhyToVirt(address);
		/*cout<<"getBlockSize: Phyaddr 0x" << std::hex << address
		    << " virtaddr " << VirtAddr << "\n";*/
		if (VirtAddr.empty())
		{
			/*cout<<"getBlockSize: VirtAddr.empty() : " << address << "\n";*/
			return 0;
		}

		DecodeAddress(VirtAddr,
				nullptr,
				&LMRefIndex,
				&BlockID,
				&BlockSize,
				nullptr);

		return BlockSize;
	}

	void LMStore::setBlockState(unsigned address, LMStore::BlockState state)
	{
		unsigned LMRefIndex = 0;
		unsigned LMDirIndex = 0;

		string VirtAddr = PhyToVirt(address);
		/*cout<<"setBlockState: Phyaddr 0x" << std::hex << address
		    << " virtaddr " << VirtAddr << "\n";*/
		if (VirtAddr.empty())
			return;

		std::string prefix = string(1, VirtAddr.at(0)) 
			+ string(1, VirtAddr.at(1)); 
		//	+ string(1, VirtAddr.at(2));
		if (prefix.compare("71") != 0 && 
				prefix.compare("72") != 0 &&
				prefix.compare("73") != 0 &&
				prefix.compare("74") != 0 &&
				prefix.compare("75") != 0 &&
				prefix.compare("76") != 0)
			return;

		DecodeAddress(VirtAddr,
				nullptr,
				&LMRefIndex,
				nullptr,
				nullptr,
				nullptr);

		LMDirIndex = LMRef[LMRefIndex].LMDirIndex1;
		LMDir[LMDirIndex].state = state;

		return;
	}

	bool LMStore::allocateBlock(unsigned address)
	{
		unsigned LMRefIndex = 0;
		unsigned BlockID = 0;
		unsigned BlockSize = 0;

		string VirtAddr = PhyToVirt(address);
		/*cout<<"LMStore::allocateBlock(): Phyaddr 0x" << std::hex << address
			<< " virtaddr " << VirtAddr << "\n";*/
		if (VirtAddr.empty())
		{
			/*cout<<"allocBlock: VirtAddr.empty()\n";*/
			return false;
		}

		std::string prefix = string(1, VirtAddr.at(0)) 
			+ string(1, VirtAddr.at(1)); 
			//+ string(1, VirtAddr.at(2));
		if (prefix.compare("71") != 0 && prefix.compare("74") != 0)
		{
			/*cout<<"allocBlock: Invalid prefix " << prefix << " , " << VirtAddr << endl;*/
			return false;
		}

		DecodeAddress(VirtAddr,
				nullptr,
				&LMRefIndex,
				&BlockID,
				&BlockSize,
				nullptr);
		
		/*cout<<"allocBlock " << BlockSize << "\n";*/

		cout<<"allocBlock BlockID-" << BlockID << " BlockSize " << BlockSize << "\n";
/*
		if (LMDirIndex == (LMDirSize - 1) || 
				(LMBIndex + BlockSize) > LMStoreSize || 
				BlockSize > LMStoreSize)
			return false;
*/
		int LMDirIndx = LMRef[LMRefIndex].LMDirIndex1;
		LMDir[LMDirIndx].LMBIndex = LMBIndex;
		LMDir[LMDirIndx].BlockID = BlockID;
		LMDir[LMDirIndx].state = LMStore::BlockExclusive;
		LMDir[LMDirIndx].BlockSize = BlockSize;
		/*cout<<"allocBlock: LMRefIndex " << LMRefIndex
	                    << " BlockState " << LMDir[LMDirIndex].state << "\n";*/

		LMBIndex += BlockSize;
		//LMStoreSize -= BlockSize;
		return true;
	}

	bool LMStore::deallocateBlock(unsigned address)
	{
		unsigned LMRefIndex = 0;
		unsigned BlockID = 0;

		string VirtAddr = PhyToVirt(address);
		/*cout<<"LMStore::deallocateBlock(): Phyaddr 0x" << std::hex << address
			<< " virtaddr " << VirtAddr << "\n";*/

		if (VirtAddr.empty())
			return false;

		/*std::string prefix = string(1, VirtAddr.at(0)) 
			+ string(1, VirtAddr.at(1)) 
			+ string(1, VirtAddr.at(2));
		if (prefix.compare("072") != 0)
			return false;*/

		DecodeAddress(VirtAddr,
				nullptr,
				&LMRefIndex,
				&BlockID,
				nullptr,
				nullptr);

		unsigned LMDirIndx = LMRef[LMRefIndex].LMDirIndex1;
		unsigned LMBIndx = LMDir[LMDirIndx].LMBIndex;
		LMDir[LMDirIndx].state = LMStore::BlockInvalid;
		//LMStoreSize += LMDir[LMDirIndex].BlockSize;
		LMBIndex -= LMDir[LMDirIndex].BlockSize;
		/*cout<<"deallocBlock: LMRefIndex " << LMRefIndex
	                    << " BlockState " << LMDir[LMDirIndex].state << "\n";*/
		//LMStorage[LMBIndx].state = BlockInvalid;

		return true;
	}


}
