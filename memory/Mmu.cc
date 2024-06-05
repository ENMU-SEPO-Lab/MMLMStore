/*
 *  Multi2Sim
 *  Copyright (C) 2012  Rafael Ubal (ubal@ece.neu.edu)
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <cassert>

#include <lib/cpp/CommandLine.h>
#include <lib/cpp/String.h>

#include "Memory.h"
#include "Mmu.h"


namespace mem
{


	//
	// Class 'Mmu::Space'
	//

	extern map<unsigned, string> mem_map;

	Mmu::Space::Space(const std::string &name, Mmu *mmu) :
		name(name),
		mmu(mmu)
	{
		// Debug
		debug << misc::fmt("[MMU %s] Space %s created\n",
				mmu->getName().c_str(),
				name.c_str());
	}


	void Mmu::Space::addPage(Page *page)
	{
		// Sanity
		unsigned virtual_address = page->getVirtualAddress();
		assert((virtual_address & ~PageMask) == 0);
		assert(virtual_pages.find(virtual_address) == virtual_pages.end());
		virtual_pages[virtual_address] = page;
	}


	Mmu::Page *Mmu::Space::getPage(unsigned virtual_address)
	{
		assert((virtual_address & ~PageMask) == 0);
		auto it = virtual_pages.find(virtual_address);
		return it == virtual_pages.end() ? nullptr : it->second;
	}




	//
	// Class 'Mmu'
	//

	std::string Mmu::debug_file;

	misc::Debug Mmu::debug;


	void Mmu::RegisterOptions()
	{
		// Get command line object
		misc::CommandLine *command_line = misc::CommandLine::getInstance();

		// Category
		command_line->setCategory("Memory");

		// Option --mmu-debug <file>
		command_line->RegisterString("--mmu-debug <file>", debug_file,
				"Dump debug information related with the memory "
				"management unit, virtual/physical memory address "
				"spaces, and address translations.");
	}


	void Mmu::ProcessOptions()
	{
		// Debug file
		if (!debug_file.empty())
			debug.setPath(debug_file);
	}


	Mmu::Mmu(const std::string &name) :
		name(name)
	{
		// Debug
		debug << misc::fmt("[MMU %s] Memory management unit created\n",
				name.c_str());
	}


	Mmu::Space *Mmu::newSpace(const std::string &name)
	{
		spaces.emplace_back(new Space(name, this));
		return spaces.back().get();
	}


	unsigned Mmu::TranslateVirtualAddress(Space *space,
			unsigned virtual_address)
	{
		// Space must belong to current MMU
		assert(space->getMmu() == this);

		// Calculate tag and offset
		unsigned virtual_tag = virtual_address & PageMask;
		unsigned page_offset = virtual_address & ~PageMask;

		// Find page, and created if not found
		Page *page = space->getPage(virtual_tag);
		if (page == nullptr)
		{
			// Create new page
			pages.emplace_back(new Page(space, virtual_tag,
						top_physical_address));

			// Add page to virtual and physical maps
			page = pages.back().get();
			physical_pages[page->getPhysicalAddress()] = page;
			space->addPage(page);

			// Increment top of physical address space
			top_physical_address += PageSize;

			// Debug
			if (debug)
				debug << misc::fmt("[MMU %s] Page created. "
						"Space %s, Virtual 0x%x => "
						"Physical 0x%x\n", name.c_str(),
						space->getName().c_str(),
						virtual_tag,
						page->getPhysicalAddress());
		}

		// Calculate physical address
		unsigned physical_address = page->getPhysicalAddress() + page_offset;

		// Debug
		if (debug)
			debug << misc::fmt("[MMU %s] Space %s, Virtual 0x%x => "
					"Physical 0x%x\n", name.c_str(),
					space->getName().c_str(),
					virtual_address,
					physical_address);

		//LMStore
		std::stringstream stream;
		stream << std::setfill ('0') 
			<< std::setw(sizeof(virtual_address)*2) 
			<< std::hex << virtual_address;

		/*std::string address(stream.str());
		std::string prefix = string(1, address.at(0)) + 
			string(1, address.at(1)) +
			string(1, address.at(2));

		if (prefix.compare("071") == 0 ||
				prefix.compare("072") == 0 ||
				prefix.compare("073") == 0)
		{
			mem_map[physical_address] = address.c_str();
		}*/

		std::string address(stream.str());
		std::string prefix = string(1, address.at(0)) + 
			string(1, address.at(1));

		if (prefix.compare("71") == 0 ||
				prefix.compare("72") == 0 ||
				prefix.compare("73") == 0 ||
				prefix.compare("74") == 0 ||
				prefix.compare("75") == 0 ||
				prefix.compare("76") == 0)
		{
			mem_map[physical_address] = address.c_str();
		}
		//fflush(stdout);
		/*	
		}
		std::stringstream stream;
		stream << std::setfill ('0')
			<< std::setw(sizeof(virtual_address)*2)
			<< std::hex << virtual_address;
		std::string address( stream.str() );
		std::string prefix = string(1, address.at(0)) + string(1,
				address.at(1));
		if (prefix.compare("071") == 0 ||
				prefix.compare("072") == 0 ||
				prefix.compare("073") == 0)
		{
			std::map<unsigned, unsigned>::iterator it = mem_map.find(
					physical_address);
			if (it == mem_map.end())
			{
				mem_map.insert(std::pair<unsigned, unsigned>(physical_address,
							virtual_address));
				printf("Mmu.cc\tmem_map.size():\t%d.\n", mem_map.size());
				it = mem_map.begin();
				int i = 0;
				while(it != mem_map.end())
				{
					std::stringstream stream;
					stream << std::setfill ('0') << std::setw(sizeof(it->second)*2)
						<< std::hex << it->second;
					std::string address( stream.str() );
					printf("%d:\tPhyAddress:\t%d\tVirtAddressi(1)\t%d\tVirtAddressi(2)\t%s.\n", i, it->first, it->second, address.c_str());
					++it;
					++i;
				}
			}
		}
		//std::stringstream stream;
		//stream << std::setfill ('0') << std::setw(sizeof(virtual_address)*2)
		//	<< std::hex << virtual_address;
		//std::string address( stream.str() );

		//printf("TranslateVirtualAddress: physical_address 0x%x virtual_address %s \n",
		//		physical_address, address.c_str()); 
		*/
	   return physical_address;
	}


	bool Mmu::TranslatePhysicalAddress(unsigned physical_address,
			Space *&space,
			unsigned &virtual_address)
	{
		// Find page
		unsigned physical_tag = physical_address & PageMask;
		unsigned page_offset = physical_address & ~PageMask;
		auto it = physical_pages.find(physical_tag);

		// Page not found
		if (it == physical_pages.end())
		{
			// Debug
			if (debug)
				debug << misc::fmt("[MMU %s] Physical 0x%x => "
						"Invalid page\n", name.c_str(),
						physical_address);

			// No page found
			space = nullptr;
			virtual_address = 0;
			return false;
		}

		// Return page information
		Page *page = it->second;
		space = page->getSpace();
		virtual_address = page->getVirtualAddress() + page_offset;

		// Debug
		if (debug)
			debug << misc::fmt("[MMU %s] Physical 0x%x => "
					"Space %s, Virtual 0x%x\n",
					name.c_str(),
					physical_address,
					space->getName().c_str(),
					virtual_address);

		// Page found
		return true;
	}


	bool Mmu::isValidPhysicalAddress(unsigned physical_address)
	{
		unsigned physical_tag = physical_address & PageMask;
		auto it = physical_pages.find(physical_tag);
		return it != physical_pages.end();
	}


} // namespace mem

