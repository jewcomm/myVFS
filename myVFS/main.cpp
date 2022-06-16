#ifndef libs
#define libs
#include "main.h"
#include "TestTask.h"

#endif // !libs

int main() {

	setlocale(LC_ALL, "Rus");

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

	TestTask::VFS myVFS = TestTask::VFS(FT, CS);

	/**
	 * Тест 1
	 * 
	 * Создаем файл, пишем в него строку, закрываем
	 * Открываем для чтения, читаем строку, сравниванием строки, закрываем.
	 * 
	 * Ожидаем совпадение значений
	 */

	TestTask::File* file1;
	file1 = myVFS.Create("file1.txt");
	char file1Data[] = "Тестовая строка для первого файла, пусть будет вот такая";
	size_t file1Len = myVFS.Write(file1, file1Data, strlen(file1Data));
	myVFS.Close(file1);

	file1 = myVFS.Open("file1.txt");
	char* file1Read = new char[strlen(file1Data) + 1];
	size_t file1ReadLen = myVFS.Read(file1, file1Read, strlen(file1Data));
	file1Read[file1ReadLen] = '\0';
	assert(file1ReadLen == file1Len);
	assert(strcmp(file1Data, file1Read) == 0);
	
	std::cout << "Тест 1 пройден \nfile1.txt прочитан успешно, данные совпадают" << std::endl << std::endl;

	delete[] file1Read;
	myVFS.Close(file1);


	/**
	 * Тест 2
	 * 
	 * Создаем файл, и пытаем открыть его еще раз
	 * 
	 * Ожидаем ошибку при повторном открытии
	 */

	TestTask::File* file2;
	file2 = myVFS.Create("file2.txt");

	TestTask::File* file2_open;
	file2_open = myVFS.Open("file2.txt");

	assert(file2_open == NULL);

	std::cout << "Тест 2 пройден \nОшибка открытия файла file2.txt т.к. он уже открыт в другом режиме" << std::endl << std::endl;

	myVFS.Close(file2);


	/**
	 * Тест 3
	 * 
	 * Создаем файл в директории, закрываем его
	 * Открываем файл с таким же названием, но без директории
	 *
	 * Ожидаем ошибку открытия файла
	 */

	TestTask::File* file3;
	file3 = myVFS.Create("folder/file3");
	myVFS.Close(file3);

	TestTask::File* file3_reopen;
	file3_reopen = myVFS.Open("file3");

	assert(file3_reopen == NULL);

	std::cout << "Тест 3 пройден \nОшибка открытия файла file3 т.к. не находится в этой директории" << std::endl << std::endl;


	/**
	 * Тест 4
	 *
	 * Создаем файл, закрываем в него большой массив (2 кластера + еще сколько то данных)
	 * Закрываем файл
	 * Открываем файл этот же файл, и пытаемся прочитать данные, сравниваем прочитанное и записанное значение
	 *
	 * Ожидаем совпадение массивов
	 */

	TestTask::File* file4;
	file4 = myVFS.Create("folder/file4");

	std::vector <char> file4buff;

	for (int i = 0; i < (CLUSTER_SIZE * 2) + 150; i++) {
		file4buff.push_back(i % CHAR_MAX);
	}
	char* f4buff = &file4buff[0];

	size_t file4wLen = myVFS.Write(file4, f4buff, (CLUSTER_SIZE * 2) + 150);

	myVFS.Close(file4);

	TestTask::File* file4Open;
	file4Open = myVFS.Open("folder/file4");

	char read4buff[(CLUSTER_SIZE * 2) + 150 + 1];

	size_t file4rLen = myVFS.Read(file4Open, read4buff, file4wLen);

	read4buff[file4rLen] = '\0';

	assert(file4wLen == file4rLen);
	assert(strcmp(f4buff, read4buff) == 0);

	std::cout << "Тест 4 пройден \nПрочитанные и записанные данные совпадают" << std::endl << std::endl;

	
	fclose(FT);
	for (auto& i : CS) {
		fclose(i);
	}

	_CrtDumpMemoryLeaks();
	return 0;
}