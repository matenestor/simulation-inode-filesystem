#include <iostream>


// priklad - verze 2019-01
// jedná se o SIMULACI souborového systému
// první i-uzel bude odkaz na hlavní adresář (1. datový blok)
// počet přímých i nepřímých odkazů je v reálném systému větší
// adresář vždy obsahuje dvojici číslo i-uzlu - název souboru
// jde jen o příklad, vlastní datové struktury si můžete upravit

const int32_t ID_ITEM_FREE = 0;

struct superblock {
	char signature[9];              //login autora FS
	char volume_descriptor[251];    //popis vygenerovaného FS
	int32_t disk_size;              //celkova velikost VFS
	int32_t cluster_size;           //velikost clusteru
	int32_t cluster_count;          //pocet clusteru
	int32_t bitmap_start_address;   //adresa pocatku bitmapy datových bloků
	int32_t inode_start_address;    //adresa pocatku  i-uzlů
	int32_t data_start_address;     //adresa pocatku datovych bloku
};


struct pseudo_inode {
	int32_t nodeid;                 //ID i-uzlu, pokud ID = ID_ITEM_FREE, je polozka volna
	bool isDirectory;               //soubor, nebo adresar
	int8_t references;              //počet odkazů na i-uzel, používá se pro hardlinky
	int32_t file_size;              //velikost souboru v bytech
	int32_t direct1;                // 1. přímý odkaz na datové bloky
	int32_t direct2;                // 2. přímý odkaz na datové bloky
	int32_t direct3;                // 3. přímý odkaz na datové bloky
	int32_t direct4;                // 4. přímý odkaz na datové bloky
	int32_t direct5;                // 5. přímý odkaz na datové bloky
	int32_t indirect1;              // 1. nepřímý odkaz (odkaz - datové bloky)
	int32_t indirect2;              // 2. nepřímý odkaz (odkaz - odkaz - datové bloky)
};


struct directory_item {
	int32_t inode;                   // inode odpovídající souboru
	char item_name[12];              //8+3 + /0 C/C++ ukoncovaci string znak
};


int main() {
	std::cout << "Hello, World!" << std::endl;
	return 0;
}