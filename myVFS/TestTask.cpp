#ifndef libs
#define libs
#include "main.h"
#include "TestTask.h"

#endif // !libs

using namespace TestTask;

VFS::VFS(FILE* _dFT, FILE** _dCS) {
	dFT = _dFT;
	for (auto& i : dCT) {
		i = *_dCS++;
	}
	if (checkFILE_TABLE(dFT)) writeMETAFT(dFT);
}

File* VFS::Create(const char* name)
{
	std::vector <std::string> path = parsePath(name); // ������ ����

	std::vector <std::string> fileName = parseFileName(path.back()); // ������ ���
	path.pop_back(); // ������� �� ���� ���

	uint32_t prevRec = 0; // ����� ���������� ������ (0 - ����� �����)

	File* createdFile = new File; // ���� ������� �������

	if (path.size() > 0) { // ���� ���� ������� ���� �� � �����
		FILE_TABLE* folder = new FILE_TABLE; 

		while (path.size() > 0) { // �������� �� ����� ����, � ������� ����� ����������, ��� ���������� e� id ���� ��� ��� ����
			strncpy(folder->FILE_NAME, (path.front().c_str()), 12);
			folder->DOT = 0;
			strncpy(folder->EXTENSION, "\0\0\0", 3);
			folder->META = ITS_FOLDER;
			folder->F_CLAST = (uint32_t)0;
			folder->NEXT_REC = 0xFFFFFFFF;
			folder->REAL_SIZE = 0;
			prevRec = createFolder(folder, prevRec);
			path.erase(path.begin());
		}
		delete(folder);
	}

	// ������� FT ��� ������ �����
	FILE_TABLE* createdFT = new FILE_TABLE;
	strncpy(createdFT->FILE_NAME, (fileName[0].c_str()), 12); // ����� ���
	createdFT->DOT = '.'; // ����� .
	if (fileName.size() == 2) { 
		strncpy(createdFT->EXTENSION, fileName[1].c_str(), 3); // ����� ����������
	}
	else {
		strncpy(createdFT->EXTENSION, "\0\0\0", 3); // ��� �������
	}
	createdFT->META = ITS_FILE; // ����� ����
	createdFT->F_CLAST = (uint32_t)0; // ����� �������� (0 �.�. ���� ������)
	createdFT->NEXT_REC = (uint32_t)0xFFFFFFFF; // ������ ����� � ����� ������� ����������
	createdFT->REAL_SIZE = (uint32_t)0; 

	// ������� ��������� FT ��� "�������" �� ������
	FILE_TABLE* tempFT = new FILE_TABLE;
	getFT(tempFT, prevRec); // ������ ������� �����, prevRec - ��������� �� �� ������������� ����� 

	if (prevRec != 0) { // ���� ������� ���� �� � �����
		if (tempFT->F_CLAST != 0) { // ���� ����� �� ������
			prevRec = tempFT->F_CLAST; // ��������� � ������� ����� � �����
			getFT(tempFT, prevRec);
			// � ���� ������
		}
		else { // ���� ����� ������
			if (cmp(tempFT, createdFT)) { // ���� ����� ���� ��� ����������
#if DEBUG
				std::cout << "This file alredy exsist on disk" << std::endl;
#endif
				createdFile->FILE_ID = prevRec; 
				createdFile->MODE = WO_MODE;

				return createCheckWithClear(createdFT, tempFT, createdFile);
			}
			// ���� �� ����������
			uint32_t res = writeFT(createdFT);

			getFT(tempFT, prevRec);
			tempFT->F_CLAST = (uint32_t)res;
			reWriteFT(tempFT, prevRec);

			createdFile->FILE_ID = prevRec;
			createdFile->MODE = WO_MODE;

			return createCheckNonClear(createdFT, tempFT, createdFile);
		}
	}

	// ���� ����� �� �����, ��� ��������� ���� � ������
	// ����� ������ ����������, �� ����� �� ��������(

	if (cmp(tempFT, createdFT)) { 
#if DEBUG
		std::cout << "This file alredy exsist on disk" << std::endl;
#endif
		createdFile->FILE_ID = prevRec;
		createdFile->MODE = WO_MODE;

		return createCheckWithClear(createdFT, tempFT, createdFile);
	}

	while (tempFT->NEXT_REC != 0xFFFFFFFF) {
		prevRec = tempFT->NEXT_REC;
		getFT(tempFT, tempFT->NEXT_REC);
		if (cmp(tempFT, createdFT)) {
#if DEBUG
			std::cout << "This file alredy exsist on disk" << std::endl;
#endif
			createdFile->FILE_ID = prevRec;
			createdFile->MODE = WO_MODE;

			return createCheckWithClear(createdFT, tempFT, createdFile);
		}
	}

	if (cmp(tempFT, createdFT)) {
#if DEBUG
		std::cout << "This file alredy exsist on disk" << std::endl;
#endif
		createdFile->FILE_ID = prevRec;
		createdFile->MODE = WO_MODE;

		return createCheckWithClear(createdFT, tempFT, createdFile);
	}

	uint32_t res = writeFT(createdFT);

	getFT(tempFT, prevRec);
	tempFT->NEXT_REC = (uint32_t)res;
	reWriteFT(tempFT, prevRec);

	createdFile->FILE_ID = prevRec;
	createdFile->MODE = WO_MODE;

	return createCheckNonClear(createdFT, tempFT, createdFile);
}

File* VFS::Open(const char* name) {
	std::vector <std::string> path = parsePath(name); // ������ ����

	std::vector <std::string> fileName = parseFileName(path.back()); // ������ ���
	path.pop_back(); // ������� �� ���� ���

	uint32_t prevRec = 0; // ����� ���������� ������ (0 - ����� �����)

	File* openedFile = new File; // ���� ������� �������

	FILE_TABLE* openedFT = new FILE_TABLE; // ���������� ��� ������� �� ���������, � ��������� �����

	FILE_TABLE* readFT = new FILE_TABLE;

	while (path.size()) { // �������� �� ����� ����, ���� �� �� ������ �� ���������� ��������
		// ����� ������ � ��������� ��������
		strncpy(openedFT->FILE_NAME, path.front().c_str(), 12);
		openedFT->DOT = 0;
		strncpy(openedFT->EXTENSION, "\0\0\0", 3);
		openedFT->META = ITS_FOLDER;

		path.erase(path.begin()); // ������� ��� ��������
		while (prevRec != end_data_block) { // ���� �� ��������� ���������� ����� � ��������
			getFT(readFT, prevRec); // ������ ������ � �����
			if (cmp(readFT, openedFT)) { // ���� �� ��������� � ������
				prevRec = readFT->F_CLAST; // ��������� � ������ � ������ ��������
				break; // ������� �� ����� �� �������� ��������
			}
			prevRec = readFT->NEXT_REC; // ��������� � ���������� ����� � ������� ��������
		}
		if (prevRec == end_data_block || prevRec == 0) { // ���� �������� ����� �������� � �� ����� �����
														// ��� ���� ������� ����
			delete openedFT;
			delete readFT;
			return nullptr;
		}
	}

	// ����� ������ � ��������� �����
	strncpy(openedFT->FILE_NAME, fileName[0].c_str(), 12);
	openedFT->DOT = '.';
	if (fileName.size() == 2 && fileName[1].size()) {
		strncpy(openedFT->EXTENSION, fileName[1].c_str(), 3);
	}
	else {
		strncpy(openedFT->EXTENSION, "\0\0\0", 3);
	}
	openedFT->META = ITS_FILE;

	while (prevRec != end_data_block) { // ���� �� ����� ��������, ��� ������� ������ ����
		getFT(readFT, prevRec); 
		if (cmp(readFT, openedFT)) { // ���� ����� ������ ����
			delete openedFT;
			delete readFT;

			File* ret = new File;
			ret->FILE_ID = prevRec;
			ret->MODE = RO_MODE;

			if (openFiles.find({ prevRec, WO_MODE }) == openFiles.end()) { // ���� ���� ��� ������ � ������ WO
				openFiles.insert(*ret);
				return ret;
			} else {
				return nullptr;
			}
		}
		prevRec = readFT->NEXT_REC;
	}

	// ������ ���� �� ������
	delete openedFT;
	delete readFT;
	return nullptr;
}

size_t VFS::Write(File* f, char* buff, size_t len) {
	FILE_TABLE* tempFT = new FILE_TABLE;

	getFT(tempFT, *f); // ������ ���� � ������� �����

	if (tempFT->F_CLAST == 0) { // ���� ���� ������
		fseek(dFT, next_clast_offset, SEEK_SET); // ��������� � ������� FAT
		uint32_t addClast = 0;

		// ���� �� EOF, ��� ���� �� ������ ��������� ������
		while (fread(&addClast, sizeof(uint32_t), 1, dFT)) { // ������� ������ 0 ���� EOF
			if (addClast == 0) { // ������� ���� ���� ������� ��������
				uint32_t z = ftell(dFT);
				fseek(dFT, -4, SEEK_CUR);
				break;
			}
		}

		uint32_t freeSpace = ftell(dFT); // ����� ��������� ������

		// � ����� � ���������� ������ ��������� � ���� ������
		//fseek(dCT[0], (freeSpace - next_clast_offset) / 4 * CLUSTER_SIZE, SEEK_SET); 

		fseek(dCT[0], recClastToAddrClast(freeSpace), SEEK_SET);

		uint32_t count_cluster = len / CLUSTER_SIZE; // ������� ����� ��������� ����� ���������
		if (len % CLUSTER_SIZE != 0) count_cluster++; // ���� �������� �������
		tempFT->F_CLAST = freeSpace; // ���������� ����� ������� �������� � ��������� �����

		uint32_t write_count = 0; // ������� ����� ��������

		for (int i = 0; i < count_cluster; i++) {
			fseek(dFT, 4, SEEK_CUR); // ��������� � ���������� ��������
			// ���� ��������� ��������� �������
			while (fread(&addClast, sizeof(uint32_t), 1, dFT)) { // ������� ������ 0 ���� EOF
				if (addClast == 0) { // ������� ���� ���� ������� ��������
					fseek(dFT, -4, SEEK_CUR);
					break;
				}
			}
			uint32_t nextBlock = ftell(dFT); // �������� ��������� �� ��������� ��������� �������

			fseek(dFT, freeSpace, SEEK_SET); // ��������� � ������� ���������� ��������
			if (i == count_cluster - 1) { // ���� ����� ���������
				fwrite(&end_data_block, sizeof(end_data_block), 1, dFT);
			}
			else {
				fwrite(&nextBlock, sizeof(nextBlock), 1, dFT);
			}
			fseek(dFT, nextBlock, SEEK_SET); // ��������� � ���������� ����������

			fseek(dCT[0], recClastToAddrClast(freeSpace), SEEK_SET); // � ���� � ���������� ���������� ������

			size_t size = fwrite(&buff[i * CLUSTER_SIZE], 1, std::min(len, CLUSTER_SIZE), dCT[0]); // ������ ������� ��������

			if (size != std::min(len, CLUSTER_SIZE)) {
				std::cout << "Error write combine file" << std::endl;
			}

			write_count += size; // ����������� ������� �����������

			len -= size; // ��������� ������� ����������� ��� ������

			freeSpace = nextBlock; // �������� ������ �� ���������� ����������
		}

		tempFT->REAL_SIZE = write_count; // ����� �������� ������
		reWriteFT(tempFT, f->FILE_ID); // �������������� ������� ��������
		return write_count;
	}

	return 0;
}

size_t VFS::Read(File* f, char* buff, size_t len) {
	FILE_TABLE* readFT = new FILE_TABLE;
	getFT(readFT, *f);

	size_t maxLen = std::min(len, readFT->REAL_SIZE); // ��� ������ ������ ������ ��������� �������
													// ��� ������ ������ ������ ��� ���������
													// ������� ������ ������� ������ - ���������� �� ����

	if (readFT->F_CLAST == 0) return -1; // ���� � ����� ������ ���
	// ������ ������� ��������� ��� ���� ����� ���������
	size_t count_cluster = maxLen / CLUSTER_SIZE;
	if (maxLen % CLUSTER_SIZE) count_cluster++;

	std::vector <uint32_t> clasters; // ��� �������� ������� ����� ������

	uint32_t clast = readFT->F_CLAST; // ������ ������� 

	// �������� �� ���� ���������
	for (int i = 0;
		i < count_cluster && // ���� �� ��������� ������ ���������� ��������� 
		clast != end_data_block; // ���� �� ������ �� ����� ������
		i++) {
			clasters.push_back(clast); 
			fseek(dFT, clast, SEEK_SET);
			fread(&clast, sizeof(uint32_t), 1, dFT);
	}

	size_t count = 0; // ������� ���������� ����������� ���� 

	for (int i = 0; i < count_cluster; i++) {
		fseek(dCT[0], recClastToAddrClast(clasters[i]), SEEK_SET);
		size_t res = fread(&buff[i * CLUSTER_SIZE], 1, std::min(CLUSTER_SIZE, maxLen), dCT[0]); 
#if DEBUG
		if (res != std::min(CLUSTER_SIZE, maxLen)) std::cout << "Error read" << std::endl;
#endif DEBUG
		maxLen -= res; // ������� �������� ��������� 
					   //(���� ��� ����� ������ ������� ��������,�� ����� ������ �������)
		count += res; 
	}

	return count;
}

void VFS::Close(File* f) {
	openFiles.erase({ f->FILE_ID, f->MODE });
	delete f;
}

uint32_t VFS::createFolder(FILE_TABLE* FT, uint32_t offset) { // ���������� ����� ������ ��������� �����
	fseek(dFT, FIRST_ELEM_OFFSET + offset * 0x20, SEEK_SET);

	FILE_TABLE* tempFT = new FILE_TABLE;
	getFT(tempFT, offset); // �������� ���� � ������������ �����
	uint32_t prevRec = offset;

	if (offset == 0) { // ���� ��� ����� - ������ �����
		while (tempFT->NEXT_REC != 0xFFFFFFFF) { 
			prevRec = tempFT->NEXT_REC;
			getFT(tempFT, tempFT->NEXT_REC);
			if (cmp(FT, tempFT)) {
				printf("ITS EQUAL FOLDER");
				return prevRec;
			}
		}

		uint32_t res = writeFT(FT);
		tempFT->NEXT_REC = res;
		reWriteFT(tempFT, prevRec);
		return res;
	}

	if (tempFT->F_CLAST == 0) { // ���� ������� ������
		uint32_t res = writeFT(FT);
		tempFT->F_CLAST = res;
		reWriteFT(tempFT, prevRec);

		delete(tempFT);
		return res;
	}
	else { // ���� �� ������
		prevRec = tempFT->F_CLAST; // �������� ������ ������� � �������� ��������
		getFT(tempFT, tempFT->F_CLAST);

		while (tempFT->NEXT_REC != 0xFFFFFFFF) { // ���� �� ������ �� �����, ��� �� �������� �����
			prevRec = tempFT->NEXT_REC;
			getFT(tempFT, tempFT->NEXT_REC);
			if (cmp(tempFT, FT)) {
				printf("ITS EQUAL FOLDER\n");
				return prevRec;
			}
		}

		if (cmp(tempFT, FT)) { // ���� � ��� � �������� ����� ���� ���� � �� �����
			printf("ITS EQUAL FOLDER\n");
			return prevRec;
		}

		uint32_t res = writeFT(FT);
		tempFT->NEXT_REC = res;
		reWriteFT(tempFT, prevRec);
		return res;
	}
}

bool VFS::clearFile(File* t) {
	FILE_TABLE* temp = new FILE_TABLE;
	getFT(temp, *t);

	if (temp->F_CLAST == 0) return true;

	std::vector <uint32_t> clasters;

	uint32_t clast = temp->F_CLAST;

	while (clast != end_data_block) {
		clasters.push_back(clast);
		fseek(dFT, clast, SEEK_SET);
		fread(&clast, sizeof(uint32_t), 1, dFT);
	}

	char* zeroArray = new char[CLUSTER_SIZE];
	std::fill(zeroArray, zeroArray + CLUSTER_SIZE, 0);

	while (clasters.size()) {
		clast = clasters.back();
		clasters.pop_back();

		fseek(dCT[0], recClastToAddrClast(clast), SEEK_SET);
		fwrite(zeroArray, CLUSTER_SIZE, 1, dCT[0]);

		fseek(dFT, clast, SEEK_SET);
		fwrite("\0", 4, 1, dFT);
	}

	temp->F_CLAST = 0;
	temp->REAL_SIZE = 0;

	reWriteFT(temp, t->FILE_ID);

	delete[] zeroArray;

	return true;
}

uint32_t VFS::recClastToAddrClast(uint32_t rec) {
	return (rec - next_clast_offset) / 4 * CLUSTER_SIZE;
}

uint32_t VFS::reWriteFT(FILE_TABLE* FT, uint32_t offset) {
	fseek(dFT, FIRST_ELEM_OFFSET + offset * 0x20, SEEK_SET);

	fwrite(FT->FILE_NAME, sizeof(char), 12, dFT);
	fwrite(&FT->DOT, sizeof(char), 1, dFT);
	fwrite(FT->EXTENSION, sizeof(char), 3, dFT);
	fwrite(&FT->META, sizeof(uint32_t), 1, dFT);
	fwrite(&FT->F_CLAST, sizeof(uint32_t), 1, dFT);
	fwrite(&FT->NEXT_REC, sizeof(uint32_t), 1, dFT);
	fwrite(&FT->REAL_SIZE, sizeof(uint32_t), 1, dFT);

	return ((ftell(dFT) / 0x20) - 2);
}

uint32_t VFS::writeFT(FILE_TABLE* FT) {
	fseek(dFT, FIRST_ELEM_OFFSET, SEEK_SET);

	char firstSymbName;
	// ���� fread ���������� 0 -> EOF, ��������� �������� �� �����
	while (fread(&firstSymbName, sizeof(char), 1, dFT) && firstSymbName != '\0') { // ���� ������ ����� �� �����
		fseek(dFT, 0x1f, SEEK_CUR);
	}
	// ���� firstSymbName ����� 0, �� ���� �������� �� 1 ������� �����
	if (ftell(dFT) % 0x20 == 1) fseek(dFT, -1, SEEK_CUR);

	fseek(dFT, 0, SEEK_CUR);

	fwrite(FT->FILE_NAME, sizeof(char), 12, dFT);
	fwrite(&FT->DOT, sizeof(char), 1, dFT);
	fwrite(FT->EXTENSION, sizeof(char), 3, dFT);
	fwrite(&FT->META, sizeof(uint32_t), 1, dFT);
	fwrite(&FT->F_CLAST, sizeof(uint32_t), 1, dFT);
	fwrite(&FT->NEXT_REC, sizeof(uint32_t), 1, dFT);
	fwrite(&FT->REAL_SIZE, sizeof(uint32_t), 1, dFT);

	return ((ftell(dFT) / 0x20) - 2);
}

File* TestTask::VFS::createCheckWithClear(FILE_TABLE* ft1, FILE_TABLE* ft2, File* f)
{
	delete ft1;
	delete ft2;

	if (openFiles.find({ f->FILE_ID, RO_MODE }) == openFiles.end()) {
		clearFile(f);
		openFiles.insert(*f);
		return f;
	}
	else {
		delete f;
		return nullptr;
	}
}

File* TestTask::VFS::createCheckNonClear(FILE_TABLE* ft1, FILE_TABLE* ft2, File* f)
{
	delete ft1;
	delete ft2;

	if (openFiles.find({ f->FILE_ID, RO_MODE }) == openFiles.end()) {
		openFiles.insert(*f);
		return f;
	}
	else {
		delete f;
		return nullptr;
	}
}

bool VFS::cmp(FILE_TABLE* ft1, FILE_TABLE* ft2) {
	return (strcmp(ft1->FILE_NAME, ft2->FILE_NAME) == 0 &&
		ft1->DOT == ft2->DOT &&
		strcmp(ft1->EXTENSION, ft2->EXTENSION) == 0 &&
		ft1->META == ft2->META);
}

void VFS::getFT(FILE_TABLE* FT, File info) {
	fseek(dFT, FIRST_ELEM_OFFSET + info.FILE_ID * 0x20, SEEK_SET);
	fread(FT->FILE_NAME, sizeof(char), 12, dFT);
	fread(&FT->DOT, sizeof(char), 1, dFT);
	fread(FT->EXTENSION, sizeof(char), 3, dFT);
	fread(&FT->META, sizeof(uint32_t), 1, dFT);
	fread(&FT->F_CLAST, sizeof(uint32_t), 1, dFT);
	fread(&FT->NEXT_REC, sizeof(uint32_t), 1, dFT);
	fread(&FT->REAL_SIZE, sizeof(uint32_t), 1, dFT);
}

void VFS::getFT(FILE_TABLE* FT, uint32_t offset) {
	fseek(dFT, FIRST_ELEM_OFFSET + offset * 0x20, SEEK_SET);
	fread(FT->FILE_NAME, sizeof(char), 12, dFT);
	fread(&FT->DOT, sizeof(char), 1, dFT);
	fread(FT->EXTENSION, sizeof(char), 3, dFT);
	fread(&FT->META, sizeof(uint32_t), 1, dFT);
	fread(&FT->F_CLAST, sizeof(uint32_t), 1, dFT);
	fread(&FT->NEXT_REC, sizeof(uint32_t), 1, dFT);
	fread(&FT->REAL_SIZE, sizeof(uint32_t), 1, dFT);
}

int VFS::writeMETAFT(FILE* file) {
	fseek(file, 0, SEEK_SET);
	fwrite(VERSION, 1, 5, file);
	fwrite(&CLUSTER_SIZE, sizeof(uint32_t), 1, file);
	// ������ ��� �������, �������� ���� � ����� ���������� ������ 0 � ����, �� ����� �� ��������(
	fseek(file, 0x20, SEEK_SET);
	fwrite("!", 1, 1, file);

	fseek(file, 0x20 + 0x10 + 0x08, SEEK_SET);
	fwrite(&end_data_block, sizeof(uint32_t), 1, file);

	return 1;
}

int VFS::checkFILE_TABLE(FILE* file) {
	fseek(file, 0, SEEK_SET);
	char buffer[32];
	switch (fread(&buffer, sizeof(char), sizeof(buffer), file)) {
	case (sizeof(buffer)):
		//all ok
		for (int i = 0; i < sizeof(VERSION); i++) {
			if (buffer[i] != VERSION[i]) return 1;
		}
		uint32_t clusterSizeGet;
		if (BIG_ENDIAN) {
			clusterSizeGet = (uint32_t)buffer[5] << 0 |
				(uint32_t)buffer[6] << 8 |
				(uint32_t)buffer[7] << 16 |
				(uint32_t)buffer[8] << 24;
		}
		else {
			clusterSizeGet = (uint32_t)buffer[5] << 24 |
				(uint32_t)buffer[6] << 16 |
				(uint32_t)buffer[7] << 8 |
				(uint32_t)buffer[8] << 0;
		}

		if (clusterSizeGet != CLUSTER_SIZE) return 2;
		return 0;

	default:
		return 1;
	}
}

struct FileCMP : File
{
	bool operator()(const File& f, const File& s) const {
		return f.FILE_ID < s.FILE_ID;
	}
};


std::vector <std::string> TestTask::parsePath(const char* name) {
	std::vector <std::string> result;

	std::string s = name;
	std::string delimiter = "/";

	size_t pos = 0;
	std::string token;
	while ((pos = s.find(delimiter)) != std::string::npos) {
		token = s.substr(0, pos);
		result.push_back(token);
		s.erase(0, pos + delimiter.length());
	}
	result.push_back(s);

	return result;
}

std::vector <std::string> TestTask::parseFileName(std::string s) {
	std::vector <std::string> result;

	std::string delimiter = ".";

	size_t pos = 0;
	std::string token;
	if ((pos = s.find(delimiter)) != std::string::npos) {
		token = s.substr(0, pos);
		result.push_back(token);
		s.erase(0, pos + delimiter.length());
	}
	result.push_back(s);

	return result;
}