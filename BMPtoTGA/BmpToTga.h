#ifndef INCLUDED_BMPTOTGA_H_
#define INCLUDED_BMPTOTGA_H_

#include <string>


namespace ImgConv {

/* 1: ���͂���bmp�t�@�C����
 * 2: �ϊ����tga�t�@�C����
 */
int BmpToTga( std::string bmp_file, std::string tga_file );

}

#endif // INCLUDED_BMP_TO_TGA_H_