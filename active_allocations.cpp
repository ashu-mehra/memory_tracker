#include <set>
#include <iostream>
#include <fstream>
#include <string>
#include <regex>
#include <iomanip>
#include <assert.h>
#include "operation.hpp"

static void updateAllocationsList(set<Operation *, AddressComparator> &allocationsList, Operation *releaseInfo);
static bool isAllocation(Operation::OperationType type);
static Operation::OperationType getTypeFromFunction(string function);
static void parseOperations(const char *filename, set<Operation *, AddressComparator> &alloationsInfo);
static void dumpAllocations(const char *outfile, set<Operation *, AddressComparator> &allocationsList);

static void
updateAllocationsList(set<Operation *, AddressComparator> &allocationsList, Operation *releaseInfo)
{
	for (set<Operation *, AddressComparator>::iterator it = allocationsList.begin(); it != allocationsList.end(); ) {
		if ((releaseInfo->getStart() == (*it)->getStart()) 
			&& ((releaseInfo->getType() == Operation::FREE) || (releaseInfo->getSize() == (*it)->getSize()))
		) {
			cout << "Removing entry: " << **it << endl;
			allocationsList.erase(it);
			return;
		} else if (releaseInfo->getStart() == (*it)->getStart()) {
			assert(releaseInfo->getSize() < (*it)->getSize());
			(*it)->setStart(releaseInfo->getStart() + releaseInfo->getSize());
			(*it)->setSize((*it)->getSize() - releaseInfo->getSize());
			//((Operation )*it).setStart(releaseInfo.getStart() + releaseInfo.getSize());
			//((Operation )*it).setSize(it->getSize() - releaseInfo.getSize());
			cout << "Resized entry: " << **it << endl;
			return;
		} else if ((releaseInfo->getStart() > (*it)->getStart()) && (releaseInfo->getStart() < ((*it)->getStart() + (*it)->getSize()))) {
			assert(releaseInfo->getSize() < (*it)->getSize());
			// split the allocation and remove the region corresponding to releaseInfo
			uint64_t newSize = releaseInfo->getStart() - (*it)->getStart();
			if ((releaseInfo->getStart() + releaseInfo->getSize()) != ((*it)->getStart() + (*it)->getSize())) {
				Operation *otherPart = new Operation(**it);
				otherPart->setStart(releaseInfo->getStart() + releaseInfo->getSize());
				otherPart->setSize((*it)->getSize() - releaseInfo->getSize() - newSize);
				cout << "Adding split entry: " << otherPart << endl;
				allocationsList.insert(otherPart);
			}
			(*it)->setSize(newSize);
			cout << "Resized entry: " << **it << endl;
			return;
		}
		++it;
	}
}

static bool
isAllocation(Operation::OperationType type)
{
	return ((Operation::MALLOC == type) || (Operation::MMAP == type) || (Operation::CALLOC == type));
}

static Operation::OperationType
getTypeFromFunction(string function)
{
	if (function.find("malloc") != string::npos) {
		return Operation::MALLOC;
	} else if (function.find("calloc") != string::npos) {
		return Operation::CALLOC;
	} else if (function.find("mmap") != string::npos) {
		return Operation::MMAP;
	} else if (function.find("free") != string::npos) {
		return Operation::FREE;
	} else if (function.find("munmap") != string::npos) {
		return Operation::MUNMAP;
	}
}

static void 
parseOperations(const char *filename, set<Operation *, AddressComparator> &allocationsList)
{
	// Open the file
	string allocInfoFile(filename);
	ifstream myfile(allocInfoFile);

	// check if successfull
	if (!myfile.is_open()) {
		cerr << "Cannot open " << filename << endl;
		exit(-1);
	}

	std::regex pattern("(\\S*):\\s*0x([0-9a-f]+)\\s*0?x?([0-9a-f]*)\\s*(\\S*)\\s*(\\S*)\\s*0?x?([0-9a-f]*)");

	string line;
	while (myfile.good()) {
		getline(myfile, line);

		std::smatch result;
		cout << "line: " << line << endl;
		bool rc = std::regex_search(line, result, pattern);
		if (rc) {
#if 1 
			cout << "func: " << result.str(1) << " start: " << result.str(2);
			if (!result.str(3).empty()) {
				cout << " size: " << result.str(3);
			}
			cout << " library: " << result.str(4) << endl;
#endif
			Operation::OperationType type = getTypeFromFunction(result.str(1));
			uint64_t start = stoull(result.str(2), NULL, 16);
			uint64_t size = 0;
			uint64_t callerAddr = 0;
			if (!result.str(3).empty() && Operation::FREE != type) {
				size = stoull(result.str(3), NULL, 16);
			}
			if (!result.str(6).empty()) {
				callerAddr = stoull(result.str(6), NULL, 16);
			}

			Operation *op = new Operation(type, start, size, result.str(4), result.str(5), callerAddr);
			if (isAllocation(type)) {
				cout << "Allocation found: " << op << endl;
				allocationsList.insert(op);
			} else {
				cout << "Free memory: " << op << endl;;
				updateAllocationsList(allocationsList, op);
			}
		} else {
			cout << "regex_search returned false\n";
		}
	}
}

static void 
dumpAllocations(const char *outfile, set<Operation *, AddressComparator> &allocationsList)
{
	ofstream out;
	out.open(outfile);	
	for (const Operation *info: allocationsList) {
		out << *info << endl;
		//cout << setw(18) << hex << info.getStart() << " " <<  setw(10) << hex << info.getSize() << " at " << info.getLibrary() << " " << info.getCaller() << " " << info.getCallerAddr();
	}
}

int 
main(int argc, char *argv[])
{
	if (argc < 3) {
		fprintf(stderr, "Insufficient arguments. Pass <input file> and <output file> as arguments.\n");
		exit(-1);
	}
	set<Operation *, AddressComparator> allocationsList;
	parseOperations(argv[1], allocationsList);
	dumpAllocations(argv[2], allocationsList);
}
