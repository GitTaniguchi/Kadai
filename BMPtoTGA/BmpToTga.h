#ifndef INCLUDED_BMPTOTGA_H_
#define INCLUDED_BMPTOTGA_H_

#include <string>


namespace ImgConv {

/* 1: 入力するbmpファイル名
 * 2: 変換後のtgaファイル名
 */
int BmpToTga( std::string bmp_file, std::string tga_file );

}

#endif // INCLUDED_BMP_TO_TGA_H_