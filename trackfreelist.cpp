#include <iostream>
#include <fstream>

static void
addToFreeList(set<Operation *, AddressComparator> &freeList, Operation *operation)
{
	Oparation *op = operation;
	bool repeat = true;
	int i = 0;

	while (repeat) {
		repeat = false;
		for (Operation *freeOp : freeList) {
			if (op->getStart() + op->getSize() == freeOp->getStart()) {
				freeOp->setStart(op->getStart());
				freeOp->setSize(op->getSize() + freeOp->getSize());
				op = freeOp;
				repeat = true;
				i += 1;
				break;
			} else if (freeOp->getStart() + freeOp->getSize() == op->getStart()) {
				freeOp->setSize(op->getSize() + freeOp->getSize());
				op = freeOp;
				repeat = true;
				i += 1;
				break;
			}
		}
	}
	if (i == 0) {
		freeList.insert(op);
	}
	return;
}

static void
updateFreeList(set<Operation *, AddressComparator> &freeList, Operation *op)
{
	for (Operation *freeOp : freeList) {
		uint64_t allocStart = op->getStart();
		uint64_t allocSize = op->getSize();
		if ((allocStart >= freeOp->getStart()) && (allocStart < (freeOp->getStart() + freeOp->getSize()))) {
			if ((allocStart == freeOp->getStart()) && (allocSize >= freeOp->getSize())) {
				freeList.erase(freeOp);
			} else {
				uint64_t origSize = freeOp->setSize();
				if (allocStart > freeOp->getStart()) {
					freeOp->setSize(allocStart - freeOp->getStart());
				}
				if (allocStart + allocSize < freeOp->getStart() + freeOp->getSize()) {
					Operation *op = new Operation(freeOp);
					op->setStart(allocStart + allocSize);
					op->setSize(freeOp->getStart() + origSize - (allocStart + allocSize));
				}
			}
		}
	}
}

static void
getFreeOpSize(vector<Operation *> &allocationsList, Operation *op)
{
	uint64_t size = 0;
	for (Operation *allocOp: allocationsList) {
		if (allocOp->getStart() == op->getStart()) {
			size = allocOp->getSize();
			/* continue searching for the last match */
		}
	}
	return size;
}

static void 
parseOperations(const char *filename, vector<Operation *> &allocationsList)
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
				updateFreeList(freeList, op);
			} else {
				if (Operation::FREE != type) {
					cout << "Free memory: " << op << endl;
					op->setSize(getFreeSize(allocationsList, op));
					addToFreeList(freeList, op);
				}
			}
		}
	}
}

int 
main(void) 
{

}
