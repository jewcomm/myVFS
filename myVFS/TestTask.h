namespace TestTask {
	struct File {
		uint32_t FILE_ID; // ������������� ����� ������ � ����� � dFT
		char MODE; // ����� �������� �����
	};

	struct FileCMP : File
	{
		bool operator()(const File& f, const File& s) const {
			return f.FILE_ID < s.FILE_ID;
		}
	};

	struct IVFS
	{
		virtual File* Open(const char* name) = 0; // ������� ���� � readonly ������. ���� ��� ������ ����� ��� �� �� ������ �� writeonly ������ - ������� nullptr
		virtual File* Create(const char* name) = 0; // ������� ��� ������� ���� � writeonly ������. ���� �����, �� ������� ��� ������ �������������, ���������� � ����. ������� nullptr, ���� ���� ���� ��� ������ � readonly ������.
		virtual size_t Read(File* f, char* buff, size_t len) = 0; // ��������� ������ �� �����. ������������ �������� - ������� ������� ���� ������� ���������
		virtual size_t Write(File* f, char* buff, size_t len) = 0; // �������� ������ � ����. ������������ �������� - ������� ������� ���� ������� ��������
		virtual void Close(File* f) = 0; // ������� ����*/

	};

	class VFS : IVFS, File {
	private:
		FILE* dFT;
		FILE* dCT[CLUSTER_FILE_COUNT]{};

		std::set <File, FileCMP> openFiles;

		int writeMETAFT(FILE* file);

		int checkFILE_TABLE(FILE* file);

		struct FILE_TABLE
		{
			char FILE_NAME[12];
			char DOT;
			char EXTENSION[3];
			uint32_t META;
			uint32_t F_CLAST;
			uint32_t NEXT_REC;
			uint32_t REAL_SIZE;
		};

		File* createCheckWithClear(FILE_TABLE* ft1, FILE_TABLE* ft2, File* f);

		File* createCheckNonClear(FILE_TABLE* ft1, FILE_TABLE* ft2, File* f);

		bool cmp(FILE_TABLE* ft1, FILE_TABLE* ft2);

		void getFT(FILE_TABLE* FT, File info);

		void getFT(FILE_TABLE* FT, uint32_t offset);

		uint32_t writeFT(FILE_TABLE* FT);

		uint32_t reWriteFT(FILE_TABLE* FT, uint32_t offset);

		uint32_t recClastToAddrClast(uint32_t rec);

		bool clearFile(File* t);

		uint32_t createFolder(FILE_TABLE* FT, uint32_t offset);


	public:
		VFS(FILE* _dFT, FILE** _dCS);

		File* Open(const char* name);
		File* Create(const char* name);
		size_t Read(File* f, char* buff, size_t len);
		size_t Write(File* f, char* buff, size_t len);
		void Close(File* f);
	};

	std::vector <std::string> parsePath(const char* name);
	std::vector <std::string> parseFileName(std::string s);
}