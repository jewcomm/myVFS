#define _CRT_SECURE_NO_WARNINGS

#include <iostream>
#include <fstream>
#include <cstring>
#include <cstdio>
#include <string>
#include <vector>
#include <set>

const uint32_t CLUSTER_SIZE = (16 * 1024);
#define FT_COUNT 1
#define CLUSTER_FILE_COUNT 2

#define BIG_ENDIAN 1
#define LIT_ENDIAN !BIG_ENDIAN

#define VERSION "1.0.0"

#define FIRST_ELEM_OFFSET 0x20

#define ITS_FOLDER 1
#define ITS_FILE 0

const uint32_t end_data_block = 0xffffffff;

namespace TestTask
{
	std::vector <std::string> parsePath(const char* name);
	std::vector <std::string> parseFileName(std::string s);

	struct File {
		uint32_t FILE_ID;
		char MODE;
	}; // Вы имеете право как угодно задать содержимое этой структуры

	struct FileCMP
	{
		bool operator()(const File& f, const File& s) const {
			return f.FILE_ID < s.FILE_ID;
		}
	};

	struct IVFS
	{
		//virtual File *Open( const char *name ) = 0; // Открыть файл в readonly режиме. Если нет такого файла или же он открыт во writeonly режиме - вернуть nullptr
		virtual File* Create(const char* name) = 0; // Открыть или создать файл в writeonly режиме. Если нужно, то создать все нужные поддиректории, упомянутые в пути. Вернуть nullptr, если этот файл уже открыт в readonly режиме.
		/*virtual size_t Read( File *f, char *buff, size_t len ) = 0; // Прочитать данные из файла. Возвращаемое значение - сколько реально байт удалось прочитать
		virtual size_t Write( File *f, char *buff, size_t len ) = 0; // Записать данные в файл. Возвращаемое значение - сколько реально байт удалось записать
		virtual void Close( File *f ) = 0; // Закрыть файл*/

	};

	class VFS : IVFS, File {
	private:
		FILE* dFT;
		FILE* dCT[CLUSTER_FILE_COUNT]{};

		std::set <File, FileCMP> openFiles;

		int writeMETAFT(FILE* file) {
			fseek(file, 0, SEEK_SET);
			fwrite(VERSION, 1, 5, file);
			fwrite(&CLUSTER_SIZE, sizeof(uint32_t), 1, file);
			// вообще это костыль, наверное есть и лучше реализация записи 0 в файл, но лучше не придумал(
			fseek(file, 0x20, SEEK_SET);
			fwrite("!", 1, 1, file);

			fseek(file, 0x20 + 0x10 + 0x08, SEEK_SET);
			fwrite(&end_data_block, sizeof(uint32_t), 1, file);

			fseek(file, 0xffffffff - 1, SEEK_SET);
			fwrite("\0", 1, 1, file);

			return 1;
		}

		int writeMETA(FILE* file) {
			fseek(file, 0, SEEK_SET);
			fwrite(VERSION, 1, 5, file);
			fwrite(&CLUSTER_SIZE, sizeof(uint32_t), 1, file);
			// вообще это костыль, наверное есть и лучше реализация записи 0 в файл, но лучше не придумал(
			fseek(file, 0xffffffff-1, SEEK_SET);
			fwrite("\0", 1, 1, file);

			return 1;
		}

		int checkFILE_TABLE(FILE* file) {
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

		struct FILE_TABLE
		{
			char FILE_NAME[12];
			char DOT;
			char EXTENSION[3];
			uint32_t META;
			uint32_t F_CLAST;
			uint32_t NEXT_REC;
			uint32_t REAL_SIZE;

			bool operator == (FILE_TABLE* ft2) {
				if (this->FILE_NAME == ft2->FILE_NAME &&
					this->DOT == ft2->DOT &&
					this->EXTENSION == ft2->EXTENSION && 
					this->META == ft2->META) return true;
				else return false;
			}

			bool operator != (FILE_TABLE* ft2) {
				return !(this == ft2);
			}

		};

		uint32_t findFreeFT() {
			char firstSymbName;
			while (fread(&firstSymbName, sizeof(char), 1, dFT) && firstSymbName != '\0') {
				fseek(dFT, 0x1f, SEEK_CUR);
			}
			return ((ftell(dFT) / 0x20) - 1);
		}

		void getFT(FILE_TABLE *FT, File info) {
			fseek(dFT, FIRST_ELEM_OFFSET + info.FILE_ID * 0x20, SEEK_SET);
			fread(FT->FILE_NAME, sizeof(char), 12, dFT);
			fread(&FT->DOT, sizeof(char), 1, dFT);
			fread(FT->EXTENSION, sizeof(char), 3, dFT);
			fread(&FT->META, sizeof(uint32_t), 1, dFT);
			fread(&FT->F_CLAST, sizeof(uint32_t), 1, dFT);
			fread(&FT->NEXT_REC, sizeof(uint32_t), 1, dFT);
			fread(&FT->REAL_SIZE, sizeof(uint32_t), 1, dFT);
		}

		void getFT(FILE_TABLE* FT, uint32_t offset) {
			fseek(dFT, FIRST_ELEM_OFFSET + offset * 0x20, SEEK_SET);
			fread(FT->FILE_NAME, sizeof(char), 12, dFT);
			fread(&FT->DOT, sizeof(char), 1, dFT);
			fread(FT->EXTENSION, sizeof(char), 3, dFT);
			fread(&FT->META, sizeof(uint32_t), 1, dFT);
			fread(&FT->F_CLAST, sizeof(uint32_t), 1, dFT);
			fread(&FT->NEXT_REC, sizeof(uint32_t), 1, dFT);
			fread(&FT->REAL_SIZE, sizeof(uint32_t), 1, dFT);
		}

		uint32_t writeFT(FILE_TABLE* FT) {
			fseek(dFT, FIRST_ELEM_OFFSET, SEEK_SET);
			char firstSymbName;

			while(fread(&firstSymbName, sizeof(char), 1, dFT) && firstSymbName != '\0') {
				fseek(dFT, 0x1f, SEEK_CUR);
			}

			fwrite(FT->FILE_NAME, sizeof(char), 12, dFT);
			fwrite(&FT->DOT, sizeof(char), 1, dFT);
			fwrite(FT->EXTENSION, sizeof(char), 3, dFT);
			fwrite(&FT->META, sizeof(uint32_t), 1, dFT);
			fwrite(&FT->F_CLAST, sizeof(uint32_t), 1, dFT);
			fwrite(&FT->NEXT_REC, sizeof(uint32_t), 1, dFT);
			fwrite(&FT->REAL_SIZE, sizeof(uint32_t), 1, dFT);

			return ((ftell(dFT) / 0x20) - 2);
		}

		uint32_t reWriteFT(FILE_TABLE* FT, uint32_t offset) {
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

		uint32_t createFolder(FILE_TABLE* FT, uint32_t offset) { // возвращает номер записи созданной папки
			fseek(dFT, FIRST_ELEM_OFFSET + offset * 0x20, SEEK_SET);
			
			FILE_TABLE* i = new FILE_TABLE;
			getFT(i, offset); // получаем инфу о родительской папке
			uint32_t curRec = offset;

			if (offset == 0) { // если это папка - корень 
				while (i->NEXT_REC != 0xFFFFFFFF) {
					curRec = i->NEXT_REC;
					getFT(i, i->NEXT_REC);
					if (i == FT) {
						printf("ITS EQUAL FOLDER");
						return curRec;
					}
				}

				uint32_t res = writeFT(FT);
				i->NEXT_REC = res;
				reWriteFT(i, curRec);
				return res;
			}

			if (i->F_CLAST == 0) {
				uint32_t res = writeFT(FT);
				i->F_CLAST = res;
				reWriteFT(i, curRec);
				
				delete(i);
				return res;
			} 
			else {
				getFT(i, i->F_CLAST);
				curRec = i->F_CLAST;

				while (i->NEXT_REC != 0xFFFFFFFF) {
					curRec = i->NEXT_REC;
					getFT(i, i->NEXT_REC);
					if (i == FT) {
						printf("ITS EQUAL FOLDER");
						return curRec;
					}
				}

				uint32_t res = writeFT(FT);
				i->NEXT_REC = res;
				reWriteFT(i, curRec);
				return res;
			}
		}

	public:
		VFS(FILE* _dFT, FILE** _dCS) {
			dFT = _dFT;
			for (auto& i : dCT) {
				i = *_dCS++;
			}

			if (checkFILE_TABLE(dFT)) writeMETAFT(dFT);
			for (auto& i : dCT) {
				if (checkFILE_TABLE(i)) writeMETA(i);
			}
		}

		//File* Open(const char* name);
		File* Create(const char* name);
		/*size_t Read(File* f, char* buff, size_t len);
		size_t Write(File* f, char* buff, size_t len);
		void Close(File* f);*/
	};

	File* VFS::Create(const char* name)
	{
		std::vector <std::string> path = parsePath(name);

		std::vector <std::string> fileName = parseFileName(path.back());
		path.pop_back();

		uint32_t curREC = 0;

		if (path.size() > 0) {
			FILE_TABLE* folder = new FILE_TABLE;

			strncpy(folder->FILE_NAME, (path[0].c_str()), 12);
			folder->DOT = 0;
			strncpy(folder->EXTENSION, "\0\0\0", 3);
			folder->META = ITS_FOLDER;
			folder->F_CLAST = (uint32_t)0;
			folder->NEXT_REC = 0xFFFFFFFF;
			folder->REAL_SIZE = 0;

			curREC = createFolder(folder, 0);
			delete(folder);
		}
		
		// ищем последний файл в текущей директории
		FILE_TABLE* i = new FILE_TABLE;

		getFT(i, curREC);

		if (curREC != 0) { // если создаем файл не в корне
			if (i->F_CLAST != 0) { // если папка не пуста
				getFT(i, i->F_CLAST);
			}
			else {
				FILE_TABLE* temp = new FILE_TABLE;
				strncpy(temp->FILE_NAME, (fileName[0].c_str()), 12);
				temp->DOT = '.';
				if (fileName.size() == 2) {
					strncpy(temp->EXTENSION, fileName[1].c_str(), 3);
				}
				else {
					strncpy(temp->EXTENSION, "\0\0\0", 3);
				}
				temp->META = ITS_FILE;
				temp->F_CLAST = (uint32_t)0;
				temp->NEXT_REC = (uint32_t)0xFFFFFFFF;
				temp->REAL_SIZE = (uint32_t)0;

				uint32_t res = writeFT(temp);

				getFT(i, curREC);
				i->F_CLAST = (uint32_t)res;
				reWriteFT(i, curREC);

				delete(temp);
				delete(i);

				openFiles.insert({ res, ITS_FILE });

				File* t = new File;

				t->FILE_ID = res;
				t->MODE = ITS_FILE;

				return t;
			}
		}

		while (i->NEXT_REC != 0xFFFFFFFF) {
			curREC = i->NEXT_REC;
			getFT(i, i->NEXT_REC);
		}

		FILE_TABLE* temp = new FILE_TABLE;
		strncpy(temp->FILE_NAME, (fileName[0].c_str()), 12);
		temp->DOT = '.';
		if (fileName.size() == 2) {
			strncpy(temp->EXTENSION, fileName[1].c_str(), 3);
		}
		else {
			strncpy(temp->EXTENSION, "\0\0\0", 3);
		}
		temp->META = ITS_FILE;
		temp->F_CLAST = (uint32_t)0;
		temp->NEXT_REC = (uint32_t)0xFFFFFFFF;
		temp->REAL_SIZE = (uint32_t)0;

		uint32_t res = writeFT(temp);

		getFT(i, curREC);
		i->NEXT_REC = (uint32_t)res;
		reWriteFT(i, curREC);

		delete(temp);
		delete(i);

		openFiles.insert({ res, ITS_FILE });

		File* t = new File;

		t->FILE_ID = res;
		t->MODE = ITS_FILE;

		return t;
	}

	std::vector <std::string> parsePath(const char * name){
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

	std::vector <std::string> parseFileName(std::string s) {
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
}


int main() {
	FILE* FT;
	char nameFT[] = "FT.vfs";
	FT = fopen(nameFT, "rb+");
	if (FT == nullptr) {
		std::cout << "Error open FILE_TABLE file. Try create this file." << std::endl;
		FT = fopen(nameFT, "wb+");

		if (FT == nullptr) {
			std::cout << "Error create FILE_TABLE file. End program." << std::endl;
			return 0;
		}
	}

	FILE* CS[CLUSTER_FILE_COUNT];
	for (int i = 0; i < CLUSTER_FILE_COUNT; i++) {
		std::string nameCS = ((std::string)"CS" + (char)(i + '0') + (std::string)".vfs");
		if ((CS[i] = fopen(nameCS.c_str(), "rb+")) == nullptr) {
			std::cout << "Error open " << nameCS << "file. Try create this file. " << std::endl;

			if ((CS[i] = fopen(nameCS.c_str(), "wb+")) == nullptr) {
				std::cout << "Error create " << nameCS << " file. End program." << std::endl;
				return 0;
			}
		}
	}

	TestTask::VFS firtsVFS = TestTask::VFS(FT, CS);

	TestTask::File* t;

	firtsVFS.Create("file.txt");
	firtsVFS.Create("example.txt");
	firtsVFS.Create("dfb/file_st.txt");
	firtsVFS.Create("txt/file_test.txt");
	firtsVFS.Create("rex_dino");

	fclose(FT);
	for (auto& i : CS) {
		fclose(i);
	}

	return 0;
}