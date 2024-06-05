#ifndef	MEMORY_LMSTORE_H
#define	MEMORY_LMSTORE_H

#include <stdio.h>
#include <memory>
#include <lib/cpp/List.h>
#include <lib/cpp/String.h>
#include <list>
#include <iomanip>

using namespace std;

namespace mem
{
	// Forward declaration
	class Module;

	class LMStore
	{
		public:
			/// Possible values for a lmstore block state
			enum BlockState
			{
				BlockInvalid,
				BlockModified,
				BlockExclusive,
				BlockShared
			};

			enum OpCodeType
			{
				InvalidCode=0,
				BlockNoReadAlloc=1,
				BlockAccess=2,
				BlockNoWriteDealloc=3,
				BlockReadAlloc=4,
				BlockWriteDealloc=5
			};
			/// String map for BlockState
			static const misc::StringMap BlockStateMap;

			class Block_t
			{
				private:

					friend class LMStore;

					// Block id
					unsigned BlockID;

					// Block state
					BlockState state = BlockInvalid;

					// Block size
					unsigned size;

					// physical addresses belong to the block
					// //set->lru_list.PushBack(block->lru_node);
					std::list<unsigned> addresses;

			};

			class LMRef_t
			{
				private:

					friend class LMStore;
					unsigned LMDirIndex1;
					unsigned LMDirIndex2;

				public:
					LMRef_t() :
						LMDirIndex1(0),
						LMDirIndex2(0) {}

			};

			class LMDir_t
			{
				private:

					friend class LMStore;
					unsigned LMBIndex;
					unsigned BlockID;
					unsigned BlockSize;
					bool info;
					BlockState state = BlockInvalid;
			};

			/// LMRef table size
			unsigned BlockSize;

			/// LMRef table size
			unsigned LMRefSize;

			/// LMDir table size
			unsigned LMDirSize;

			/// LMStore size
			unsigned LMStoreSize;

			/// Block descriptor size
			unsigned NumberOfBlocks = 10;

			/// A starting address to free space in the LMStorage
			unsigned LMBIndex = 0;

			/// An index to a free ientry in the LMDir table
			unsigned LMDirIndex = 0;
			// Array of blocks
			std::unique_ptr<Block_t[]> BlkDescriptor;

			// Array of blocks
			std::unique_ptr<LMRef_t[]> LMRef;

			// Array of blocks
			std::unique_ptr<LMDir_t[]> LMDir;

			// Array of blocks
			std::unique_ptr<Block_t[]> LMStorage;


			/*
			 * 001	1	Alloc
			 * 010	2	Dealloc
			 * 100	4	Access
			 * 011	3	Alloc, Dealloc
			 * 101	5	Alloc, Access
			 * 110	6	Dealloc, Access
			 * 111	7	All
			 */
			unsigned EnabledOps;

			/// Initialize a block descriptor structure.
			void initBlkDescriptor(unsigned NumberOfBlocks);

			/// Initialize a LMRef table
			void initLMRef(unsigned LMRefSize);

			/// Initialize a LMDir table
			void initLMDir(unsigned LMDirSize);

			/// Initialize a LMStorage table
			void initLMStorage(unsigned LMDirSize);

			string PhyToVirt(unsigned phyAddr) const;

			bool ServesAddress(unsigned address) const;

			Module *L1Cache;

			///Decode a virtual address
			void DecodeAddress(string VirtAddr,
					unsigned *OpCode,
					unsigned *LMRefIndex,
					unsigned *BlockID,
					unsigned *BlockSize,
					unsigned *offset);


		public:

			/// Constructor
			LMStore(unsigned BlockSize,
					unsigned LMRefSize,
					unsigned LMDirSize,
					unsigned NumberOfBlocks,
					unsigned lmsStoreSize);

			bool FindBlock(unsigned address, unsigned &blockstate);

			Block_t* getBlock(unsigned address);

			void setBlock(unsigned address, LMStore::BlockState state);

			void setBlockState(unsigned address,
					LMStore::BlockState state);

			bool allocateBlock(unsigned address);

			bool deallocateBlock(unsigned address);

			Module *getL1Cache() { return L1Cache; }

			void setL1Cache(Module *cache) { L1Cache = cache; }

			OpCodeType getOpCode(unsigned phyAddress);

			unsigned getBlockID(unsigned address);
			
			unsigned getBlockSize(unsigned address);

			unsigned getLMStoreSize() { return LMStoreSize; }

			unsigned getLMRefSize() { return LMRefSize; }

			unsigned getLMDirSize() { return LMDirSize; }

	};

}	//	namespace mem

#endif

