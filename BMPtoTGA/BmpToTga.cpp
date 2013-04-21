#include "BmpToTga.h"
#include <cstdint>
#include <iostream>
#include <fstream>
#include <memory>
#include <cstdlib>


namespace ImgConv {

struct TGAHEADER {
	char thID;
	char thColorMap;
	char thImgType;
	uint16_t thClorMapIndex;
	uint16_t thColorMapLength;
	char thColorMapSize;
	uint16_t thImageOriginX;
	uint16_t thImageOriginY;
	uint16_t thImageWidth;
	uint16_t thImageHeight;
	char thBitPerPixel;
	char thDiscripter;
};

struct TGAFOOTER {
	uint32_t tfPosition;
	uint32_t tfDirectory;
	char tfStr[ 18 ];
};

struct BITMAPFILEHEADER {
	uint16_t bfType; // ファイルタイプ
	uint32_t bfSize; // ファイルサイズ
	uint16_t bfReserved1; // 拡張用に割り当てられている
	uint16_t bfReserved2; // 拡張用に割り当てられている
	uint32_t bfOffBits; // イメージデータオフセット
};

struct BITMAPINFOHEADER {
	uint32_t biSize;	// ヘッダサイズ
	uint32_t biWidth;	// 幅(pixel)
	uint32_t biHeight;	// 高さ(pixel)
	uint16_t biPlanes;	// プレーン数
	uint16_t biBitCount;	// ピクセル毎のビット数
	uint32_t biCompression;	// 圧縮タイプ
	uint32_t biSizeImage;	// イメージデータサイズ
	uint32_t biXPelsPerMeter;	// 水平解像度
	uint32_t biYPelsPerMeter;	// 垂直解像度
	uint32_t biClrUsed;	// カラーインデックス数
	uint32_t biClrImportant;	// 重要インデックス数
};

int LoadFileHeader( std::ifstream& ifs, BITMAPFILEHEADER& header );
int LoadInfoHeader( std::ifstream& ifs, BITMAPINFOHEADER& header );
void CreateBMP( BITMAPFILEHEADER const & f_header, BITMAPINFOHEADER const & i_header, char const * data );
void CreateTGA( TGAHEADER const & tgah, TGAFOOTER const & tgaf, char const * data );

int BmpToTga( std::string bmp_file, std::string tga_file )
{
	using namespace std;
	const char* msg = "ファイルが破損している\n";

	BITMAPFILEHEADER f_header = {0};
	BITMAPINFOHEADER i_header = {0};
	unique_ptr< char[] > data( new char[ 0 ] );


	// ファイルヘッダ読み込み
	{ 
		ifstream ifs( bmp_file, ios::binary );
		if ( !ifs ) {
			std::cerr << "ファイルが開けなかった\n";
			return 1;
		}

		if ( LoadFileHeader( ifs, f_header ) ) {
			std::cerr << msg;
			return 1;
		}

		// 情報ヘッダの読み込み
		uint32_t f_header_size;
		ifs.read( reinterpret_cast< char* >( &f_header_size ), 4 );
		if ( ifs.fail() ) {
			std::cerr << msg;
			return 1;
		}

		if ( f_header_size == 40 ) { // INFOタイプのヘッダ
			i_header.biSize = f_header_size;
			LoadInfoHeader( ifs, i_header );
		} else {
			cerr << "INFOタイプのヘッダ以外を読み込む処理は実装されていない\n";
			return 1;
		}
	
		// ビットフィールドの読み込み
		if ( i_header.biSize == 40
		&& ( i_header.biBitCount == 16 || i_header.biBitCount == 32 )
		&& i_header.biCompression == 3 ) { // ビットフィールドが存在する
			cerr << "ビットフィールドの読み込み処理はまだ実装されていない\n";
			return 1;
		}

		// カラーパレットの読み込み処理
		if ( i_header.biBitCount == 1 || i_header.biBitCount == 4 
		|| i_header.biBitCount == 8 || i_header.biClrUsed >= 1 ) {
			cerr << "カラーパレットの読み込み処理はまだ実装されていない\n";
			return 1;
		}

		// イメージデータの読み込み
		if ( f_header.bfOffBits == 0 )
			f_header.bfOffBits = static_cast< uint32_t >( ifs.tellg() );
		if ( ifs.tellg() < f_header.bfOffBits ) {
			ifs.seekg( 0, f_header.bfOffBits );
		}
		int const byte_per_pixel = i_header.biBitCount / 8;
		int const rest = ( i_header.biWidth * byte_per_pixel ) % 4; // 各行に含まれる余りバイトの数
		data.reset( new char[ i_header.biHeight * i_header.biWidth * byte_per_pixel ] );
		for ( size_t h = 0; h < i_header.biHeight; ++h ) {
			ifs.read( &data[ i_header.biWidth * byte_per_pixel * h ], i_header.biWidth * byte_per_pixel );
			if ( rest == 0 )
				continue;
			// 余りバイトを捨てる
			unique_ptr< char[] > tmp( new char[ rest ] );
			ifs.read( tmp.get(), rest );
		}
		if ( ifs.fail() ) {
			std::cerr << msg;
			return 1;
		}
	}

	
	TGAHEADER tgah = {0};
	if ( i_header.biBitCount == 24 )
		tgah.thImgType = 0x02;
	tgah.thImageWidth = i_header.biWidth;
	tgah.thImageHeight = i_header.biHeight;
	tgah.thBitPerPixel = i_header.biBitCount;
	tgah.thDiscripter = strtol( "00000000", nullptr, 2 );



	TGAFOOTER tgaf = {0};
	char tmp[] = "TERUVISION=TARGA";
	for ( int i = 0; i < sizeof( tmp ); ++i )
		tgaf.tfStr[ i ] = tmp[ i ];

	CreateTGA( tgah, tgaf, data.get() );
	int x = 0;

	return 0;
}

int LoadFileHeader( std::ifstream& ifs, BITMAPFILEHEADER& header ) {
	ifs.read( reinterpret_cast< char* >( &header.bfType ), 2 );
	ifs.read( reinterpret_cast< char* >( &header.bfSize ), 4 );
	ifs.read( reinterpret_cast< char* >( &header.bfReserved1 ), 2 );
	ifs.read( reinterpret_cast< char* >( &header.bfReserved2 ), 2 );
	ifs.read( reinterpret_cast< char* >( &header.bfOffBits ), 4 );
	return ifs.fail();
}

// biSize以外を読み込む．biSizeはヘッダ識別のために既に読み込んでいるはずだから飛ばす．
int LoadInfoHeader( std::ifstream& ifs, BITMAPINFOHEADER& header ) {
	ifs.read( reinterpret_cast< char* >( &header.biWidth ), 4 );
	ifs.read( reinterpret_cast< char* >( &header.biHeight ), 4 );
	ifs.read( reinterpret_cast< char* >( &header.biPlanes ), 2 );
	ifs.read( reinterpret_cast< char* >( &header.biBitCount ), 2 );
	ifs.read( reinterpret_cast< char* >( &header.biCompression ), 4 );
	ifs.read( reinterpret_cast< char* >( &header.biSizeImage ), 4 );
	ifs.read( reinterpret_cast< char* >( &header.biXPelsPerMeter ), 4 );
	ifs.read( reinterpret_cast< char* >( &header.biYPelsPerMeter ), 4 );
	ifs.read( reinterpret_cast< char* >( &header.biClrUsed ), 4 );
	ifs.read( reinterpret_cast< char* >( &header.biClrImportant ), 4 );
	return ifs.fail();
}

// テスト用関数．
void CreateBMP( BITMAPFILEHEADER const & f_header, BITMAPINFOHEADER const & i_header, char const * data )
{
	std::ofstream ofs( "test_out.bmp", std::ios::binary );
	ofs.write( reinterpret_cast< char const * >( &f_header.bfType ), 2 );
	ofs.write( reinterpret_cast< char const * >( &f_header.bfSize ), 4 );
	ofs.write( reinterpret_cast< char const * >( &f_header.bfReserved1 ), 2 );
	ofs.write( reinterpret_cast< char const * >( &f_header.bfReserved2 ), 2 );
	ofs.write( reinterpret_cast< char const * >( &f_header.bfOffBits ), 4 );

	ofs.write( reinterpret_cast< char const * >( &i_header.biSize ), 4 );
	ofs.write( reinterpret_cast< char const * >( &i_header.biWidth ), 4 );
	ofs.write( reinterpret_cast< char const * >( &i_header.biHeight ), 4 );
	ofs.write( reinterpret_cast< char const * >( &i_header.biPlanes ), 2 );
	ofs.write( reinterpret_cast< char const * >( &i_header.biBitCount ), 2 );
	ofs.write( reinterpret_cast< char const * >( &i_header.biCompression ), 4 );
	ofs.write( reinterpret_cast< char const * >( &i_header.biSizeImage ), 4 );
	ofs.write( reinterpret_cast< char const * >( &i_header.biXPelsPerMeter ), 4 );
	ofs.write( reinterpret_cast< char const * >( &i_header.biYPelsPerMeter ), 4 );
	ofs.write( reinterpret_cast< char const * >( &i_header.biClrUsed ), 4 );
	ofs.write( reinterpret_cast< char const * >( &i_header.biClrImportant ), 4 );

	int const byte_per_pixel = i_header.biBitCount / 8;
	int const rest = ( i_header.biWidth * byte_per_pixel ) % 4;
	for ( size_t h = 0; h < i_header.biHeight; ++h ) {
		int const next = i_header.biWidth * byte_per_pixel * h;
		ofs.write( &data[ next ], i_header.biWidth * byte_per_pixel );
		if ( rest == 0 )
			continue;
		uint32_t const tmp = 0;
		ofs.write( reinterpret_cast< char const * >( &tmp ), rest );
	}
	int y = 0;
}

void CreateTGA( TGAHEADER const & tgah, TGAFOOTER const & tgaf, char const * data )
{
	std::ofstream ofs( "test_out.tga", std::ios::binary );

	ofs.write( reinterpret_cast< char const * >( &tgah.thID ), 1 );
	ofs.write( reinterpret_cast< char const * >( &tgah.thColorMap ), 1 );
	ofs.write( reinterpret_cast< char const * >( &tgah.thImgType ), 1 );
	ofs.write( reinterpret_cast< char const * >( &tgah.thClorMapIndex ), 2 );
	ofs.write( reinterpret_cast< char const * >( &tgah.thColorMapLength ), 2 );
	ofs.write( reinterpret_cast< char const * >( &tgah.thColorMapSize ), 1 );
	ofs.write( reinterpret_cast< char const * >( &tgah.thImageOriginX ), 2 );
	ofs.write( reinterpret_cast< char const * >( &tgah.thImageOriginY ), 2 );
	ofs.write( reinterpret_cast< char const * >( &tgah.thImageWidth ), 2 );
	ofs.write( reinterpret_cast< char const * >( &tgah.thImageHeight ), 2 );
	ofs.write( reinterpret_cast< char const * >( &tgah.thBitPerPixel ), 1 );
	ofs.write( reinterpret_cast< char const * >( &tgah.thDiscripter ), 1 );

	int const byte_per_pixel = tgah.thBitPerPixel / 8;
	ofs.write( data, tgah.thImageWidth * tgah.thImageHeight * byte_per_pixel );

	ofs.write( reinterpret_cast< char const * >( &tgaf.tfPosition ), 4 );
	ofs.write( reinterpret_cast< char const * >( &tgaf.tfDirectory ), 4 );
	ofs.write( const_cast< char const * >( tgaf.tfStr ), 18 );
}

} // namespace ImgConv