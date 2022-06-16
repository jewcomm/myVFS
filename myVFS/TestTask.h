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
		FILE* dFT; // ���� � FT � NEXT_CLAST
		FILE* dCS[CLUSTER_FILE_COUNT]{}; // ���� � ����������

		std::set <File, FileCMP> openFiles; // ��������� ��� �������� �������� ������

		/// @brief ���������� ���� � file
		/// @param file ��������� �� ����
		void writeMETAFT(FILE* file); 

		/// @brief ��������� ���� �� ������� ����
		/// @param file ��������� �� ����������� ����
		/// @return 0 - ��� ������
		///			1 - ������ ������
		///			2 - ������ ������ ��������
		///			3 - �� ������� ��������� ����
		int checkFILE_TABLE(FILE* file);

		struct FILE_TABLE
		{
			char FILE_NAME[12]; // ��� ����
			char DOT;			// ����� 
								// 0 ���� �� ��� (������������ ������ � ���������)
								// . ���� ����
			char EXTENSION[3];  // ���������� ����� (��� �����, ��� ���� ��� �� ����� ��� 0x00
			uint32_t META;		// ���� �����
			uint32_t F_CLAST;	// ������ ������ � ����������� � ���������� (��. ������������)
			uint32_t NEXT_REC;	// ��������� ������ � �������� 
			uint32_t REAL_SIZE; // �������� ������ ����� (��� �������� 0)
		};

		/// @brief ��������������� ������� ��� Create. ������� ����, �.�. �� ����������
		/// @return ��������� ����, ��� nullptr ���� �� ��� ������
		File* createCheckWithClear(FILE_TABLE* ft1, FILE_TABLE* ft2, File* f);

		/// @brief ��������������� ������� ��� Create
		/// @return ��������� ����, ��� nullptr ���� �� ��� ������
		File* createCheckNonClear(FILE_TABLE* ft1, FILE_TABLE* ft2, File* f);

		/// @brief ���������� ��� FILE_TABLE(���, �����, ����������, ����)
		/// @param ft1 ������ ���� ��� ���������
		/// @param ft2 ������ ���� ��� ���������
		/// @return 1 ���� ���������, ����� 0
		bool cmpFT(FILE_TABLE* ft1, FILE_TABLE* ft2);

		/// @brief ������ � FT ���������� � info
		/// @param FT ���� ���������� ����������� ������
		/// @param info ���� � ������� ������ ������
		void getFT(FILE_TABLE* FT, File info);

		/// @brief ������ � FT ���������� ������������� � ������� offset 
		/// @param FT ���� ���������� ����������� ������
		/// @param offset ������������� ��������� ������ FT
		void getFT(FILE_TABLE* FT, uint32_t offset);

		/// @brief ���������� FT � ��������� ����� 
		/// @return ������������� ����� �����������, ��� 0xffffffff ���� ��������� �����
		uint32_t writeFT(FILE_TABLE* FT);

		/// @brief �������������� FT �� �������������� ������ offset
		/// @param FT ���������������� ��������
		/// @param offset ������������� �����
		uint32_t reWriteFT(FILE_TABLE* FT, uint32_t offset);

		/// @brief ����������� ����� �������� � FT ������������� � ����������� ����� �������� � CS
		uint32_t recClastToAddrClast(uint32_t rec);

		/// @brief ������� ���� t
		bool clearFile(File* t);

		/// @brief ������� ������� FT � �������� �������������� �� �������������� ������ offset 
		uint32_t createFolder(FILE_TABLE* FT, uint32_t offset);


	public:
		VFS(FILE* _dFT, FILE** _dCS);

		/// @brief ��������� ���� � ������ readonly
		/// @param name ���� ������� ��������� �������� � ������ �����
		/// @return �������� ����, ��� nullptr ���� ���� ���� ��� ������, ��� �� ����������
		File* Open(const char* name);

		/// @brief ������� ���� � ������ writeonle, ���� �� ��� ������ �� ��������� 
		/// @param name ���� ������� ��������� �������� � ������ �����
		/// @return ��������� ����, ��� nullptr ���� ���� ���� ��� ������
		File* Create(const char* name);

		/// @brief ��������� ������ �� �����
		/// @param f ���� �� �������� ������
		/// @param buff ������ � ������� ����� �����������
		/// @param len ������� ��������� ��������� 
		/// @return 0 ���� ���� ������ � ������ ������
		///			��� ���������� ����������� ����
		size_t Read(File* f, char* buff, size_t len);

		/// @brief �������� ������ � ����
		/// @param f ���� � ������� �����
		/// @param buff ������ �� �������� ����� � ����
		/// @param len ������� ��������� ��������
		/// @return 0 ���� ���� ������ � ������ ������, ��� �� ������
		///			��� ���������� ���������� ����
		size_t Write(File* f, char* buff, size_t len);

		/// @brief ��������� ����
		/// @param f ���� ������� ��������� �������
		void Close(File* f);
	};

	/// @brief ������ ���� � �����
	/// @param name ������ ���� � �����
	/// @return ������ � ������� ������� ��������� ������� ��� ���������, ��������� ������ ��� �����
	std::vector <std::string> parsePath(const char* name);

	/// @brief ������ ��� �����
	/// @param s ��� �����
	/// @return ������ � ������ � �����������(���� ����������)
	///			��� 0-�� ������� �������
	///			���������� 1-�� ������� �������
	std::vector <std::string> parseFileName(std::string s);
}