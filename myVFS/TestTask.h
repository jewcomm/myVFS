const uint32_t CLUSTER_SIZE = (16 * 1024);
#define FT_COUNT 1
#define CLUSTER_FILE_COUNT 2

#define BIG_ENDIAN 1
#define LIT_ENDIAN !BIG_ENDIAN

#define VERSION "1.0.0"

#define FIRST_ELEM_OFFSET 0x20

#define ITS_FOLDER 1
#define ITS_FILE 0

#define WO_MODE 0
#define RO_MODE 1

#define DEBUG 0

const uint32_t end_data_block = 0xffffffff;
const uint32_t next_clast_offset = 0x01000000;

extern std::mutex mutexLockVFS;

namespace TestTask {
	struct File {
		uint32_t FILE_ID; // относительный номер записи о файле в dFT
		char MODE; // режим открытия файла
	};

	struct FileCMP : File
	{
		bool operator()(const File& f, const File& s) const {
			return f.FILE_ID < s.FILE_ID;
		}
	};

	struct IVFS
	{
		virtual File* Open(const char* name) = 0; // Открыть файл в readonly режиме. Если нет такого файла или же он открыт во writeonly режиме - вернуть nullptr
		virtual File* Create(const char* name) = 0; // Открыть или создать файл в writeonly режиме. Если нужно, то создать все нужные поддиректории, упомянутые в пути. Вернуть nullptr, если этот файл уже открыт в readonly режиме.
		virtual size_t Read(File* f, char* buff, size_t len) = 0; // Прочитать данные из файла. Возвращаемое значение - сколько реально байт удалось прочитать
		virtual size_t Write(File* f, char* buff, size_t len) = 0; // Записать данные в файл. Возвращаемое значение - сколько реально байт удалось записать
		virtual void Close(File* f) = 0; // Закрыть файл*/

	};

	class VFS : IVFS, File {
	private:
		FILE* dFT; // файл с FT и NEXT_CLAST
		FILE* dCS[CLUSTER_FILE_COUNT]{}; // файл с кластерами

		std::set <File, FileCMP> openFiles; // контейнер для хранения открытых файлов

		/// @brief записывает мету в file
		/// @param file указатель на файл
		void writeMETAFT(FILE* file); 

		/// @brief Проверяет файл на наличии меты
		/// @param file указатель на проверяемый файл
		/// @return 0 - все хорошо
		///			1 - разная версия
		///			2 - разный размер кластера
		///			3 - не удалось прочитать мету
		int checkFILE_TABLE(FILE* file);

		struct FILE_TABLE
		{
			char FILE_NAME[12]; // имя файл
			char DOT;			// точка 
								// 0 если ее нет (используется только в каталогах)
								// . если файл
			char EXTENSION[3];  // расширение файла (для файла, или если нет то пишем три 0x00
			uint32_t META;		// мета файла
			uint32_t F_CLAST;	// первая запись с информацией о содержимом (см. документацию)
			uint32_t NEXT_REC;	// следующая запись в каталоге 
			uint32_t REAL_SIZE; // реальный размер файла (для каталога 0)
		};

		/// @brief вспомогательная функция для Create. Очищает файл, т.к. он существует
		/// @return созданный файл, или nullptr если он уже открыт
		File* createCheckWithClear(FILE_TABLE* ft1, FILE_TABLE* ft2, File* f);

		/// @brief вспомогательная функция для Create
		/// @return созданный файл, или nullptr если он уже открыт
		File* createCheckNonClear(FILE_TABLE* ft1, FILE_TABLE* ft2, File* f);

		/// @brief сравнивает два FILE_TABLE(имя, точку, расширение, мету)
		/// @param ft1 первый файл для сравнения
		/// @param ft2 второй файл для сравнения
		/// @return 1 если совпадают, иначе 0
		bool cmpFT(FILE_TABLE* ft1, FILE_TABLE* ft2);

		/// @brief читает в FT информацию о info
		/// @param FT сюда записываем прочитанные данные
		/// @param info файл о котором читаем данные
		void getFT(FILE_TABLE* FT, File info);

		/// @brief читает в FT информацию расположенную в секторе offset 
		/// @param FT сюда записываем прочитанные данные
		/// @param offset относительная адресация записи FT
		void getFT(FILE_TABLE* FT, uint32_t offset);

		/// @brief записывает FT в свободное место 
		/// @return относительный адрес записанного, или 0xffffffff если кончилось место
		uint32_t writeFT(FILE_TABLE* FT);

		/// @brief перезаписывает FT по относительному адресу offset
		/// @param FT перезаписываемое значение
		/// @param offset относительный адрес
		uint32_t reWriteFT(FILE_TABLE* FT, uint32_t offset);

		/// @brief фактический адрес кластера в FT преобразуется в фактический адрес кластера в CS
		uint32_t recClastToAddrClast(uint32_t rec);

		/// @brief очищает файл t
		bool clearFile(File* t);

		/// @brief создает каталог FT в каталоге расположенному по относительному адресу offset 
		uint32_t createFolder(FILE_TABLE* FT, uint32_t offset);


	public:
		VFS(FILE* _dFT, FILE** _dCS);

		/// @brief открывает файл в режиме readonly
		/// @param name файл который требуется передать с полным путем
		/// @return открытый файл, или nullptr если этот файл уже открыт, или не существует
		File* Open(const char* name);

		/// @brief создает файл в режиме writeonle, если он уже создан то очищается 
		/// @param name файл который требуется передать с полным путем
		/// @return созданный файл, или nullptr если этот файл уже открыт
		File* Create(const char* name);

		/// @brief прочитать данные из файла
		/// @param f файл из которого читаем
		/// @param buff буффер в который пишем прочитанное
		/// @param len сколько требуется прочитать 
		/// @return 0 если файл открыт в другом режиме
		///			или количество прочитанных байт
		size_t Read(File* f, char* buff, size_t len);

		/// @brief записать данные в файл
		/// @param f файл в который пишем
		/// @param buff буффер из которого пишем в файл
		/// @param len сколько требуется записать
		/// @return 0 если файл открыт в другом режиме, или не пустой
		///			или количество записанных байт
		size_t Write(File* f, char* buff, size_t len);

		/// @brief закрывает файл
		/// @param f файл который требуется закрыть
		void Close(File* f);
	};

	/// @brief парсим путь к файлу
	/// @param name полный путь к файлу
	/// @return вектор с папками которые требуется создать или прочитать, последняя запись имя файла
	std::vector <std::string> parsePath(const char* name);

	/// @brief парсим имя файла
	/// @param s имя файла
	/// @return вектор с именем и расширением(если существует)
	///			имя 0-ой элемент вектора
	///			расширение 1-ый элемент вектора
	std::vector <std::string> parseFileName(std::string s);
}