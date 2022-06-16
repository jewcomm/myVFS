#ifndef libs
#define libs
#include "main.h"
#include "TestTask.h"

#endif // !libs

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

	TestTask::File* file1;

	file1 = firtsVFS.Create("data/file1.txt");

	firtsVFS.Close(file1);

	TestTask::File* file2;

	file2 = firtsVFS.Open("data/file1.txt");

	/*char file1Data[] = "1312412";
	//size_t file1Size = firtsVFS.Write(file1, file1Data, strlen(file1Data));

	/*file1 = firtsVFS.Create("data/file1.txt");

	char file1Data[] = "1312412";
	size_t file1Size = firtsVFS.Write(file1, file1Data, strlen(file1Data));*/

	/*TestTask::File* a;

	TestTask::File* b;
	TestTask::File* c;
	TestTask::File* d;
	TestTask::File* e;
	TestTask::File* g;
	TestTask::File* h;

	a = firtsVFS.Create("file.txt");
	std::cout << a->FILE_ID << " file.txt" << std::endl;

	char str[] = "Its test string for first file";

	size_t value = firtsVFS.Write(a, str, sizeof(str) / sizeof(char)); std::cout << value << std::endl;

	b = firtsVFS.Create("example.txt");
	std::cout << b->FILE_ID << " example.txt" << std::endl;

	char str1[] = "String for file 2";

	//std::cout << std::strlen(str) << std::endl;

	std::vector <char> newstr;

	for (int i = 0; i < (CLUSTER_SIZE * 2); i++) {
		newstr.push_back(i % CHAR_MAX);
	}

	char* n = &newstr[0];
	
	//value = firtsVFS.Write(b, n, CLUSTER_SIZE * 2); std::cout << value << std::endl;

	//value = firtsVFS.Write(b, str1, sizeof(str1));
	/*c = firtsVFS.Create("dfb/file_st.txt");
	std::cout << c->FILE_ID << " dfb/file_st.txt" << std::endl;

	d = firtsVFS.Create("txt/file_test.txt");
	std::cout << d->FILE_ID << " txt/file_test.txt" << std::endl;

	e = firtsVFS.Create("rex_dino");
	std::cout << e->FILE_ID << " rex_din" << std::endl;

	g = firtsVFS.Create("txt/result/front/file_est.txt");
	std::cout << g->FILE_ID << " txt/result/front/file_est.txt" << std::endl;

	h = firtsVFS.Create("txt/result/front/file_est.txt");
	std::cout << h->FILE_ID << " txt/result/front/file_est.txt" << std::endl;
	*/
	
	fclose(FT);
	for (auto& i : CS) {
		fclose(i);
	}

	return 0;
}